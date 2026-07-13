#include <boost/asio.hpp>
#include <iostream>
#include "ConfigManager.h"
#include "CServer.h"
#include "AsioIOContextThreadPool.h"

int main()
{
	// 这个io_context是用来负责新用户的连接的，而io_contextPool的中是负责通信的
	boost::asio::io_context ioc;
	boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

	signals.async_wait([&ioc](auto, auto) {
		ioc.stop();
		AsioIOContextThreadPool::getInstance()->stop();
		});

	ConfigManager cfg = ConfigManager::getInstance();
	std::string port = cfg["SelfServer"]["Port"];

	CServer s(ioc, port);
	ioc.run();

	return 0;
}
