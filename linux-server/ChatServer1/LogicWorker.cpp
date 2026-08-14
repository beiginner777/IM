#include "LogicWorker.h"
#include "LogicSystem.h"
#include "CSession.h"

LogicWorker::LogicWorker(int id, LogicSystem* parent)
	: id_(id), parent_(parent), b_stop_(false)
{
	work_thread_ = std::thread(&LogicWorker::dealTask, this);
}

LogicWorker::~LogicWorker()
{
	b_stop_ = true;
	cond_.notify_all();
	if (work_thread_.joinable())
		work_thread_.join();
}

void LogicWorker::postMsg(std::shared_ptr<CSession> session, short msgId, std::string msgData, std::string uuid)
{
	std::lock_guard<std::mutex> lock(mtx_);
	que_.push(std::make_shared<WorkerTask>(session, msgId, msgData, uuid));
	cond_.notify_one();
}

void LogicWorker::dealTask()
{
	while (!b_stop_)
	{
		std::unique_lock<std::mutex> lock(mtx_);
		cond_.wait(lock, [this]() { return b_stop_ || !que_.empty(); });
		if (b_stop_) break;

		auto task = que_.front();
		que_.pop();
		lock.unlock();

		parent_->dispatch(task->session_, task->msgId_, task->msgData_, task->uuid_);
	}
}
