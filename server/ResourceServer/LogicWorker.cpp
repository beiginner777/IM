#include "LogicWorker.h"
#include "MsgNode.h"
#include "LogicSystem.h"
#include "ConfigManager.h"
#include "CSession.h"
#include "CServer.h"
#include "FileSystem.h"
#include "FileWorker.h"
#include "DownloadWorker.h"
LogicWorker::LogicWorker()
	: b_stop_(false)
{
	registerFunctionCallbacks();
	work_thread_ = std::thread(&LogicWorker::dealTask, this);
}	
LogicWorker::~LogicWorker()
{
	// todo ...
}
void LogicWorker::registerFunctionCallbacks()
{
	handlers_[ID_UPLOAD_HEAD_ICON_REQ] = std::bind(&LogicWorker::uploadHeadIcon, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	handlers_[ID_IMAGE_CHAT_MSG_REQ] = std::bind(&LogicWorker::uploadFile, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	handlers_[ID_SYNC_FILE_REQ] = std::bind(&LogicWorker::syncFile, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	handlers_[ID_DOWN_LOAD_FILE_REQ] = std::bind(&LogicWorker::downloadFile, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	handlers_[ID_IMG_CHAT_CONTINUE_UPLOAD_REQ] = std::bind(&LogicWorker::imgChatContinueUpload, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	handlers_[ID_FILE_CONTINUE_DOWNLOAD_REQ] = std::bind(&LogicWorker::fileContinueDownload, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	// StatusServer 响应处理
	handlers_[ID_REGISTER_RSP] = std::bind(&LogicWorker::handleRegisterRsp, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	handlers_[ID_HEADT_CHECK_RSP] = std::bind(&LogicWorker::handleHeartCheckRsp, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}
void LogicWorker::dealTask()
{
	while (true)
	{
		std::unique_lock<std::mutex> locker(mtx_);
		while (que_.empty() && !b_stop_)
		{
			std::cout << "LoginSystem is waiting for data . . ." << std::endl;
			cond_.wait(locker);
		}
		// 逻辑层停止工作
		if (b_stop_)
		{
			while (!que_.empty())
			{
				std::shared_ptr<LogicNode> node = que_.front();
				que_.pop();
				// 获取逻辑结点对应的 消息id
				short msgId = node->recvNode_->msg_id_;
				if (handlers_.count(msgId)) {
					std::cout << "handle task whose id = " << msgId << ":" << std::endl;
					handlers_[msgId](node->session_, msgId, std::string(node->recvNode_->data_, node->recvNode_->totol_len_));
				}
				else {
					std::cout << "system error: can't find FunctinCallback: " << msgId << std::endl;
				}
			}
			// detail break
			break;
		}
		// 逻辑层没有退出，那么就正常取数据
		if (!que_.empty())
		{
			std::shared_ptr<LogicNode> node = que_.front();
			que_.pop();
			// 获取逻辑结点对应的 消息id
			short msgId = node->recvNode_->msg_id_;
			if (handlers_.count(msgId)) {
				std::cout << "handle task whose id = " << msgId << ":" << std::endl;
				handlers_[msgId](node->session_, msgId, std::string(node->recvNode_->data_, node->recvNode_->totol_len_));
			}
			else {
				std::cout << "system error: can't find FunctinCallback: " << msgId << std::endl;
			}
		}
	}
}
void LogicWorker::uploadHeadIcon(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	std::cout << "upload head icon msgId = " << msgId << std::endl;
	Json::Value root;
	Json::Reader reader;
	Json::Value rtvalue;
	if (!reader.parse(msgData, root))
	{
		std::cout << "parse uploadFile msgData failed." << std::endl;
		rtvalue["code"] = 3;
		rtvalue["msg"] = "parse msgData to json failed";
		return;
	}
	std::string fileName = root["filename"].asString();
	int seq = root["seq"].asInt();
	int lastSeq = root["lastseq"].asInt();
	int transferredSize = root["transferredsize"].asInt();
	int totolSize = root["totolsize"].asInt();
	std::string data = root["data"].asString();
	std::string md5 = root["md5"].asString();
	int uid = root["uid"].asInt();
	std::string token = root["token"].asString();
	session->setUserId(uid);
	auto cfg = ConfigManager::getInstance();
	std::string uploadPath = cfg["SelfServer"]["UploadPath"];
	std::string fullPath = uploadPath + "/" + fileName;
	if (seq == 1)
	{
		// 第一个包验证token是否正确 to do ...
		//构造数据存储
		auto file_info = std::make_shared<FileInfo>();
		file_info->uid_ = uid;
		file_info->filePath_ = fullPath;
		file_info->name_ = fileName;
		file_info->seq_ = seq;
		file_info->totolSize_ = totolSize;
		file_info->transfferredSize_ = transferredSize;
		file_info->last_seq_ = lastSeq;
		bool ret = LogicSystem::getInstance()->addMd5FileInfo(fileName, file_info);
		if (!ret) {
			// 文件信息保存到 redis 失败
			rtvalue["error"] = 5;
			rtvalue["message"] = "save file info to redis failed";
			return;
		}
		else {
			std::cout << "file info has been saved to redis successfully." << std::endl;
		}
	}
	else
	{
		auto file_info = LogicSystem::getInstance()->getFileInfo(fileName);
		if (file_info == nullptr) {
			// 没有将文件Add到MD5FileMap中
			rtvalue["error"] = 4;
			return;
		}
	}
	std::shared_ptr<FileTask> task = std::make_shared<FileTask>(session, msgId, md5, fileName, seq, totolSize, transferredSize, lastSeq, data);
	// 根据文件名字来决定 投递 到 哪个 FileWorker 线程
	std::hash<std::string> hash_fn;
	size_t hash_value = hash_fn(fileName); // 根据 文件名 生成哈希值
	int index = hash_value % FILEWORKER_COUNT;
	FileSystem::getInstance()->postTaskToQue(task, index);
}
void LogicWorker::uploadFile(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	// 在发送前添加点小延迟，避免快速连续发送
	//std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10ms延迟
	std::cout << "uploadFile msgId = " << msgId << std::endl;
	Json::Value root;
	Json::Reader reader;
	Json::Value rtvalue;
	if(!reader.parse(msgData, root))
	{
		std::cout << "parse uploadFile msgData failed." << std::endl;
		rtvalue["code"] = 3;
		rtvalue["msg"] = "parse msgData to json failed";
		return;
	}
	std::string fileName = root["filename"].asString(); // unqiue_name
	int seq = root["seq"].asInt();
	int lastSeq = root["lastseq"].asInt();
	int transferredSize = root["transferredsize"].asInt();
	int totolSize = root["totolsize"].asInt();
	std::string data = root["data"].asString();
	std::string md5 = root["md5"].asString();
	int type = root["type"].asInt();
	int uid = root["uid"].asInt();
	std::string token = root["token"].asString();
	auto cfg = ConfigManager::getInstance();
	std::string uploadPath = cfg["SelfServer"]["UploadPath"];
	std::string fullPath = uploadPath + "/" + fileName;
	if (seq == 1) 
	{
		session->setUserId(uid);
		// 第一个包验证token是否正确 to do ...
		//构造数据存储
		auto file_info = std::make_shared<FileInfo>();
		file_info->uid_ = uid;
		file_info->filePath_ = fullPath;
		file_info->name_ = fileName;
		file_info->seq_ = seq;
		file_info->totolSize_ = totolSize;
		file_info->transfferredSize_ = transferredSize;
		file_info->last_seq_ = lastSeq;
		LogicSystem::getInstance()->addMd5FileInfo(fileName, file_info);
	}
	else
	{
		auto file_info = LogicSystem::getInstance()->getFileInfo(fileName);
		if (file_info == nullptr) {
			// 没有将文件Add到MD5FileMap中
			rtvalue["error"] = 4;
			return;
		}
		file_info->seq_ = seq;
		file_info->transfferredSize_ = transferredSize;
		LogicSystem::getInstance()->addMd5FileInfo(fileName, file_info);
	}
	std::shared_ptr<FileTask> task = std::make_shared<FileTask>(session, msgId, md5, fileName, seq, totolSize, transferredSize, lastSeq, data, type);
	// 根据文件名字来决定 投递 到 哪个 FileWorker 线程
	std::hash<std::string> hash_fn;
	size_t hash_value = hash_fn(fileName); // 根据 文件名 生成哈希值
	int index = hash_value % FILEWORKER_COUNT;
	FileSystem::getInstance()->postTaskToQue(task, index);
	/*rtvalue["code"] = SUCCESS;
	rtvalue["mesage"] = "upload file task has been posted to FileSystem";
	rtvalue["filenname"] = fileName;
	rtvalue["totol_size"] = totolSize;
	rtvalue["trans_size"] = transferredSize;
	rtvalue["seq"] = seq;
	rtvalue["lastseq"] = lastSeq;
	rtvalue["md5"] = md5;
	rtvalue["uid"] = uid;*/
}
void LogicWorker::syncFile(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	Json::Value root;
	Json::Reader reader;
	Json::Value rtvalue;
	
	Defer defer([session, this, &rtvalue] {
		session->Send(rtvalue.toStyledString(), ID_SYNC_FILE_RSP);
		});
	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "sync file request processed successfully";
	if (!reader.parse(msgData, root)) {
		std::cout << "parse syncFile msgData failed." << std::endl;
		rtvalue["code"] = 3;
		rtvalue["msg"] = "parse msgData to json failed";
		return;
	}
	std::string md5 = root["md5"].asString();
	auto file_info = LogicSystem::getInstance()->getFileInfo(md5);
	if (file_info == nullptr) {
		rtvalue["code"] = 4;
		rtvalue["message"] = "file info not found";
		return;
	}
	rtvalue["lastseq"] = file_info->last_seq_;
	rtvalue["seq"] = file_info->seq_;
	rtvalue["transfer_size"] = file_info->transfferredSize_;
	rtvalue["total_size"] = file_info->totolSize_;
	rtvalue["md5"] = md5;
	rtvalue["file_name"] = file_info->name_;
}
void LogicWorker::downloadFile(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	std::cout << "Download File msgId = " << msgId << std::endl;
	std::cout << "msgData = " << msgData << std::endl;
	Json::Value root;
	Json::Reader reader;
	Json::Value rtvalue;
	if (!reader.parse(msgData, root))
	{
		std::cout << "parse Download File msgData failed." << std::endl;
		rtvalue["code"] = 3;
		rtvalue["msg"] = "parse msgData to json failed";
		return;
	}
	std::string download_file = root["download_file"].asString();
	int seq = root["seq"].asInt();
	int last_seq = root["last_seq"].asInt();
	int trans_size = root["trans_size"].asInt();
	int total_size = root["total_size"].asInt();
	int uid = root["uid"].asInt();
	std::string token = root["token"].asString();
	int icon_uid = root["icon_uid"].asInt();
	std::string client_save_path = root["client_save_path"].asString();	
	int download_file_type = root["download_file_type"].asInt();
	session->setUserId(uid);
	auto cfg = ConfigManager::getInstance();
	if (seq == 1)
	{
		// 第一个包验证token是否正确 to do ...
		
	}
	else
	{
		auto file_info = LogicSystem::getInstance()->GetDownloadFileInfo(download_file);
		if (file_info == nullptr) {
			// 没有将文件Add到DownloadFileMap中，那么就返回错误
			rtvalue["error"] = 4;
			return;
		}
	}
	std::shared_ptr<DownloadTask> task = std::make_shared<DownloadTask>(session, download_file,seq,last_seq,trans_size,total_size, client_save_path, (Download_File_Type)download_file_type, icon_uid);
	// 根据文件名字来决定 投递 到 哪个 DownloadWorker 线程
	std::hash<std::string> hash_fn;
	size_t hash_value = hash_fn(download_file); // 根据 文件名 生成哈希值
	int index = hash_value % FILEWORKER_COUNT;
	FileSystem::getInstance()->PostDownloadTaskToQue(task, index);
}
void LogicWorker::imgChatContinueUpload(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	Json::Value root;
	Json::Reader reader;
	Json::Value rtvalue;
	Defer defer([session, this, &rtvalue] {
		session->Send(rtvalue.toStyledString(), ID_IMG_CHAT_CONTINUE_UPLOAD_RSP);
		});
	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "sync file request processed successfully";
	if (!reader.parse(msgData, root)) {
		std::cout << "parse syncFile msgData failed." << std::endl;
		rtvalue["code"] = 3;
		rtvalue["msg"] = "parse msgData to json failed";
		return;
	}
	int uid = root["uid"].asInt();
	std::string token = root["token"].asString();
	std::string unique_name = root["unique_name"].asString();
	std::string md5 = root["md5"].asString();
	std::cout << "uid = " << uid << " request to continue upload chat image.";
	session->setUserId(uid);
	auto file_info = LogicSystem::getInstance()->getFileInfo(unique_name);
	if(file_info == nullptr) {
		rtvalue["code"] = 4;
		rtvalue["message"] = "file info not found";
		return;
	}
	rtvalue["uid"] = file_info->uid_;
	rtvalue["last_seq"] = file_info->last_seq_;
	rtvalue["seq"] = file_info->seq_;
	rtvalue["trans_size"] = file_info->transfferredSize_;
	rtvalue["total_size"] = file_info->totolSize_;
	rtvalue["md5"] = md5;
	rtvalue["unique_name"] = file_info->name_;
}
void LogicWorker::fileContinueDownload(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	Json::Value root;
	Json::Reader reader;
	Json::Value rtvalue;
	Defer defer([session, this, &rtvalue] {
		session->Send(rtvalue.toStyledString(), ID_FILE_CONTINUE_DOWNLOAD_RSP);
		});
	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "sync file request processed successfully";
	if (!reader.parse(msgData, root)) {
		std::cout << "parse syncFile msgData failed." << std::endl;
		rtvalue["code"] = 3;
		rtvalue["msg"] = "parse msgData to json failed";
		return;
	}
	int uid = root["uid"].asInt();
	std::string token = root["token"].asString();
	std::string unique_name = root["unique_name"].asString();
	std::cout << "uid = " << uid << " request to continue download chat image.";
	session->setUserId(uid);
	std::shared_ptr<DownloadFileInfo> file_info = LogicSystem::getInstance()->GetDownloadFileInfo(unique_name);
	if (file_info == nullptr) {
		rtvalue["code"] = 4;
		rtvalue["message"] = "file info not found";
		return;
	}
	rtvalue["uid"] = file_info->uid_;
	rtvalue["download_file"] = file_info->download_file_;
	rtvalue["seq"] = file_info->seq_;
	rtvalue["last_seq"] = file_info->last_seq_;
	rtvalue["trans_size"] = file_info->trans_size_;
	rtvalue["total_size"] = file_info->total_size_;
	rtvalue["client_save_path"] = file_info->client_save_path_;
	rtvalue["download_file_type"] = file_info->download_file_type_;
}
std::string LogicWorker::base64_decode(const std::string& in)
{
	const std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";
	// 创建解码表
	std::vector<int> decoding_table(256, -1);
	for (int i = 0; i < 64; i++) {
		decoding_table[base64_chars[i]] = i;
	}
	int input_length = in.size();
	int i = 0;
	std::string out;
	out.reserve((input_length * 3) / 4);
	while (i < input_length) {
		// 解码4个字符为3个字节
		int sextet_a = in[i] == '=' ? 0 & i++ : decoding_table[static_cast<int>(in[i++])];
		int sextet_b = in[i] == '=' ? 0 & i++ : decoding_table[static_cast<int>(in[i++])];
		int sextet_c = in[i] == '=' ? 0 & i++ : decoding_table[static_cast<int>(in[i++])];
		int sextet_d = in[i] == '=' ? 0 & i++ : decoding_table[static_cast<int>(in[i++])];
		if (sextet_a == -1 || sextet_b == -1 || sextet_c == -1 || sextet_d == -1) {
			throw std::runtime_error("Invalid base64 character");
		}
		int triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);
		if (in.length() > i - 3 && in[i - 2] == '=') {
			// 2个填充字符，只输出1个字节
			out.push_back(static_cast<char>((triple >> 16) & 0xFF));
		}
		else if (in.length() > i - 2 && in[i - 1] == '=') {
			// 1个填充字符，输出2个字节
			out.push_back(static_cast<char>((triple >> 16) & 0xFF));
			out.push_back(static_cast<char>((triple >> 8) & 0xFF));
		}
		else {
			// 无填充字符，输出3个字节
			out.push_back(static_cast<char>((triple >> 16) & 0xFF));
			out.push_back(static_cast<char>((triple >> 8) & 0xFF));
			out.push_back(static_cast<char>(triple & 0xFF));
		}
	}
	return out;
}
void LogicWorker::postMsgToQue(std::shared_ptr<LogicNode> logicNode)
{
	std::lock_guard<std::mutex> locker(mtx_);
	que_.push(logicNode);
	cond_.notify_one();
}
// 处理 StatusServer 注册响应
void LogicWorker::handleRegisterRsp(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(msgData, root)) {
		std::cout << "[ResourceServer] handleRegisterRsp: parse json failed." << std::endl;
		return;
	}
	int code = root["code"].asInt();
	if (code != SUCCESS) {
		std::cout << "[ResourceServer] Register to StatusServer failed: " << root["message"].asString() << std::endl;
		exit(-1);
	}
	std::cout << "[ResourceServer] Registered to StatusServer successfully." << std::endl;
	session->server_->startReceiceConnections();
}
// 处理 StatusServer 心跳响应（更新心跳时间戳）
void LogicWorker::handleHeartCheckRsp(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	// 心跳响应只需记录，无需额外操作
	// CSession 本身不需要心跳超时检测（StatusServer 的 CSession 负责）
}
