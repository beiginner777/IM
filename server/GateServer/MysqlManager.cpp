#include "MysqlManager.h"

void MysqlManager::initBloomFilter()
{
	bloomFilter_ = std::make_unique<BloomFilter>(1000000, 0.01);

	// 从 Redis 恢复（ChatServer 首次启动时已从 MySQL 构建好了）
	if (bloomFilter_->loadFromRedis("bloom:user_search")) {
		std::cout << "[BloomFilter] Loaded from Redis ("
		          << bloomFilter_->count() << " bits set, "
		          << bloomFilter_->bitSize()/8/1024 << "KB)" << std::endl;
	} else {
		std::cout << "[BloomFilter] No Redis bitmap yet — will be built by ChatServer" << std::endl;
	}
}

int MysqlManager::registerUser(const std::string& name, const std::string& email, const std::string& password)
{
	return dao_.registerUser(name, email, password);
}

int MysqlManager::userLogin(std::string name, std::string password, std::shared_ptr<UserInfo> userInfo)
{
	return dao_.userLogin(name, password, userInfo);
}
