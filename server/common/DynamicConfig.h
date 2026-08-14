#ifndef DYNAMIC_CONFIG_H
#define DYNAMIC_CONFIG_H

#include "SingleTon.h"
#include <string>
#include <mutex>
#include <thread>
#include <atomic>

// 轻量配置中心（Redis Hash 存储 + version 版本检测 + 定期拉取）
//
// 设计：
//   Redis Hash "im:config" 存所有配置项
//     enable_bloom     "true"   是否启用布隆过滤器
//     enable_nullcache "true"   是否启用缓存空值
//     bloom_threshold  "0.30"   穿透率升级阈值
//     rate_limit       "100"    限流阈值
//     version          "1"      配置版本号（人工 HINCRBY 自增）
//
// 用法：
//   DynamicConfig::getInstance()->startPolling();  // 服务启动时调用一次
//   if (DynamicConfig::getInstance()->enableBloom()) { ... }
//
// 人工升降级：改 Redis 值 + version 自增，服务 30s 内自动生效，不重启。
class DynamicConfig : public SingleTon<DynamicConfig>
{
    friend class SingleTon<DynamicConfig>;
public:
    ~DynamicConfig();

    // 启动时加载 + 后台线程定期拉取（每 30s 检查 version）
    void startPolling();
    void stopPolling();

    // 开关查询（线程安全，读内存缓存）
    bool   enableBloom()     const;
    bool   enableNullCache() const;
    double bloomThreshold()  const;
    int    rateLimit()       const;

private:
    DynamicConfig();
    void loadAll();      // 全量拉取 HGetAll（用 HGet 逐个读）
    void pollWorker();   // 后台线程：每 30s 检查 version，变了就 reload

    std::mutex mtx_;
    bool   enableBloom_     = false;
    bool   enableNullCache_ = true;
    double bloomThreshold_  = 0.30;
    int    rateLimit_       = 100;
    int    version_         = 0;

    std::thread pollThread_;
    std::atomic_bool stop_{false};
};

#endif
