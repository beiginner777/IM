#include "CServer.h"
#include "AsioIOContextThreadPool.h"
#include "CSession.h"
#include "ConfigManager.h"

CServer::CServer(boost::asio::io_context& ioc, std::string port)
	: ioc_(ioc),
	port_(static_cast<unsigned short>(atoi(port.c_str()))),
	acceptor_(ioc_, tcp::endpoint(tcp::v4(), port_)),
	timer_(ioc),
	heartCheckTimer_(ioc)
{
	auto& start_server_ioc = AsioIOContextThreadPool::getInstance()->getIOContext();
	connectionToStatusServer_ = std::make_shared<CSession>(start_server_ioc, this);
	if (!connectToStatusServer()) {
		std::cout << "[ResourceServer] Connect to StatusServer failed." << std::endl;
		exit(-1);
	}
}

CServer::~CServer()
{
	std::cout << "CServer::destructed." << std::endl;
	for (auto kv : sessions_) {
		kv.second->Close();
	}
}

void CServer::startReceiceConnections()
{
	startHeartCheckToStatusServer();

	std::cout << "[ResourceServer] Registered to StatusServer, start accepting..." << std::endl;
	std::cout << "Server starting on port: " << port_ << std::endl;
	startAccept();
	startTimer();
}

std::string CServer::getConnectionToStatusServerUuid()
{
	return connectionToStatusServer_->getUuid();
}

bool CServer::connectToStatusServer()
{
	ConfigManager cfg = ConfigManager::getInstance();
	std::string start_server_ip = cfg["StatusServer"]["Host"];
	short start_server_port = atoi(cfg["StatusServer"]["TCP_port"].c_str());

	connectionToStatusServer_->getSocket().open(tcp::v4());
	boost::system::error_code ec;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(start_server_ip, ec), start_server_port);
	if (ec.value()) {
		std::cout << "[ResourceServer] Bad StatusServer IP: " << ec.message() << std::endl;
		return false;
	}
	connectionToStatusServer_->getSocket().connect(ep, ec);
	if (ec.value()) {
		std::cout << "[ResourceServer] Connect to StatusServer failed: " << ec.message() << std::endl;
		return false;
	}
	std::cout << "[ResourceServer] Connected to StatusServer." << std::endl;
	connectionToStatusServer_->start();

	// 发送注册消息
	Json::Value root;
	root["server_type"] = ServerType::RESOURCE_SERVER;
	root["my_ip"] = cfg["SelfServer"]["Host"];
	root["my_port"] = cfg["SelfServer"]["Port"];
	root["my_name"] = cfg["SelfServer"]["Name"];
	connectionToStatusServer_->Send(root.toStyledString(), ID_REGISTER_REQ);
	return true;
}

void CServer::startAccept()
{
	auto& ioc = AsioIOContextThreadPool::getInstance()->getIOContext();
	std::shared_ptr<CSession> session = std::make_shared<CSession>(ioc, this);
	acceptor_.async_accept(session->getSocket(), std::bind(&CServer::handleAccept, this, session, std::placeholders::_1));
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
		sessions_.insert(std::pair<std::string, std::shared_ptr<CSession>>(session->getUuid(), session));
	}
	startAccept();
}

void CServer::checkConnectionIsOverTime(boost::system::error_code ec)
{
	if (ec == boost::asio::error::operation_aborted) {
		std::cout << "Timer was cancelled\n";
		return;
	}

	std::map<std::string, std::shared_ptr<CSession>> copy_sessions_;
	{
		std::lock_guard<std::mutex> locker(mtx_);
		copy_sessions_ = sessions_;
	}

	std::vector<std::shared_ptr<CSession>> expiredSession;
	for (auto& [uuid, session] : copy_sessions_) {
		if (session->isClientOverTime()) {
			expiredSession.push_back(session);
		}
	}

	if (!expiredSession.empty()) {
		std::cout << "[INFO] " << expiredSession.size() << " connection(s) overTime in ResourceServer.\n";
	}

	for (auto session : expiredSession) {
		session->Close();
	}

	timer_.expires_after(std::chrono::seconds(HEART_CHRCK_INTERVAL));
	timer_.async_wait([this](boost::system::error_code ec) {
		checkConnectionIsOverTime(ec);
		});
}

void CServer::startHeartCheckToStatusServer()
{
	heartCheckTimer_.expires_after(std::chrono::seconds(HEART_CHRCK_INTERVAL));
	heartCheckTimer_.async_wait(std::bind(&CServer::sendHeartCheckMsgToStatusServer, this, std::placeholders::_1));
}

void CServer::sendHeartCheckMsgToStatusServer(boost::system::error_code ec)
{
	connectionToStatusServer_->Send("", ID_HEADT_CHECK_REQ);
	startHeartCheckToStatusServer();
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
	timer_.expires_after(std::chrono::seconds(HEART_CHRCK_INTERVAL));
	auto self = shared_from_this();
	timer_.async_wait([self](boost::system::error_code ec) {
		self->checkConnectionIsOverTime(ec);
		});
}

void CServer::cancelTimer()
{
	timer_.cancel();
}
