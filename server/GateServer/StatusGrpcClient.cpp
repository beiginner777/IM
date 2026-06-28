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
		// ÉèÖÃ´íÎóÂë
		std::cout << "StatusGrpcClient connect StatusServer failed !" << std::endl;
		std::cout << "Error code: " << status.error_code() << std::endl;
		std::cout << "Error message: " << status.error_message() << std::endl;
		std::cout << "Error details: " << status.error_details() << std::endl;
		StatusConnPool::getInstance()->returnConnection(std::move(stub));
		reply.set_error(ERROE_CODR::ERROR_RPC);
		return reply;
	}
}
