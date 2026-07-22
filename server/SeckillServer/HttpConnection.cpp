#include "global.h"
#include "HttpConnection.h"
#include "LogicSystem.h"
#include "SeckillServer.h"
#include "JWT.h"

HttpConnection::HttpConnection(tcp::socket&& sock, SeckillServer* server)
	: sock_(std::move(sock))
	, deadline_(sock_.get_executor(), std::chrono::seconds(60))
	, server_(server)
	, connectionClosed_(false)
{
}
HttpConnection::~HttpConnection()
{
	closeConnection();
}
void HttpConnection::closeConnection()
{
	// 只执行一次：减连接数 + 关 socket
	if (connectionClosed_) return;
	connectionClosed_ = true;
	if (server_) {
		server_->decrementConnCount();
	}
	boost::system::error_code ec;
	sock_.close(ec);
}
void HttpConnection::resetDeadline()
{
	// 重置 60s 空闲超时：取消旧定时器（回调收到 operation_aborted 直接返回）→ 重新计时
	deadline_.cancel();
	deadline_.expires_after(std::chrono::seconds(60));
	auto self = shared_from_this();
	deadline_.async_wait([self](boost::system::error_code ec) {
		if (ec) return;             // operation_aborted：主动取消了，不做任何事
		// ec == 0：60 秒内没有任何请求，空闲超时
		self->closeConnection();
	});
}
void HttpConnection::start()
{
	resetDeadline();
	read_request();
}
void HttpConnection::read_request()
{
	auto self = shared_from_this();
	http::async_read(
		sock_,
		buffer_,
		request_,
		[self](beast::error_code ec, std::size_t bytes_transferred)
		{
			if (!ec.value())
			{
				self->prase_request();
				self->send_response();
			}
			else
			{
				// 客户端断开 或 HTTP 协议错误 → 清理连接
				if (ec != http::error::end_of_stream) {
					std::cout << "[HttpConnection] read error: " << ec.message() << std::endl;
				}
				self->closeConnection();
			}
		});
}
void HttpConnection::prase_request()
{
	auto self = shared_from_this();
	response_.version(request_.version());
	response_.keep_alive(true);   // ← 告诉客户端：我不会主动关，你可以复用这条连接
	// CORS：前端跨端口访问，所有响应统一带上允许跨域的头
	response_.set(http::field::access_control_allow_origin, "*");
	if (request_.method() == http::verb::options)
	{
		// CORS 预检请求：直接放行
		response_.result(http::status::no_content);
		response_.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
		response_.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
		response_.set(http::field::access_control_max_age, "86400");
	}
	else if (request_.method() == http::verb::get)
	{
		LogicSystem::getInstance()->handleGetRequest(self);
	}
	else if (request_.method() == http::verb::post)
	{
		LogicSystem::getInstance()->handlePostRequest(self);
	}
	else
	{
		std::cout << "Unknowed request method: " << request_.method() << std::endl;
		response_.result(http::status::method_not_allowed);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "method not allowed\n";
	}
}
void HttpConnection::send_response()
{
	auto self = shared_from_this();
	response_.content_length(response_.body().size());
	response_.set(http::field::connection, "keep-alive");
	http::async_write(
		sock_,
		response_,
		[self](beast::error_code ec, std::size_t bytesTransfered)
		{
			if (ec.value()) {
				std::cout << "[HttpConnection] write error: " << ec.message() << std::endl;
				self->closeConnection();
				return;
			}
			// 响应写完 → 重置空闲超时 → 清 buffer → 读下一个请求（keep-alive 循环）
			self->resetDeadline();
			self->buffer_.consume(self->buffer_.size());
			self->request_ = {};
			self->response_ = {};
			self->read_request();
		});
}

bool HttpConnection::authenticate()
{
	auto authIt = request_.find(http::field::authorization);
	if (authIt == request_.end()) {
		return false;
	}
	std::string auth(authIt->value());
	if (auth.size() < 8 || auth.substr(0, 7) != "Bearer ") {
		return false;
	}
	return JWT::verify(auth.substr(7), uid_, "");
}

