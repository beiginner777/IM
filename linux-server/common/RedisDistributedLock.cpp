#include "RedisDistributedLock.h"
#include "RedisManager.h"
#include <unistd.h>
#include <functional>
#include <chrono>
#include <vector>

// 可重入加锁 Lua：
//   锁不存在 -> 首次加锁（hset + pexpire）
//   是自己持有 -> 重入计数 +1（hincrby + pexpire 顺带刷新过期）
//   别人持有 -> 返回 0
static const char* LOCK_ACQUIRE_LUA =
    "if redis.call('exists', KEYS[1]) == 0 then "
    "    redis.call('hset', KEYS[1], ARGV[1], 1) "
    "    redis.call('pexpire', KEYS[1], ARGV[2]) "
    "    return 1 "
    "elseif redis.call('hexists', KEYS[1], ARGV[1]) == 1 then "
    "    redis.call('hincrby', KEYS[1], ARGV[1], 1) "
    "    redis.call('pexpire', KEYS[1], ARGV[2]) "
    "    return 1 "
    "else "
    "    return 0 "
    "end";

// 可重入解锁 Lua：
//   不是自己持有 -> 返回 0（防误删别人的锁）
//   计数 -1：>0 只减计数，=0 删锁
static const char* LOCK_RELEASE_LUA =
    "if redis.call('hexists', KEYS[1], ARGV[1]) == 0 then "
    "    return 0 "
    "end "
    "local count = redis.call('hincrby', KEYS[1], ARGV[1], -1) "
    "if count > 0 then "
    "    redis.call('pexpire', KEYS[1], ARGV[2]) "
    "    return 1 "
    "else "
    "    redis.call('del', KEYS[1]) "
    "    return 1 "
    "end";

// 续期 Lua：仅当锁仍是自己持有才续期
static const char* LOCK_RENEW_LUA =
    "if redis.call('hexists', KEYS[1], ARGV[1]) == 1 then "
    "    return redis.call('pexpire', KEYS[1], ARGV[2]) "
    "else "
    "    return 0 "
    "end";

RedisDistributedLock::RedisDistributedLock(const std::string& key, int leaseTimeMs)
    : key_(key), leaseTimeMs_(leaseTimeMs)
{
}

RedisDistributedLock::~RedisDistributedLock()
{
    if (locked_) {
        unlock();
    }
}

bool RedisDistributedLock::tryLock(int waitTimeoutMs)
{
    if (locked_) {
        return true;  // 本对象已持有，幂等
    }
    // 首次加锁时计算 holderId，保证与加锁线程一致（可重入的前提：同一线程用同一 ID）
    if (holderId_.empty()) {
        holderId_ = std::to_string(::getpid()) + ":" +
                    std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }

    auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(waitTimeoutMs);
    while (true) {
        long long ret = RedisManager::getInstance()->evalLuaInt(
            LOCK_ACQUIRE_LUA, {key_}, {holderId_, std::to_string(leaseTimeMs_)});
        if (ret == 1) {
            locked_ = true;
            startWatchdog();
            return true;
        }
        if (std::chrono::steady_clock::now() >= end) {
            return false;  // 等待超时，仍被别人持有
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void RedisDistributedLock::unlock()
{
    if (!locked_) {
        return;
    }
    stopWatchdog();
    RedisManager::getInstance()->evalLuaInt(
        LOCK_RELEASE_LUA, {key_}, {holderId_, std::to_string(leaseTimeMs_)});
    locked_ = false;
}

bool RedisDistributedLock::renewExpire()
{
    long long ret = RedisManager::getInstance()->evalLuaInt(
        LOCK_RENEW_LUA, {key_}, {holderId_, std::to_string(leaseTimeMs_)});
    return ret == 1;
}

void RedisDistributedLock::startWatchdog()
{
    released_ = false;
    watchdog_ = std::thread([this]() {
        // 续期周期 = 租期/3，留足余量，保证业务跑多久锁都不过期
        while (!released_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(leaseTimeMs_ / 3));
            if (released_.load()) {
                break;
            }
            renewExpire();
        }
    });
}

void RedisDistributedLock::stopWatchdog()
{
    released_ = true;
    if (watchdog_.joinable()) {
        watchdog_.join();  // 防止看门狗线程泄漏
    }
}
