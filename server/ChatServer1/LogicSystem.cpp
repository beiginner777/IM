#include "LogicSystem.h"
#include "MsgNode.h"
#include "CSession.h"
#include "RedisManager.h"
#include "MysqlManager.h"
#include "MessageDeduplicator.h"
#include "UserManager.h"
#include "ChatGrpcClient.h"
#include "CServer.h"
#include "utils.h"
#include "BatchMessageWriter.h"

LogicSystem::LogicSystem() : b_stop_(false)
{
	registerFunctionCallbacks();
	work_thread_ = std::thread(&LogicSystem::dealTask, this);
}

LogicSystem::~LogicSystem()
{
	std::cout << "LogicSystem is destructed." << std::endl;
}

void LogicSystem::dealTask()
{
	while (true)
	{
		std::unique_lock<std::mutex> locker(mtx_);
		
		while (que_.empty() && !b_stop_)
		{
			std::cout << "LoginSystem is waiting for data . . ." << std::endl;
			cond_.wait(locker);
		}

		if (b_stop_)
		{
			while (!que_.empty())
			{
				std::shared_ptr<LogicNode> node = que_.front();
				que_.pop();

				short msgId = node->recvNode_->msg_id_;
				std::string uuid = node->recvNode_->uuid_;
			
				if (handlers_.count(msgId)) {
					std::cout << "handle task whose id = " << msgId << ":" << std::endl;
					handlers_[msgId](node->session_, msgId,
					                 std::string(node->recvNode_->data_, node->recvNode_->totol_len_), uuid);
				}
				else {
					std::cout << "system error: can't find FunctinCallback: " << msgId << std::endl;
				}
			}
			break;
		}

		if (!que_.empty())
		{
			std::shared_ptr<LogicNode> node = que_.front();
			que_.pop();

			short msgId = node->recvNode_->msg_id_;
			std::string uuid = node->recvNode_->uuid_;

			if (handlers_.count(msgId)) {
				std::cout << "handle task whose id = " << msgId << ":" << std::endl;
				handlers_[msgId](node->session_, msgId, std::string(node->recvNode_->data_, node->recvNode_->totol_len_), uuid);
			}
			else {
				std::cout << "system error: can't find FunctinCallback: " << msgId << std::endl;
			}
		}
	}
}

void LogicSystem::registerFunctionCallbacks()
{
	handlers_[ID_REGISTER_REQ] = std::bind(&LogicSystem::registerToStatusServer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_CHAT_LOGIN] = std::bind(&LogicSystem::loginHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::searchHandle, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_APPLY_FRIEND_REQ] = std::bind(&LogicSystem::applyHandle, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::authAccess, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::dealTextChatMsg, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_HEADT_CHECK_REQ] = std::bind(&LogicSystem::heartCheck, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_LOAD_CHAT_THREAD_REQ] = std::bind(&LogicSystem::loadChatList, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_CREATE_PRIVATE_CHAT_THREAD_REQ] = std::bind(&LogicSystem::createPrivateThread, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_LOAD_MORE_FRIEND_REQ] = std::bind(&LogicSystem::loadConnList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_IMAGE_CHAT_MSG_REQ] = std::bind(&LogicSystem::dealImageChatMsg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_LOAD_FRIEND_APPLY_REQ] = std::bind(&LogicSystem::loadFriendApplyList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_REGISTER_RSP] = std::bind(&LogicSystem::registerToStatusServer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handlers_[ID_HEADT_CHECK_RSP] = std::bind(&LogicSystem::heartCheckWithStatusServer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
}

void LogicSystem::registerToStatusServer(std::shared_ptr<CSession> session, short msgId, std::string msgData,
                                         std::string uuid)
{
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(msgData, root)) {
		std::cout << "registerToStatusServer parse json failed." << std::endl;
		return;
	}
	int code = root["code"].asInt();
	std::string msg = root["msg"].asString();

	if (code != SUCCESS) {
		std::cout << "registerToStatusServer failed." << msg << std::endl;
		exit(-1);
	}
	else {
		std::cout << "Register to StatusServer successfully." << std::endl;
		session->server_->startReceiceConnections();
	}
}

bool LogicSystem::isAllDigits(const std::string& str)
{
	return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}

bool LogicSystem::getUserByUid(std::string uid, Json::Value& rtvalue)
{
	rtvalue["code"] = ERROE_CODR::SUCCESS;

	std::string base_key = USERBASEINFO + uid;

	std::string info_str = RedisManager::getInstance()->Get(base_key);

	if (!info_str.empty()) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["password"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		auto icon = root["icon"].asString();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email << " icon is " << icon << std::endl;
		rtvalue["uid"] = uid;
		rtvalue["password"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		rtvalue["icon"] = icon;
		return true;
	}

	std::shared_ptr<UserInfo> userInfo = MysqlManager::getInstance()->getUserByUid(std::atoi(uid.c_str()));

	if (userInfo == nullptr) {
		rtvalue["code"] = ERROE_CODR::ERROR_SEARCH_INVALIDUID;
		return false;
	}

	Json::Value redis_root;
	redis_root["uid"] = userInfo->uid_;
	redis_root["password"] = userInfo->pwd_;
	redis_root["name"] = userInfo->name_;
	redis_root["email"] = userInfo->email_;
	redis_root["nick"] = userInfo->nick_;
	redis_root["desc"] = userInfo->desc_;
	redis_root["sex"] = userInfo->sex_;
	redis_root["icon"] = userInfo->icon_;

	if (!RedisManager::getInstance()->Set(base_key, redis_root.toStyledString())) {
		std::cout << "read user("<< uid << ") to redis failed." << std::endl;
	}

	rtvalue["uid"] = userInfo->uid_;
	rtvalue["password"] = userInfo->pwd_;
	rtvalue["name"] = userInfo->name_;
	rtvalue["email"] = userInfo->email_;
	rtvalue["nick"] = userInfo->nick_;
	rtvalue["desc"] = userInfo->desc_;
	rtvalue["sex"] = userInfo->sex_;
	rtvalue["icon"] = userInfo->icon_;

	return true;
}

bool LogicSystem::getUserByName(std::string name, Json::Value& rtvalue)
{
	rtvalue["code"] = ERROE_CODR::SUCCESS;

	std::string base_key = USERBASEINFO + name;

	std::string info_str = RedisManager::getInstance()->Get(base_key);

	if (!info_str.empty()) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto name = root["name"].asString();
		auto pwd = root["password"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		auto icon = root["icon"].asString();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email << " icon is " << icon << std::endl;

		rtvalue["uid"] = uid;
		rtvalue["name"] = name;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		rtvalue["icon"] = icon;
		return true;
	}

	std::shared_ptr<UserInfo> userInfo = MysqlManager::getInstance()->getUserByName(name);

	if (userInfo == nullptr) {
		rtvalue["code"] = ERROE_CODR::ERROR_SEARCH_INVALIDNAME;
		return false;
	}

	Json::Value redis_root;
	redis_root["uid"] = userInfo->uid_;
	redis_root["password"] = userInfo->pwd_;
	redis_root["name"] = userInfo->name_;
	redis_root["email"] = userInfo->email_;
	redis_root["nick"] = userInfo->nick_;
	redis_root["desc"] = userInfo->desc_;
	redis_root["sex"] = userInfo->sex_;
	redis_root["icon"] = userInfo->icon_;

	if (!RedisManager::getInstance()->Set(base_key, redis_root.toStyledString())) {
		std::cout << "set user(uid) to redis failed." << std::endl;
	}
	
	rtvalue["uid"] = userInfo->uid_;
	rtvalue["name"] = userInfo->name_;
	rtvalue["nick"] = userInfo->nick_;
	rtvalue["desc"] = userInfo->desc_;
	rtvalue["sex"] = userInfo->sex_;
	rtvalue["icon"] = userInfo->icon_;

	return true;
}

void LogicSystem::postMsgToQue(std::shared_ptr<LogicNode> logicNode)
{
	std::lock_guard<std::mutex> locker(mtx_);
	que_.push(logicNode);
	cond_.notify_all();
}

void LogicSystem::dealTextChatMsg(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Reader reader;
	Json::Value root;
	reader.parse(msgData, root);

	int uid = root["fromuid"].asInt();
	int touid = root["touid"].asInt();
	int thread_id = root["thread_id"].asInt();
	const Json::Value arrays = root["text_array"];

	std::cout << "uid = " << session->getUserId() << "request to send TextChatMsg to uid = " << touid << std::endl;

	Json::Value rtvalue;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;
	rtvalue["thread_id"] = thread_id;

	std::vector<std::shared_ptr<ChatMessage>> chat_datas;
	auto redis = RedisManager::getInstance();
	auto batchWriter = BatchMessageWriter::getInstance();

	for (const auto& text_obj : arrays) {
		std::string content = text_obj["content"].asString();
		std::string unique_id = text_obj["unique_id"].asString();

		std::shared_ptr<ChatMessage> chatMsg = std::make_shared<ChatMessage>();
		// Generate distributed message ID (Redis INCR or Snowflake fallback)
		chatMsg->message_id = redis->generateMsgId();
		chatMsg->thread_id = thread_id;
		chatMsg->unique_id = unique_id;
		chatMsg->sender_id = uid;
		chatMsg->recv_id = touid;
		chatMsg->content = content;
		chatMsg->status = 2;
		chatMsg->chat_time = GetCurrentTimestamp();
		chatMsg->type = CHAT_MSG_TYPE::TEXT_MSG;

		// Queue for async batch write (returns immediately, no DB I/O wait)
		batchWriter->enqueue(chatMsg);
		chat_datas.push_back(chatMsg);
	}

	// Build ACK with pre-generated IDs (respond BEFORE DB write completes)
	for (auto& chat_data : chat_datas) {
		Json::Value  chat_msg;
		chat_msg["message_id"] = chat_data->message_id;
		chat_msg["unique_id"] = chat_data->unique_id;
		chat_msg["content"] = chat_data->content;
		chat_msg["status"] = chat_data->status;
		chat_msg["chat_time"] = chat_data->chat_time;
		rtvalue["chat_datas"].append(chat_msg);
	}

	Defer defer([session,this,rtvalue,uuid]() {
		session->Send(rtvalue.toStyledString(), ID_TEXT_CHAT_MSG_RSP, uuid);
	});

	auto to_str = std::to_string(touid);
	auto peer_ip_key = USERIPPREFIX + to_str;

	std::string peerIP = RedisManager::getInstance()->Get(peer_ip_key);

	if (peerIP.empty())
	{
		rtvalue["code"] = ERROR_FIND_PEER_IP;
		rtvalue["messgae"] = "???ip?????????????????.";
		return;
	}

	rtvalue["code"] = ERROE_CODR::SUCCESS;
	rtvalue["message"] = "Send TextChatMsg success.";

	auto cfg = ConfigManager::getInstance();
	auto selfName = cfg["SelfServer"]["Name"];

	std::cout << "PeerIP: " << peerIP << " " << "SelfIP: " << selfName << std::endl;

	rtvalue["text_array"] = arrays;

	if (peerIP == selfName) {
		auto session = UserManager::getInstance()->GetSession(touid);
		if (session) {
			session->Send(rtvalue.toStyledString(), ID_NOTIFY_TEXT_CHAT_MSG_REQ, uuid);
		}
		else
		{
			rtvalue["code"] = ERROE_CODR::ERROR_SEND_MSG_FAILED;
			rtvalue["message"] = "��??????";
			std::cout << "Unknow error: can't find session in memory for uid = " << touid << std::endl;
		}
		return;
	}

	std::cout << "Need to call rpc Service.(??????????)" << std::endl;

	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(uid);
	text_msg_req.set_touid(touid);
	for (const auto& txt_obj : arrays) {
		auto content = txt_obj["content"].asString();
		auto msgid = txt_obj["msgid"].asString();
		std::cout << "content is " << content << std::endl;
		std::cout << "msgid is " << msgid << std::endl;
		auto* text_msg = text_msg_req.add_textmsgs();
		text_msg->set_msgid(msgid);
		text_msg->set_msgcontent(content);
	}

	ChatGrpcClient::getInstance()->NotifyTextChatMsg(peerIP, text_msg_req, rtvalue);
}

void LogicSystem::receiveFriendApply(std::shared_ptr<CSession> session, short msgId, std::string msgData,
                                     std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(msgData, root))
	{
		Json::Value value;
		value["code"] = ERROE_CODR::ERROR_JSON;
		value["message"] = "prase json failed.";
		session->Send(value.toStyledString(), msgId, uuid);
		return;
	}

	int applyuid = root["applyuid"].asInt();
	std::string name = root["name"].asString();
	std::string desc = root["desc"].asString();

	std::cout << "uid = " << session->getUserId()  << " receive FriendApply from uid = " << applyuid << std::endl;

	Json::Value rtvalue;
	bool ret = getUserByUid(std::to_string(applyuid), rtvalue);
	if (!ret) {
		std::cout << "notify Apply to uid = " << session->getUserId() << " failed." << std::endl;
		return;
	}
	
	session->Send(rtvalue.toStyledString(), ID_NOTIFY_ADD_FRIEND_REQ, uuid);
}

void LogicSystem::loginHandle(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Value root;
	Json::Reader reader;
	
	if (!reader.parse(msgData, root))
	{
		Json::Value value;
		value["code"] = ERROE_CODR::ERROR_JSON;
		value["message"] = "prase json failed.";
		session->Send(value.toStyledString(), msgId, uuid);
		return;
	}
	
	int uid = root["uid"].asInt();
	std::string token = root["token"].asString();

	std::cout << "uid = " << uid << " request login ChatServer,token = " << token << std::endl;

	Json::Value value;
	
	std::string TokenValue = RedisManager::getInstance()->Get(USERUIDPREFIX + std::to_string(uid));

	if (TokenValue == "")
	{
		value["code"] = ERROR_INVALIDUID;
		value["message"] = "uid does not exists.";
		session->Send(value.toStyledString(), msgId, uuid);
		return;
	}

	if (TokenValue !=  USERTOKENPREFIX + token)
	{
		value["code"] = ERROR_INVALIDTOKEN;
		value["message"] = "Token does not exists.";
		session->Send(value.toStyledString(), msgId, uuid);
		return;
	}

	std::string lock_key = LOCKPREFIX + std::to_string(uid);
	std::string identifier = RedisManager::getInstance()->acqueireLock(lock_key, LOCK_TIMEOUT, ACQUIRE_TIMEOUT);
	
	std::string ip = RedisManager::getInstance()->Get(USERIPPREFIX + std::to_string(uid));
	auto cfg = ConfigManager::getInstance();
	if (ip == "") {

	}
	else if (ip == cfg["SelfServer"]["Name"]) {
		auto oldSession = UserManager::getInstance()->GetSession(uid);
		if (oldSession) {
			oldSession->notifyOffLine(uid);
			std::cout << "old session = " << oldSession->getUuid() << std::endl;
		}
	}
	else {
		KickUserReq req;
		req.set_uid(uid);
		ChatGrpcClient::getInstance()->NotifyKickUser(ip, req);
	}

	std::string uid_str = std::to_string(uid);
	if (!getUserByUid(uid_str, value)) {
		session->Send(value.toStyledString(), ID_CHAT_LOGIN_RSP, uuid);
		return;
	}

	std::cout << "Get offline notify messages for uid = " << uid << std::endl;
	std::vector<std::string> notify_messages = RedisManager::getInstance()->popOfflineMessages(uid);
	for (std::string& msg : notify_messages) {
		Json::Reader tr;
		Json::Value tv;
		Json::Value vv;
		if (tr.parse(msg, tv)) {
			int redis_id = tv["redis_id"].asInt();
			int friend_id = tv["friend_id"].asInt();
			std::string message = tv["message"].asString();
			std::string friend_icon = tv["friend_icon"].asString();
			vv["redis_id"] = redis_id;
			vv["friend_id"] = friend_id;
			vv["friend_icon"] = friend_icon;
			vv["message"] = message;
			value["notify_messages"].append(vv);
		}
	}

	value["code"] = ERROE_CODR::SUCCESS;
	value["message"] = "Verify success from ChatServer.";
	value["token"] = token;
	std::string name = cfg["SelfServer"]["Name"];
	std::string res = RedisManager::getInstance()->HGet(LOGINCOUNT, name);
	int count = std::stoi(res);
	count++;
	RedisManager::getInstance()->HSet(LOGINCOUNT, name, std::to_string(count));

	session->setUserId(uid);

	std::string ipkey = USERIPPREFIX + std::to_string(uid);
	RedisManager::getInstance()->Set(ipkey, name);

	std::cout << "after user login: " << std::endl;
	UserManager::getInstance()->printSessions();

	UserManager::getInstance()->addSession(uid, session);

	session->Send(value.toStyledString(), ID_CHAT_LOGIN_RSP, uuid);

	std::cout << "after user login: " << std::endl;
	UserManager::getInstance()->printSessions();

	RedisManager::getInstance()->releaseLock(lock_key, identifier);
}

void LogicSystem::authAccess(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Value root;
	Json::Reader reader;
	
	Json::Value rtvalue;

	if (!reader.parse(msgData, root))
	{
		rtvalue["code"] = ERROE_CODR::ERROR_JSON;
		rtvalue["message"] = "prase json failed.";
		session->Send(rtvalue.toStyledString(), ID_AUTH_FRIEND_RSP, uuid);
		return;
	}

	int fromuid = root["fromuid"].asInt();
	int touid = root["touid"].asInt();
	int status = root["status"].asInt();

	std::cout << "uid = " << fromuid << " request to verify FriendApply from uid = " << fromuid << " to uid = " << touid << std::endl;

	int ret = getUserByUid(std::to_string(fromuid),rtvalue);
	if (!ret) {
		rtvalue["code"] = ERROE_CODR::ERROR_AUTH_APPLY;
		rtvalue["message"] = "verify friendApply failed.";
		session->Send(rtvalue.toStyledString(), ID_AUTH_FRIEND_RSP, uuid);
		return;
	}

	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "verify friendApply Success.";
	rtvalue["peeruid"] = fromuid;

	int friend_id = -1;
	if (MysqlManager::getInstance()->setFriendApplyStatus(fromuid, touid, status) != SUCCESS) {
		rtvalue["code"] = ERROE_CODR::ERROR_AUTH_APPLY;
		rtvalue["message"] = "verify friendApply failed.";
	}
	
	int thread_id1 = 0;
	int thread_id2 = 0;
	int friend_id1 = 0;
	int friend_id2 = 0;

	if (status == FRIEND_APPLY::ACCEPTED) {
		rtvalue["status"] = FRIEND_APPLY::ACCEPTED;
		ret = MysqlManager::getInstance()->addFriendRelation(fromuid, touid, thread_id1, thread_id2, friend_id1, friend_id2);
		if (ret != SUCCESS){
			return;
		}
		else {
			rtvalue["status"] = FRIEND_APPLY::ACCEPTED;
			rtvalue["friend_id"] = friend_id2;
			rtvalue["thread_id"] = thread_id1;
			session->Send(rtvalue.toStyledString(), ID_AUTH_FRIEND_RSP, uuid);
		}
	}
	else {
		rtvalue["status"] = FRIEND_APPLY::REFUSED;
		session->Send(rtvalue.toStyledString(), ID_AUTH_FRIEND_RSP, uuid);
		return;
	}

	auto to_str = std::to_string(fromuid);
	auto peer_ip_key = USERIPPREFIX + to_str;
	std::string peerIP = RedisManager::getInstance()->Get(peer_ip_key);
	auto cfg = ConfigManager::getInstance();
	auto selfName = cfg["SelfServer"]["Name"];

	std::cout << "PeerIP: " << peerIP << " " << "SelfIP: " << selfName << std::endl;

	if (peerIP == selfName) {
		auto session = UserManager::getInstance()->GetSession(fromuid);
		if (session) {
			Json::Value notify;
			ret = getUserByUid(std::to_string(touid), notify);
			if (!ret) {
				session->Send(notify.toStyledString(), ID_NOTIFY_ACCESS_VERIFY, uuid);
				return;
			}
			notify["password"] = "";

			notify["code"] = ERROE_CODR::SUCCESS;
			notify["messgae"] = "your friendApply is accepted.";
			notify["friend_id"] = friend_id1;
			notify["thread_id"] = thread_id2;
			
			session->Send(notify.toStyledString(), ID_NOTIFY_ACCESS_VERIFY, uuid);
		}
		
		return;
	}

	std::cout << "Need to call rpc Service.???????????????????" << std::endl;

	AuthFriendReq req;
	req.set_fromuid(fromuid);
	req.set_touid(touid);

	ChatGrpcClient::getInstance()->NotifyAuthFriend(peerIP, req);
}

void LogicSystem::searchHandle(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	std::cout << "uid = " << session->getUserId() << " request to search user." << std::endl;

	Json::Value root;
	Json::Reader reader;

	if (!reader.parse(msgData, root))
	{
		Json::Value value;
		value["code"] = ERROE_CODR::ERROR_JSON;
		value["message"] = "prase json failed.";
		session->Send(value.toStyledString(), msgId, uuid);
		return;
	}

	std::string uid = root["uid"].asString();

	Json::Value value;

	bool searchRes = false;

	if (isAllDigits(uid))
	{
		std::cout << "Client wants to search User( uid = " << uid << " )." << std::endl;
		searchRes = getUserByUid(uid,value);
	}
	else 
	{
		std::cout << "Client wants to search User( name = " << uid << " )." << std::endl;
		searchRes = getUserByName(uid, value);
	}

	if (!searchRes)
	{
		std::cout << "user(" << uid << ") not exist." << std::endl;
	}

	session->Send(value.toStyledString(), ID_SEARCH_USER_RSP, uuid);
}

void LogicSystem::applyHandle(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Reader reader;
	Json::Value root;
	reader.parse(msgData, root);
	auto fromuid = root["fromuid"].asInt();
	auto applyname = root["applyname"].asString();
	auto bakname = root["bakname"].asString();
	auto touid = root["touid"].asInt();
	int status = root["status"].asInt();
	std::cout << "uid =   " << fromuid << " reuquest to apply friend," << " applyname  is " << applyname << " bakname is " << bakname << " touid is " << touid << " status = " << status << std::endl;

	Json::Value  rtvalue;
	
	int current_id = -1;
	std::string apply_time;
	int ret = MysqlManager::getInstance()->addFriendApply(fromuid, touid,current_id, apply_time);
	if(ret == ERROE_CODR::ERROR_MULTIPLE_FRIEND_APPLY)
	{
		rtvalue["code"] = ERROE_CODR::ERROR_MULTIPLE_FRIEND_APPLY;
		rtvalue["message"] = "???????????????";
		session->Send(rtvalue.toStyledString(), ID_APPLY_FRIEND_RSP, uuid);
		return;
	}
	else if (ret == ERROE_CODR::ERROR_FRIEND_APPLY)
	{
		rtvalue["code"] = ERROR_FRIEND_APPLY;
		rtvalue["message"] = "???????????";
		session->Send(rtvalue.toStyledString(), ID_APPLY_FRIEND_RSP, uuid);
		return;
	}

	rtvalue["code"] = ERROE_CODR::SUCCESS;
	rtvalue["messgae"] = "send appply success.";
	session->Send(rtvalue.toStyledString(), ID_APPLY_FRIEND_RSP, uuid);

	auto to_str = std::to_string(touid);
	auto peer_ip_key = USERIPPREFIX + to_str;
	std::string peerIP = RedisManager::getInstance()->Get(peer_ip_key);
	if (peerIP.empty()) {
		std::cout << "user " << touid << " is offline,apply is store in db." << std::endl;
		return;
	}
	else {
		std::cout << "user " << touid << " is online, notifying peer to verify apply ...\n";
	}

	Json::Value v;
	bool searchRes = getUserByUid(std::to_string(fromuid), v);
	if (!searchRes) {
		std::cerr << "[ERROR]: User " << fromuid << " info can not find in redis & mysql.\n";
		return;
	}

	auto cfg = ConfigManager::getInstance();
	auto selfName = cfg["SelfServer"]["Name"];

	if (peerIP == selfName) {
		auto session = UserManager::getInstance()->GetSession(touid);
		if (session) {
			Json::Value notify;
			notify["id"] = current_id;
			notify["fromuid"] = fromuid;
			notify["touid"] = touid;
			notify["name"] = applyname;
			notify["email"] = v["email"].asString();
			notify["icon"] = v["icon"].asString();
			notify["desc"] = v["desc"].asString();
			notify["sex"] = v["sex"].asInt();
			notify["apply_time"] = apply_time;
			notify["status"] = status;
			notify["code"] = ERROE_CODR::SUCCESS;
			notify["message"] = "you have a new friend apply.";
			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ, uuid);
		}
		return;
	}

	std::cout << "Neeed to call rpc Service." << std::endl;

	/*
	message AddFriendReq {
	int32  applyuid = 1;
	string name = 2;
	string desc = 3;
	string icon = 4;
	string nick = 5;
	int32  sex = 6;
	int32  touid = 7;
}
	*/
	AddFriendReq add_req;
	if (searchRes) 
	{	
		add_req.set_applyuid(fromuid);
		add_req.set_touid(touid);
		add_req.set_name(applyname);
		add_req.set_icon(v["icon"].asString());
		add_req.set_sex(v["sex"].asInt());
		add_req.set_desc(v["desc"].asString());
		add_req.set_nick(v["nick"].asString());
	}

	ChatGrpcClient::getInstance()->NotifyAddFriend(peerIP, add_req);
}

void LogicSystem::heartCheck(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Reader reader;
	Json::Value root;
	reader.parse(msgData, root);
	
	int uid = root["uid"].asInt();
	std::cout << "uid = " << session->getUserId() << " request to update heartCheckTime." << std::endl;

	session->setHeartCheckTime(time(NULL));
	
	Json::Value  rtvalue;
	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "update HeartCheckTime Success.\n";
	session->Send(rtvalue.toStyledString(), ID_HEADT_CHECK_RSP, uuid);
}

void LogicSystem::heartCheckWithStatusServer(std::shared_ptr<CSession> session, short msgId, std::string msgData,
                                             std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msgData, root);
	std::string heartUuid = root["uuid"].asString();
	int code = root["code"].asInt();
	std::string msg = root["msg"].asString();

	if (code != SUCCESS) {
		std::cout << "error code = " << code << ",error message = " << msg << std::endl;
	}
	else {
		std::cout << "heart check with status server success, uuid = " << heartUuid << std::endl;
	}
}

void LogicSystem::loadChatList(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Reader reader;
	Json::Value root;
	Json::Value rtvalue;

	Defer defer([session, this, &rtvalue, uuid]() {
		session->Send(rtvalue.toStyledString(), ID_LOAD_CHAT_THREAD_RSP, uuid);
		});

	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "load ChatList Success.";
	rtvalue["load_more"] = true;
	rtvalue["max_thread_id"] = 0;

	if (!reader.parse(msgData, root)) {
		rtvalue["code"] = ERROE_CODR::ERROR_JSON;
		rtvalue["message"] = "prase json failed.";
		rtvalue["load_more"] = false;
		rtvalue["max_thread_id"] = 0;

		std::cout << "[FATAL] load ChatList Failed for uid = " << session->getUserId() << std::endl;

		return;
	}

	int uid = root["uid"].asInt();
	int last_thread_id = root["last_thread_id"].asInt();
	int page_size = root["page_size"].asInt();

	rtvalue["uid"] = uid;

	std::cout << "uid = " << session->getUserId() << " request to load ChatList, last_thread_id = "
		<< last_thread_id << ", page_size = " << page_size << std::endl;

	std::vector<std::shared_ptr<ChatThreadInfo>> chatThreadInfos_;
	bool load_more = false;
	int max_thread_id = 0;
	int res = GetUserThreadInfos(uid, last_thread_id, page_size, chatThreadInfos_, load_more, max_thread_id);
	rtvalue["load_more"] = load_more;
	rtvalue["max_thread_id"] = max_thread_id;

	if(res != SUCCESS) {
		rtvalue["code"] = ERROR_LOAD_CHAT_THREAD;
		rtvalue["message"] = "load ChatList Failed.";
		rtvalue["load_more"] = false;
		rtvalue["max_thread_id"] = last_thread_id;

		std::cout << "[FATAL] load ChatList Failed for uid = " << uid << std::endl;
		return;
	}

	for (auto& info : chatThreadInfos_)
	{
		Json::Value obj;
		obj["thread_id"] = info->threadId_;
		obj["thread_type"] = info->threadType_;
		obj["user1_id"] = info->user1_id_;
		obj["user2_id"] = info->user2_id_;
		rtvalue["threads"].append(obj);
	}
}

int LogicSystem::GetUserThreadInfos(int uid, int last_thread_id, int page_size, std::vector<std::shared_ptr<ChatThreadInfo>>& infos, bool& load_more, int& max_thread_id)
{
	return MysqlManager::getInstance()->GetUserThreadInfos(uid, last_thread_id, page_size, infos, load_more, max_thread_id);
}

bool LogicSystem::AddUserThreadImageMsg(std::string unique_name, std::shared_ptr<ChatMessage> image_data)
{
	if(image_datas_.count(unique_name)) {
		std::cout << "Add image msg, unique_name = " << unique_name << " failed.";
		return false;
	}
	image_datas_[unique_name] = image_data;
	std::cout << "Add image msg, unique_name = " << unique_name << " success.";
	return true;
}

bool LogicSystem::RemoveUserThreadImageMsg(std::string unique_name)
{
	if (image_datas_.count(unique_name) == 0) {
		std::cout << "Remove image msg, unique_name = " << unique_name << " failed.";
		return false;
	}
	image_datas_.erase(unique_name);
	std::cout << "Remove image msg, unique_name = " << unique_name << " success.";
	return true;
}

void LogicSystem::createPrivateThread(std::shared_ptr<CSession> session, short msgId, std::string msgData,
                                      std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Reader reader;
	Json::Value root;
	Json::Value rtvalue;

	Defer defer([session, this, &rtvalue, uuid]() {
		session->Send(rtvalue.toStyledString(), ID_CREATE_PRIVATE_CHAT_THREAD_RSP, uuid);
		});

	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "Create PrivateChat Success.";
	rtvalue["thread_id"] = 0;

	if (!reader.parse(msgData, root)) {
		rtvalue["code"] = ERROE_CODR::ERROR_CREATE_PRIVATE_THREAD;
		rtvalue["message"] = "prase json failed.";
		rtvalue["thread_id"] = 0;
		std::cout << "uid = "  << session->getUserId() << " create PrivateChat failed " << std::endl;
		return;
	}

	std::cout << "uid = " << session->getUserId() << " create PrivateChat success. " << std::endl;

	int user1_id = root["user1_id"].asInt();
	int user2_id = root["user2_id"].asInt();
	int thread_id = 0;

	int ret = MysqlManager::getInstance()->createPrivateThread(user1_id, user2_id, thread_id);
	if(ret != SUCCESS) {
		rtvalue["code"] = ERROR_CREATE_PRIVATE_THREAD;
		rtvalue["message"] = "Create PrivateChat Failed.";
		rtvalue["thread_id"] = 0;
		std::cout << "uid = " << session->getUserId() << " create PrivateChat failed " << std::endl;
		return;
	}

	rtvalue["thread_id"] = thread_id;
	rtvalue["create_uid"] = user1_id;
	rtvalue["peer_uid"] = user2_id;
}

void LogicSystem::loadConnList(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Reader reader;
	Json::Value root;
	Json::Value rtvalue;

	Defer defer([session, this, &rtvalue, uuid]() {
		session->Send(rtvalue.toStyledString(), ID_LOAD_MORE_FRIEND_RSP, uuid);
		});

	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "Load FriendList Success.";
	rtvalue["max_friend_id"] = 0;

	if (!reader.parse(msgData, root)) {
		rtvalue["code"] = ERROE_CODR::ERROR_CREATE_PRIVATE_THREAD;
		rtvalue["message"] = "prase json failed.";
		rtvalue["max_friend_id"] = 0;

		std::cout << "uid = " << session->getUserId() << " load friendlist failed " << std::endl;

		return;
	}

	int uid = root["uid"].asInt();
	int last_friend_id = root["last_friend_id"].asInt();

	std::cout << "uid = " << uid << " load friendlist success. " << std::endl;
	std::cout << "last_friend_id = " << last_friend_id << std::endl;

	std::map<int,std::shared_ptr<UserInfo>> friendList;
	int ret = MysqlManager::getInstance()->getUserFriendListByLastId(uid, last_friend_id, friendList);
	if (ret != SUCCESS) {
		rtvalue["code"] = ERROE_CODR::ERROR_LOAD_MORE_FRIEND;
		rtvalue["message"] = "load more friend faliled.";
		rtvalue["max_friend_id"] = last_friend_id;
		return;
	}
	
	Json::Value friendsArray;
	int max_id = last_friend_id;
	for (auto fr : friendList) {
		Json::Value obj;
		obj["id"] = fr.first;
		obj["uid"] = fr.second->uid_;
		obj["name"] = fr.second->name_;
		obj["nick"] = fr.second->nick_;
		obj["desc"] = fr.second->desc_;
		obj["sex"] = fr.second->sex_;
		obj["email"] = fr.second->email_;
		obj["icon"] = fr.second->icon_;
		friendsArray.append(obj);

		max_id = std::max(max_id, fr.first);
	}
	rtvalue["friends"] = friendsArray;
	rtvalue["max_friend_id"] = max_id;
}

void LogicSystem::dealImageChatMsg(std::shared_ptr<CSession> session, short msgId, std::string msgData,
                                   std::string uuid)
{
	Json::Value root;
	Json::Reader reader;
	Json::Value rtvalue;

	Defer defer([session, this, &rtvalue, uuid]() {
		session->Send(rtvalue.toStyledString(), ID_IMAGE_CHAT_MSG_RSP, uuid);
		});

	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "Send ImageChatMsg Success.";

	if (!reader.parse(msgData, root)) {
		rtvalue["code"] = ERROE_CODR::ERROR_JSON;
		rtvalue["message"] = "prase json failed.";
		std::cout << "uid = " << session->getUserId() << " deal image message failed " << std::endl;
		return;
	}

	int fromuid = root["fromuid"].asInt();
	int touid = root["touid"].asInt();
	int thread_id = root["thread_id"].asInt();
	std::string md5 = root["md5"].asString();
	std::string file_name = root["name"].asString();
	std::string token = root["token"].asString();
	std::string unique_id = root["unique_id"].asString();
	int type = root["type"].asInt();

	std::cout << "uid = " << fromuid << " deal image message which sent to " << touid << " success. " << std::endl;

	std::vector<std::shared_ptr<ChatMessage>> image_msgs;
	std::shared_ptr<ChatMessage> chat_msg = std::make_shared<ChatMessage>();
	// Generate distributed message ID
	chat_msg->message_id = RedisManager::getInstance()->generateMsgId();
	chat_msg->sender_id = fromuid;
	chat_msg->recv_id = touid;
	chat_msg->thread_id = thread_id;
	chat_msg->unique_id = unique_id;
	chat_msg->chat_time = GetCurrentTimestamp();
	chat_msg->content = file_name;
	chat_msg->status = MsgStatus::UN_READ;
	chat_msg->type = static_cast<CHAT_MSG_TYPE>(type);
	image_msgs.push_back(chat_msg);

	// Async batch write (returns immediately, no DB I/O wait)
	BatchMessageWriter::getInstance()->enqueue(chat_msg);

	this->AddUserThreadImageMsg(file_name, chat_msg);

	for(auto msg : image_msgs) {
		rtvalue["message_id"] = msg->message_id;
	}
	rtvalue["send_id"] = fromuid;
	rtvalue["recv_id"] = touid;
	rtvalue["thread_id"] = thread_id;
	rtvalue["unique_id"] = unique_id;
	rtvalue["chat_time"] = chat_msg->chat_time;
	rtvalue["unique_name"] = file_name;
	rtvalue["status"] = MsgStatus::UN_READ;
	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "send new image message success.";
	rtvalue["type"] = type;
}

void LogicSystem::loadFriendApplyList(std::shared_ptr<CSession> session, short msgId, std::string msgData,
                                      std::string uuid)
{
	std::cout << "handle id = " << msgId;

	Json::Value root;
	Json::Reader reader;
	Json::Value rtvalue;
	
	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "load friendApply list Success.";

	Defer defer([session,this,&rtvalue,uuid]() {
			session->Send(rtvalue.toStyledString(), ID_LOAD_FRIEND_APPLY_RSP, uuid);
		});	

	if (!reader.parse(msgData, root)) {
		rtvalue["code"] = ERROE_CODR::ERROR_JSON;
		rtvalue["message"] = "prase json failed.";
		return;
	}

	int uid = root["uid"].asInt();
	int last_friend_apply_id = root["last_friend_apply_id"].asInt();
	int page_size = root["page_size"].asInt();

	int max_friend_apply_id = last_friend_apply_id;
	std::vector<std::shared_ptr<ApplyInfo>> applyList;
	bool is_load_more = false;

	int ret = MysqlManager::getInstance()->getUserFriendApplyByLastId(uid, last_friend_apply_id, page_size, applyList, is_load_more, max_friend_apply_id);

	if (ret != SUCCESS) {
		std::cout << "[ERROR]: Get FriendApply List.";
		rtvalue["code"] = ERROE_CODR::ERROR_GET_FRIEND_APPLY_LIST;
		rtvalue["message"] = "Get Friend Apply List failed.";
		return;
	}
	
	rtvalue["is_load_more"] = is_load_more;
	rtvalue["last_friend_apply_id"] = max_friend_apply_id;
	
	for (auto& apply : applyList) {
		Json::Value value;
		value["id"] = apply->id_;
		value["uid"] = apply->uid_;
		value["name"] = apply->name_;
		value["email"] = apply->email_;
		value["desc"] = apply->desc_;
		value["icon"] = apply->icon_;
		value["sex"] = apply->sex_;
		value["apply_time"] = apply->apply_time_;
		value["status"] = apply->status_;
		rtvalue["apply_friend_list"].append(value);
	}
}

void LogicSystem::loadChatMsg(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	Json::Value root;
	Json::Reader reader;
	Json::Value rtvalue;

	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "Load ChatMessage Success.";

	Defer defer([session, this, &rtvalue, uuid]() {
		session->Send(rtvalue.toStyledString(), ID_LOAD_CHAT_MSG_RSP, uuid);
		});

	if (!reader.parse(msgData, root)) {
		rtvalue["code"] = ERROE_CODR::ERROR_JSON;
		rtvalue["message"] = "prase json failed.";
		return;
	}

	int uid = root["uid"].asInt();
	int peerUid = root["peerUid"].asInt();
	int thread_id = root["thread_id"].asInt();
	int page_size = root["page_size"].asInt();
	int min_message_id = root["min_message_id"].asInt();
	int max_message_id = root["max_message_id"].asInt();
	
	if (thread_id == 1) {
		std::cout << "thread_id = 1 . . . . . . . . . . . . . .. . ";
	}

	std::cout << "uid = " << uid << " thread_id = " << thread_id << " request to load chatmessage.(min_message_id = " 
		<< min_message_id << ",max_message_id = " << max_message_id << ").\n";

	bool is_load_more = true;
	std::vector<ChatMessage> msgs;
	int ret = MysqlManager::getInstance()->loadChatMessage(thread_id , min_message_id, max_message_id, page_size, is_load_more, msgs);
	if (ret != SUCCESS) {
		std::cout << "[ERROR]: Get ChatMessage.";
		rtvalue["code"] = ERROE_CODR::ERROR_LOAD_CHAT_MESSAGE;
		rtvalue["message"] = "Get ChatMessage failed.";
		return;
	}
	
	for (auto& msg : msgs) {
		Json::Value value;
		value["message_id"] = msg.message_id;
		value["thread_id"] = msg.thread_id;
		value["sender_id"] = msg.sender_id;
		value["recv_id"] = msg.recv_id;
		value["content"] = msg.content;
		value["chat_time"] = msg.chat_time;
		value["status"] = msg.status;
		value["message_type"] = msg.type;
		rtvalue["chat_messages"].append(value);
	}
	rtvalue["uid"] = uid;
	rtvalue["peerUid"] = peerUid;
	rtvalue["is_load_more"] = is_load_more;
	rtvalue["min_message_id"] = min_message_id;
	rtvalue["max_message_id"] = max_message_id;
	rtvalue["thread_id"] = thread_id;
}

std::shared_ptr<ChatMessage> LogicSystem::GetUserThreadImageMsg(std::string unique_name)
{
	if (image_datas_.count(unique_name) != 0) {
		return image_datas_[unique_name];
	}
	return nullptr;
}
