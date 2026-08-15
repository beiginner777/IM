#ifndef METRICS_REGISTRY_H
#define METRICS_REGISTRY_H

#include "SingleTon.h"
#include <string>
#include <unordered_map>
#include <mutex>

// 轻量监控指标注册表（内存计数器 + Prometheus 文本格式输出）
//
// 不依赖 prometheus-cpp，手写 /metrics 端点，Prometheus 可直接 scrape。
//
// 用法：
//   MetricsRegistry::getInstance()->incCounter("im_msg_total");
//   MetricsRegistry::getInstance()->incCounter("im_user_penetration_total");
//   MetricsRegistry::getInstance()->setGauge("im_conn_count", 100);
//   std::string text = MetricsRegistry::getInstance()->render();
//
// 输出格式（Prometheus text format）：
//   # HELP im_msg_total total messages
//   # TYPE im_msg_total counter
//   im_msg_total 12345
class MetricsRegistry : public SingleTon<MetricsRegistry>
{
    friend class SingleTon<MetricsRegistry>;
public:
    // Counter（只增不减）
    void incCounter(const std::string& name, double val = 1.0);

    // Gauge（可增可减）
    void setGauge(const std::string& name, double val);
    void incGauge(const std::string& name, double val = 1.0);
    void decGauge(const std::string& name, double val = 1.0);

    // 渲染为 Prometheus 文本格式
    std::string render();

private:
    MetricsRegistry();
    std::mutex mtx_;
    std::unordered_map<std::string, double> counters_;
    std::unordered_map<std::string, double> gauges_;
};

#endif
