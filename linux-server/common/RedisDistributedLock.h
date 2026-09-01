#ifndef REDIS_DISTRIBUTED_LOCK_H
#define REDIS_DISTRIBUTED_LOCK_H

#include <string>
#include <thread>
#include <atomic>

// 可重入 + 自动续期的 Redis 分布式锁（RAII）
// 锁结构：Redis Hash，field=holderId，value=重入计数
// 加锁/解锁/续期全部 Lua 原子；看门狗线程周期续期（租期/3），防止长业务锁过期
class RedisDistributedLock {
public:
    RedisDistributedLock(const std::string& key, int leaseTimeMs = 5000);
    ~RedisDistributedLock();                       // 析构兜底解锁

    RedisDistributedLock(const RedisDistributedLock&) = delete;
    RedisDistributedLock& operator=(const RedisDistributedLock&) = delete;

    bool tryLock(int waitTimeoutMs);               // 可重入加锁（带重试），成功返回 true
    void unlock();                                 // 可重入解锁 + 停看门狗

private:
    void startWatchdog();
    void stopWatchdog();
    bool renewExpire();                            // Lua 续期

    std::string key_;
    std::string holderId_;                         // pid + thread_id，同线程稳定（支持可重入）
    int leaseTimeMs_;
    bool locked_{false};
    std::atomic<bool> released_{false};
    std::thread watchdog_;
};
#endif
