#ifndef CSERVER_H

#define SERVER_H



#include "global.h"



class CSession;



class CServer : public std::enable_shared_from_this<CServer>

{

public:

	CServer(boost::asio::io_context& ioc,std::string port);

	~CServer();

	// 清除 key值为 uuid 的连接

	void clearSession(std::string uuid);

	// 开启定时器

	void startTimer();

	// 关闭定时器

	void cancelTimer();
	// 公开访问器，供 StatusServiceImpl 获取活跃服务器列表
	const std::map<std::string, std::shared_ptr<CSession>>& getSessions() const { return sessions_; }
	
	const std::map<std::string, std::shared_ptr<CSession>>& getResourceSessions() const { return resource_sessions_; }
	
	std::mutex& getMutex() { return mtx_; }
	
	void storeInServer(std::shared_ptr<CSession> session, ServerType server_type);


private:

	// 接收连接

	void startAccept();

	// 接收连接的回调函数

	void handleAccept(std::shared_ptr<CSession> session, const boost::system::error_code& ec);

	// 定时检测所有的连接
	void checkConnectionIsOverTime(boost::system::error_code ec);
	// 定时检测ChatServer的连接
	void checkChatServerConnIsOverTime();
	// 定时检测ResourceServer的连接
	void checkResourceSercerConnIsOverTime();


private:

	boost::asio::io_context& ioc_;

	// 需要放在 acceptor 前初始化

	unsigned short port_;

	// 负责监听和接受连接

	tcp::acceptor acceptor_;

	// 存放连接 uuid : ChatServer_CSession

	std::map<std::string, std::shared_ptr<CSession>> sessions_; 

	// 存放连接 uuid : ResourceServer_CSession

	std::map<std::string, std::shared_ptr<CSession>> resource_sessions_; 

	// 以上map是共享资源

	std::mutex mtx_;

	// 定时器

	boost::asio::steady_timer timer_;

};



#endif

