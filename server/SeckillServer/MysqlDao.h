#ifndef MYSQLDAO_SECKILL_H
#define MYSQLDAO_SECKILL_H
#include "global.h"
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include <map>
#include <condition_variable>

class MysqlDao {
public:
	MysqlDao();
	~MysqlDao() = default;
	double getBalance(int uid);
	bool updateBalance(int uid, double newBalance);
	bool verifyPassword(int uid, const std::string& password);
	struct Product { int id; std::string name; double price; int stock; std::string imageUrl; };
	struct Order { int id; int uid; int productId; std::string productName; double price; std::string time; };
	std::vector<Product> getProducts();
	bool updateStock(int productId, int newStock);
	bool insertOrder(int uid, int productId, const std::string& productName, double price);
	std::vector<Order> getOrders();
	std::map<int,int> getBuyCounts();

private:
	struct SqlConnection {
		std::unique_ptr<sql::Connection> con_;
		SqlConnection(std::unique_ptr<sql::Connection> c) : con_(std::move(c)) {}
		SqlConnection(sql::Connection* c) : con_(c) {}
	};
	std::queue<std::unique_ptr<SqlConnection>> pool_;
	std::mutex mtx_;
	std::condition_variable cond_;
	std::unique_ptr<SqlConnection> getConn();
	void returnConn(std::unique_ptr<SqlConnection> conn);
};
#endif
