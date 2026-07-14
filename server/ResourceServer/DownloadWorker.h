#ifndef DOWNLOADWORKER_H
#define DOWNLOADWORKER_H
#include "global.h"
class CSession;
class DownloadTask
{
public:
	DownloadTask(std::shared_ptr<CSession> session, std::string download_file, int seq,
		int last_seq, int trans_size, int total_size,std::string client_save_path, Download_File_Type type, int icon_uid)
		: session_(session), download_file_(download_file), seq_(seq),
		last_seq_(last_seq), trans_size_(trans_size), total_size_(total_size)
		, client_save_path_(client_save_path), type_(type), icon_uid_(icon_uid)
	{
	}
	~DownloadTask() {} 
	std::shared_ptr<CSession> session_;
	std::string download_file_;
	int seq_;
	int last_seq_;
	int trans_size_;
	int total_size_;
	std::string client_save_path_;
	Download_File_Type type_;
	int icon_uid_;
};
class DownloadWorker
{
	using functionCallback = std::function<void(std::shared_ptr<DownloadTask>)>;
public:
	DownloadWorker();
	~DownloadWorker();
	// 向LogicSystem提供接口，向当前逻辑线程的队列中投递消息
	void postTaskToQue(std::shared_ptr<DownloadTask> task);
private:
	// 工作线程
	void dealTask();
	// 处理 FileTask 结点 的 函数
	void taskHandler(std::shared_ptr<DownloadTask> task);
	// 将数据进行base64编码
	std::string base64_encode(const std::string& data);
private:
	std::queue<std::shared_ptr<DownloadTask>> que_;
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
