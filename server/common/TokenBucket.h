#ifndef TOKENBUCKET_H
#define TOKENBUCKET_H

#include <chrono>
#include <algorithm>

/// 令牌桶限流器
///
/// 工作原理：
///   - 以恒定速率 rate_ 生成 token（token/s）
///   - 桶容量 burst_ 限制最大积压 token 数（允许短期突发）
///   - consume(n) 尝试消费 n 个 token，成功返回 true
///
/// 使用示例：
///   TokenBucket bucket(10, 15);  // 10 token/s, 最多积压 15
///   if (bucket.consume(1)) { /* 允许 */ } else { /* 拒绝 */ }
///
class TokenBucket
{
public:
    /// @param rate  每秒生成的 token 数
    /// @param burst 桶容量（最大积压 token 数）
    TokenBucket(double rate, double burst)
        : rate_(rate)
        , burst_(burst)
        , tokens_(burst)  // 初始满桶，避免刚启动就限流
        , lastRefill_(std::chrono::steady_clock::now())
    {
    }

    /// 尝试消费 n 个 token
    /// @return true = 允许通过, false = 被限流
    bool consume(int n = 1)
    {
        refill();
        if (tokens_ >= n) {
            tokens_ -= n;
            return true;
        }
        return false;
    }

    /// 当前可用 token 数（调试用）
    double available() const { return tokens_; }

private:
    void refill()
    {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - lastRefill_).count();
        tokens_ += elapsed * rate_;
        if (tokens_ > burst_) tokens_ = burst_;
        lastRefill_ = now;
    }

    double rate_;     // token/s
    double burst_;    // 最大 token 数
    double tokens_;   // 当前 token 数
    std::chrono::steady_clock::time_point lastRefill_;
};

#endif
