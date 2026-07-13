#ifndef STATUSGRPCCLIENT_H
#define STATUSGRPCCLIENT_H

// 可以为队列中的共享智能指针指定一个删除器
// std::shared_ptr<X> p1;
// std::shared_ptr<X> p2(p1.get(),删除器)

#include <grpc++/grpc++.h>
#include "global.h"
#include "SingleTon.h"
#include "ConfigManager.h"
#include "message.grpc.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

class StatusConnPool : public SingleTon<StatusConnPool>
{
	friend class SingleTon<StatusConnPool>;
public:
	StatusConnPool() : b_stop_(false)
	{
		ConfigManager cfg = ConfigManager::getInstance();
		std::string host = cfg["StatusServer"]["Host"];
		std::string port = cfg["StatusServer"]["Port"];

		this->init(host, port);
	}

	void init(std::string host, std::string port, std::size_t conn_size = DEFAULT_STATUSGRPCCLIENT_CONN_SIZE)
	{
		host_ = host;
		port_ = port;
		poolSize_ = conn_size;
		for (size_t i = 0; i < poolSize_; ++i) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
			connections_.push(StatusService::NewStub(channel));
		}
	}

	~StatusConnPool()
	{
		std::lock_guard<std::mutex> locker_(mtx_);
		b_stop_ = true;
		cond_.notify_all(); // 唤醒执行 getConnecton 的线程，防止线程过久等待
		while (!connections_.empty())
		{
			connections_.pop();
		}
	}

	std::unique_ptr<StatusService::Stub> getConnection()
	{
		std::unique_lock<std::mutex> locker_(mtx_);
		while (!b_stop_ && connections_.empty())
		{
			if (std::cv_status::timeout == cond_.wait_for(locker_, std::chrono::milliseconds(100))) {
				std::cout << "StatusClientGrpc Pool busy . . ." << std::endl;
				return nullptr;
			}
			else 
			{
				break;
			}
		}
		if (b_stop_)
		{
			std::cout << "Get Connection failed,StatusClientGrpc Pool stoped work." << std::endl;
			return nullptr;
		}
		std::unique_ptr<StatusService::Stub> conn = std::move(connections_.front());
		connections_.pop();
		return conn; // 自动移动
	}

	void returnConnection(std::unique_ptr<StatusService::Stub> conn)
	{
		std::unique_lock<std::mutex> locker_(mtx_);
		if (b_stop_)
		{
			std::cout << "StatusGrpcClient Pool stoped work,return Connection failed." << std::endl;
			return;
		}
		connections_.push(std::move(conn)); // 细节移动
		cond_.notify_all();
	}

private:
	std::atomic_bool b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<StatusService::Stub>> connections_;
	std::mutex mtx_;
	std::condition_variable cond_;
};

class StatusGrpcClient
{
public:
	StatusGrpcClient();
	~StatusGrpcClient() {

	}
	GetChatServerRsp GetChatServer(int uid);
};


#endif