#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "global.h"
class SeckillServer;
// HTTP 连接（参考 GateServer，增加 CORS + Keep-Alive 支持）：
// - 前端登录后 setBaseURL 直接跨端口访问本服务，浏览器会发 OPTIONS 预检
// - 所有响应统一带 Access-Control-Allow-Origin 头
// - keep_alive(true)：服务端不主动关连接，客户端可复用，TIME-WAIT 转移到客户端侧
// - 60s 空闲超时：期间无新请求则服务端主动关
class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;

public:
	HttpConnection(tcp::socket&& sock, SeckillServer* server);
	~HttpConnection();
	tcp::socket& getSock() { return sock_; }
	void start();
	bool authenticate(); // JWT 验证，成功则设置 uid_
	int uid() const { return uid_; }

	tcp::socket sock_;
	net::steady_timer deadline_;

private:
	void read_request();
	void prase_request();
	void send_response();

	void resetDeadline();
	void closeConnection();
	SeckillServer* server_;
	bool connectionClosed_;
	int uid_{0}; // 认证后的用户 ID
	boost::beast::flat_buffer buffer_{8192};
	http::request<http::dynamic_body> request_;
	http::response<http::dynamic_body> response_;
};
#endif
