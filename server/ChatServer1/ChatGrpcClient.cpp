#include "ChatGrpcClient.h"
#include "ConfigManager.h"
#include "ChatServiceImpl.h"
#include "RedisManager.h"
#include "MysqlManager.h"
ChatGrpcClient::ChatGrpcClient()
{
	ConfigManager cfg = ConfigManager::getInstance();
	std::string servers = cfg["PeerServers"]["Servers"];
	
	std::stringstream ss(servers);
	std::vector<std::string> words;
	std::string word;
	while (std::getline(ss, word, ','))
	{
		words.push_back(word);
	}
	for (auto& word : words)
	{
		std::string name = cfg[word]["Name"];
		std::string host = cfg[name]["Host"];
		std::string port = cfg[name]["RPCPort"];// 注意：这里应该连接的是 RPC 端口
		// c++14 推出的 make_unique
		pools_[name] = std::make_unique<ChatConnPool>(DEFAULT_CHATCONNPOOL_SIZE, host, port);
	}
}
ChatGrpcClient::~ChatGrpcClient()
{
}
AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req)
{
	AddFriendRsp rsp;
	// 首先找到对应在所在的连接池
	auto it = pools_.find(server_ip);
	if (it == pools_.end()) {
		// 不存在
		return rsp;
	}
	auto& pool = it->second;
	ClientContext context;
	auto stub = pool->getConnection();
	Status status = stub->NotifyAddFriend(&context, req, &rsp);
	pool->returnConnection(std::move(stub));
	if (!status.ok()) {
		rsp.set_error(ERROR_RPC_VISIT_CHATSERVER);
		return rsp;
	}
	return rsp;
}
AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req)
{
	AuthFriendRsp rsp;
	// 首先找到对应在所在的连接池
	auto it = pools_.find(server_ip);
	if (it == pools_.end()) {
		rsp.set_error(ERROE_CODR::ERROR_FIND_CONN_IN_RPCPOOLS);
		return rsp;
	}
	auto& pool = it->second;
	ClientContext context;
	auto stub = pool->getConnection();
	Status status = stub->NotifyAuthFriend(&context, req, &rsp);
	pool->returnConnection(std::move(stub));
	if (!status.ok()) {
		rsp.set_error(ERROR_RPC_VISIT_CHATSERVER);
		return rsp;
	}
	return rsp;
}
bool ChatGrpcClient::GetBaseInfo(int uid, std::shared_ptr<UserInfo>& userinfo)
{
	std::string base_key = USERBASEINFO + uid;
	//优先查redis中查询用户信息
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
		userinfo->uid_ = uid;
		userinfo->name_ = name;
		userinfo->nick_ = nick;
		userinfo->desc_ = desc;
		userinfo->sex_ = sex;
		userinfo->icon_ = icon;
		return true;
	}
	//查询mysql数据库
	std::shared_ptr<UserInfo> userInfo = MysqlManager::getInstance()->getUserByUid(uid);
	if (userInfo == nullptr) {
		std::cout << "Can't find uid = " << uid << " in Mysql.";
		return false;
	}
		//将数据库内容写入redis缓存
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
		std::cout << "read user(uid) to redis failed." << std::endl;
	}
	//返回数据
	userinfo->uid_ = userInfo->uid_;
	userinfo->name_ = userInfo->name_;
	userinfo->nick_ = userInfo->nick_;
	userinfo->desc_ = userInfo->desc_;
	userinfo->sex_ = userInfo->sex_;
	userinfo->icon_ = userInfo->icon_;
	return true;
}
TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue)
{
	TextChatMsgRsp rsp;
	
	rsp.set_fromuid(req.fromuid());
	rsp.set_touid(req.touid());
	for (const auto& text_data : req.textmsgs()) {
		TextChatData* new_msg = rsp.add_textmsgs();
		new_msg->set_msgid(text_data.msgid());
		new_msg->set_msgcontent(text_data.msgcontent());
	}
	auto it = pools_.find(server_ip);
	if (it == pools_.end()) {
		rsp.set_error(ERROE_CODR::ERROR_FIND_CONN_IN_RPCPOOLS);
		return rsp;
	}
	std::cout << "find " << server_ip << " in pools.";
	auto& pool = it->second;
	ClientContext context;
	if (pool) {
		auto stub = std::move(pool->getConnection());
		Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
		pool->returnConnection(std::move(stub));
		if (!status.ok()) {
			rsp.set_error(ERROR_RPC_VISIT_CHATSERVER);
			return rsp;
		}
		return rsp;
	}
	else
	{
		std::cout << "Pool is nullptr." << std::endl;
	}
	
}
KickUserRsp ChatGrpcClient::NotifyKickUser(std::string server_ip, const KickUserReq& req)
{
	KickUserRsp rsp;
	// 首先找到对应在所在的连接池
	auto it = pools_.find(server_ip);
	if (it == pools_.end()) {
		rsp.set_error(ERROE_CODR::ERROR_FIND_CONN_IN_RPCPOOLS);
		return rsp;
	}
	auto& pool = it->second;
	ClientContext context;
	auto stub = pool->getConnection();
	Status status = stub->NotifyKickUser(&context, req, &rsp);
	pool->returnConnection(std::move(stub));
	if (!status.ok()) {
		rsp.set_error(ERROR_RPC_VISIT_CHATSERVER);
		return rsp;
	}
	return rsp;
}
