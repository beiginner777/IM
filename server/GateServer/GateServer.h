#ifndef GATESERVER_H
#define GATESERVER_H

#include "global.h"
#include "AsioIOContextThreadPool.h"

class GateServer : public std::enable_shared_from_this<GateServer>
{
public:
	GateServer(net::io_context& ioc, unsigned int port);
	~GateServer();
	void start();

private:
	net::io_context& ioc_;
	tcp::acceptor acceptor_;
};

#endif