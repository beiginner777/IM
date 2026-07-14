#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H
#include "global.h"
#include "MysqlDao.h"
#include "BloomFilter.h"
#include <memory>
#include <unordered_map>
#include "ShardRouter.h"
class User;
struct ApplyInfo;
class MysqlManager : public SingleTon<MysqlManager>
{
	friend class SingleTon<MysqlManager>;
public:
	~MysqlManager() {}
	// 启动时从 MySQL 加载所有用户，构建布隆过滤器
	void initBloomFilter();
	int registerUser(const std::string& name, const std::string& email, const std::string& password);
	int getUserFriendApply(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool forceMaster = false);
	int getUserFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& friendList, bool forceMaster = false);
	int addFriendApply(int fromuid, int touid, int& current_id, std::string& apply_time);
	int addFriendRelation(int fromuid, int touid, int& thread_id, int& thread_id2, int& friend_id1, int& friend_id2);
	std::shared_ptr<UserInfo> getUserByUid(int uid, bool forceMaster = false);
	std::shared_ptr<UserInfo> getUserByName(std::string name, bool forceMaster = false);
	int setFriendApplyStatus(int fromuid, int touid, int status);
	int GetUserThreadInfos(int uid, int last_thread_id, int page_size, std::vector<std::shared_ptr<ChatThreadInfo>>& infos, bool& load_more, int& max_thread_id, bool forceMaster = false);
	int createPrivateThread(int user1_id, int user2_id, int& thread_id);
	int AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas);
	int getUserFriendListByLastId(int uid, int last_friend_id, std::map<int, std::shared_ptr<UserInfo>>& friend_list, bool forceMaster = false);
	int getUserFriendApplyByLastId(int uid, int last_friend_id, int page_size, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool& load_more, int& max_friend_apply_id, bool forceMaster = false);
	int updateChatMsgStatus(int thread_id, int message_id, MsgStatus status);
	int loadChatMessage(int thread_id, int& min_message_id, int& max_message_id, int page_size, bool& is_more, std::vector<ChatMessage>& msgs, bool forceMaster = false);
private:
	MysqlManager() {}
	MysqlDao dao_;
	std::unique_ptr<BloomFilter> bloomFilter_;  // 用户搜索加速
};
#endif
