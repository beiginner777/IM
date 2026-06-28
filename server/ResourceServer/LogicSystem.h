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
		int trans_size = 0, int last_seq = 0, std::string file_path_str = "")
		:uid_(uid), seq_(seq), name_(name), totolSize_(total_size),
		transfferredSize_(trans_size), last_seq_(last_seq), filePath_(file_path_str) 
	{
	}
	// 
	int uid_;
	// 这个文件已经接收的序号数
	int seq_;
	//  唯一的文件名标识
	std::string name_;
	// 文件总大小
	int totolSize_;
	// 已经传输的大小
	int transfferredSize_;
	// 文件在资源服务器存放的路径
	std::string filePath_;
	// 文件最后一个包的序号
	int last_seq_;
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
	int download_file_type_; // 下载文件的类型
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
	// 向外提供接口，根据连接是 uuid 来判断 投递 到 哪个逻辑线程
	void postMsgToQue(std::shared_ptr<LogicNode> logicNode,int index);
	// 上传的文件信息保存到 Redis 中
	bool addMd5FileInfo(std::string name, std::shared_ptr<FileInfo> fileInfo);
	std::shared_ptr<FileInfo> getFileInfo(std::string name);
	bool DeleteMd5FileInfo(const std::string& name);
	// 下载的文件信息保存到 Redis 中
	bool addDownloadFileInfo(std::string name, std::shared_ptr<DownloadFileInfo> fileInfo);
	std::shared_ptr<DownloadFileInfo> GetDownloadFileInfo(std::string name);
	bool DeleteDownloadFileInfo(const std::string& name);
private:
	LogicSystem();
	std::vector<std::shared_ptr<LogicWorker>> logicWorkers_;
	//std::map<std::string, std::shared_ptr<FileInfo>> md5FileMap_;// file_name : FileInfo
	//std::mutex mtx_;
};

#endif
