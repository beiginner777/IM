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
			double b = (double)res->getDouble("balance");
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

std::vector<MysqlDao::Product> MysqlDao::getProducts() {
	std::vector<Product> result;
	auto conn = getConn(); if (!conn) return result;
	try {
		auto stmt = conn->con_->createStatement();
		auto res = stmt->executeQuery("SELECT id,name,price,stock,image_url FROM seckill_product ORDER BY id");
		while (res->next()) {
			result.push_back({res->getInt("id"), res->getString("name"),
				(double)res->getDouble("price"), res->getInt("stock"), res->getString("image_url")});
		}
	} catch(sql::SQLException& e) { std::cerr<<"[MysqlDao] getProducts: "<<e.what()<<std::endl; }
	returnConn(std::move(conn));
	return result;
}

bool MysqlDao::updateStock(int productId, int newStock) {
	auto conn = getConn(); if (!conn) return false;
	try {
		auto stmt = conn->con_->prepareStatement("UPDATE seckill_product SET stock=? WHERE id=?");
		stmt->setInt(1, newStock); stmt->setInt(2, productId); stmt->executeUpdate();
	} catch(sql::SQLException& e) { std::cerr<<"[MysqlDao] updateStock: "<<e.what()<<std::endl; returnConn(std::move(conn)); return false; }
	returnConn(std::move(conn));
	return true;
}

bool MysqlDao::insertOrder(int uid, int productId, const std::string& productName, double price) {
	auto conn = getConn(); if (!conn) return false;
	try {
		auto stmt = conn->con_->prepareStatement("INSERT INTO seckill_order(uid,product_id,product_name,price) VALUES(?,?,?,?)");
		stmt->setInt(1,uid); stmt->setInt(2,productId); stmt->setString(3,productName); stmt->setDouble(4,price);
		stmt->executeUpdate();
	} catch(sql::SQLException& e) { std::cerr<<"[MysqlDao] insertOrder: "<<e.what()<<std::endl; returnConn(std::move(conn)); return false; }
	returnConn(std::move(conn));
	return true;
}

std::vector<MysqlDao::Order> MysqlDao::getOrders() {
	std::vector<Order> result;
	auto conn = getConn(); if (!conn) return result;
	try {
		auto stmt = conn->con_->createStatement();
		auto res = stmt->executeQuery("SELECT id,uid,product_id,product_name,price,created_at FROM seckill_order ORDER BY id DESC LIMIT 100");
		while (res->next()) {
			result.push_back({res->getInt("id"), res->getInt("uid"), res->getInt("product_id"),
				res->getString("product_name"), (double)res->getDouble("price"), res->getString("created_at")});
		}
	} catch(sql::SQLException& e) { std::cerr<<"[MysqlDao] getOrders: "<<e.what()<<std::endl; }
	returnConn(std::move(conn));
	return result;
}

std::map<int,int> MysqlDao::getBuyCounts() {
	std::map<int,int> result;
	auto conn = getConn(); if (!conn) return result;
	try {
		auto stmt = conn->con_->createStatement();
		auto res = stmt->executeQuery("SELECT product_id, COUNT(*) cnt FROM seckill_order GROUP BY product_id");
		while (res->next()) result[res->getInt("product_id")] = res->getInt("cnt");
	} catch(sql::SQLException& e) { std::cerr<<"[MysqlDao] getBuyCounts: "<<e.what()<<std::endl; }
	returnConn(std::move(conn));
	return result;
}
