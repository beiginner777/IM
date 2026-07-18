#ifndef SECKILLSERVER_H
#define SECKILLSERVER_H

#include "global.h"
class StatusClientSession;
// HTTP acceptor + 心跳定时器（参考 GateServer，增加向 StatusServer 注册和心跳的逻辑）
class SeckillServer : public std::enable_shared_from_this<SeckillServer>
{
public:
	SeckillServer(boost::asio::io_context& ioc, unsigned int port);
	~SeckillServer();
	void start();
private:
	void startAccept();
	// 每 HEART_CHRCK_INTERVAL 秒触发一次：连接断了就重连注册，连接正常就发心跳
	void scheduleHeartbeat();
	void onHeartbeatTick(boost::system::error_code ec);
	std::shared_ptr<StatusClientSession> getOrCreateStatusSession();
	void stopHeartbeat();
	boost::asio::io_context& ioc_;
	boost::asio::ip::tcp::acceptor acceptor_;
	// 心跳
	boost::asio::steady_timer heartbeatTimer_;
	std::shared_ptr<StatusClientSession> statusSession_;
	std::mutex sessionMtx_;
	std::atomic_bool heartbeatRunning_;
};
#endif
