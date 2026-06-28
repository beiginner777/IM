#include "MysqlManager.h"

int MysqlManager::registerUser(const std::string& name, const std::string& email, const std::string& password)
{
	return dao_.registerUser(name, email, password);
}

int MysqlManager::userLogin(std::string name, std::string password, std::shared_ptr<UserInfo> userInfo)
{
	return dao_.userLogin(name, password, userInfo);
}