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
	tcp::socket& getSocket() { return socket_; }
	std::string& getUuid() { return uuid_; }
	// 发送数据
	void Send(const char* msg, size_t max_length, short msgid);
	void Send(std::string msg, short msgid);
	void setHeartCheckTime(time_t tm);
	bool isHeartOverTime();
	void SetServerInfo(Server_Info info) { server_info_ = info; }
	Server_Info GetServerInfo() { return server_info_; }
public:
	// 当前 CSession 对应的 CServer 对象指针
	CServer* server_;
private:
	// 读取头部消息
	void AsyncReadHead(std::size_t len);
	// 出现错误 或者 读取指定长度之后，触发回调函数
	void AsyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code, std::size_t)>handler);
	// 继续读取 长度为 totolLen - readLen 的剩余数据，出现错误 或者 读取完成之后触发回调函数
	void AsyncReadLen(std::size_t readLen, std::size_t totolLen, std::function<void(const boost::system::error_code, std::size_t)> handler);
	// 读取消息主体
	void AsyncReadBody(std::size_t len);
	// 发送消息的回调函数
	void handleWrite(boost::system::error_code ec, std::shared_ptr<CSession> session);
private:
	boost::asio::io_context& ioc_;
	// 连接是否关闭
	bool b_close_;
	// 当前 CSession 对应的socket，用于与客户端通信
	boost::asio::ip::tcp::socket socket_;
	// uuid标识符,存放在CServer的map中
	std::string uuid_;
	// 存储接收到的消息
	char data_[MAX_RECV_LENGTH];
	// 存放SendNode消息结点的队列是共享资源
	std::queue<std::shared_ptr<SendNode>> que_;
	std::mutex send_mtx_;
	// 接收消息头部的结点
	std::shared_ptr<RecvNode> recv_head_node_;
	// 接收消息主体的结点
	std::shared_ptr<RecvNode> recv_msg_node_;
	// socket_ 是共享资源
	std::mutex mtx_;
	// 心跳检测的时间戳(加锁)
	std::time_t heartCheckTime_;
	std::mutex timeMtx_;
	// 当前这个连接的Server信息
	Server_Info server_info_;
};
#endif
