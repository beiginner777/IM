#ifndef CSERVER_H
#define SERVER_H

#include "global.h"


class CSession;

class CServer : public std::enable_shared_from_this<CServer>
{
public:
	//
	CServer(boost::asio::io_context& ioc,std::string port);
	//
	~CServer();
	// 清除 key值为 uuid 的连接
	void clearSession(std::string uuid);
	// 开启定时器
	void startTimer();
	// 关闭定时器
	void cancelTimer();
	// 开始接收连接
	void startReceiceConnections();
	// 获取与 StatusServer 的连接的uuid
	std::string getConnectionToStatusServerUuid();
	// 开启与 StatusServer 的心跳检测
	void startHeartCheckToStatusServer();

private:
	// 连接 StatusServer
	bool connectToStatusServer();
	// 接收连接
	void startAccept();
	// 接收连接的回调函数
	void handleAccept(std::shared_ptr<CSession> session, const boost::system::error_code& ec);
	// 定时检测所有的连接
	void checkConnectionIsOverTime(boost::system::error_code ec);
	// 发送心跳消息给 StatusServer
	void sendHeartCheckMsgToStatusServer(boost::system::error_code ec);

private:
	//
	boost::asio::io_context& ioc_;
	// 需要放在 acceptor 前初始化
	unsigned short port_;
	// 监听器
	tcp::acceptor acceptor_;
	// 存放客户端连接
	std::map<std::string, std::shared_ptr<CSession>> sessions_;
	// 以上map是共享资源
	std::mutex mtx_;
	// 定时器
	boost::asio::steady_timer timer_;
	// 维护与StausServer的连接
	std::shared_ptr<CSession> connectionToStatusServer_;
	// 定时向服务器发送心跳的子线程
	std::thread heartCheckThread_;
	// 向StatusServer发送心跳的定时器
	boost::asio::steady_timer heartCheckTimer_;
};

#endif
