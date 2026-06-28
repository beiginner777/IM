#include "CServer.h"
#include "AsioIOContextThreadPool.h"
#include "CSession.h"
#include "UserManager.h"
#include "RedisManager.h"
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
		std::cout << "Connect to StatusServer failed, please check the StatusServer is running or not." << std::endl;
		exit(-1);
	}
}

CServer::~CServer()
{
	std::cout << "CServer::destructed." << std::endl;
	for(auto kv : sessions_) {
		kv.second->Close();
	}
}

void CServer::startReceiceConnections()
{
	// 定时向StatusServer发送心跳消息
	startHeartCheckToStatusServer();

	// 可以开始接受客户端的连接了
	std::cout << "connect StatusServer success !" << std::endl;
	std::cout << "Server starting on port: " << port_ << std::endl;
	std::cout << "Local endpoint: " << acceptor_.local_endpoint().address().to_string()
		<< ":" << acceptor_.local_endpoint().port() << std::endl;
	// 开始接收新连接
	startAccept();
	// 启动检测客户端连接的定时器
	startTimer();
}

std::string CServer::getConnectionToStatusServerUuid()
{
	return connectionToStatusServer_->getUuid();
}


bool CServer::connectToStatusServer()
{
	// 获取StatusServer的ip和端口
	ConfigManager cfg = ConfigManager::getInstance();
	std::string start_server_ip = cfg["StatusServer"]["Host"];
	short start_server_port = atoi(cfg["StatusServer"]["TCP_port"].c_str());
	// 打开Socket
	connectionToStatusServer_->getSocket().open(tcp::v4());
	// 连接服务器
	boost::system::error_code ec;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(start_server_ip, ec), start_server_port);
	if (ec.value()) {
		std::cout << "error code: " << ec.value() << std::endl;
		std::cout << "error message " << ec.message() << std::endl;
		return false;
	}
	connectionToStatusServer_->getSocket().connect(ep, ec);
	if (ec.value()) {
		std::cout << "error code: " << ec.value() << std::endl;
		std::cout << "error message " << ec.message() << std::endl;
		return false;
	}
	std::cout << "Connect to StatusServer successfuly." << std::endl;
	// 连接成功之后，开启接收StatuaServer消息的功能
	connectionToStatusServer_->start();
	// 连接成功，发送注册消息
	Json::Value root;
	root["server_type"] = ServerType::CHAT_SERVER;
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
		std::cout << "Session(uid = " << session->getUserId() << " heartCheckOverTime, kick Connection.\n";
		session->Close();
	}

	// 继续开启定时检测任务
	startTimer();
}

void CServer::sendHeartCheckMsgToStatusServer(boost::system::error_code ec)
{
	if (ec == boost::asio::error::operation_aborted) {
		// 定时器被取消，执行这里的逻辑
		std::cout << "Timer was cancelled\n";
		return;
	}
	connectionToStatusServer_->Send("", ID_HEADT_CHECK_REQ);
	// 继续定时发送心跳检测包
	startHeartCheckToStatusServer();
}

void CServer::clearSession(std::string uuid)
{
	std::lock_guard<std::mutex> locker(mtx_);
	if (sessions_.count(uuid)) {
		UserManager::getInstance()->removeSession(sessions_[uuid]->getUserId(), uuid);
		sessions_.erase(uuid);
		std::cout << "erase session whose uuid is " << uuid << std::endl;
	}
	else {
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
	heartCheckTimer_.cancel();
}

void CServer::startHeartCheckToStatusServer()
{
	// 开启定时检测任务
	heartCheckTimer_.expires_after(std::chrono::seconds(HEART_CHRCK_INTERVAL));

	auto self = shared_from_this();
	heartCheckTimer_.async_wait([self](boost::system::error_code ec) {
		self->sendHeartCheckMsgToStatusServer(ec);
		});
}
