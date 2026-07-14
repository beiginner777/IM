#include <boost/asio.hpp>
#include <iostream>
#include "ConfigManager.h"
#include "AsioIOContextThreadPool.h"
#include "CServer.h"
#include "RedisManager.h"
#include "ChatServiceImpl.h"
#include "BatchMessageWriter.h"
#include "MysqlManager.h"
// to do ...
// ������ uid_ token_ uip_ ��ʱ����Ҫ�����Լ����ù���ʱ�䡣
// ��Ҫͬʱ���� tcp���� �� rpc����
int main()
{
	try
	{
		ConfigManager cfg = ConfigManager::getInstance();
		std::string host = cfg["SelfServer"]["Host"];
		std::string port = cfg["SelfServer"]["Port"];
		std::string RPCPort = cfg["SelfServer"]["RPCPort"];
		// rpc����
		std::string serverAddr = host + ":" + RPCPort;
		::grpc::ServerBuilder builder;
		builder.AddListeningPort(serverAddr,grpc::InsecureServerCredentials());
		ChatServiceImpl service;
		builder.RegisterService(&service);
		std::unique_ptr<::grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << serverAddr << std::endl;
		//��������һ���̴߳���grpc����
		std::thread  grpc_server_thread([&]() {
			server->Wait();
			});
		// tcp����
		// ���io_context�������������û������ӵģ���io_contextPool�����Ǹ���ͨ�ŵ�
		boost::asio::io_context ioc;
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		std::shared_ptr<CServer> s = std::make_shared<CServer>(ioc, port);
		// 设置 Snowflake 降级的 server_id（不同服务器用不同编号）
		RedisManager::getInstance()->setServerId(1);
		// 构建布隆过滤器（从 MySQL 加载用户列表，用户搜索加速）
		MysqlManager::getInstance()->initBloomFilter();
		// 启动异步批量写入线程
		BatchMessageWriter::getInstance()->start();
		signals.async_wait([&ioc, &server, &s](auto, auto) {
			std::cout << "io_context is stop." << std::endl;
			s->cancelTimer();
			BatchMessageWriter::getInstance()->stop();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
