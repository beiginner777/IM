#ifndef CHATGRPCCLIENT_H
#define CHATGRPCCLIENT_H
// 是为了实现 ChatServer 之间的通讯，这个类是作为客户端
#include "global.h"
#include "message.grpc.pb.h"
#include "UserManager.h"
using namespace message;
using namespace grpc;
class ChatConnPool
{
public:
	ChatConnPool(std::size_t poolSize, std::string host, std::string port)
		: poolSize_(poolSize),host_(host),port_(port),b_stop_(false)
	{
		for (std::size_t i = 0; i < poolSize; ++i)
		{
			std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
			connections_.push(ChatService::NewStub(channel));
		}
	}
	std::unique_ptr<ChatService::Stub> getConnection()
	{
		std::unique_lock<std::mutex> locker(mtx_);
		while (!b_stop_ && connections_.empty())
		{
			if (std::cv_status::timeout == cond_.wait_for(locker, std::chrono::milliseconds(100)))
			{
				std::cout << "ChatGrpcPool is  busy." << std::endl;
				return nullptr;
			}
			else
			{
				break;
			}
		}
		if (b_stop_)
		{
			std::cout << "ChatGrpcPool stoped." << std::endl;
			return nullptr;
		}
		auto conn = std::move(connections_.front());
		connections_.pop();
		return conn;
	}
	void returnConnection(std::unique_ptr<ChatService::Stub> conn)
	{
		if (b_stop_)
		{
			std::cout << "return Connection to ChatConnPool failed: ChatConnPool is closed." << std::endl;
			return;
		}
		std::lock_guard<std::mutex> locker(mtx_);
		connections_.push(std::move(conn));
		cond_.notify_one();
		std::cout << "ChatGrpcPool size = " << connections_.size() << std::endl;
		return;
	}
private:
	std::size_t poolSize_;
	std::string host_;
	std::string port_;
	std::atomic_bool b_stop_;
	std::queue<std::unique_ptr<ChatService::Stub>> connections_;
	std::mutex mtx_;
	std::condition_variable cond_;
};
class ChatGrpcClient : public SingleTon<ChatGrpcClient>
{
	friend class SingleTon<ChatGrpcClient>;
public:
	~ChatGrpcClient();
	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	bool GetBaseInfo(int uid, std::shared_ptr<UserInfo>& userinfo);
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);
	KickUserRsp NotifyKickUser(std::string server_ip, const KickUserReq& req);
private:
	ChatGrpcClient();
	std::unordered_map<std::string, std::unique_ptr<ChatConnPool>> pools_;
};
#endif
