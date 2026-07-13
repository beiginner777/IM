#include "global.h"
#include "GateServer.h"
#include "ConfigManager.h"
#include "RedisManager.h"
#include "MysqlManager.h"

int main()
{
    try {
        // 加载布隆过滤器（从 Redis 恢复）
        MysqlManager::getInstance()->initBloomFilter();

        net::io_context ioc{ 1 };
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, int) { ioc.stop(); });
        std::shared_ptr<GateServer> server = std::make_shared<GateServer>(ioc, PORT);
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