#include "SeckillServer.h"
#include "HttpConnection.h"
#include "StatusClientSession.h"
#include "AsioIOContextThreadPool.h"
#include "RedisManager.h"
SeckillServer::SeckillServer(boost::asio::io_context& ioc, unsigned int port)
	: ioc_(ioc)
	, acceptor_(ioc, tcp::endpoint(tcp::v4(), port))
	, heartbeatTimer_(ioc)
	, heartbeatRunning_(false)
{
	std::cout << "SeckillServer starts, listening on port: " << port << std::endl;
}
SeckillServer::~SeckillServer()
{
	stopHeartbeat();
	acceptor_.close();
}
void SeckillServer::start()
{
	// 启动 HTTP 接收
	startAccept();
	// 立即尝试向 StatusServer 注册一次，之后由心跳定时器维持/重连
	heartbeatRunning_ = true;
	getOrCreateStatusSession();
	scheduleHeartbeat();
}
void SeckillServer::scheduleHeartbeat()
{
	auto self = shared_from_this();
	heartbeatTimer_.expires_after(std::chrono::seconds(HEART_CHRCK_INTERVAL));
	heartbeatTimer_.async_wait([self](boost::system::error_code ec) {
		self->onHeartbeatTick(ec);
		});
}
void SeckillServer::onHeartbeatTick(boost::system::error_code ec)
{
	if (ec == boost::asio::error::operation_aborted) {
		// 定时器被取消
		std::cout << "[SeckillServer] heartbeat timer was cancelled." << std::endl;
		return;
	}
	// 连接断了就重连注册；连接正常就发心跳
	auto session = getOrCreateStatusSession();
	if (session && session->isConnected()) {
		session->Send("", ID_HEADT_CHECK_REQ);
	}
	// 继续下一轮心跳
	if (heartbeatRunning_) {
		scheduleHeartbeat();
	}
}
void SeckillServer::startAccept()
{
	boost::asio::io_context& ioc = AsioIOContextThreadPool::getInstance()->getIOContext();
	auto socket = std::make_shared<tcp::socket>(ioc);
	auto self = shared_from_this();
	acceptor_.async_accept(*socket, [self, socket](boost::system::error_code ec) {
		if (ec) {
			std::cout << "[SeckillServer] accept error: " << ec.message() << std::endl;
			self->startAccept();
			return;
		}
		auto conn = std::make_shared<HttpConnection>(std::move(*socket), self.get());
		conn->start();
		self->incrementConnCount();
		std::cout << "[SeckillServer] new connection accepted." << std::endl;
		self->startAccept();
	});
}
std::shared_ptr<StatusClientSession> SeckillServer::getOrCreateStatusSession()
{
	std::lock_guard<std::mutex> lock(sessionMtx_);
	if (statusSession_ && statusSession_->isConnected()) {
		return statusSession_;
	}
	// 首次连接 / 断线重连：新建会话并注册
	boost::asio::io_context& ioc = AsioIOContextThreadPool::getInstance()->getIOContext();
	auto session = std::make_shared<StatusClientSession>(ioc);
	if (session->connect()) {
		session->sendRegister();
		statusSession_ = session;
		std::cout << "[SeckillServer] register to StatusServer success." << std::endl;
	}
	else {
		std::cout << "[SeckillServer] connect StatusServer failed, will retry in "
			<< HEART_CHRCK_INTERVAL << "s." << std::endl;
	}
	return statusSession_;
}
void SeckillServer::incrementConnCount()
{
	ConfigManager cfg = ConfigManager::getInstance();
	std::string selfName = cfg["SelfServer"]["Name"];
	std::string jsonStr = RedisManager::getInstance()->HGet(SECKILLSERVERS, selfName);
	Json::Reader reader;
	Json::Value json;
	int count = 0;
	if (reader.parse(jsonStr, json)) {
		count = json["con_count"].asInt();
	}
	count++;
	json["con_count"] = count;
	RedisManager::getInstance()->HSet(SECKILLSERVERS, selfName, json.toStyledString());
	std::cout << "[SeckillServer] connection_count = " << count << std::endl;
}
void SeckillServer::decrementConnCount()
{
	ConfigManager cfg = ConfigManager::getInstance();
	std::string selfName = cfg["SelfServer"]["Name"];
	std::string jsonStr = RedisManager::getInstance()->HGet(SECKILLSERVERS, selfName);
	Json::Reader reader;
	Json::Value json;
	int count = 0;
	if (reader.parse(jsonStr, json)) {
		count = json["con_count"].asInt();
		count = std::max(0, count - 1);
	}
	json["con_count"] = count;
	RedisManager::getInstance()->HSet(SECKILLSERVERS, selfName, json.toStyledString());
	std::cout << "[SeckillServer] connection_count = " << count << std::endl;
}
void SeckillServer::stopHeartbeat()
{
	heartbeatRunning_ = false;
	heartbeatTimer_.cancel();
}
