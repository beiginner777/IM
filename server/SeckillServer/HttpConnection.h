#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "global.h"
// HTTP 连接（参考 GateServer，增加 CORS 支持）：
// - 前端登录后 setBaseURL 直接跨端口访问本服务，浏览器会发 OPTIONS 预检
// - 所有响应统一带 Access-Control-Allow-Origin 头
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
	// 接收缓冲区
	boost::beast::flat_buffer buffer_{ 8192 };
	http::request<http::dynamic_body> request_;
	http::response<http::dynamic_body> response_;
};
#endif
