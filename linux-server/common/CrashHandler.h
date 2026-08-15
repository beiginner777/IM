#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

// 崩溃时打印调用栈，用于定位 Exit 139 (SIGSEGV) / SIGABRT 等段错误
// 用法：在 main() 开头调用 installCrashHandler();
// 依赖：glibc 的 backtrace()（Linux 下可用），链接时需加 -rdynamic 才能显示函数名

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <cstring>

static void crashHandler(int sig)
{
    const char* header = "\n========== FATAL SIGNAL (backtrace) ==========\n";
    ::write(STDERR_FILENO, header, std::strlen(header));

    void* array[64];
    int size = backtrace(array, 64);
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    const char* footer = "========== end of backtrace ==========\n";
    ::write(STDERR_FILENO, footer, std::strlen(footer));

    // 恢复进程以标准信号退出码结束，方便 docker 上报 Exit 139/134 等
    _exit(128 + sig);
}

static void installCrashHandler()
{
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = crashHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;  // 处理一次后恢复默认，避免崩溃处理函数自身递归

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
}

#endif
