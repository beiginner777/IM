#include "UserManager.h"
#include "CSession.h"
UserManager::UserManager()
{
}
UserManager::~UserManager()
{
	std::lock_guard<std::mutex> locker(mtx_);
	sessions_.clear();
}
std::shared_ptr<CSession> UserManager::GetSession(int uid)
{
	std::lock_guard<std::mutex> locker(mtx_);
	if (sessions_.count(uid) != 0) {
		return sessions_.find(uid)->second;
	}
	else {
		return nullptr;
	}
}
void UserManager::addSession(int uid, std::shared_ptr<CSession> session)
{
	std::lock_guard<std::mutex> locker(mtx_);
	sessions_[uid] = session;
}
void UserManager::removeSession(int uid, std::string uuid)
{
	std::shared_ptr<CSession> session = nullptr;
	if (sessions_.count(uid) != 0) {
		session = sessions_.find(uid)->second;
	}
	else {
		session = nullptr;
	}
	// 说明uuid已经被修改过了，也就是出现了异地登录。那么就直接下线，不需要做修改
	if (session != nullptr && session->getUuid() != uuid) {
		return;
	}
	sessions_.erase(uid);
}
void UserManager::printSessions()
{
	for (auto session : sessions_) {
		std::cout << "session uid = " << session.first << " uuid = " << session.second->getUuid() << std::endl;
	}
}
