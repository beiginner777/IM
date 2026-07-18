#include "global.h"
#include "HttpConnection.h"
#include "LogicSystem.h"
#include "SeckillServer.h"
HttpConnection::HttpConnection(tcp::socket&& sock, SeckillServer* server)
	: sock_(std::move(sock))
	, deadline_(sock_.get_executor(), std::chrono::seconds(60))
	, server_(server)
	, conCountDecremented_(false)
{
}
HttpConnection::~HttpConnection()
{
	// 兜底：如果 send_response 的 callback 没有正常触发 decrement（极端情况），
	// 在析构时补一刀
	decrementIfNeeded();
	sock_.close();
}
void HttpConnection::decrementIfNeeded()
{
	if (!conCountDecremented_ && server_) {
		conCountDecremented_ = true;
		server_->decrementConnCount();
	}
}
void HttpConnection::check_deadline()
{
	auto self = shared_from_this();
	deadline_.async_wait([self](boost::system::error_code ec) {
		if (ec)
		{
			// 超时：关闭 socket 并减少连接计数
			self->decrementIfNeeded();
			self->sock_.close();
			return;
		}
		self->deadline_.cancel();
		});
}
void HttpConnection::start()
{
	read_request();
	check_deadline();
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
				std::cout << __FILE__ << ":" << __LINE__ << std::endl;
				std::cout << "error code: " << ec.value() << std::endl;
				std::cout << "error message: " << ec.message() << std::endl;
			}
		});
}
void HttpConnection::prase_request()
{
	auto self = shared_from_this();
	response_.version(request_.version());
	response_.keep_alive(false);
	// CORS：前端跨端口访问，所有响应统一带上允许跨域的头
	response_.set(http::field::access_control_allow_origin, "*");
	if (request_.method() == http::verb::options)
	{
		// CORS 预检请求：直接放行
		response_.result(http::status::no_content);
		response_.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
		response_.set(http::field::access_control_allow_headers, "Content-Type");
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
	http::async_write(
		sock_,
		response_,
		[self](beast::error_code ec, std::size_t bytesTransfered)
		{
			self->sock_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
			// 响应已完成，减少连接计数
			self->decrementIfNeeded();
			if (ec.value()) {
				std::cout << "error code: " << ec.value() << std::endl;
				std::cout << "error message: " << ec.message() << std::endl;
			}
		});
}
