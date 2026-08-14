#ifndef ASIOTIMER_H
#define ASIOTIMER_H

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <atomic>
#include <chrono>

// Boost.Asio 定时器封装：支持单次/重复、自定义间隔、回调
//
// 用法:
//   auto timer = AsioTimer::create(ioc, []{ ... }, 1000);        // 每 1s 触发
//   auto timer = AsioTimer::create(ioc, []{ ... }, 5000, false); // 5s 后单次触发
//   timer->start();
//   timer->stop();

class AsioTimer : public std::enable_shared_from_this<AsioTimer>
{
public:
    using Callback = std::function<void()>;

    // 工厂方法（必须用 shared_ptr 管理，async_wait 需要 shared_from_this）
    static std::shared_ptr<AsioTimer> create(
        boost::asio::io_context& ioc,
        Callback callback,
        int intervalMs,
        bool repeat = true)
    {
        return std::shared_ptr<AsioTimer>(
            new AsioTimer(ioc, std::move(callback), intervalMs, repeat));
    }

    ~AsioTimer() { stop(); }

    // 启动定时器（如果已在运行，先停再启）
    void start()
    {
        stop();
        running_ = true;
        scheduleNext();
    }

    // 停止定时器
    void stop()
    {
        running_ = false;
        boost::system::error_code ec;
        timer_.cancel(ec); // 忽略错误码（定时器可能已经过期）
    }

    bool isRunning() const { return running_; }

private:
    AsioTimer(boost::asio::io_context& ioc,
              Callback callback,
              int intervalMs,
              bool repeat)
        : timer_(ioc)
        , callback_(std::move(callback))
        , interval_(std::chrono::milliseconds(intervalMs))
        , repeat_(repeat)
        , running_(false)
    {}

    void scheduleNext()
    {
        if (!running_) return;

        timer_.expires_after(interval_);
        auto self = shared_from_this();
        timer_.async_wait([self](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted) return; // 被 stop() 取消
            if (!self->running_) return;

            if (ec) {
                // 其他错误（极少），记录后继续
                std::cerr << "[AsioTimer] error: " << ec.message() << std::endl;
                return;
            }

            // 执行回调
            if (self->callback_) self->callback_();

            // 重复定时器 → 自动重新调度
            if (self->repeat_ && self->running_) {
                self->scheduleNext();
            } else {
                self->running_ = false;
            }
        });
    }

    boost::asio::steady_timer timer_;
    Callback callback_;
    std::chrono::milliseconds interval_;
    bool repeat_;
    std::atomic<bool> running_;
};

#endif
