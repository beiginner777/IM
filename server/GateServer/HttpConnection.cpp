#include "global.h"
#include "HttpConnection.h"
#include "LogicSystem.h"
HttpConnection::HttpConnection(tcp::socket&& sock)
	: sock_(std::move(sock))
	, remote_ip("")
	, remote_port(0)
	, deadline_(sock_.get_executor(), std::chrono::seconds(60))
{
	boost::uuids::uuid  a_uuid = boost::uuids::random_generator()();
	uuid_ = boost::uuids::to_string(a_uuid);
	std::cout << "uuid = " << uuid_ << " connection builds." << std::endl;
}
HttpConnection::~HttpConnection()
{
	sock_.close();
	std::cout << "uuid = " << uuid_ << " connection destructed." << std::endl;
}
void HttpConnection::check_deadline()
{
	auto self = shared_from_this();
	deadline_.async_wait([self](boost::system::error_code ec) {
		if (ec)
		{
			std::cout << __FILE__ << ":" << __LINE__ << std::endl;
			std::cout << "error code: " << ec.value() << std::endl;
			std::cout << "error message: " << ec.message() << std::endl;
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
				std::cout  <<  "error code: " << ec.value() << std::endl;
				std::cout  <<  "error message: " << ec.message() << std::endl;
			}
		});
}
void HttpConnection::prase_request()
{
	auto self = shared_from_this();
	response_.version(request_.version());
	response_.keep_alive(false);
	if (request_.method() == http::verb::get)
	{
		/*LogicSystem::getInstance()->postTaskToQue([self,this]() {
			LogicSystem::getInstance()->handleGetRequest(self);
		});*/
		LogicSystem::getInstance()->handleGetRequest(self);
	}
	else if (request_.method() == http::verb::post)
	{
		/*LogicSystem::getInstance()->postTaskToQue([self, this]() {
			LogicSystem::getInstance()->handlePostRequest(self);
			});*/
		LogicSystem::getInstance()->handlePostRequest(self);
	}
	else
	{
		std::cout << "Unknowed request method: " << request_.method() << std::endl;
	}
}
void HttpConnection::send_response()
{
	auto self = shared_from_this();
	response_.content_length(response_.body().size());
	std::cout << "return message = " << boost::beast::buffers_to_string(response_.body().data()) << std::endl;
	http::async_write(
		sock_,
		response_,
		[self,this](beast::error_code ec, std::size_t bytesTransfered)
		{
			if(ec.value()) {
				self->sock_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
				self->deadline_.cancel();
				std::cout << "error code: " << ec.value() << std::endl;
				std::cout << "error message: " << ec.message() << std::endl;
			}
			else {
				std::cout << bytesTransfered << " bytes was sent." << std::endl;
			}
		});
}
void HttpConnection::clear()
{
	buffer_.consume(buffer_.size());
	request_ = {};
	response_ = {};
}
