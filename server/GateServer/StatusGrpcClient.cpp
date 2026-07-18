#include "StatusGrpcClient.h"
StatusGrpcClient::StatusGrpcClient()
{
}

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
    ClientContext context;
    GetChatServerRsp reply;
    GetChatServerReq request;
    request.set_uid(uid);
    
    std::unique_ptr<StatusService::Stub> stub = std::move(StatusConnPool::getInstance()->getConnection());
    Status status = stub->GetChatServer(&context, request, &reply);
    
	std::cout << "[GateServer]: " << "receive ChatServer(" << reply.host() << ":" << reply.port() << ") for client whose uid = " << uid << " from StatusServer.\n";
	if (status.ok()) {
		std::cout << "gRpc connect StatusServer Success ! \n";
		StatusConnPool::getInstance()->returnConnection(std::move(stub));
		return reply;
	}
	else {
		// 设置错误码
		std::cout << "StatusGrpcClient connect StatusServer failed !" << std::endl;
		std::cout << "Error code: " << status.error_code() << std::endl;
		std::cout << "Error message: " << status.error_message() << std::endl;
		std::cout << "Error details: " << status.error_details() << std::endl;
		StatusConnPool::getInstance()->returnConnection(std::move(stub));
		reply.set_error(ERROE_CODR::ERROR_RPC);
		return reply;
	}
}

GetSeckillServerRsp StatusGrpcClient::GetSeckillServer(int uid)
{
	ClientContext context;
	GetSeckillServerRsp reply;
	GetSeckillServerReq request;
	request.set_uid(uid);

	std::unique_ptr<StatusService::Stub> stub = std::move(StatusConnPool::getInstance()->getConnection());
	if (stub == nullptr) {
		// 连接池繁忙，拿不到可用连接
		reply.set_error(ERROE_CODR::REDISCONNPOOL_BUSY);
		return reply;
	}
	Status status = stub->GetSeckillServer(&context, request, &reply);

	std::cout << "[GateServer]: " << "receive SeckillServer(" << reply.host() << ":" << reply.port() << ") for client whose uid = " << uid << " from StatusServer.\n";
	if (status.ok()) {
		std::cout << "gRpc connect StatusServer Success ! \n";
		StatusConnPool::getInstance()->returnConnection(std::move(stub));
		return reply;
	}
	else {
		// 设置错误码
		std::cout << "StatusGrpcClient connect StatusServer failed !" << std::endl;
		std::cout << "Error code: " << status.error_code() << std::endl;
		std::cout << "Error message: " << status.error_message() << std::endl;
		std::cout << "Error details: " << status.error_details() << std::endl;
		StatusConnPool::getInstance()->returnConnection(std::move(stub));
		reply.set_error(ERROE_CODR::ERROR_RPC);
		return reply;
	}
}
