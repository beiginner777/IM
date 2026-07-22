#include "MysqlDao.h"
#include "ConfigManager.h"
#include "crypto/BCryptHasher.h"

MysqlDao::MysqlDao()
{
	auto cfg = ConfigManager::getInstance();
	std::string host = cfg["Mysql"]["Host"];
	int port = atoi(cfg["Mysql"]["Port"].c_str());
	std::string user = cfg["Mysql"]["User"];
	std::string pwd  = cfg["Mysql"]["Password"];
	std::string schema = cfg["Mysql"]["Schema"];
	for (int i = 0; i < 4; i++) {
		try {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto conn = driver->connect(host.c_str(), user.c_str(), pwd.c_str());
			conn->setSchema(schema);
			std::cout << "[SeckillServer] MySQL connect OK" << std::endl;
			pool_.push(std::make_unique<MysqlDao::SqlConnection>(std::move(conn)));
		} catch (sql::SQLException& e) {
			std::cerr << "[SeckillServer] MySQL connect failed: " << e.what() << std::endl;
		}
	}
}

auto MysqlDao::getConn() -> std::unique_ptr<SqlConnection>
{
	std::unique_lock<std::mutex> lock(mtx_);
	cond_.wait_for(lock, std::chrono::milliseconds(500), [this]{ return !pool_.empty(); });
	if (pool_.empty()) return nullptr;
	auto conn = std::move(pool_.front());
	pool_.pop();
	return conn;
}

void MysqlDao::returnConn(std::unique_ptr<SqlConnection> conn)
{
	if (!conn) return;
	std::lock_guard<std::mutex> lock(mtx_);
	pool_.push(std::move(conn));
	cond_.notify_one();
}

double MysqlDao::getBalance(int uid)
{
	auto conn = getConn();
	if (!conn) return -1;
	try {
		auto stmt = conn->con_->createStatement();
		auto res = stmt->executeQuery("SELECT balance FROM user WHERE uid = " + std::to_string(uid));
		if (res->next()) {
			double b = res->getDouble("balance");
			returnConn(std::move(conn));
			return b;
		}
	} catch (sql::SQLException& e) {
		std::cerr << "[SeckillServer] getBalance error: " << e.what() << std::endl;
	}
	returnConn(std::move(conn));
	return -1;
}

bool MysqlDao::updateBalance(int uid, double newBalance)
{
	auto conn = getConn();
	if (!conn) return false;
	try {
		auto stmt = conn->con_->prepareStatement("UPDATE user SET balance = ? WHERE uid = ?");
		stmt->setDouble(1, newBalance);
		stmt->setInt(2, uid);
		stmt->executeUpdate();
		returnConn(std::move(conn));
		return true;
	} catch (sql::SQLException& e) {
		std::cerr << "[SeckillServer] updateBalance error: " << e.what() << std::endl;
	}
	returnConn(std::move(conn));
	return false;
}

bool MysqlDao::verifyPassword(int uid, const std::string& password)
{
	auto conn = getConn();
	if (!conn) return false;
	try {
		auto stmt = conn->con_->createStatement();
		auto res = stmt->executeQuery("SELECT password FROM user WHERE uid = " + std::to_string(uid));
		if (res->next()) {
			std::string hash = res->getString("password");
			returnConn(std::move(conn));
			return BCryptHasher::verifyPassword(password, hash);
		}
	} catch (sql::SQLException& e) {
		std::cerr << "[SeckillServer] verifyPassword error: " << e.what() << std::endl;
	}
	returnConn(std::move(conn));
	return false;
}
