#include <boost/asio.hpp>
#include <iostream>
#include "ConfigManager.h"
#include "AsioIOContextThreadPool.h"
#include "CServer.h"
#include "RedisManager.h"
#include "ChatServiceImpl.h"

// to do ... 
// 在设置 uid_ token_ uip_ 的时候，需要加锁以及设置过期时间。

// 需要同时启动 tcp监听 和 rpc监听

int main()
{
	try
	{
		ConfigManager cfg = ConfigManager::getInstance();
		std::string host = cfg["SelfServer"]["Host"];
		std::string port = cfg["SelfServer"]["Port"];
		std::string RPCPort = cfg["SelfServer"]["RPCPort"];

		// rpc服务
		std::string serverAddr = host + ":" + RPCPort;
		::grpc::ServerBuilder builder;
		builder.AddListeningPort(serverAddr,grpc::InsecureServerCredentials());
		ChatServiceImpl service;
		builder.RegisterService(&service);
		std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << serverAddr << std::endl;
		//单独启动一个线程处理grpc服务
		std::thread  grpc_server_thread([&]() {
			server->Wait();
			});

		// tcp服务
		// 这个io_context是用来负责新用户的连接的，而io_contextPool的中是负责通信的
		boost::asio::io_context ioc;
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

		std::shared_ptr<CServer> s = std::make_shared<CServer>(ioc, port);

		signals.async_wait([&ioc, &server, &s](auto, auto) {
			std::cout << "io_context is stop." << std::endl;
			s->cancelTimer();
			std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 给定时器回调函数的触发时间
			ioc.stop();
			AsioIOContextThreadPool::getInstance()->stop();
			server->Shutdown();
			});

		ioc.run();
		grpc_server_thread.join();
	}
	catch (const std::exception& e) {
		std::cout << "error message: " << e.what();
	}
}
