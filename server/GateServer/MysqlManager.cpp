#include "MysqlManager.h"
void MysqlManager::initBloomFilter()
{
	bloomFilter_ = std::make_shared<BloomFilter>(1000000, 0.01);
	// 优先从 Redis 恢复
	if (bloomFilter_->loadFromRedis("bloom:user_search")) {
		std::cout << "[BloomFilter] Restored from Redis ("
		          << bloomFilter_->count() << " bits set, "
		          << bloomFilter_->bitSize()/8/1024 << "KB)" << std::endl;
		return;
	}
	// Redis 无数据 → 从 MySQL 全量构建
	// 直接从 ConfigManager 读取配置，创建 MySQL 连接
	auto cfg = ConfigManager::getInstance();
	std::string host = cfg["Mysql"]["Host"];
	std::string port = cfg["Mysql"]["Port"];
	std::string user = cfg["Mysql"]["User"];
	std::string pwd  = cfg["Mysql"]["Password"];
	std::string schema = cfg["Mysql"]["Schema"];
	sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
	std::string url = "tcp://" + host + ":" + port;
	std::unique_ptr<sql::Connection> conn(driver->connect(url, user, pwd));
	if (!conn) {
		std::cerr << "[BloomFilter] Cannot connect to MySQL" << std::endl;
		return;
	}
	conn->setSchema(schema);
	try {
		std::unique_ptr<sql::Statement> stmt(conn->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT uid, name FROM user"));
		int count = 0;
		while (res->next()) {
			// uid 和 name 都写入布隆（搜索时两种方式都能拦截）
			bloomFilter_->add((uint64_t)res->getInt("uid"));
			bloomFilter_->add(res->getString("name"));
			count++;
		}
		std::cout << "[BloomFilter] Built from MySQL: " << count
		          << " users, " << count*2 << " entries ("
		          << bloomFilter_->bitSize()/8/1024 << "KB, "
		          << bloomFilter_->hashCount() << " hashes)" << std::endl;
		bloomFilter_->saveToRedis("bloom:user_search");
	}
	catch (sql::SQLException& e) {
		std::cerr << "[BloomFilter] MySQL error: " << e.what() << std::endl;
	}
}

int MysqlManager::registerUser(const std::string& name, const std::string& email, const std::string& password)
{
	// Bloom pre-check
	auto bf = getBloomFilter();
	if (bf && !bf->contains(name))
	{
		return ERROR_USER_NOT_EXIST;
	}
	return dao_.registerUser(name, email, password);
}

int MysqlManager::userLogin(std::string name, std::string password, std::shared_ptr<UserInfo> userInfo)
{
	// Bloom pre-check
	auto bf = getBloomFilter();
	if (bf && !bf->contains(name))
	{
		return ERROR_USER_NOT_EXIST;
	}
	return dao_.userLogin(name, password, userInfo);
}
