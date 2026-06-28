#include "MysqlManager.h"

std::shared_ptr<UserInfo> MysqlManager::getUserByUid(int uid)
{
	return dao_.getUserByUid(uid);
}

int MysqlManager::updateUserIcon(int uid, const std::string& icon)
{
	return dao_.updateUserIcon(uid, icon);
}

bool MysqlManager::GetFriendList(int uid, std::vector<int>& friend_list)
{
	return dao_.GetFriendList(uid,friend_list);
}

