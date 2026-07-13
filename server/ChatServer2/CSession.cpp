#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include "RedisManager.h"
#include "MessageDeduplicator.h"

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	: ioc_(ioc)
	, socket_(ioc)
	, server_(server)
	, userId_(0)
	, b_close_(false)
    , recv_head_node_(std::make_shared<RecvNode>(HEAD_TOTOL_LEN_WITH_UUID, -1, ""))
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
	AsyncReadHead(HEAD_TOTOL_LEN_WITH_UUID);
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
	server_->clearSession(uuid_);
	auto cfg = ConfigManager::getInstance();
	std::string selfServer = cfg["SelfServer"]["Name"];
	std::string jsonStr = RedisManager::getInstance()->HGet(CHATSERVERS, selfServer);
	Json::Reader reader;
	Json::Value json;
	int con_count = 0;
	if (reader.parse(jsonStr, json)) {
		con_count = json["con_count"].asInt();
		con_count = std::max(0, con_count - 1);
		json["con_count"] = con_count;
		RedisManager::getInstance()->HSet(CHATSERVERS, selfServer, json.toStyledString());
	}

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
		return;
	}

	auto sendnode = que_.front();
	boost::asio::async_write(socket_, boost::asio::buffer(sendnode->data_, sendnode->totol_len_),
		std::bind(&CSession::handleWrite, this, std::placeholders::_1, shared_from_this()));
}

void CSession::Send(std::string msg, short msgid, std::string uuid)
{
	std::cout << "To session(" << this->getUuid() << ") return id = " << msgid << " return message = " << msg << std::endl;
	Send(msg.c_str(), msg.length(), msgid);

	if (!uuid.empty()) {
		auto dedup = MessageDeduplicator::getInstance();
		dedup->cacheResult(uuid, msg);
	}
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
	double t = std::difftime(now, heartCheckTime_);
	return t > HEART_CHECK_OVERTIME;
}

void CSession::AsyncReadHead(std::size_t len)
{
	auto self = shared_from_this();
	AsyncReadFull(HEAD_TOTOL_LEN_WITH_UUID, [self,this](boost::system::error_code ec,std::size_t bytesTransfered)
	{
		if (ec) {
			std::cout << "Read MessageHead failed." << std::endl;
			Close();
			return;
		}

		recv_head_node_->clear();
		memcpy(recv_head_node_->data_, data_, bytesTransfered);

		std::string msg_uuid(recv_head_node_->data_, HEAD_UUID_LEN);
		msg_uuid.erase(msg_uuid.find('\0'));
		std::cout << "msg_uuid = " << msg_uuid << std::endl;
		
		short msg_id_net = 0;
		memcpy(&msg_id_net, recv_head_node_->data_ + HEAD_UUID_LEN, HEAD_ID_LEN);
		std::cout << "msg_id_net = " << msg_id_net << std::endl;
		short msg_id_host = boost::asio::detail::socket_ops::network_to_host_short(msg_id_net);
		std::cout << "msg_id_host = " << msg_id_host << std::endl;

		if (msg_id_host > MAX_MSG_ID)
		{
			std::cout << "invalid msg_id ." << std::endl;
			Close();
			return;
		}

		short msg_len_net = 0;
		memcpy(&msg_len_net, recv_head_node_->data_ + HEAD_UUID_LEN + HEAD_ID_LEN, HEAD_DATA_LEN);
		std::cout << "msg_len_net = " << msg_len_net << std::endl;
		short msg_len_host = boost::asio::detail::socket_ops::network_to_host_short(msg_len_net);
		std::cout << "msg_len_host = " << msg_len_host << std::endl;

		if (msg_len_host > MAX_MSG_LEN){
			std::cout << "invalid msg_len ." << std::endl;
			Close();
			return;
		}

		recv_msg_node_ = std::make_shared<RecvNode>(msg_len_host, msg_id_host, msg_uuid);

		AsyncReadBody(msg_len_host);
		});
}

void CSession::AsyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code, std::size_t)> handler)
{
	::memset(data_, 0, maxLength);
	AsyncReadLen(0, maxLength, handler);
}

void CSession::AsyncReadLen(std::size_t readLen, std::size_t totolLen, std::function<void(const boost::system::error_code, std::size_t)> handler)
{
	auto self = shared_from_this();
	socket_.async_read_some(boost::asio::buffer(data_ + readLen, totolLen - readLen), [readLen, totolLen, handler, self](boost::system::error_code ec, std::size_t bytesTransfered) {
		if (ec) {
			handler(ec, readLen + bytesTransfered);
			return;
		}
		
		if (readLen + bytesTransfered >= totolLen)
		{
			handler(ec, readLen + bytesTransfered);
			return;
		}

		self->AsyncReadLen(readLen + bytesTransfered, totolLen, handler);
		});
}

void CSession::AsyncReadBody(std::size_t len)
{
	auto self = shared_from_this();
	AsyncReadFull(len, [&](boost::system::error_code ec,std::size_t bytesTransfered) {
		if (ec)
		{
			Close();
			return;
		}

		::memcpy(recv_msg_node_->data_, data_, bytesTransfered);
		recv_msg_node_->cur_len_ += bytesTransfered;
		recv_msg_node_->data_[recv_msg_node_->totol_len_] = '\0';

		std::cout << "RecvNode is " << recv_msg_node_->data_ << std::endl;

		auto dedup = MessageDeduplicator::getInstance();
		if (!recv_msg_node_->uuid_.empty() && dedup->isDuplicate(recv_msg_node_->uuid_)) {
			std::cout << "Duplicate uuid = " << recv_msg_node_->uuid_ << ", returning cached ACK." << std::endl;
			std::string cachedAck = dedup->getCachedAck(recv_msg_node_->uuid_);
			if (!cachedAck.empty()) {
				Send(cachedAck, recv_msg_node_->msg_id_);
			}
			AsyncReadHead(HEAD_TOTOL_LEN_WITH_UUID);
			return;
		}

	std::shared_ptr<LogicNode> logic_node_ = std::make_shared<LogicNode>(shared_from_this(), recv_msg_node_);
		LogicSystem::getInstance()->postMsgToQue(logic_node_);

		AsyncReadHead(HEAD_TOTOL_LEN_WITH_UUID);
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
