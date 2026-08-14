#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H
// init 函数连接的数据库名是：jerrychat
// 处理用户名重复插入的时候，需要数据库层面升级一下，不要去判断返回的错误值
// 将mysqlc库升级为mysqlc++库
#include "global.h"
#include "MysqlDao.h"
#include "SingleTon.h"
class User;
struct ApplyInfo;
class MysqlManager : public SingleTon<MysqlManager>
{
	friend class SingleTon<MysqlManager>;
	
public:
	~MysqlManager() {}	
	std::shared_ptr<UserInfo> getUserByUid(int uid);
	int updateUserIcon(int uid, const std::string& icon);
	bool GetFriendList(int uid, std::vector<int>& friend_list);
private:
	MysqlManager() {}
	MysqlDao dao_;
};
#endif