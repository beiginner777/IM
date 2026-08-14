#ifndef LOGINSTSREM_H
#define LOGINSTSREM_H

#include "global.h"
#include "SingleTon.h"
class RecvNode;
class CSession;
class LogicWorker;
class FileInfo {
public:
	FileInfo(int uid = 0, int seq = 0, std::string name = "", int total_size = 0,
		int trans_size = 0, int last_seq = 0, std::string file_path_str = "",
		int last_acked = 0)
		:uid_(uid), seq_(seq), name_(name), totolSize_(total_size),
		transfferredSize_(trans_size), last_seq_(last_seq), filePath_(file_path_str),
		last_acked_seq_(last_acked)
	{
	}
	int uid_;
	int seq_;
	std::string name_;
	int totolSize_;
	int transfferredSize_;
	std::string filePath_;
	int last_seq_;
	int last_acked_seq_;                         // 连续确认值
	std::set<int> pending_seqs_;                  // 已收到但不连续的 seq（填补空洞后逐个消化）
};
class DownloadFileInfo
{
public:
	DownloadFileInfo(int uid = 0, std::string download_file = "", int seq = 0, int last_seq = 0, int trans_size = 0, int total_size = 0,
		std::string client_save_path = "", int download_file_type = -1)
		:uid_(uid), download_file_(download_file), seq_(seq), last_seq_(last_seq), trans_size_(trans_size), total_size_(total_size),
		client_save_path_(client_save_path), download_file_type_(download_file_type)
	{
	}
	int uid_;
	std::string download_file_;
	int seq_;
	int last_seq_;
	int trans_size_;
	int total_size_;
	std::string client_save_path_;
	int download_file_type_;
};
class LogicNode
{
	friend class LogicWorker;
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
public:
	~LogicSystem();
	void postMsgToQue(std::shared_ptr<LogicNode> logicNode,int index);
	bool addMd5FileInfo(std::string name, std::shared_ptr<FileInfo> fileInfo);
	std::shared_ptr<FileInfo> getFileInfo(std::string name);
	bool DeleteMd5FileInfo(const std::string& name);
	bool addDownloadFileInfo(std::string name, std::shared_ptr<DownloadFileInfo> fileInfo);
	std::shared_ptr<DownloadFileInfo> GetDownloadFileInfo(std::string name);
	bool DeleteDownloadFileInfo(const std::string& name);
private:
	LogicSystem();
	std::vector<std::shared_ptr<LogicWorker>> logicWorkers_;
};
#endif
