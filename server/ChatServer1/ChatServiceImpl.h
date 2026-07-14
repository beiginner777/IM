#ifndef CHATSERVICEIMPL_H
#define CHATSERVICEIMPL_H

#include "global.h"
#include "message.grpc.pb.h"
using namespace grpc;
using namespace message;
class ChatServiceImpl : public ChatService::Service
{
public:
	ChatServiceImpl();
	~ChatServiceImpl();
    Status NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
        AddFriendRsp* reply) override;
    Status NotifyAuthFriend(ServerContext* context,
        const AuthFriendReq* request, AuthFriendRsp* response) override;
    Status NotifyTextChatMsg(::grpc::ServerContext* context,
        const TextChatMsgReq* request, TextChatMsgRsp* response) override;
    bool GetBaseInfo(int uid, std::shared_ptr<UserInfo>& userinfo);
    Status NotifyKickUser(ServerContext* context, const KickUserReq* req, KickUserRsp* resoonse) override;
    Status NotifyChatServerImg(ServerContext* context, const NotifyChatServerImgReq* req, NotifyChatServerImgRsp* response) override;
    Status NotifyFriendIconChange(ServerContext* context, const NotifyFriendIconChangeReq* req, NotifyFriendIconChangeRsp* response) override;
private:
};
#endif
