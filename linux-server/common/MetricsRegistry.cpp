#include "MetricsRegistry.h"
#include <sstream>
#include <iomanip>

MetricsRegistry::MetricsRegistry()
{
}

void MetricsRegistry::incCounter(const std::string& name, double val)
{
    std::lock_guard<std::mutex> lock(mtx_);
    counters_[name] += val;
}

void MetricsRegistry::setGauge(const std::string& name, double val)
{
    std::lock_guard<std::mutex> lock(mtx_);
    gauges_[name] = val;
}

void MetricsRegistry::incGauge(const std::string& name, double val)
{
    std::lock_guard<std::mutex> lock(mtx_);
    gauges_[name] += val;
}

void MetricsRegistry::decGauge(const std::string& name, double val)
{
    std::lock_guard<std::mutex> lock(mtx_);
    gauges_[name] -= val;
}

std::string MetricsRegistry::render()
{
    std::lock_guard<std::mutex> lock(mtx_);
    std::ostringstream oss;

    // Counter 指标
    for (const auto& kv : counters_) {
        oss << "# HELP " << kv.first << " " << kv.first << "\n";
        oss << "# TYPE " << kv.first << " counter\n";
        oss << kv.first << " " << std::fixed << std::setprecision(0)
            << kv.second << "\n\n";
    }

    // Gauge 指标
    for (const auto& kv : gauges_) {
        oss << "# HELP " << kv.first << " " << kv.first << "\n";
        oss << "# TYPE " << kv.first << " gauge\n";
        oss << kv.first << " " << std::fixed << std::setprecision(0)
            << kv.second << "\n\n";
    }

    return oss.str();
}
