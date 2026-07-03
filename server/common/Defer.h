#ifndef DEFER_H
#define DEFER_H

#include <functional>

/// RAII 延迟执行器（Go 语言的 defer 在 C++ 中的实现）
/// 用法:
///   Defer d([&]{ cleanup(); }); // 离开作用域时自动执行
class Defer
{
public:
    Defer(std::function<void()> func) : func_(func) {}
    ~Defer() {
        if (func_) func_();
    }
    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;

private:
    std::function<void()> func_;
};

#endif // DEFER_H
