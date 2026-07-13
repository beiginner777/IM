#ifndef LOGINSTSREM_H

#define LOGINSTSREM_H



#include "global.h"



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



	using functionCallback = std::function<void(std::shared_ptr<CSession>, short msgId, std::string msgData)>;

public:

	~LogicSystem();

	// 向提供接口，向逻辑队列投递 逻辑结点

	void postMsgToQue(std::shared_ptr<LogicNode> logicNode);



private:

	LogicSystem();

	// 注册逻辑层的回调函数

	void registerFunctionCallbacks();

	// 其它服务注册的回调函数

	void registerService(std::shared_ptr<CSession> session, short msgId, std::string msgData);

	// 与ChatServer心跳检测的回调函数

	void heartCheck(std::shared_ptr<CSession> session, short msgId, std::string msgData);

	// 存储服务的信息在 Redis 中

	void storeServerInfoInRedis(const Server_Info& info);



private:

	// 工作线程

	void dealTask();



	std::queue<std::shared_ptr<LogicNode>> que_;

	std::map<int, functionCallback> handlers_;

	// 子线程来执行任务

	std::thread work_thread_;

	// 逻辑队列是共享资源

	std::mutex mtx_;

	std::condition_variable cond_;

	// 是否停止工作

	std::atomic_bool b_stop_;

};



#endif

