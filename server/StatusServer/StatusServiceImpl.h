#ifndef STATUSSERVICEIMPL_H
#define STATUSSERVICEIMPL_H

#include <grpcpp/grpcpp.h>
#include "global.h"
#include "message.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;
using namespace message;

class StatusServiceImpl final : public StatusService::Service
{
public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
        GetChatServerRsp* reply) override;
private:
    ChatServer getChatServer();

    //void insertToken(int uid, std::string token);
	
	// 因为将uid 和 token 写在redis中了，因此，ChatServer 直接去访问 redis 也可以
	// ChatServer 需要在 StatusServer 来验证用户的 Token
	// Status login(ServerContext* context, const LoginReq* request, LoginRsp* reply);
    std::mutex mtx_;
    std::map<std::string, ChatServer> servers_;
};

#endif