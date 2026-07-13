#include <iostream>
#include <boost/asio.hpp>
#include <grpcpp/grpcpp.h>
#include <memory>
#include "ConfigManager.h"
#include "StatusServiceImpl.h"
#include "CServer.h"

int main() {
    // 1. 获取配置
    auto cfg = ConfigManager::getInstance();
    std::string status_addr = cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"];
    std::string port = cfg["StatusServer"]["TCP_port"];

    // 2. 先创建 asio 环境和 CServer（TCP 服务）
    boost::asio::io_context io_ctx;
    auto session_server = std::make_shared<CServer>(io_ctx, port);
    session_server->startTimer();            // 启动心跳定时器

    // 3. 启动 gRPC 服务（StatusServer），注入 CServer 指针
    StatusServiceImpl status_service;
    status_service.setCServer(session_server.get());

    grpc::ServerBuilder builder;
    builder.AddListeningPort(status_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&status_service);
    std::unique_ptr<grpc::Server> status_server(builder.BuildAndStart());
    std::cout << "StatusServer listening on " << status_addr << std::endl;

    // 4. 信号处理：优雅关闭
    boost::asio::signal_set signals(io_ctx, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code&, int) {
        std::cout << "Shutting down..." << std::endl;
        session_server->cancelTimer();       // 取消定时器（避免回调访问已销毁对象）
        status_server->Shutdown();        // 停止 gRPC 服务
        io_ctx.stop();                    // 停止 asio 事件循环
        });

    // 5. 在另一个线程运行 asio 事件循环
    std::thread io_thread([&io_ctx] { io_ctx.run(); });

    // 6. 主线程等待 gRPC 服务器结束（会阻塞直到 Shutdown）
    status_server->Wait();

    // 7. 等待 asio 线程结束
    if (io_thread.joinable()) {
        io_thread.join();
    }

    std::cout << "StatusServer stopped." << std::endl;
    return 0;
}
