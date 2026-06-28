#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

// init 函数连接的数据库名是：jerrychat

// 处理用户名重复插入的时候，需要数据库层面升级一下，不要去判断返回的错误值

// 将mysqlc库升级为mysqlc++库

#include "global.h"
#include "MysqlDao.h"

class User;
struct ApplyInfo;

class MysqlManager : public SingleTon<MysqlManager>
{
	friend class SingleTon<MysqlManager>;
	
public:
	~MysqlManager() {}
	// 注册新用户
	int registerUser(const std::string& name, const std::string& email, const std::string& password);
	// 获取对应用户的好友申请
	int getUserFriendApply(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList);
	// 获取某用户的好友列表
	int getUserFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& friendList);
	// 将好友申请添加到数据库
	int addFriendApply(int fromuid, int touid,int& current_id, std::string& apply_time);
	// 添加好友关系
	int addFriendRelation(int fromuid, int touid, int& thread_id, int& thread_id2, int& friend_id1, int& friend_id2);
	// 根据uid获取用户信息
	std::shared_ptr<UserInfo> getUserByUid(int uid);
	// 根据name获取用户信息
	std::shared_ptr<UserInfo> getUserByName(std::string name);
	// 设置好友申请的状态
	int setFriendApplyStatus(int fromuid, int touid, int status);
	// 获取用户的聊天列表
	int GetUserThreadInfos(int uid, int last_thread_id, int page_size, std::vector<std::shared_ptr<ChatThreadInfo>>& infos, bool& load_more, int& max_thread_id);
	// 创建私聊线程
	int createPrivateThread(int user1_id, int user2_id, int& thread_id);
	// 添加聊天消息
	int AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas);
	// 加载好友列表
	int getUserFriendListByLastId(int uid, int last_friend_id, std::map<int, std::shared_ptr<UserInfo>>& friend_list);
	// 加载好友申请列表
	int getUserFriendApplyByLastId(int uid, int last_friend_id, int page_size, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool& load_more, int& max_friend_apply_id);
	// 修改ChatMessage消息的状态
	int updateChatMsgStatus(int message_id, MsgStatus status);
	// 加载聊天消息
	int loadChatMessage(int thread_id, int& min_message_id, int& max_message_id, int page_size, bool& is_more, std::vector<ChatMessage>& msgs);

private:
	MysqlManager() {}
	MysqlDao dao_;
};

#endif