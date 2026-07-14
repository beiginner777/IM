#include "StatusGrpcClient.h"
StatusGrpcClient::StatusGrpcClient()
{
}

GetResourceServerRsp StatusGrpcClient::GetResourceServer(const std::string& chatserver_name)
{
	GetResourceServerRsp reply;
	reply.set_error(ERROR_RPC);
	auto stub = StatusConnPool::getInstance()->getConnection();
	if (!stub) {
		std::cerr << "[StatusGrpcClient] GetResourceServer: no available connection" << std::endl;
		return reply;
	}
	ClientContext context;
	GetResourceServerReq request;
	request.set_chatserver_name(chatserver_name);
	Status status = stub->GetResourceServer(&context, request, &reply);
	if (status.ok()) {
		std::cout << "[StatusGrpcClient] GetResourceServer OK: "
		          << reply.host() << ":" << reply.port() << std::endl;
	} else {
		std::cerr << "[StatusGrpcClient] GetResourceServer RPC failed: "
		          << status.error_message() << std::endl;
	}
	StatusConnPool::getInstance()->returnConnection(std::move(stub));
	return reply;
}
