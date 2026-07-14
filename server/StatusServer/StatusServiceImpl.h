#ifndef STATUSSERVICEIMPL_H
#define STATUSSERVICEIMPL_H

#include <grpcpp/grpcpp.h>
#include "global.h"
#include "message.grpc.pb.h"
class CServer;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::GetResourceServerReq;
using message::GetResourceServerRsp;
using message::StatusService;
using namespace message;
class StatusServiceImpl final : public StatusService::Service
{
public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
        GetChatServerRsp* reply) override;
    Status GetResourceServer(ServerContext* context, const GetResourceServerReq* request,
        GetResourceServerRsp* reply) override;
    // 注入 CServer 指针（在 main 中 CServer 创建后调用）
    void setCServer(CServer* server) { server_ = server; }
private:
    ChatServer getChatServer();
    CServer* server_ = nullptr;  // TCP 服务端，获取活跃 sessions_ / resource_sessions_
};
#endif
