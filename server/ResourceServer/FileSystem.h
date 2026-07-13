#ifndef  FILESYSTEM_H
#define  FILESYSTEM_H

#include "global.h"
#include "SingleTon.h"
#include "FileWorker.h"
#include "DownloadWorker.h"

class CSession;

class FileSystem : public SingleTon<FileSystem>
{
	friend class SingleTon<FileSystem>;
public:
	~FileSystem();
	// 向外提供接口，根据连接是 uuid 来判断 投递 到 哪个文件上传的逻辑线程
	void postTaskToQue(std::shared_ptr<FileTask> task, int index);
	// 向外提供接口，根据连接是 uuid 来判断 投递 到 哪个文件下载的逻辑线程
	void PostDownloadTaskToQue(std::shared_ptr<DownloadTask> msg, int index);
private:
	FileSystem();
	std::vector<std::shared_ptr<FileWorker>> fileWorkers_;
	std::vector<std::shared_ptr<DownloadWorker>> downloadWorkers_;
};
#endif


