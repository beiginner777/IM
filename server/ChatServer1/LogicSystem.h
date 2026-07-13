#ifndef LOGINSTSREM_H

#define LOGINSTSREM_H



#include "global.h"
#include "TokenBucket.h"
#include <unordered_map>



class RecvNode;

class CSession;



class LogicNode

{

	friend class LogicSystem;

public:

	LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvNode) : session_(session), recvNode_(recvNode)

	{

	}

	~LogicNode()

	{

	}



private:

	std::shared_ptr<CSession> session_;

	std::shared_ptr<RecvNode> recvNode_;

};



class LogicSystem : public SingleTon<LogicSystem>

{

	friend class SingleTon<LogicSystem>;



	using functionCallback = std::function<void(std::shared_ptr<CSession>, short msgId, std::string msgData, std::string uuid)>;

public:

	~LogicSystem();

	// 向提供接口，向逻辑队列投递 逻辑结点

	void postMsgToQue(std::shared_ptr<LogicNode> logicNode);

	// ChatServerImpl 请求相关的信息

	std::shared_ptr<ChatMessage> GetUserThreadImageMsg(std::string unique_name);



private:

	LogicSystem();

	// 注册逻辑层的回调函数

	void registerFunctionCallbacks();

	// 向状态服务器注册的回调函数

	void registerToStatusServer(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 登录的回调函数（向状态服务器验证token 并且 再次验证用户的 name 和 password）

	void loginHandle(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 搜索的回调函数（向ChatServer发送搜索某个用户的请求）

	void searchHandle(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 申请好友的回调函数

	void applyHandle(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 通过好友申请的回调函数

	void authAccess(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 收到好友申请的回调函数

	void receiveFriendApply(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 收到文本消息的回调函数

	void dealTextChatMsg(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 心跳检测的回调函数

	void heartCheck(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 加载用户的聊天列表

	void loadChatList(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 创建私聊线程

	void createPrivateThread(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 加载用户的好友列表

	void loadConnList(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 收到图片消息的回调函数

	void dealImageChatMsg(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 加载好友申请列表

	void loadFriendApplyList(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 加载聊天消息

	void loadChatMsg(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);

	// 与StatusServer心跳检测的回包

	void heartCheckWithStatusServer(std::shared_ptr<CSession> session, short msgId, std::string msgData,

	                                std::string uuid);



private:

	// 工作线程

	void dealTask();

	// 判断字符串是否全是数字

	bool isAllDigits(const std::string& str);



	// void getBaseInfo(std::string& key, int uid, std::shared_ptr<UserInfo>& userInfo);

	

	// 根据uid获取用户信息

	bool getUserByUid(std::string uid, Json::Value& rtvalue);

	// 根据name获取用户信息

	bool getUserByName(std::string name,Json::Value& rtvalue);

	// 获取用户的聊天列表

	int GetUserThreadInfos(int uid, int last_thread_id, int page_size, std::vector<std::shared_ptr<ChatThreadInfo>>& infos, bool& load_more, int& max_thread_id);

	// 添加用户在聊天线程中的图片消息

	bool AddUserThreadImageMsg(std::string unique_name, std::shared_ptr<ChatMessage> image_data);

	// 删除用户在聊天线程中的图片消息

	bool RemoveUserThreadImageMsg(std::string unique_name);



	std::queue<std::shared_ptr<LogicNode>> que_;

	std::map<int, functionCallback> handlers_;

	// 子线程来执行任务

	std::thread work_thread_;

	// 逻辑队列是共享资源

	std::mutex mtx_;

	std::condition_variable cond_;

	// 是否停止工作

	std::atomic_bool b_stop_;

	// 

	std::map <std::string, std::shared_ptr<ChatMessage>> image_datas_;

	// 限流：单用户令牌桶（uid → bucket）
	std::unordered_map<int, TokenBucket> userBuckets_;
	// 全局 QPS 上限（ChatServer 级）
	TokenBucket globalBucket_{5000.0, 6000.0};

};



#endif

