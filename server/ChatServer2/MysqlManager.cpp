#include "MysqlManager.h"

int MysqlManager::registerUser(const std::string& name, const std::string& email, const std::string& password)
{
	return dao_.registerUser(name, email, password);
}

int MysqlManager::getUserFriendApply(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool forceMaster)
{
	return dao_.getUserFriendApply(uid, applyList, forceMaster);
}

int MysqlManager::getUserFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& friendList, bool forceMaster)
{
	return dao_.getUserFriendList(uid, friendList, forceMaster);
}

int MysqlManager::addFriendApply(int fromuid, int touid, int& current_id, std::string& apply_time)
{
	return dao_.addFriendApply(fromuid, touid, current_id, apply_time);
}

int MysqlManager::addFriendRelation(int fromuid, int touid, int& thread_id1,int& thread_id2,int& friend_id1, int& friend_id2)
{
	return dao_.addFriendRelation(fromuid, touid, thread_id1, thread_id2, friend_id1, friend_id2);
}

std::shared_ptr<UserInfo> MysqlManager::getUserByUid(int uid, bool forceMaster)
{
	return dao_.getUserByUid(uid, forceMaster);
}

std::shared_ptr<UserInfo> MysqlManager::getUserByName(std::string name, bool forceMaster)
{
	return dao_.getUserByName(name, forceMaster);
}

int MysqlManager::setFriendApplyStatus(int fromuid, int touid, int status)
{
	return dao_.setFriendApplyStatus(fromuid, touid, status);
}

int MysqlManager::GetUserThreadInfos(int uid, int last_thread_id, int page_size,std::vector<std::shared_ptr<ChatThreadInfo>>& infos, bool& load_more, int& max_thread_id, bool forceMaster)
{
	return dao_.GetUserThreadInfos(uid, last_thread_id, page_size, infos, load_more, max_thread_id, forceMaster);
}

int MysqlManager::createPrivateThread(int user1_id, int user2_id, int& thread_id)
{
	return dao_.createPrivateThread(user1_id, user2_id, thread_id);
}

int MysqlManager::AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas)
{
	return dao_.AddChatMsg(chat_datas);
}

int MysqlManager::getUserFriendListByLastId(int uid, int last_friend_id, std::map<int, std::shared_ptr<UserInfo>>& friend_list, bool forceMaster)
{
	return dao_.getUserFriendListByLastId( uid, last_friend_id, friend_list, forceMaster);
}

int MysqlManager::getUserFriendApplyByLastId(int uid, int last_friend_id, int page_size, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool& load_more, int& max_friend_apply_id, bool forceMaster)
{
	return dao_.getUserFriendApplyByLastId(uid,last_friend_id, page_size, applyList,load_more,max_friend_apply_id, forceMaster);
}

int MysqlManager::updateChatMsgStatus(int message_id, MsgStatus status)
{
	return dao_.updateChatMsgStatus(message_id, status);
}

int MysqlManager::loadChatMessage(int thread_id, int& min_message_id, int& max_message_id, int page_size, bool& is_more, std::vector<ChatMessage>& msgs, bool forceMaster)
{
	return dao_.loadChatMessage(thread_id, min_message_id, max_message_id, page_size, is_more, msgs, forceMaster);
}

//bool MysqlManager::pushOfflineMessage(int uid, std::string message)
//{
//	return dao_.pushOfflineMessage(uid, message);
//}
//
//std::vector<std::string> MysqlManager::popOfflineMessages(int uid, int& max_id)
//{
//	return dao_.popOfflineMessages(uid,max_id);
//}
