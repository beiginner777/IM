#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

#include "global.h"
#include "MysqlDao.h"
#include "BloomFilter.h"
#include <memory>

class User;
struct ApplyInfo;

class MysqlManager : public SingleTon<MysqlManager>
{
	friend class SingleTon<MysqlManager>;

public:
	~MysqlManager() {}

	// 启动时加载布隆过滤器（从 Redis 恢复，首次从 MySQL 构建）
	void initBloomFilter();

	// 提供给 Dao 层操作布隆
	BloomFilter* getBloomFilter() { return bloomFilter_.get(); }

	// 注册新用户
	int registerUser(const std::string& name, const std::string& email, const std::string& password);

	// 用户登录
	int userLogin(std::string name, std::string password, std::shared_ptr<UserInfo> userInfo);

private:
	MysqlManager() {}
	MysqlDao dao_;
	std::unique_ptr<BloomFilter> bloomFilter_;
};

#endif
