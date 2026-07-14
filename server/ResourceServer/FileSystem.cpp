#include "FileSystem.h"
FileSystem::FileSystem()
{
	for (int i = 0; i < FILEWORKER_COUNT; ++i) {
		std::shared_ptr<FileWorker> worker = std::make_shared<FileWorker>();
		fileWorkers_.push_back(worker);
	}
	for (int i = 0; i < DOWNLOADWORKER_COUNT; ++i) {
		std::shared_ptr<DownloadWorker> worker = std::make_shared<DownloadWorker>();
		downloadWorkers_.push_back(worker);
	}
}
FileSystem::~FileSystem()
{
}

void FileSystem::postTaskToQue(std::shared_ptr<FileTask> task, int index)
{
	fileWorkers_[index]->postTaskToQue(task);
}

void FileSystem::PostDownloadTaskToQue(std::shared_ptr<DownloadTask> msg, int index)
{
	downloadWorkers_[index]->postTaskToQue(msg);
}
