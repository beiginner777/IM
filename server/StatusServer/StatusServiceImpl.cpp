#include "StatusServiceImpl.h"
#include "ConfigManager.h"
#include "RedisManager.h"
#include "RedisLocker.h"

StatusServiceImpl::StatusServiceImpl()
{
	// 添加 ChatServer
	auto cfg = ConfigManager::getInstance();
	auto server_list = cfg["ChatServers"]["List"];

	std::vector<std::string> words;

	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}

		ChatServer server;
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];
		servers_[server.name] = server;

		server.printInfo();
	}
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("Jerry StatusServer has received: ");

	ChatServer chatServer = getChatServer();
	reply->set_host(chatServer.host);
	reply->set_port(chatServer.port);
	reply->set_token(generate_unique_string());	

	std::cout << "select ChatServer(" << chatServer.host << ":" << chatServer.port << ") for client whose uid = " << request->uid() << ".\n";

	// 在redis缓存中插入用户的token信息
	RedisManager::getInstance()->Set( USERUIDPREFIX + std::to_string(request->uid()), USERTOKENPREFIX + reply->token());

	reply->set_error(ERROE_CODR::SUCCESS);

	return Status::OK;
}

ChatServer StatusServiceImpl::getChatServer()
{
	std::lock_guard<std::mutex> locker_(mtx_);
	// host(""), port(""), name(""), con_count(0)
	ChatServer indexServer;
	indexServer.con_count = INT_MAX;
	std::string countStr = RedisManager::getInstance()->HGet(LOGINCOUNT, indexServer.name);
	for (auto& s : servers_)
	{
		//std::cout << s.second.con_count << std::endl;
		//std::cout << indexServer.con_count << std::endl;
		//std::cout << indexServer.name << std::endl;
		// 需要先在redis中获取最新的数据
		std::string countStr = RedisManager::getInstance()->HGet(LOGINCOUNT, s.second.name);
		if (countStr == "") {
			std::cout << s.second.name << " 's ConnCount is nullptr." << std::endl;
		}
		s.second.con_count = std::stoi(countStr);
		std::cout << "*******" << s.second.name << " 's connection number is " << s.second.con_count << "*******" << std::endl;
		if (s.second.con_count < indexServer.con_count)
		{
			// 所以需要重载 ChatServer的 bool operator = ()
			std::cout << s.second.name << " < " << indexServer.name << std::endl;
			indexServer = s.second;
		}
	}
	std::cout << "[" << "StatusServer]: " << indexServer.name << " adds new Connection." << std::endl;
	return indexServer;
}

//void StatusServiceImpl::insertToken(int uid, std::string token)
//{
//	// 在redis缓存中插入用户的token信息
//	auto conn = RedisConnPool::getInstance()->getConnection();
//	if (conn == nullptr)
//	{
//		reply->set_error(ERROE_CODR::SUCCESS);
//		return;
//	}
//	reply->set_error(ERROE_CODR::SUCCESS);
//	RedisManager redisManager(conn);
//	redisManager.Set(USERTOKENPREFIX + std::to_string(uid), token);
//	RedisConnPool::getInstance()->returnConnection(conn);
//}

//Status StatusServiceImpl::login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
//{
//	int uid = request->uid();
//	std::string token = request->token();
//
//	std::string keyToken = USERTOKENPREFIX + std::to_string(uid);
//	RedisManager redisManager(RedisConnPool::getInstance()->getConnection());
//	std::string valueToken = redisManager.Get(USERTOKENPREFIX + std::to_string(uid));
//	// 说明在redis缓存当中，没有当前用户的 token
//	if(valueToken == ""){
//	
//		reply->set_error(ERROE_CODR::ERROR_INVALIDUID);
//		return Status::OK;
//	}
//	// token 不匹配
//	if (token != valueToken)
//	{
//		reply->set_error(ERROE_CODR::ERROR_INVALIDTOKEN);
//		return Status::OK;
//	}
//	
//	// token 匹配成功的情况
//	reply->set_error(ERROE_CODR::SUCCESS);
//	reply->set_uid(uid);
//	reply->set_token(token);
//}

