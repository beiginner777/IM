#ifndef CHATGRPCCLIENT_H
#define CHATGRPCCLIENT_H

// 是为了实现 ChatServer 之间的通讯

#include "global.h"
#include "SingleTon.h"

class ChatConnPool
{
public:
	ChatConnPool(std::size_t poolSize, std::string host, std::string port)
		: poolSize_(poolSize),host_(host),port_(port),b_stop_(false)
	{
		for (std::size_t i = 0; i < poolSize; ++i) 
		{
			std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
			
		}
	}
private:
	std::size_t poolSize_;
	std::string host_;
	std::string port_;

	std::atomic_bool b_stop_;

	std::queue<std::unique_ptr<std::unique_ptr<grpc::>>>
};

class ChatGrpcClient : public SingleTon<ChatGrpcClient>
{
	friend class SingleTon<ChatGrpcClient>;
public:
	
private:

};

#endif
