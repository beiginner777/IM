#include "global.h"
#include "AuthServer.h"
#include "ConfigManager.h"
#include "RedisManager.h"
#include "MysqlManager.h"
int main(int argc, char* argv[])
{
    // 支持命令行传 config.ini 路径: ./AuthServer /path/to/config.ini
    if (argc > 1) {
        ConfigManager::setConfigPath(argv[1]);
    }
    try {
        MysqlManager::getInstance()->initBloomFilter();
        net::io_context ioc{ 1 };
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int) { ioc.stop(); });
        std::shared_ptr<AuthServer> server = std::make_shared<AuthServer>(ioc, PORT);
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
