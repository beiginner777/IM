#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
// 这里应该封装开辟一个新的线程来处理数据，
// 例如在LogicSystem中新加一个队列来存储任务
#include "global.h"
class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
public:
	HttpConnection(tcp::socket&& sock);
	~HttpConnection();
	tcp::socket& getSock() { return sock_; }
	void start();
	
	tcp::socket sock_;
	net::steady_timer deadline_;
private:
	void read_request();
	void check_deadline();
	
	void prase_request();
	void send_response();
	void clear();
	std::string uuid_;
	// 对方ip
	std::string remote_ip;
	// 对方端口
	unsigned short remote_port;
	// 接收缓冲区
	boost::beast::flat_buffer buffer_{ 8192 };
	http::request<http::dynamic_body> request_;
	http::response<http::dynamic_body> response_;
};
#endif
