#ifndef AUTHSERVER_H
#define AUTHSERVER_H
#include "global.h"
#include "AsioIOContextThreadPool.h"
class AuthServer : public std::enable_shared_from_this<AuthServer>
{
public:
	AuthServer(net::io_context& ioc, unsigned int port);
	~AuthServer();
	void start();
private:
	net::io_context& ioc_;
	tcp::acceptor acceptor_;
};
#endif