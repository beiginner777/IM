#ifndef LOGICWORKER_CHAT_H
#define LOGICWORKER_CHAT_H
#include "global.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

class CSession;
class LogicSystem;

struct WorkerTask
{
	WorkerTask(std::shared_ptr<CSession> s, short m, std::string d, std::string u)
		: session_(s), msgId_(m), msgData_(d), uuid_(u) {}
	std::shared_ptr<CSession> session_;
	short msgId_;
	std::string msgData_;
	std::string uuid_;
};

class LogicWorker
{
public:
	LogicWorker(int id, LogicSystem* parent);
	~LogicWorker();
	void postMsg(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid);
private:
	void dealTask();
	int id_;
	LogicSystem* parent_;
	std::queue<std::shared_ptr<WorkerTask>> que_;
	std::thread work_thread_;
	std::mutex mtx_;
	std::condition_variable cond_;
	std::atomic_bool b_stop_;
};

#endif
