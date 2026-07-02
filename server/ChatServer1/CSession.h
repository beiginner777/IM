#ifndef CSESSION_H
#define CSESSION_H

#include "global.h"
#include "MsgNode.h"

class CServer;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& ioc, CServer* server);
	~CSession();

	void start();
	void Close();

	void setUserId(int userId) { userId_ = userId; }

	tcp::socket& getSocket() { return socket_; }
	std::string& getUuid() { return uuid_;  }
	int getUserId() { return userId_; }
	bool isStatusServerConnection();

	void Send(const char* msg, size_t max_length, short msgid, std::string uuid = "");
	void Send(std::string msg, short msgid, std::string uuid = "");

	void notifyOffLine(int uid);
	
	void setHeartCheckTime(time_t tm);
	bool isHeartOverTime();

public:
	CServer* server_;

private:
	void AsyncReadHead(std::size_t len);
	void AsyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code, std::size_t)>handler);
	void AsyncReadLen(std::size_t readLen, std::size_t totolLen, std::function<void(const boost::system::error_code, std::size_t)> handler);
	void AsyncReadBody(std::size_t len);
	void handleWrite(boost::system::error_code ec,std::shared_ptr<CSession> session);
	void handleNotifyOffLine(boost::system::error_code ec, std::shared_ptr<CSession> session);

private:
	bool b_close_;

	boost::asio::io_context& ioc_; 
	boost::asio::ip::tcp::socket socket_; 
	std::string uuid_; 
	int userId_; 
	char data_[MAX_RECV_LENGTH];

	std::queue<std::shared_ptr<SendNode>> que_;
	std::mutex send_mtx_;

	std::shared_ptr<RecvNode> recv_head_node_;
	std::shared_ptr<RecvNode> recv_msg_node_;

	std::mutex mtx_;

	std::time_t heartCheckTime_;
	std::mutex timeMtx_;
};

#endif
