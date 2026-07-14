#include "GateServer.h"
#include "HttpConnection.h"
GateServer::GateServer(boost::asio::io_context& ioc, unsigned int port) 
	: ioc_(ioc)
	, acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
{
	std::cout << "GateServer starts,listening on port: " << port << std::endl;
}
GateServer::~GateServer()
{
	acceptor_.close();  // 停止接受新连接
}

void GateServer::start()
{
    boost::asio::io_context& ioc = AsioIOContextThreadPool::getInstance()->getIOContext();
    auto socket = std::make_shared<tcp::socket>(ioc);
    auto self = shared_from_this();
    acceptor_.async_accept(*socket, [self, socket](boost::system::error_code ec) {
        if (ec)
        {
            std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << std::endl;
            std::cout << "error code: " << ec.value() << " error message: " << ec.message() << std::endl;
            self->start();
            return;
        }
        auto conn = std::make_shared<HttpConnection>(std::move(*socket));
        conn->start();
        std::cout << "new connection connected." << std::endl;
        self->start();
    });
}
