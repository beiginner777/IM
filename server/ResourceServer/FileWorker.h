#ifndef FILEWORKER_H
#define FILEWORKER_H

#include "global.h"

class FileTask;
class CSession;

struct FileTask {
	FileTask(std::shared_ptr<CSession> session,int req_id, std::string md5,std::string name,
		int seq, int totolSize, int transfferredSize, int lastSeq,
		std::string data, int type = -1) :session_(session), req_id_(req_id),md5_(md5),
		seq_(seq), name_(name), totolSize_(totolSize),
		transfferredSize_(transfferredSize), lastSeq_(lastSeq), data_(data), type_(type)
	{
	}
	~FileTask() {}
	std::shared_ptr<CSession> session_;
	int req_id_;
	std::string md5_;
	int seq_;
	std::string name_;
	int totolSize_;
	int transfferredSize_;
	int lastSeq_;
	std::string data_;
	int type_;
};

class FileWorker
{
	using functionCallback = std::function<void(std::shared_ptr<FileTask>)>;
public:
	FileWorker();
	~FileWorker();
	// 向LogicSystem提供接口，向当前逻辑线程的队列中投递消息
	void postTaskToQue(std::shared_ptr<FileTask> task);
private:
	// 注册逻辑层的回调函数
	void registerHandlers();
	// 工作线程
	void dealTask();
	// 解密base64编码的数据
	std::string base64_decode(const std::string& in);
	// 处理 FileTask 结点 的 函数
	void taskHandler(std::shared_ptr<FileTask> task);
	// 通知好友有新头像上传
	void notifyFriendNewHeadIcon(int self_id, std::string fileName);
	// 有新的头像上传
	void handleUploadHeadIcon(std::shared_ptr<FileTask> task);
	// 有文件上传
	void handleUploadFile(std::shared_ptr<FileTask> task);

private:
	std::queue<std::shared_ptr<FileTask>> que_;
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

