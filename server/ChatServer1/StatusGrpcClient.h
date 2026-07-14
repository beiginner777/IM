#ifndef STATUSGRPCCLIENT_H
#define STATUSGRPCCLIENT_H
#include <grpc++/grpc++.h>
#include "global.h"
#include "SingleTon.h"
#include "ConfigManager.h"
#include "message.grpc.pb.h"
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::GetResourceServerReq;
using message::GetResourceServerRsp;
using message::StatusService;
/// StatusServer gRPC 连接池（ChatServer 专用，获取 ResourceServer 地址）
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
		cond_.notify_all();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}
	std::unique_ptr<StatusService::Stub> getConnection()
	{
		std::unique_lock<std::mutex> locker_(mtx_);
		while (!b_stop_ && connections_.empty()) {
			if (std::cv_status::timeout == cond_.wait_for(locker_, std::chrono::milliseconds(100))) {
				std::cout << "StatusClientGrpc Pool busy . . ." << std::endl;
				return nullptr;
			}
			else {
				break;
			}
		}
		if (b_stop_) {
			std::cout << "Get Connection failed, StatusClientGrpc Pool stoped work." << std::endl;
			return nullptr;
		}
		std::unique_ptr<StatusService::Stub> conn = std::move(connections_.front());
		connections_.pop();
		return conn;
	}
	void returnConnection(std::unique_ptr<StatusService::Stub> conn)
	{
		std::unique_lock<std::mutex> locker_(mtx_);
		if (b_stop_) {
			std::cout << "StatusGrpcClient Pool stoped work, return Connection failed." << std::endl;
			return;
		}
		connections_.push(std::move(conn));
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
/// StatusServer gRPC 客户端（ChatServer → StatusServer）
class StatusGrpcClient
{
public:
	StatusGrpcClient();
	~StatusGrpcClient() {}
	/// 从 StatusServer 获取可用的 ResourceServer 地址
	/// @param chatserver_name 请求方 ChatServer 名称
	GetResourceServerRsp GetResourceServer(const std::string& chatserver_name);
};
#endif
