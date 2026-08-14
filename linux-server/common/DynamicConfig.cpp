#include "DynamicConfig.h"
#include "RedisManager.h"
#include <iostream>
#include <chrono>

namespace {
    constexpr const char* CONFIG_KEY = "im:config";
    constexpr int POLL_INTERVAL_SEC = 30;
}

DynamicConfig::DynamicConfig()
{
}

DynamicConfig::~DynamicConfig()
{
    stopPolling();
}

void DynamicConfig::startPolling()
{
    // 启动时先全量加载一次
    loadAll();

    // 后台线程定期检查 version
    if (!stop_) {
        stop_ = false;
        pollThread_ = std::thread(&DynamicConfig::pollWorker, this);
        std::cout << "[DynamicConfig] started, enable_bloom=" << enableBloom_
                  << ", enable_nullcache=" << enableNullCache() << std::endl;
    }
}

void DynamicConfig::stopPolling()
{
    stop_ = true;
    if (pollThread_.joinable()) {
        pollThread_.join();
    }
}

void DynamicConfig::loadAll()
{
    auto redis = RedisManager::getInstance();
    // 逐个 HGet 读取配置项（配置项少，HGetAll 未封装）
    std::string enableBloom     = redis->HGet(CONFIG_KEY, "enable_bloom");
    std::string enableNullCache = redis->HGet(CONFIG_KEY, "enable_nullcache");
    std::string bloomThreshold  = redis->HGet(CONFIG_KEY, "bloom_threshold");
    std::string rateLimit       = redis->HGet(CONFIG_KEY, "rate_limit");
    std::string version         = redis->HGet(CONFIG_KEY, "version");

    std::lock_guard<std::mutex> lock(mtx_);
    if (!enableBloom.empty())
        enableBloom_ = (enableBloom == "true" || enableBloom == "1");
    if (!enableNullCache.empty())
        enableNullCache_ = (enableNullCache == "true" || enableNullCache == "1");
    if (!bloomThreshold.empty())
        bloomThreshold_ = std::stod(bloomThreshold);
    if (!rateLimit.empty())
        rateLimit_ = std::stoi(rateLimit);
    if (!version.empty())
        version_ = std::stoi(version);
}

void DynamicConfig::pollWorker()
{
    while (!stop_) {
        std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL_SEC));

        // 读 Redis 当前 version，和本地 version 比较
        auto redis = RedisManager::getInstance();
        std::string remoteVersion = redis->HGet(CONFIG_KEY, "version");
        int remote = remoteVersion.empty() ? version_ : std::stoi(remoteVersion);

        if (remote != version_) {
            std::cout << "[DynamicConfig] version changed " << version_
                      << " -> " << remote << ", reloading" << std::endl;
            loadAll();
        }
    }
}

bool DynamicConfig::enableBloom() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return enableBloom_;
}

bool DynamicConfig::enableNullCache() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return enableNullCache_;
}

double DynamicConfig::bloomThreshold() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return bloomThreshold_;
}

int DynamicConfig::rateLimit() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return rateLimit_;
}
