#include "StatusServiceImpl.h"
#include "ConfigManager.h"
#include "RedisManager.h"
#include "RedisLocker.h"
#include "CServer.h"
#include "CSession.h"

// 从指定 sessions 中选出连接数最少的 Server（静态工具函数）
static Server_Info getLeastLoadedServer(
	const std::map<std::string, std::shared_ptr<CSession>>& sessions,
	ServerType expectedType,
	const std::string& redisKey);

StatusServiceImpl::StatusServiceImpl()
{
	// 不再从 config.ini 加载 ChatServer 列表
	// ChatServer/ResourceServer 地址从 CServer 动态获取
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	if (!server_)
	{
		std::cerr << "[GetChatServer] CServer not set" << std::endl;
		reply->set_error(ERROR_RPC);
		return Status::OK;
	}

	Server_Info selected =
	        getLeastLoadedServer(server_->getSessions(), ServerType::CHAT_SERVER, CHATSERVERS);

	if (selected.name.empty())
	{
		std::cerr << "[GetChatServer] No available ChatServer" << std::endl;
		reply->set_error(ERROR_RPC);
		return Status::OK;
	}

	reply->set_host(selected.host);
	reply->set_port(selected.port);
	reply->set_error(SUCCESS);

	std::cout << "[GetChatServer] Return " << selected.name << " (" << selected.host << ":" << selected.port << ")"
	          << " con_count=" << selected.con_count << " for " << "GateServer" << std::endl;
	return Status::OK;
}

Status StatusServiceImpl::GetResourceServer(ServerContext* context, const GetResourceServerReq* request, GetResourceServerRsp* reply)
{
	if (!server_) {
		std::cerr << "[GetResourceServer] CServer not set" << std::endl;
		reply->set_error(ERROR_RPC);
		return Status::OK;
	}

	Server_Info selected = getLeastLoadedServer(
		server_->getResourceSessions(),
		ServerType::RESOURCE_SERVER,
		RESOURCESERVERS);

	if (selected.name.empty()) {
		std::cerr << "[GetResourceServer] No available ResourceServer" << std::endl;
		reply->set_error(ERROR_RPC);
		return Status::OK;
	}

	reply->set_host(selected.host);
	reply->set_port(selected.port);
	reply->set_error(SUCCESS);

	std::cout << "[GetResourceServer] Return " << selected.name
	          << " (" << selected.host << ":" << selected.port << ")"
	          << " con_count=" << selected.con_count
	          << " for " << request->chatserver_name() << std::endl;
	return Status::OK;
}

static Server_Info getLeastLoadedServer(
	const std::map<std::string, std::shared_ptr<CSession>>& sessions,
	ServerType expectedType,
	const std::string& redisKey)
{
	Server_Info best;
	best.con_count = INT_MAX;

	for (auto& kv : sessions) {
		auto session = kv.second;
		Server_Info info = session->GetServerInfo();
		if (info.server_type != expectedType) continue;

		// 从 Redis 读取该 Server 的最新连接数（JSON 格式）
		std::string jsonStr = RedisManager::getInstance()->HGet(redisKey, info.name);
		int con_count = 0;
		if (!jsonStr.empty()) {
			Json::Reader reader;
			Json::Value json;
			if (reader.parse(jsonStr, json)) {
				con_count = json["con_count"].asInt();
			}
		}

		std::cout << "[LeastLoaded] " << info.name << " (" << info.host << ":" << info.port
		          << ") con_count=" << con_count << std::endl;

		if (con_count < best.con_count) {
			best.host = info.host;
			best.port = info.port;
			best.name = info.name;
			best.con_count = con_count;
		}
	}

	return best;
}
