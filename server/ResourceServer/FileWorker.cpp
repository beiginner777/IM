#include "FileWorker.h"
#include "CSession.h"
#include "ConfigManager.h"
#include "LogicSystem.h"
#include "MysqlManager.h"
#include "RedisManager.h"
#include "ResouceServerClient.h"

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

	/*std::cout << "=================================================" << std::endl;
	std::cout << "receive from client: " << fileName << "(" << seq << "/ " << lastSeq << ")"
		<< ",transferredSize = " << transferredSize << ",totolSize = " << totolSize << std::endl
		<< ",decodedData size = " << decodedData.size() << std::endl;
	std::cout << "data = " << data << std::endl;
	std::cout << "=================================================" << std::endl << std::endl;*/

	auto cfg = ConfigManager::getInstance();
	std::string uploadPath = cfg["SelfServer"]["UploadPath"];

	std::string fullPath = uploadPath + "/" + fileName;

	std::ofstream ofs;
	// 第一个包，那么就需要创建文件来保存这个文件
	if (seq == 1) {
		// 存在就清空，不存在就创建
		ofs.open(fullPath, std::ios::binary | std::ios::out);
	}
	else {
		// 以二进制的形式对文件进行追加
		ofs.open(fullPath, std::ios::binary | std::ios::app);
	}

	if (!ofs.is_open()) {
		std::cout << "文件" << fullPath << "打开失败" << std::endl;
		rtvalue["code"] = 1;
		rtvalue["msg"] = "open file failed";
		rtvalue["seq"] = seq;
		session->Send(rtvalue.toStyledString(), ID_UPLOAD_HEAD_ICON_RSP);
		return;
	}

	ofs.write(decodedData.c_str(), decodedData.size());

	if (!ofs) {
		std::cout << "写入" << fullPath << "失败" << std::endl;
		rtvalue["code"] = 2;
		rtvalue["message"] = "write into file failed";
		rtvalue["seq"] = seq;
		session->Send(rtvalue.toStyledString(), ID_UPLOAD_HEAD_ICON_RSP);
		return;
	}

	ofs.close();

	std::cout << "write " << fileName << "(" << seq << "/ " << lastSeq << ")" << " into " << fullPath << " success." << std::endl;

	rtvalue["uid"] = uid;
	rtvalue["seq"] = seq;
	rtvalue["lastseq"] = lastSeq;
	rtvalue["file"] = fileName;
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
		int ret = MysqlManager::getInstance()->updateUserIcon(uid, fileName);
		if (ret != 0) {
			std::cout << "update user icon in mysql failed. uid = " << uid << std::endl;
			rtvalue["code"] = ERROR_UPDATE_HEAD_ICON;
			rtvalue["message"] = "update user icon in mysql failed";
			return;
		}
		// 通知好友有新的头像上传
		notifyFriendNewHeadIcon(uid, fileName);
	}
	else {
		LogicSystem::getInstance()->addMd5FileInfo(fileName,
			std::make_shared<FileInfo>(uid, seq, fileName, totolSize, transferredSize, lastSeq, fullPath));
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

	/*std::cout << "=================================================" << std::endl;
	std::cout << "receive from client: " << fileName << "(" << seq << "/ " << lastSeq << ")"
		<< ",transferredSize = " << transferredSize << ",totolSize = " << totolSize << std::endl
		<< ",decodedData size = " << decodedData.size() << std::endl;
	std::cout << "data = " << data << std::endl;
	std::cout << "=================================================" << std::endl << std::endl;*/

	auto cfg = ConfigManager::getInstance();
	std::string uploadPath = cfg["SelfServer"]["UploadPath"];

	std::string fullPath = uploadPath + "/" + fileName;

	std::ofstream ofs;
	// 第一个包，那么就需要创建文件来保存这个文件
	if (seq == 1) {
		// 存在就清空，不存在就创建
		ofs.open(fullPath, std::ios::binary | std::ios::out);
	}
	else {
		// 以二进制的形式对文件进行追加
		ofs.open(fullPath, std::ios::binary | std::ios::app);
	}

	if (!ofs.is_open()) {
		std::cout << "文件" << fullPath << "打开失败" << std::endl;
		rtvalue["code"] = 1;
		rtvalue["msg"] = "open file failed";
		rtvalue["seq"] = seq;
		session->Send(rtvalue.toStyledString(), ID_UPLOAD_FILE_RSP);
		return;
	}

	ofs.write(decodedData.c_str(), decodedData.size());

	if (!ofs) {
		std::cout << "写入" << fullPath << "失败" << std::endl;
		rtvalue["code"] = 2;
		rtvalue["message"] = "write into file failed";
		rtvalue["seq"] = seq;
		session->Send(rtvalue.toStyledString(), ID_UPLOAD_FILE_RSP);
		return;
	}

	ofs.close();

	std::cout << "write " << fileName << "(" << seq << "/ " << lastSeq << ")" << " into " << fullPath << " success." << std::endl;

	rtvalue["seq"] = seq;
	rtvalue["lastseq"] = lastSeq;
	rtvalue["file"] = fileName;
	rtvalue["md5"] = md5;
	rtvalue["totol_size"] = totolSize;
	rtvalue["trans_size"] = transferredSize;
	rtvalue["type"] = type;

	if (seq == lastSeq) {
		// 将Redis中相关的信息删除
		LogicSystem::getInstance()->DeleteMd5FileInfo(fileName);
		// Grpc调用ChatServer去通知Client端有新的图片消息
		std::string key = USERIPPREFIX + std::to_string(session->getUserId());
		std::string server_ip = RedisManager::getInstance()->Get(key);
		if (server_ip == "") {
			rtvalue["code"] = ERROE_CODR::ERROR_USER_IP_NOT_FIND;
			rtvalue["message"] = "Can not find User_IP by uid,ImageMsg transfer failed.";
			std::cout << "[ERROR]: Can not find User_IP by uid,ImageMsg transfer failed.\n";
			return;
		}
		std::cout << "Call ChatServer to Notifu uid = " << session->getUserId() << " ImageMsg success.\n";
		NotifyChatServerImgReq req;
		req.set_uid(session->getUserId());
		req.set_unique_name(fileName);
		NotifyChatServerImgRsp rsp = ResouceServerClient::getInstance()->NotifyChatServerImg(server_ip, req);
		if (rsp.error() != 0) {
			std::cout << "Notify Client ChatImg failed.\n";
		}
	}
	else {
		LogicSystem::getInstance()->addMd5FileInfo(fileName,
			std::make_shared<FileInfo>(uid,seq,fileName,totolSize,transferredSize,lastSeq, fullPath));
	}
}

void FileWorker::postTaskToQue(std::shared_ptr<FileTask> task)
{
	// 将文件结点 根据 文件名 放入FileSystem的某个FileWorker中
	std::lock_guard<std::mutex> locket(mtx_);
	que_.push(task);
	cond_.notify_one();
}