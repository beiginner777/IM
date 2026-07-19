#ifndef STATUSCLIENTSESSION_H
#define STATUSCLIENTSESSION_H

#include "global.h"
#include "MsgNode.h"
// 与 StatusServer 的 TCP 长连接（客户端侧）：
// 1. connect() 同步连接 StatusServer 的 TCP_port
// 2. sendRegister() 发送 ID_REGISTER_REQ 注册本服务（server_type = SECKILL_SERVER）
// 3. Send(..., ID_HEADT_CHECK_REQ) 定时心跳（由 SeckillServer 的定时器驱动）
// 4. 异步读回包（ID_REGISTER_RSP / ID_HEADT_CHECK_RSP），仅打印日志
// 连接断开后由 SeckillServer 心跳定时器重建新的 session 实现重连
class StatusClientSession : public std::enable_shared_from_this<StatusClientSession>
{
public:
	StatusClientSession(boost::asio::io_context& ioc);
	~StatusClientSession();
	bool connect();
	void sendRegister();
	void Send(const std::string& msg, short msgid);
	void Close();
	bool isConnected() { return connected_; }
private:
	void AsyncReadHead();
	void AsyncReadBody(short msg_id, short msg_len);
	void handleWrite(boost::system::error_code ec, std::shared_ptr<StatusClientSession> self);
private:
	boost::asio::io_context& ioc_;
	tcp::socket socket_;
	std::atomic_bool connected_;
	char head_[HEAD_TOTOL_LEN];
	char body_[MAX_RECV_LENGTH];
	std::queue<std::shared_ptr<SendNode>> que_;
	std::mutex send_mtx_;
};
#endif
