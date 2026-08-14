#include "LogicSystem.h"
#include "MsgNode.h"
#include "CSession.h"
#include "json/json.h"
#include "ConfigManager.h"
#include <fstream>
#include "LogicWorker.h"
#include "RedisManager.h"
LogicSystem::LogicSystem()
{
	for (int i = 0; i < LOGICWORKER_COUNT; ++i) {
		std::shared_ptr<LogicWorker> worker = std::make_shared<LogicWorker>();
		logicWorkers_.push_back(worker);
	}
}
LogicSystem::~LogicSystem()
{
	std::cout << "LogicSystem is destructed." << std::endl;
}

void LogicSystem::postMsgToQue(std::shared_ptr<LogicNode> logicNode, int index)
{
	logicWorkers_[index]->postMsgToQue(logicNode);
}

bool LogicSystem::addMd5FileInfo(std::string name, std::shared_ptr<FileInfo> fileInfo)
{
	// 将文件上传进度的消息保存在 redis中
	std::string key = FILEUPLOADFREFIX + name;
	Json::Value root;
	root["uid"] = fileInfo->uid_;
	root["file_path_str"] = fileInfo->filePath_;
	root["name"] = fileInfo->name_;
	root["seq"] = fileInfo->seq_;
	root["last_seq"] = fileInfo->last_seq_;
	root["total_size"] = fileInfo->totolSize_;
	root["trans_size"] = fileInfo->transfferredSize_;
	root["last_acked"] = fileInfo->last_acked_seq_;
	Json::Value pending(Json::arrayValue);
	for (int s : fileInfo->pending_seqs_) pending.append(s);
	root["pending_seqs"] = pending;
	auto file_info_str = root.toStyledString();
	auto redis_key = FILEUPLOADFREFIX + name;
	return RedisManager::getInstance()->SetExp(redis_key, file_info_str, FILEINFOEXISTTIME);
}

std::shared_ptr<FileInfo> LogicSystem::getFileInfo(std::string name)
{
	// 从 redis 中获取文件上传进度的信息
	std::shared_ptr<FileInfo> file_info = std::make_shared<FileInfo>();
	std::string redis_key = FILEUPLOADFREFIX + name;
	auto file_info_str = RedisManager::getInstance()->Get(redis_key, true);
	if(file_info_str.empty()) {
		return nullptr;
	}
	// 解析 json 字符串
	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(file_info_str, root)) {
		std::cout << "parse file_info_str from redis failed." << std::endl;
		return nullptr;
	}
	file_info->uid_ = root["uid"].asInt();
	file_info->filePath_ = root["file_path_str"].asString();
	file_info->name_ = root["name"].asString();
	file_info->seq_ = root["seq"].asInt();
	file_info->last_seq_ = root["last_seq"].asInt();
	file_info->totolSize_ = root["total_size"].asInt();
	file_info->transfferredSize_ = root["trans_size"].asInt();
	file_info->last_acked_seq_ = root.get("last_acked", 0).asInt();
	file_info->pending_seqs_.clear();
	if (root.isMember("pending_seqs")) {
		for (const auto& v : root["pending_seqs"]) file_info->pending_seqs_.insert(v.asInt());
	}
	return file_info;
}

bool LogicSystem::DeleteMd5FileInfo(const std::string& name)
{
	// 删除 redis 中保存的文件上传进度的信息
	return RedisManager::getInstance()->Del(FILEUPLOADFREFIX + name);
}

bool LogicSystem::addDownloadFileInfo(std::string name, std::shared_ptr<DownloadFileInfo> fileInfo)
{
	// 将文件上传进度的消息保存在 redis中
	std::string key = FILEUPLOADFREFIX + name;
	Json::Value root;
	root["uid"] = fileInfo->uid_;
	root["download_file"] = fileInfo->download_file_;
	root["seq"] = fileInfo->seq_;
	root["last_seq"] = fileInfo->last_seq_;
	root["trans_size"] = fileInfo->trans_size_;
	root["total_size"] = fileInfo->total_size_;
	root["client_save_path"] = fileInfo->client_save_path_;
	root["download_file_type"] = fileInfo->download_file_type_;
	auto file_info_str = root.toStyledString();
	auto redis_key = FILEDOWNLOADFREFIX + name;
	return RedisManager::getInstance()->SetExp(redis_key, file_info_str, DOWNLOADFILEEXISTTIME);
}

std::shared_ptr<DownloadFileInfo> LogicSystem::GetDownloadFileInfo(std::string name)
{
	// 从 redis 中获取文件上传进度的信息
	std::shared_ptr<DownloadFileInfo> file_info = std::make_shared<DownloadFileInfo>();
	std::string redis_key = FILEDOWNLOADFREFIX + name;
	auto file_info_str = RedisManager::getInstance()->Get(redis_key);
	if (file_info_str.empty()) {
		return nullptr;
	}
	// 解析 json 字符串
	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(file_info_str, root)) {
		std::cout << "parse file_info_str from redis failed." << std::endl;
		return nullptr;
	}
	file_info->uid_ = root["uid"].asInt();
	file_info->download_file_ = root["download_file"].asString();
	file_info->seq_ = root["seq"].asInt();
	file_info->last_seq_ = root["last_seq"].asInt();
	file_info->trans_size_ = root["trans_size"].asInt();
	file_info->total_size_ = root["total_size"].asInt();
	file_info->client_save_path_ = root["client_save_path"].asString();
	file_info->download_file_type_ = root["download_file_type"].asInt();
	return file_info;
}

bool LogicSystem::DeleteDownloadFileInfo(const std::string& name)
{
	// 删除 redis 中保存的文件上传进度的信息
	return RedisManager::getInstance()->Del(FILEDOWNLOADFREFIX + name);
}
