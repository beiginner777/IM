#include "global.h"
#include "SeckillServer.h"
int main()
{
    try {
        ConfigManager cfg = ConfigManager::getInstance();
        unsigned int port = static_cast<unsigned int>(atoi(cfg["SelfServer"]["Port"].c_str()));
        if (port == 0) {
            std::cout << "read [SelfServer] Port from config.ini failed, use default 8100." << std::endl;
            port = 8100;
        }
        net::io_context ioc{ 1 };
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int) { ioc.stop(); });
        std::shared_ptr<SeckillServer> server = std::make_shared<SeckillServer>(ioc, port);
        server->start();
        ioc.run();
    }
    catch (const std::exception& e) {
        std::cout << __FILE__ << ":" << __LINE__ << std::endl;
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
