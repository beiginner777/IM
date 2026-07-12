#ifndef CSERVER_H
#define SERVER_H

#include "global.h"

class CSession;

class CServer
{
public:
	//
	CServer(boost::asio::io_context& ioc,std::string port);
	// 清除 key值为 uuid 的连接
	void clearSession(std::string uuid);
private:
	// 接收连接
	void startAccept();
	// 接收连接的回调函数
	void handleAccept(std::shared_ptr<CSession> session, const boost::system::error_code& ec);
private:
	// 
	boost::asio::io_context& ioc_;

	// 需要放在 acceptor 前初始化
	unsigned short port_;
	// 
	tcp::acceptor acceptor_;
	// 

	// 存放连接
	std::map<std::string, std::shared_ptr<CSession>> sessions_; 
	// 
	std::mutex mtx_;
};

#endif
