#include <boost/asio.hpp>
#include <iostream>
#include "ConfigManager.h"
#include "CServer.h"
#include "AsioIOContextThreadPool.h"
int main()
{
	// io_context 负责新用户连接，AsioIOContextThreadPool 负责通信 + StatusServer 心跳
	boost::asio::io_context ioc;
	boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
	signals.async_wait([&ioc](auto, auto) {
		ioc.stop();
		AsioIOContextThreadPool::getInstance()->stop();
		});
	ConfigManager cfg = ConfigManager::getInstance();
	std::string port = cfg["SelfServer"]["Port"];
	// CServer 构造时会连接 StatusServer 并发送 ID_REGISTER_REQ
	// 收到 ID_REGISTER_RSP 后自动调用 startReceiceConnections() 开始接收客户端连接
	CServer s(ioc, port);
	ioc.run();
	return 0;
}
