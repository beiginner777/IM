#include "CServer.h"
#include "AsioIOContextThreadPool.h"
#include "CSession.h"

CServer::CServer(boost::asio::io_context& ioc, std::string port)
	: ioc_(ioc),
	port_(static_cast<unsigned short>(atoi(port.c_str()))),
	acceptor_(ioc_, tcp::endpoint(tcp::v4(), port_))
{
	std::cout << "Server starting on port: " << port_ << std::endl;
	std::cout << "Local endpoint: " << acceptor_.local_endpoint().address().to_string()
		<< ":" << acceptor_.local_endpoint().port() << std::endl;
	startAccept();
}

void CServer::startAccept()
{
	auto& ioc = AsioIOContextThreadPool::getInstance()->getIOContext();
	std::shared_ptr<CSession> session = std::make_shared<CSession>(ioc, this);
	acceptor_.async_accept(session->getSocket(), std::bind(&CServer::handleAccept,this,session,std::placeholders::_1));
}

void CServer::handleAccept(std::shared_ptr<CSession> session, const boost::system::error_code& ec)
{
	if (ec.value())
	{
		std::cout << "accept connection failed." << std::endl;
		std::cout << "error code: " << ec.value() << std::endl;
		std::cout << "error message: " << ec.message() << std::endl;
	}
	else 
	{
		session->start();
		std::lock_guard<std::mutex> locker_(mtx_);
		sessions_.insert(std::pair<std::string, std::shared_ptr<CSession>>(session->getUuid(),session));
	}
	startAccept();
}

void CServer::clearSession(std::string uuid)
{
	std::lock_guard<std::mutex> locker(mtx_);
	if (sessions_.count(uuid)) 
	{
		sessions_.erase(uuid);
		//UserManager::getInstance()->removeSession(sessions_[uuid]->getUserId());
		std::cout << "erase session whose uuid is " << uuid << std::endl;
	}
	else 
	{
		std::cout << "session has been erased." << std::endl;
	}
}
