#include "CServer.h"
#include "CSession.h"
#include "ConfigManager.h"

CServer::CServer(boost::asio::io_context& ioc, std::string port)
	: ioc_(ioc),
	port_(static_cast<unsigned short>(atoi(port.c_str()))),
	acceptor_(ioc_, tcp::endpoint(tcp::v4(), port_)),
	timer_(ioc)
{
	std::cout << "Server starting on port: " << port_ << std::endl;
	std::cout << "Local endpoint: " << acceptor_.local_endpoint().address().to_string()
		<< ":" << acceptor_.local_endpoint().port() << std::endl;

	startAccept();
}

CServer::~CServer()
{
	std::cout << "CServer::destructed." << std::endl;
	for(auto kv : sessions_) {
		kv.second->Close();
	}
}

void CServer::startAccept()
{
	std::shared_ptr<CSession> session = std::make_shared<CSession>(ioc_, this);
	acceptor_.async_accept(session->getSocket(), std::bind(&CServer::handleAccept,this,session,std::placeholders::_1));
}

void CServer::handleAccept(std::shared_ptr<CSession> session, const boost::system::error_code& ec)
{
	if (ec.value()){
		std::cout << "accept connection failed." << std::endl;
		std::cout << "error code: " << ec.value() << std::endl;
		std::cout << "error message: " << ec.message() << std::endl;
	} else {
		session->start();
		std::lock_guard<std::mutex> locker_(mtx_);
		sessions_.insert(std::pair<std::string, std::shared_ptr<CSession>>(session->getUuid(),session));
	}
	startAccept();
}

void CServer::checkConnectionIsOverTime(boost::system::error_code ec)
{
	if (ec == boost::asio::error::operation_aborted) {
		// 定时器被取消，执行这里的逻辑
		std::cout << "Timer was cancelled\n";
		return;
	}
	// detail: 控制锁的精度
	std::map<std::string, std::shared_ptr<CSession>> copy_sessions_;
	{
		std::lock_guard<std::mutex> lcoker(mtx_);
		copy_sessions_ = sessions_;
	}
	int sessionCount = 0;
	std::vector<std::shared_ptr<CSession>> expiredSession;
	{
		for (auto it = copy_sessions_.begin(); it != copy_sessions_.end(); ++it) {
			std::shared_ptr<CSession> session = it->second;
			bool expired = session->isHeartOverTime();
			if (expired) {
				// 过期连接
				expiredSession.push_back(session);
			}
			else {
				// 没过期
				sessionCount++;
			}
		}
	}
	
	if (expiredSession.size()) {
		std::cout << "[INFO] There are " << expiredSession.size() << " connection heartCheckOverTime.\n";
	}

	for (auto session : expiredSession) {
		//std::cout << "Session(uid = " << session->getUserId() << " heartCheckOverTime, kick Connection.\n";
		session->Close();
	}

	// 开启下一个定时检测任务
	timer_.expires_after(std::chrono::seconds(HEART_CHRCK_INTERVAL));
	timer_.async_wait([this](boost::system::error_code ec) {
		checkConnectionIsOverTime(ec);
		});
}

void CServer::clearSession(std::string uuid)
{
	std::lock_guard<std::mutex> locker(mtx_);
	if (sessions_.count(uuid)) 
	{
		sessions_.erase(uuid);
		std::cout << "erase session whose uuid is " << uuid << std::endl;
	}
	else 
	{
		std::cout << "session has been erased." << std::endl;
	}
}

void CServer::startTimer()
{
	// 开启定时检测任务
	timer_.expires_after(std::chrono::seconds(HEART_CHRCK_INTERVAL));

	auto self = shared_from_this();
	timer_.async_wait([self](boost::system::error_code ec) {
		self->checkConnectionIsOverTime(ec);
		});
}

void CServer::cancelTimer()
{
	// 取消定时任务,但是会立即触发回调函数
	timer_.cancel();
}
