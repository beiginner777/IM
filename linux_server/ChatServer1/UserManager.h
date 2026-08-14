#ifndef USERMANAGER_H
#define USERMANAGER_H
// 这个类是用来管理当前连在当前ChatServer上的Session
// why ... 那么为什么不直接用 CServer 来管理呢
#include "global.h"
class CSession;
class UserManager : public SingleTon<UserManager>
{
	friend class SingleTon<UserManager>;
public:
	~UserManager();
	std::shared_ptr<CSession> GetSession(int uid);
	void addSession(int uid, std::shared_ptr<CSession> session);
	// removeSession只会在clearSession中调用，因此不需要再次加锁
	void removeSession(int uid, std::string uuid);
	// 打印当前所有的session信息
	void printSessions();
private:
	UserManager();
	std::mutex mtx_;
	std::unordered_map<int, std::shared_ptr<CSession>> sessions_;
};
#endif
