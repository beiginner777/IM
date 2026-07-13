#include "StatusServiceImpl.h"
#include "ConfigManager.h"
#include "RedisManager.h"
#include "RedisLocker.h"
#include "CServer.h"
#include "CSession.h"

StatusServiceImpl::StatusServiceImpl()
{
	// 不再从 config.ini 加载 ChatServer 列表
	// ChatServer/ResourceServer 地址从 CServer::sessions_ 动态获取
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

Status StatusServiceImpl::GetResourceServer(ServerContext* context, const GetResourceServerReq* request, GetResourceServerRsp* reply)
{
	if (!server_) {
		std::cerr << "[GetResourceServer] CServer not set" << std::endl;
		reply->set_error(ERROR_RPC);
		return Status::OK;
	}

	// 遍历所有 TCP 会话，找 RESOURCE_SERVER 类型的活跃连接
	std::lock_guard<std::mutex> locker(server_->getMutex());
	const auto& sessions = server_->getSessions();

	for (const auto& [uuid, session] : sessions) {
		Server_Info info = session->GetServerInfo();
		if (info.server_type != ServerType::RESOURCE_SERVER) continue;

		reply->set_host(info.host);
		reply->set_port(info.port);
		reply->set_error(SUCCESS);

		std::cout << "[GetResourceServer] Return " << info.name
		          << " (" << info.host << ":" << info.port << ") for uuid=" << request->uuid() << std::endl;
		return Status::OK;
	}

	// 没有可用的 ResourceServer
	std::cerr << "[GetResourceServer] No available ResourceServer" << std::endl;
	reply->set_error(ERROR_RPC);
	return Status::OK;
}

ChatServer StatusServiceImpl::getChatServer()
{
	std::lock_guard<std::mutex> locker_(mtx_);

	ChatServer indexServer;
	indexServer.con_count = INT_MAX;

	// 从 CServer 的 TCP 会话中获取活跃 ChatServer 列表（不再依赖 config.ini）
	const auto& sessions = server_ ? server_->getSessions() : std::map<std::string, std::shared_ptr<CSession>>{};

	if (sessions.empty()) {
		std::cerr << "[GetChatServer] No active sessions, fallback to config.ini" << std::endl;
		// fallback: 从 config.ini 读取（兼容开发环境无 ChatServer 连接时）
		auto cfg = ConfigManager::getInstance();
		auto server_list = cfg["ChatServers"]["List"];
		std::stringstream ss(server_list);
		std::string word;
		while (std::getline(ss, word, ',')) {
			if (cfg[word]["Name"].empty()) continue;
			ChatServer s;
			s.port = cfg[word]["Port"];
			s.host = cfg[word]["Host"];
			s.name = cfg[word]["Name"];
			std::string jsonStr = RedisManager::getInstance()->HGet(CHATSERVERS, s.name);
			if (!jsonStr.empty()) {
				Json::Reader reader;
				Json::Value json;
				if (reader.parse(jsonStr, json)) {
					s.con_count = json["con_count"].asInt();
				}
			}
			if (s.con_count < indexServer.con_count) {
				indexServer = s;
			}
		}
		return indexServer;
	}

	for (const auto& [uuid, session] : sessions) {
		Server_Info info = session->GetServerInfo();
		if (info.server_type != ServerType::CHAT_SERVER) continue;

		// 从 Redis 读取该 ChatServer 的最新连接数（JSON 格式）
		std::string jsonStr = RedisManager::getInstance()->HGet(CHATSERVERS, info.name);
		int con_count = 0;
		if (!jsonStr.empty()) {
			Json::Reader reader;
			Json::Value json;
			if (reader.parse(jsonStr, json)) {
				con_count = json["con_count"].asInt();
			}
		}

		std::cout << "[GetChatServer] " << info.name << " (" << info.host << ":" << info.port
		          << ") con_count=" << con_count << std::endl;

		if (con_count < indexServer.con_count) {
			indexServer.host = info.host;
			indexServer.port = info.port;
			indexServer.name = info.name;
			indexServer.con_count = con_count;
		}
	}

	std::cout << "[" << "StatusServer]: " << indexServer.name << " adds new Connection." << std::endl;
	return indexServer;
}
