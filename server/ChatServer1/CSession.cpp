#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include "RedisManager.h"

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	: ioc_(ioc)
	, socket_(ioc)
	, server_(server)
	, userId_(0)
	, b_close_(false)
	, recv_head_node_ (std::make_shared<RecvNode>(HEAD_TOTOL_LEN,-1))
{
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	uuid_ = boost::uuids::to_string(a_uuid);
	std::cout << "Session " << uuid_ << " is constructed. " << std::endl;
}

CSession::~CSession()
{
	std::cout << "CSession(uid = " << userId_ << ") is destructed. " << std::endl;
}

void CSession::start()
{
	std::cout << "session: " << uuid_ << " is started ." << std::endl;
	heartCheckTime_ = time(NULL);
	AsyncReadHead(HEAD_TOTOL_LEN);
}

void CSession::Close()
{
	if(this->uuid_ == server_->getConnectionToStatusServerUuid()) {
		std::cout << "Connection to StatusServer is closed." << std::endl;
		std::lock_guard<std::mutex> locker_(mtx_);
		socket_.close();
		b_close_ = true;
		return;
	}

	std::cout << "Session(uuid = " << uuid_ << " ) is closed, userId = " << userId_ << std::endl;
	std::lock_guard<std::mutex> locker_(mtx_);
	socket_.close();
	b_close_ = true;
	// 将相关的信息给清除
	//std::string identifier = RedisManager::getInstance()->acqueireLock(LOCKPREFIX + std::to_string(userId_), LOCK_TIMEOUT, ACQUIRE_TIMEOUT);
	server_->clearSession(uuid_);
	auto cfg = ConfigManager::getInstance();
	std::string selfServer = cfg["SelfServer"]["Name"];
	std::string sessionCount = RedisManager::getInstance()->HGet(LOGINCOUNT, selfServer);
	int sessionCount_int = atoi(sessionCount.c_str());
	RedisManager::getInstance()->HSet(LOGINCOUNT, selfServer, std::to_string(sessionCount_int - 1));
	//RedisManager::getInstance()->Del(USERUIDPREFIX + std::to_string(userId_));
	//RedisManager::getInstance()->Del(USERIPPREFIX + std::to_string(userId_));
	//RedisManager::getInstance()->Del(USERTOKENPREFIX + std::to_string(userId_));
	//RedisManager::getInstance()->releaseLock(LOCKPREFIX + std::to_string(userId_), identifier);

	if (this->getUserId() == 0) {
		std::cout << "Error occurred when clear session information, because userId is 0." << std::endl;
	}

	std::cout << "Clear Session(uid = "<< this->getUserId() << " )Information in redis & Server & UserManager" << std::endl;

	return;
}

void CSession::Send(const char* msg, size_t max_length, short msgid)
{
	std::lock_guard<std::mutex> locker_(mtx_);

	if (que_.size() > MAX_SENDQUEUE_SIZE)
	{
		std::cout << "session: " << uuid_ << " send que fulled, size is " << MAX_SENDQUEUE_SIZE << std::endl;
		return;
	}

	que_.push(std::make_shared<SendNode>(msg, max_length, msgid));

	if (que_.size() > 1){
		// 说明当前有结点正在发送
		return;
	}

	auto sendnode = que_.front();
	boost::asio::async_write(socket_, boost::asio::buffer(sendnode->data_, sendnode->totol_len_),
		std::bind(&CSession::handleWrite, this, std::placeholders::_1, shared_from_this()));

	/*if(sendnode->GetMsgId() == ID_NOTIFY_OFFLINE){
		boost::asio::async_write(socket_, boost::asio::buffer(sendnode->data_, sendnode->totol_len_),
			std::bind(&CSession::handleNotifyOffLine, this, std::placeholders::_1, shared_from_this()));
	}
	else {
		boost::asio::async_write(socket_, boost::asio::buffer(sendnode->data_, sendnode->totol_len_),
			std::bind(&CSession::handleWrite, this, std::placeholders::_1, shared_from_this()));
	}*/
}

void CSession::Send(std::string msg, short msgid)
{
	std::cout << "To session(" << this->getUuid() << ") return id = " << msgid << " return message = " << msg << std::endl;
	Send(msg.c_str(), msg.length(), msgid);
}

void CSession::notifyOffLine(int uid)
{
	this->Send("", ID_NOTIFY_OFFLINE);
}

void CSession::setHeartCheckTime(time_t tm)
{
	std::lock_guard<std::mutex> locker(timeMtx_);
	heartCheckTime_ = tm;
}

bool CSession::isHeartOverTime()
{
	std::lock_guard<std::mutex> locker(timeMtx_);
	time_t now = time(NULL);
	double t = std::difftime(now, heartCheckTime_);// (单位：s)
	return t > HEART_CHECK_OVERTIME;
}

void CSession::AsyncReadHead(std::size_t len)
{
	auto self = shared_from_this();
	AsyncReadFull(HEAD_TOTOL_LEN, [self,this](boost::system::error_code ec,std::size_t bytesTransfered) 
	{
		if (ec) {
			// 出现错误
			std::cout << "Read MessageHead failed." << std::endl;
			Close();
			return;
		}

		// 读取完成,将读取的数据放在RecvNode结点当中
		recv_head_node_->clear();
		memcpy(recv_head_node_->data_, data_, bytesTransfered); // 读取完成的时候，bytesTransfered 就一定等于目的消息长度
		
		// 获取头部的 消息id （此时是网络字节序）
		short msg_id_net = 0;
		memcpy(&msg_id_net, recv_head_node_->data_, HEAD_ID_LEN);
		std::cout << "msg_id_net = " << msg_id_net << std::endl;
		// 将 消息id 转化为 主机字节序
		short msg_id_host = boost::asio::detail::socket_ops::network_to_host_short(msg_id_net);
		std::cout << "msg_id_host = " << msg_id_host << std::endl;

		// 消息id非法。直接断开连接
		if (msg_id_host > MAX_MSG_ID)
		{
			std::cout << "invalid msg_id ." << std::endl;
			Close();
			return;
		}

		// 获取头部的 消息长度（此时是网络字节序）
		short msg_len_net = 0;
		memcpy(&msg_len_net, recv_head_node_->data_ + HEAD_ID_LEN, HEAD_DATA_LEN);
		std::cout << "msg_len_net = " << msg_len_net << std::endl;
		// 将 消息长度 转化为 主机字节序
		short msg_len_host = boost::asio::detail::socket_ops::network_to_host_short(msg_len_net);
		std::cout << "msg_len_host = " << msg_len_host << std::endl;

		// 消息长度非法。直接断开连接
		if (msg_len_host > MAX_MSG_LEN)
		{
			std::cout << "invalid msg_len ." << std::endl;
			Close();
			return;
		}

		recv_msg_node_ = std::make_shared<RecvNode>(msg_len_host, msg_id_host);

		// 头部读取完成，开始读取 消息主体
		AsyncReadBody(msg_len_host);
		});
}

void CSession::AsyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code, std::size_t)> handler)
{
	// 清空对应的缓冲区
	::memset(data_, 0, maxLength);
	AsyncReadLen(0, maxLength, handler);
}

void CSession::AsyncReadLen(std::size_t readLen, std::size_t totolLen, std::function<void(const boost::system::error_code, std::size_t)> handler)
{
	auto self = shared_from_this();
	// 捕获 self 的目的之一是 延长生命周期
	socket_.async_read_some(boost::asio::buffer(data_ + readLen, totolLen - readLen), [readLen, totolLen, handler, self](boost::system::error_code ec, std::size_t bytesTransfered) {
		if (ec) {
			// 出现错误
			handler(ec, readLen + bytesTransfered);
			return;
		}
		
		// 消息读取完成，调用回调函数
		if (readLen + bytesTransfered >= totolLen)
		{
			handler(ec, readLen + bytesTransfered);
			return;
		}

		// 没有出现错误，但是还没有读取完成，则继续调用当前函数
		self->AsyncReadLen(readLen + bytesTransfered, totolLen, handler);
		});
}

void CSession::AsyncReadBody(std::size_t len)
{
	auto self = shared_from_this();
	// 延长 session 的生命周期
	AsyncReadFull(len, [&](boost::system::error_code ec,std::size_t bytesTransfered) {
		if (ec)
		{
			Close();
			return;
		}

		// 读取完成，将数据放在 RecvNode 结点当中
		::memcpy(recv_msg_node_->data_, data_, bytesTransfered);
		recv_msg_node_->cur_len_ += bytesTransfered;
		recv_msg_node_->data_[recv_msg_node_->totol_len_] = '\0';

		std::cout << "RecvNode is " << recv_msg_node_->data_ << std::endl;

		// 将 RecvNode 结点放入 逻辑队列当中
		std::shared_ptr<LogicNode> logic_node_ = std::make_shared<LogicNode>(shared_from_this(), recv_msg_node_);
		LogicSystem::getInstance()->postMsgToQue(logic_node_);

		// 继续 接收头部消息
		AsyncReadHead(HEAD_TOTOL_LEN);
		});
}

void CSession::handleWrite(boost::system::error_code ec,std::shared_ptr<CSession> session)
{
	if (ec) {
		Close();
		return;
	}
	std::lock_guard<std::mutex> locket(mtx_);
	que_.pop();
	if (!que_.empty()){
		auto& sendnode = que_.front();
		boost::asio::async_write(socket_, boost::asio::buffer(sendnode->data_, sendnode->totol_len_),
			std::bind(&CSession::handleWrite, this, std::placeholders::_1, shared_from_this()));
	}

}

void CSession::handleNotifyOffLine(boost::system::error_code ec, std::shared_ptr<CSession> session)
{
	session->Close();
}
