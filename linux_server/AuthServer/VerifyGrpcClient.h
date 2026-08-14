#ifndef VERIFYGRPCCLIENT_H
#define VERIFYGRPCCLIENT_H
// 可以为队列中的共享智能指针指定一个删除器
// std::shared_ptr<X> p1;
// std::shared_ptr<X> p2(p1.get(),删除器)
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "global.h"
#include "SingleTon.h"
#include "ConfigManager.h"
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;
const std::string HOST_PORT = "127.0.0.1:50051";
class RpcConnPool : public SingleTon<RpcConnPool>
{
	friend class SingleTon<RpcConnPool>;
public:
	void init(std::string host, std::string port, size_t conn_size = DEFAULT_GRPC_CONN_SIZE)
	{
		host_ = host;
		port_ = port;
		pool_size_ = conn_size;
		b_stop_ = false;
		for (int i = 0; i < conn_size; ++i)
		{
			// 注意这里使用 host + ":" + port 就会导致redisConn--》stub 的status 失败
 			std::shared_ptr<Channel> channel = grpc::CreateChannel(HOST_PORT, grpc::InsecureChannelCredentials());
			auto stub = VarifyService::NewStub(channel);
			std::shared_ptr<VarifyService::Stub> _stub = std::move(stub);
			connections_.emplace(_stub);
		}
	}
	std::shared_ptr<VarifyService::Stub> getRpcConn()
	{
		std::unique_lock<std::mutex> locker_(mtx_);
		while (!b_stop_ && connections_.empty())
		{
			if (std::cv_status::timeout == cond_.wait_for(locker_, std::chrono::milliseconds(100)))
			{
				std::cout << "gRpc Server busy." << std::endl;
				return nullptr;
			}
		}
		if (b_stop_)
		{
			std::cout << "gRpc Service stop. " << std::endl;
			return nullptr;
		}
		auto stub = connections_.front();
		connections_.pop();
		return stub;
	}
	void returnRpcConn(std::shared_ptr<VarifyService::Stub> stub)
	{
		if (b_stop_) {
			std::cout << "gRpc Service stop. " << std::endl;
			return;
		}
		std::unique_lock<std::mutex> locker_(mtx_);
		connections_.push(std::move(stub));
		cond_.notify_all();
	}
	~RpcConnPool()
	{
		b_stop_ = true;
		cond_.notify_all();
		std::unique_lock<std::mutex> locker_(mtx_);
		while (!connections_.empty())
		{
			connections_.pop();
		}
		std::cout << "RpcConnPool destructed ." << std::endl;
	}
private:
	RpcConnPool()
	{
		auto cfg = ConfigManager::getInstance();
		std::string host = cfg["VerifyServer"]["Host"];
		std::string port = cfg["VerifyServer"]["Port"];
		// 初始化连接池
		this->init(host, port);
	}
	std::atomic_bool b_stop_;
	std::mutex mtx_;
	std::condition_variable cond_;
	std::queue<std::shared_ptr<VarifyService::Stub>> connections_;
	std::string host_;
	std::string port_;
	size_t pool_size_;
};
class VerifyGrpcClient
{
public:
	VerifyGrpcClient() = default;
	~VerifyGrpcClient() = default;
	GetVarifyRsp GetVerifyCode(std::string email)
	{
		// grpc 上下文，用于控制grpc的生命周期和元数据
		ClientContext context;
		// 响应对象
		GetVarifyRsp reply;
		// 请求对象
		GetVarifyReq request;
		// 设置请求参数
		request.set_email(email.c_str());
		// 发起远程调用：调用VarifyService服务的 GetVarifyCode方法
		auto stub = RpcConnPool::getInstance()->getRpcConn();
		if (!stub)
		{
			return reply;
		}
		Status status = stub->GetVarifyCode(&context, request, &reply);
		// 检查是否调用成功
		if (status.ok()){
			std::cout << "gRpc connect emailServer Success ! \n";
			RpcConnPool::getInstance()->returnRpcConn(stub);
			return reply;
		}
		else {
			// 设置错误码
			std::cout << "gRpc connect emailServer failed !" << std::endl;
			std::cout << "Error code: " << status.error_code() << std::endl;
			std::cout << "Error message: " << status.error_message() << std::endl;
			std::cout << "Error details: " << status.error_details() << std::endl;
			RpcConnPool::getInstance()->returnRpcConn(stub);
			reply.set_error(ERROE_CODR::ERROR_RPC);
			return reply;
		}
	}
};
#endif