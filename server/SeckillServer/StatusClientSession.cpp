#include "StatusClientSession.h"
StatusClientSession::StatusClientSession(boost::asio::io_context& ioc)
	: ioc_(ioc)
	, socket_(ioc)
	, connected_(false)
{
}
StatusClientSession::~StatusClientSession()
{
	Close();
}
bool StatusClientSession::connect()
{
	// 获取 StatusServer 的 ip 和 TCP 端口
	ConfigManager cfg = ConfigManager::getInstance();
	std::string status_ip = cfg["StatusServer"]["Host"];
	short status_port = static_cast<short>(atoi(cfg["StatusServer"]["TCP_port"].c_str()));
	boost::system::error_code ec;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(status_ip, ec), status_port);
	if (ec.value()) {
		std::cout << "[StatusClientSession] make_address failed, error message: " << ec.message() << std::endl;
		return false;
	}
	socket_.connect(ep, ec);
	if (ec.value()) {
		std::cout << "[StatusClientSession] connect StatusServer(" << status_ip << ":" << status_port
			<< ") failed, error message: " << ec.message() << std::endl;
		return false;
	}
	connected_ = true;
	std::cout << "[StatusClientSession] connect StatusServer successfully." << std::endl;
	// 连接成功之后，开启接收 StatusServer 回包的读循环
	AsyncReadHead();
	return true;
}
void StatusClientSession::sendRegister()
{
	// 发送注册消息，协议与 StatusServer 的 registerService 一致
	ConfigManager cfg = ConfigManager::getInstance();
	Json::Value root;
	root["server_type"] = ServerType::SECKILL_SERVER;
	root["my_ip"] = cfg["SelfServer"]["Host"];
	root["my_port"] = cfg["SelfServer"]["Port"];
	root["my_name"] = cfg["SelfServer"]["Name"];
	Send(root.toStyledString(), ID_REGISTER_REQ);
}
void StatusClientSession::Send(const std::string& msg, short msgid)
{
	std::lock_guard<std::mutex> locker(send_mtx_);
	if (!connected_) {
		std::cout << "[StatusClientSession] not connected, send msg(id = " << msgid << ") failed." << std::endl;
		return;
	}
	bool pending = !que_.empty();
	que_.push(std::make_shared<SendNode>(msg.c_str(), static_cast<short>(msg.size()), msgid));
	// 已有发送任务在执行，入队即可，由 handleWrite 链式发送
	if (pending) {
		return;
	}
	auto node = que_.front();
	auto self = shared_from_this();
	boost::asio::async_write(socket_, boost::asio::buffer(node->data_, node->totol_len_),
		std::bind(&StatusClientSession::handleWrite, this, std::placeholders::_1, self));
}
void StatusClientSession::handleWrite(boost::system::error_code ec, std::shared_ptr<StatusClientSession> self)
{
	if (ec.value()) {
		std::cout << "[StatusClientSession] write failed, error message: " << ec.message() << std::endl;
		Close();
		return;
	}
	std::lock_guard<std::mutex> locker(send_mtx_);
	que_.pop();
	if (!que_.empty()) {
		auto node = que_.front();
		boost::asio::async_write(socket_, boost::asio::buffer(node->data_, node->totol_len_),
			std::bind(&StatusClientSession::handleWrite, this, std::placeholders::_1, self));
	}
}
void StatusClientSession::AsyncReadHead()
{
	auto self = shared_from_this();
	boost::asio::async_read(socket_, boost::asio::buffer(head_, HEAD_TOTOL_LEN),
		[self, this](boost::system::error_code ec, std::size_t bytesTransfered) {
			if (ec.value()) {
				std::cout << "[StatusClientSession] read head failed, error message: " << ec.message() << std::endl;
				Close();
				return;
			}
			short msg_id_net = 0;
			short msg_len_net = 0;
			::memcpy(&msg_id_net, head_, HEAD_ID_LEN);
			::memcpy(&msg_len_net, head_ + HEAD_ID_LEN, HEAD_DATA_LEN);
			short msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id_net);
			short msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len_net);
			if (msg_len < 0 || msg_len > MAX_RECV_LENGTH) {
				std::cout << "[StatusClientSession] invalid msg_len = " << msg_len << ", close connection." << std::endl;
				Close();
				return;
			}
			AsyncReadBody(msg_id, msg_len);
		});
}
void StatusClientSession::AsyncReadBody(short msg_id, short msg_len)
{
	auto self = shared_from_this();
	boost::asio::async_read(socket_, boost::asio::buffer(body_, msg_len),
		[self, this, msg_id, msg_len](boost::system::error_code ec, std::size_t bytesTransfered) {
			if (ec.value()) {
				std::cout << "[StatusClientSession] read body failed, error message: " << ec.message() << std::endl;
				Close();
				return;
			}
			std::string msg(body_, msg_len);
			if (msg_id == ID_REGISTER_RSP) {
				std::cout << "[StatusClientSession] receive REGISTER_RSP: " << msg << std::endl;
			}
			else if (msg_id == ID_HEADT_CHECK_RSP) {
				std::cout << "[StatusClientSession] receive HEART_CHECK_RSP from StatusServer." << std::endl;
			}
			else {
				std::cout << "[StatusClientSession] receive unknown msg(id = " << msg_id << "): " << msg << std::endl;
			}
			// 继续读下一条消息
			AsyncReadHead();
		});
}
void StatusClientSession::Close()
{
	if (!connected_) {
		return;
	}
	connected_ = false;
	boost::system::error_code ec;
	socket_.close(ec);
	std::cout << "[StatusClientSession] connection to StatusServer closed." << std::endl;
}
