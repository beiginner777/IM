#include "ResouceServerClient.h"
#include "ConfigManager.h"

ResouceServerClient::ResouceServerClient()
{
	ConfigManager cfg = ConfigManager::getInstance();

	std::string servers = cfg["ChatServers"]["Servers"];

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

NotifyChatServerImgRsp ResouceServerClient::NotifyChatServerImg(std::string server_ip, NotifyChatServerImgReq& request)
{
	NotifyChatServerImgRsp rsp;

	// 首先找到对应在所在的连接池
	auto it = pools_.find(server_ip);
	if (it == pools_.end()) {
		rsp.set_error(ERROE_CODR::ERROR_FIND_CONN_IN_RPCPOOLS);
		return rsp;
	}
	auto& pool = it->second;

	auto stub = pool->getConnection();
	Defer defer([&pool,&stub]() {
		pool->returnConnection(std::move(stub));
		});

	ClientContext context;
	Status status = stub->NotifyChatServerImg(&context, request, &rsp);
	if (!status.ok()) {
		rsp.set_error(ERROR_RPC_VISIT_CHATSERVER);
		return rsp;
	}
	return rsp;
}

NotifyFriendIconChangeRsp ResouceServerClient::NotifyFriendIconChange(std::string server_ip, NotifyFriendIconChangeReq& request)
{
	NotifyFriendIconChangeRsp rsp;

	// 首先找到对应在所在的连接池
	auto it = pools_.find(server_ip);
	if (it == pools_.end()) {
		rsp.set_error(ERROE_CODR::ERROR_FIND_CONN_IN_RPCPOOLS);
		return rsp;
	}
	auto& pool = it->second;

	auto stub = pool->getConnection();
	Defer defer([&pool, &stub]() {
		pool->returnConnection(std::move(stub));
		});

	ClientContext context;
	Status status = stub->NotifyFriendIconChange(&context, request, &rsp);
	if (!status.ok()) {
		rsp.set_error(ERROR_RPC_VISIT_CHATSERVER);
		return rsp;
	}
	return rsp;
}
