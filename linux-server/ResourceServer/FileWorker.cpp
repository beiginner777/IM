#include "FileWorker.h"
#include "CSession.h"
#include "ConfigManager.h"
#include "LogicSystem.h"
#include "MysqlManager.h"
#include "RedisManager.h"
#include "ResouceServerClient.h"
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <unordered_set>
FileWorker::FileWorker()
	: b_stop_(false)
{
	registerHandlers();
	work_thread_ = std::thread(&FileWorker::dealTask, this);
}
FileWorker::~FileWorker()
{
	// todo ...
}

void FileWorker::registerHandlers()
{
	handlers_[ID_UPLOAD_HEAD_ICON_REQ] = std::bind(&FileWorker::handleUploadHeadIcon, this, std::placeholders::_1);
	handlers_[ID_IMAGE_CHAT_MSG_REQ] = std::bind(&FileWorker::handleUploadFile, this, std::placeholders::_1);
}

void FileWorker::dealTask()
{
	while (true)
	{
		std::unique_lock<std::mutex> locker(mtx_);
		while (que_.empty() && !b_stop_)
		{
			std::cout << "LoginSystem is waiting for data . . ." << std::endl;
			cond_.wait(locker);
		}
		// 文件处理层停止工作
		if (b_stop_)
		{
			while (!que_.empty())
			{
				std::shared_ptr<FileTask> task = que_.front();
				que_.pop();
				taskHandler(task);
			}
			// detail break
			break;
		}
		// 文件处理层没有退出，那么就正常取数据
		if (!que_.empty())
		{
			std::shared_ptr<FileTask> task = que_.front();
			que_.pop();
			taskHandler(task);
		}
	}
}

std::string FileWorker::base64_decode(const std::string& in)
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

// ===== 文件上传安全校验（治路径穿越 / 稀疏文件 DoS / 恶意文件）=====

// md5 校验：严格 32 位 hex，防止恶意字符串混入落盘名
static bool isValidMd5(const std::string& md5)
{
	if (md5.size() != 32) return false;
	for (char c : md5) {
		if (!isxdigit((unsigned char)c)) return false;
	}
	return true;
}

// 提取扩展名（小写）
static std::string extractExt(const std::string& name)
{
	size_t dot = name.find_last_of('.');
	if (dot == std::string::npos || dot + 1 >= name.size()) return "";
	std::string ext = name.substr(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return (char)::tolower(c); });
	return ext;
}

// 扩展名白名单
static bool isExtensionAllowed(const std::string& ext)
{
	static const std::unordered_set<std::string> whitelist = {
		"jpg", "jpeg", "png", "gif", "webp",   // 图片
		"pdf", "doc", "docx", "txt",           // 文档
		"zip",                                  // 压缩包
		"mp4", "mp3"                            // 音视频
	};
	return whitelist.count(ext) > 0;
}

// 魔数校验：读文件头判断真实类型，防「改扩展名伪装」
static bool validateMagicBytes(const std::string& ext, const std::string& data)
{
	if (data.empty()) return false;
	auto b = [&data](size_t i) { return (unsigned char)data[i]; };
	if (ext == "jpg" || ext == "jpeg") {
		return data.size() >= 3 && b(0) == 0xFF && b(1) == 0xD8 && b(2) == 0xFF;
	}
	if (ext == "png") {
		return data.size() >= 4 && b(0) == 0x89 && b(1) == 0x50 && b(2) == 0x4E && b(3) == 0x47;
	}
	if (ext == "gif") {
		return (data.size() >= 6 && data.compare(0, 6, "GIF87a") == 0) ||
		       (data.size() >= 6 && data.compare(0, 6, "GIF89a") == 0);
	}
	if (ext == "webp") {
		return data.size() >= 12 && data.compare(0, 4, "RIFF") == 0 && data.compare(8, 4, "WEBP") == 0;
	}
	if (ext == "pdf") {
		return data.size() >= 4 && data.compare(0, 4, "%PDF") == 0;
	}
	if (ext == "zip" || ext == "docx") {
		return data.size() >= 4 && b(0) == 0x50 && b(1) == 0x4B;
	}
	if (ext == "mp4") {
		return data.size() >= 12 && data.compare(4, 4, "ftyp") == 0;
	}
	if (ext == "mp3") {
		return (data.size() >= 3 && data.compare(0, 3, "ID3") == 0) ||
		       (data.size() >= 2 && b(0) == 0xFF && (b(1) & 0xE0) == 0xE0);
	}
	// doc/txt 无可靠魔数，白名单兜底放行
	return true;
}

// 显示名清洗：白名单字符 + 长度 ≤ 128，防日志/响应注入
static std::string sanitizeDisplayName(const std::string& name)
{
	std::string out;
	out.reserve(name.size());
	for (char c : name) {
		if (isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-') {
			out.push_back(c);
		}
		if (out.size() >= 128) break;
	}
	return out;
}

// 校验上传参数 + 生成服务端落盘名。返回 0 成功，非 0 为错误码
static int validateUploadAndBuildName(int uid, const std::string& md5, const std::string& fileName,
                                      int seq, int lastSeq, int totolSize, size_t chunkLen,
                                      std::string& serverName, std::string& displayName, std::string& ext)
{
	// 分片参数校验（防稀疏文件 DoS）
	if (seq < 1 || lastSeq < 1 || seq > lastSeq) {
		return ERROR_FILE_SEQ_INVALID;
	}
	if (chunkLen > (size_t)MAX_FILE_LEN) {
		return ERROR_FILE_CHUNK_TOO_LARGE;
	}
	auto cfg = ConfigManager::getInstance();
	long long maxFileSize = std::atoll(cfg["FileLimit"]["MaxFileSize"].c_str());
	int maxSeq = std::atoi(cfg["FileLimit"]["MaxSeq"].c_str());
	if (maxFileSize <= 0) maxFileSize = 104857600;  // 默认 100MB
	if (maxSeq <= 0) maxSeq = 51200;
	if (lastSeq > maxSeq) {
		return ERROR_FILE_SEQ_INVALID;
	}
	if ((long long)totolSize > maxFileSize) {
		return ERROR_FILE_SIZE_TOO_LARGE;
	}
	// md5 校验（32 位 hex，杜绝客户端可控字符进路径）
	if (!isValidMd5(md5)) {
		return ERROR_FILE_MD5_INVALID;
	}
	// 扩展名白名单
	ext = extractExt(fileName);
	if (!isExtensionAllowed(ext)) {
		return ERROR_FILE_TYPE_NOT_ALLOWED;
	}
	// 文件名白名单校验：仅允许 [a-zA-Z0-9._-]，拒绝路径穿越字符（../ \ 等）
	for (char c : fileName) {
		if (!(isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-')) {
			return ERROR_FILE_TYPE_NOT_ALLOWED;
		}
	}
	// 落盘名 = 校验后的 fileName（客户端 UUID 名天然安全，白名单兜底防穿越）
	serverName = fileName;
	// 显示名清洗
	displayName = sanitizeDisplayName(fileName);
	return 0;
}

void FileWorker::taskHandler(std::shared_ptr<FileTask> task)
{
	if(handlers_.count(task->req_id_) != 0){
		handlers_[task->req_id_](task);
		return;
	}
	std::cout << "system error: can't find FunctinCallback: " << task->req_id_ << std::endl;
}

void FileWorker::notifyFriendNewHeadIcon(int self_id, std::string fileName)
{
	// 1 MysqlManager去查找self_id的好友列表
	std::vector<int> friendList;
	bool ret = MysqlManager::getInstance()->GetFriendList(self_id, friendList);
	if (ret != SUCCESS) {
		std::cout << "Get friend list from mysql failed. uid = " << self_id << std::endl;
		return;
	}
	// 2 RedisManager去查找每个好友当前在线的ChatServer
	std::vector<std::string> keys;
	for(int& friend_id : friendList){
		std::string friend_chat_server_key = USERIPPREFIX + std::to_string(friend_id);
		keys.push_back(friend_chat_server_key);
	}
	std::unordered_map<std::string, std::string> servers;
	ret = RedisManager::getInstance()->MGet(keys, servers);
	if(ret == false){
		std::cout << "Redis MGet friend chat server failed. uid = " << self_id << std::endl;
		return;
	}
	// 3 将在线的好友和离线的好友分开处理
	std::vector<int> onlineFriends;
	std::vector<int> offlineFriends;
	for (auto& server : servers) {
		std::string key = server.first;
		std::string value = server.second;
		if (value != "") {
			std::cout << "friend online: " << key << " in chat server: " << value << std::endl;
			onlineFriends.push_back(std::stoi(key.substr(strlen(USERIPPREFIX))));
		}
		else {
			offlineFriends.push_back(std::stoi(key.substr(strlen(USERIPPREFIX))));
		}
	}
	// 4 通过Rpc调用对应的ChatServer，通知在线的好友更新头像
	for (int online_friend_id : onlineFriends) {
		// Grpc调用ChatServer去通知Client端有新的图片消息
		std::string key = USERIPPREFIX + std::to_string(online_friend_id);
		std::string server_ip = servers[key];
		if (server_ip == "") {
			std::cerr << "online_friend_id: " << online_friend_id << " is not in any ChatSerevr." << std::endl;
			return;
		}
		std::cout << "Call ChatServer to Notify uid = " << online_friend_id << " friend icon change success.\n";

		NotifyFriendIconChangeReq req;
		req.set_uid(online_friend_id);
		req.set_redis_id(REDIS_ID::REDIS_ID_FRIEND_ICON_CHANGE);
		req.set_friend_id(self_id);
		req.set_messgae("Friend Icon Change");
		req.set_friend_icon(fileName);
		NotifyFriendIconChangeRsp rsp = ResouceServerClient::getInstance()->NotifyFriendIconChange(server_ip, req);
		if (rsp.error() == SUCCESS) {
			std::cout << "Notify msg(friend icon change) to Server(" << server_ip << ") friend_id = " << online_friend_id << " success.";
		}
		else {
			// 将消息存在redis中，等待下次上线再获取
			Json::Value offlineMsg;
			offlineMsg["redis_id"] = REDIS_ID::REDIS_ID_FRIEND_ICON_CHANGE;
			offlineMsg["friend_id"] = self_id;
			offlineMsg["messgae"] = "好友头像更新";
			offlineMsg["friend_icon"] = fileName;
			RedisManager::getInstance()->pushOfflineMessage(online_friend_id, offlineMsg.toStyledString());
		}
	}
	// 5 将消息存储在Redis，离线的好友在下次登录时获取最新的头像信息
	for (int offline_friend_id : offlineFriends) {
		Json::Value offlineMsg;
		offlineMsg["redis_id"] = REDIS_ID::REDIS_ID_FRIEND_ICON_CHANGE;
		offlineMsg["friend_id"] = self_id;
		offlineMsg["messgae"] = "好友头像更新";
		offlineMsg["friend_icon"] = fileName;
		RedisManager::getInstance()->pushOfflineMessage(offline_friend_id, offlineMsg.toStyledString());
	}
}

void FileWorker::handleUploadHeadIcon(std::shared_ptr<FileTask> task)
{
	std::shared_ptr<CSession> session = task->session_;
	int uid = task->session_->getUserId();
	Json::Value rtvalue;
	Defer defer([session, this, &rtvalue]() {
		// 发送响应给客户端
		session->Send(rtvalue.toStyledString(), ID_UPLOAD_HEAD_ICON_RSP);
		});
	rtvalue["code"] = 0;
	rtvalue["message"] = "upload success";
	std::string md5 = task->md5_;
	int seq = task->seq_;
	int lastSeq = task->lastSeq_;
	std::string fileName = task->name_;
	int transferredSize = task->transfferredSize_;
	int totolSize = task->totolSize_;
	std::string data = task->data_;
	// 对base64编码的数据进行解码
	std::string decodedData = base64_decode(task->data_);
	// 安全校验 + 生成服务端落盘名（防路径穿越 / 稀疏文件 DoS / 恶意文件）
	std::string serverName, displayName, ext;
	int vret = validateUploadAndBuildName(uid, md5, fileName, seq, lastSeq, totolSize, decodedData.size(), serverName, displayName, ext);
	if (vret != 0) {
		rtvalue["code"] = vret;
		rtvalue["message"] = "file upload rejected";
		return;
	}
	if (seq == 1 && !validateMagicBytes(ext, decodedData)) {
		rtvalue["code"] = ERROR_FILE_MAGIC_MISMATCH;
		rtvalue["message"] = "file magic bytes mismatch";
		return;
	}
	auto cfg = ConfigManager::getInstance();
	std::string uploadPath = cfg["SelfServer"]["UploadPath"];
	std::string fullPath = uploadPath + "/" + serverName;
	std::ofstream ofs;
	// 定位写入（非追加）：乱序/重传包也写到正确的文件偏移
	if (seq == 1) {
		ofs.open(fullPath, std::ios::binary | std::ios::out);
	}
	else {
		ofs.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
		if (!ofs.is_open()) { ofs.clear(); ofs.open(fullPath, std::ios::binary | std::ios::out); }
	}
	if (!ofs.is_open()) {
		std::cout << "文件" << fullPath << "打开失败" << std::endl;
		rtvalue["code"] = 1;
		rtvalue["msg"] = "open file failed";
		rtvalue["seq"] = seq;
		return;
	}
	ofs.seekp((seq - 1) * MAX_FILE_LEN);
	ofs.write(decodedData.c_str(), decodedData.size());
	if (!ofs) {
		std::cout << "写入" << fullPath << "失败" << std::endl;
		rtvalue["code"] = 2;
		rtvalue["message"] = "write into file failed";
		rtvalue["seq"] = seq;
		return;
	}
	ofs.close();
	std::cout << "write " << displayName << " -> " << serverName << "(" << seq << "/ " << lastSeq << ")" << " into " << fullPath << " success." << std::endl;
	rtvalue["uid"] = uid;
	rtvalue["seq"] = seq;
	rtvalue["lastseq"] = lastSeq;
	rtvalue["file"] = serverName;
	rtvalue["md5"] = md5;
	rtvalue["totol_size"] = totolSize;
	rtvalue["trans_size"] = transferredSize;
	if (seq == lastSeq) {
		// 删除上传文件的信息
		LogicSystem::getInstance()->DeleteMd5FileInfo(fileName);
		// 将redis中的用户信息删除(to do ... 最好是重新设置新的数据)
		std::string base_info = USERBASEINFO + std::to_string(uid);
		RedisManager::getInstance()->Del(base_info);
		// 将头像信息修改到Mysql数据库
		int ret = MysqlManager::getInstance()->updateUserIcon(uid, serverName);
		if (ret != 0) {
			std::cout << "update user icon in mysql failed. uid = " << uid << std::endl;
			rtvalue["code"] = ERROR_UPDATE_HEAD_ICON;
			rtvalue["message"] = "update user icon in mysql failed";
			return;
		}
		// 通知好友有新的头像上传
		notifyFriendNewHeadIcon(uid, serverName);
	}
		else {
		    /* if (!LogicSystem::getInstance()->addMd5FileInfo(fileName, fi))
		    {
				std::cerr << "[ResourceServer] CRITICAL: save FileInfo to Redis failed, file="
				          << serverName << " last_acked=" << lastAcked << std::endl;
			}*/
	}
}

void FileWorker::handleUploadFile(std::shared_ptr<FileTask> task)
{
	std::shared_ptr<CSession> session = task->session_;
	int uid = task->session_->getUserId();
	Json::Value rtvalue;
	Defer defer([session, this, &rtvalue]() {
		// 发送响应给客户端
		session->Send(rtvalue.toStyledString(), ID_IMAGE_CHAT_MSG_RSP);
		});
	rtvalue["code"] = 0;
	rtvalue["message"] = "upload success";
	std::string md5 = task->md5_;
	int seq = task->seq_;
	int lastSeq = task->lastSeq_;
	std::string fileName = task->name_;
	int transferredSize = task->transfferredSize_;
	int totolSize = task->totolSize_;
	std::string data = task->data_;
	int type = task->type_;
	// 对base64编码的数据进行解码
	std::string decodedData = base64_decode(task->data_);
	// 安全校验 + 生成服务端落盘名（防路径穿越 / 稀疏文件 DoS / 恶意文件）
	std::string serverName, displayName, ext;
	int vret = validateUploadAndBuildName(uid, md5, fileName, seq, lastSeq, totolSize, decodedData.size(), serverName, displayName, ext);
	if (vret != 0) {
		rtvalue["code"] = vret;
		rtvalue["message"] = "file upload rejected";
		return;
	}
	if (seq == 1 && !validateMagicBytes(ext, decodedData)) {
		rtvalue["code"] = ERROR_FILE_MAGIC_MISMATCH;
		rtvalue["message"] = "file magic bytes mismatch";
		return;
	}
	auto cfg = ConfigManager::getInstance();
	std::string uploadPath = cfg["SelfServer"]["UploadPath"];
	std::string fullPath = uploadPath + "/" + serverName;
	std::ofstream ofs;
	// 定位写入（非追加）：乱序/重传包也写到正确的文件偏移
	if (seq == 1) {
		ofs.open(fullPath, std::ios::binary | std::ios::out);
	}
	else {
		ofs.open(fullPath, std::ios::binary | std::ios::in | std::ios::out);
		if (!ofs.is_open()) { ofs.clear(); ofs.open(fullPath, std::ios::binary | std::ios::out); }
	}
	if (!ofs.is_open()) {
		std::cout << "文件" << fullPath << "打开失败" << std::endl;
		rtvalue["code"] = 1;
		rtvalue["msg"] = "open file failed";
		rtvalue["seq"] = seq;
		return;
	}
	ofs.seekp((seq - 1) * MAX_FILE_LEN);
	ofs.write(decodedData.c_str(), decodedData.size());
	if (!ofs) {
		std::cout << "写入" << fullPath << "失败" << std::endl;
		rtvalue["code"] = 2;
		rtvalue["message"] = "write into file failed";
		rtvalue["seq"] = seq;
		return;
	}
	ofs.close();
	std::cout << "write " << displayName << " -> " << serverName << "(" << seq << "/ " << lastSeq << ")" << " into " << fullPath << " success." << std::endl;
	rtvalue["seq"] = seq;
	rtvalue["lastseq"] = lastSeq;
	rtvalue["file"] = serverName;
	rtvalue["md5"] = md5;
	rtvalue["totol_size"] = totolSize;
	rtvalue["trans_size"] = transferredSize;
	rtvalue["type"] = type;

	// 计算连续确认的 last_acked（支持乱序到达 + 重传 + 死锁恢复）
	int lastAcked = 0;
	
	auto fi = LogicSystem::getInstance()->getFileInfo(fileName);
	if (fi) {
		fi->seq_ = seq;
		fi->transfferredSize_ = transferredSize;
		// 默认保持当前连续值，不污染 Redis
		lastAcked = fi->last_acked_seq_;
		if (seq == fi->last_acked_seq_ + 1) {
			fi->last_acked_seq_ = seq;
			while (fi->pending_seqs_.count(fi->last_acked_seq_ + 1)) {
				fi->pending_seqs_.erase(fi->last_acked_seq_ + 1);
				fi->last_acked_seq_++;
			}
			if (fi->last_acked_seq_ > seq) {
				std::cout << "[ResourceServer] catch-up: file=" << serverName
					        << " last_acked " << seq << " -> " << fi->last_acked_seq_ << std::endl;
			}
			lastAcked = fi->last_acked_seq_;
		}
		else if (seq > fi->last_acked_seq_ + 1) 
		{
			fi->pending_seqs_.insert(seq);
			std::cout << "[ResourceServer] PACKET LOSS: file=" << serverName
			        << " expected=" << (fi->last_acked_seq_ + 1)
			        << " received=" << seq << " pending=" << fi->pending_seqs_.size() << std::endl;
		}
		rtvalue["last_acked"] = lastAcked;
		std::cout << "[ResourceServer] ACK: file=" << serverName
				<< " seq=" << seq << " last_acked=" << lastAcked << std::endl;
	} 
	else
	{
		std::cout << "[ResourceServer] Fatal Error!" << std::endl;
	}
	
	if (lastAcked == lastSeq) {
		//LogicSystem::getInstance()->DeleteMd5FileInfo(fileName);
		std::string key = USERIPPREFIX + std::to_string(session->getUserId());
		std::string server_ip = RedisManager::getInstance()->Get(key);
		if (server_ip == "") {
			std::cout << "[DEBUG]: Can not find User_IP by uid,ImageMsg transfer failed.\n";
			return;
		}
		std::cout << "Call ChatServer to Notify uid = " << session->getUserId() << " ImageMsg success.\n";
		NotifyChatServerImgReq req;
		req.set_uid(session->getUserId());
		req.set_unique_name(serverName);
		NotifyChatServerImgRsp rsp = ResouceServerClient::getInstance()->NotifyChatServerImg(server_ip, req);
		if (rsp.error() != 0) {
			std::cout << "Notify Client ChatImg failed.\n";
		}
	}
	if (!LogicSystem::getInstance()->addMd5FileInfo(fileName, fi)) {
		std::cerr << "[ResourceServer] CRITICAL: save FileInfo to Redis failed, file="
				    << serverName << " last_acked=" << lastAcked << std::endl;
	}
}

void FileWorker::postTaskToQue(std::shared_ptr<FileTask> task)
{
	std::lock_guard<std::mutex> locket(mtx_);
	que_.push(task);
	cond_.notify_one();
}
