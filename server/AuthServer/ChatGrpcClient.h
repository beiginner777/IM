#ifndef AUTH_CHATGRPCCLIENT_H
#define AUTH_CHATGRPCCLIENT_H
// AuthServer 专用的轻量 gRPC 客户端 —— 仅用于 NotifyKickUser
#include "global.h"
#include "message.grpc.pb.h"
#include "ConfigManager.h"
#include <unordered_map>
#include <memory>
#include <mutex>

using namespace message;

class KickUserClient : public SingleTon<KickUserClient>
{
	friend class SingleTon<KickUserClient>;
public:
	// 通知指定 ChatServer 踢掉 uid 的旧连接
	KickUserRsp NotifyKickUser(const std::string& serverName, int uid)
	{
		KickUserRsp rsp;
		auto it = channels_.find(serverName);
		if (it == channels_.end()) {
			// 首次使用，创建 gRPC channel
			auto cfg = ConfigManager::getInstance();
			std::string host = cfg[serverName]["Host"];
			std::string rpcPort = cfg[serverName]["RPCPort"];
			if (host.empty() || rpcPort.empty()) {
				std::cerr << "[KickUser] No config for " << serverName << std::endl;
				rsp.set_error(-1);
				return rsp;
			}
			auto channel = grpc::CreateChannel(host + ":" + rpcPort,
			                                    grpc::InsecureChannelCredentials());
			auto stub = ChatService::NewStub(channel);
			channels_[serverName] = std::move(stub);
			it = channels_.find(serverName);
		}

		KickUserReq req;
		req.set_uid(uid);
		ClientContext context;
		auto status = it->second->NotifyKickUser(&context, req, &rsp);
		if (!status.ok()) {
			std::cerr << "[KickUser] gRPC failed: " << status.error_message() << std::endl;
			rsp.set_error(-1);
		}
		return rsp;
	}

private:
	KickUserClient() {}
	std::unordered_map<std::string, std::unique_ptr<ChatService::Stub>> channels_;
};

#endif
