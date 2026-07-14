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
	// 启动时加载布隆过滤器（优先 Redis，首次从 MySQL 构建）
	void initBloomFilter();
	// 提供给 Dao 层操作布隆
	std::shared_ptr<BloomFilter> getBloomFilter() { return bloomFilter_; }
	int registerUser(const std::string& name, const std::string& email, const std::string& password);
	int userLogin(std::string name, std::string password, std::shared_ptr<UserInfo> userInfo);
private:
	MysqlManager() {}
	MysqlDao dao_;
	std::shared_ptr<BloomFilter> bloomFilter_;
};
#endif
