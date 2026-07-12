#include "DownloadWorker.h"
#include "CSession.h"
#include "ConfigManager.h"
#include "LogicSystem.h"

DownloadWorker::DownloadWorker()
{
	work_thread_ = std::thread(&DownloadWorker::dealTask, this);
}

DownloadWorker::~DownloadWorker()
{
}

void DownloadWorker::dealTask()
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
				std::shared_ptr<DownloadTask> task = que_.front();
				que_.pop();
				taskHandler(task);
			}
			// detail break
			break;
		}

		// 文件处理层没有退出，那么就正常取数据
		if (!que_.empty())
		{
			std::shared_ptr<DownloadTask> task = que_.front();
			que_.pop();
			taskHandler(task);
		}
	}
}

void DownloadWorker::taskHandler(std::shared_ptr<DownloadTask> task)
{
	//std::this_thread::sleep_for(std::chrono::milliseconds(5));

	std::shared_ptr<CSession> session = task->session_;
	std::string download_file = task->download_file_;
	int seq = task->seq_;
	int last_seq = task->last_seq_;
	int trans_size = task->trans_size_;
	int total_size = task->total_size_;
	std::string client_save_path = task->client_save_path_;
	Download_File_Type type = task->type_;
	int icon_uid = task->icon_uid_;

	Json::Value rtvalue;
	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "download file task processed successfully";

	Defer defer([session, this, &rtvalue]() {
		session->Send(rtvalue.toStyledString(), ID_DOWN_LOAD_FILE_RSP);
		});

	// 检查文件是否存在
	auto cfg = ConfigManager::getInstance();
	std::string uploadPath = cfg["SelfServer"]["DownloadPath"];
	std::string fullPath = uploadPath + "/" + download_file;
	if (!boost::filesystem::exists(fullPath)) {
		std::cerr << "文件不存在: " << fullPath << std::endl;
		rtvalue["code"] = ERROE_CODR::ERROR_FILE_NOT_EXIST;
		rtvalue["message"] = "download file failed, file not exist.";
		return;
	}

	// 打开文件
	std::ifstream infile(fullPath, std::ios::binary);
	if (!infile) {
		std::cerr << "无法打开文件进行读取。" << std::endl;
		rtvalue["code"] = ERROE_CODR::ERROR_OPEN_FILE;
		rtvalue["message"] = "download file failed, open file error.";
		return;
	}

	if (seq == 1) {
		// 获取文件大小
		infile.seekg(0, std::ios::end);
		std::streamsize file_size = infile.tellg();
		infile.seekg(0, std::ios::beg);
		// 在redis中设置文件下载信息
		std::shared_ptr<DownloadFileInfo> file_info = std::make_shared<DownloadFileInfo>();
		file_info->uid_ = session->getUserId();
		file_info->download_file_ = download_file;
		file_info->seq_ = 0; // 已经下载的包的序号
		file_info->total_size_ = file_size; // 文件总大小
		file_info->trans_size_ = 0; // 已经下载的大小
		file_info->client_save_path_ = client_save_path;
		file_info->download_file_type_ = type; // 下载文件的类型

		if (file_size % MAX_FILE_LEN == 0) {
			file_info->last_seq_ = file_size / MAX_FILE_LEN;
		}
		else {
			file_info->last_seq_ = file_size / MAX_FILE_LEN + 1;
		}
		std::cout << "[新下载] 文件: " << download_file
			<< ", 大小: " << file_size << " 字节" << std::endl;

		// 添加到redis
		LogicSystem::getInstance()->addDownloadFileInfo(download_file, file_info);

		last_seq = file_info->last_seq_;
		trans_size = file_info->trans_size_;
		total_size = file_info->total_size_;
	}

	// 计算当前偏移量
	std::streamsize offset = ((std::streamsize)seq - 1) * MAX_FILE_LEN;
	if (offset >= total_size) {
		std::cerr << "偏移量超出文件大小。" << std::endl;
		rtvalue["code"] = ERROE_CODR::ERROR_FILE_OFFSET_INVALID;
		rtvalue["message"] = "download file failed, offset invalid.";
		infile.close();
		return;
	}

	// 定位到指定偏移量
	infile.seekg(offset);

	// 读取最多MAX_FILE_LEN字节
	char buffer[MAX_FILE_LEN];
	infile.read(buffer, MAX_FILE_LEN);
	//获取read实际读取多少字节
	std::streamsize bytes_read = infile.gcount();
	if (bytes_read <= 0) {
		std::cerr << "读取文件失败。" << std::endl;
		rtvalue["code"] = ERROE_CODR::ERROR_READ_FILE;
		rtvalue["message"] = "download file failed, read file error.";
		infile.close();
		return;
	}
	// 已经下载的文件大小
	trans_size += bytes_read;

	// 将读取的数据进行base64编码
	std::string data_to_encode(buffer, bytes_read);
	//std::cout << "origin data: " << data_to_encode << std::endl;
	std::string encoded_data = base64_encode(data_to_encode);

	/*std::cout << "---------------------------------------" << std::endl;
	std::cout << "seq = " << seq << ", last_seq = " << last_seq << std::endl;
	std::cout << "origin data: " << data_to_encode << std::endl;
	std::cout << "base64 encode data:  " << encoded_data << std::endl;*/

	// 设置返回结果
	rtvalue["download_file"] = download_file;
	rtvalue["data"] = encoded_data;
	rtvalue["seq"] = seq;
	rtvalue["last_seq"] = last_seq;
	rtvalue["total_size"] = total_size;
	rtvalue["trans_size"] = trans_size;
	rtvalue["client_save_path"] = client_save_path;
	rtvalue["download_file_type"] = type;
	rtvalue["icon_uid"] = icon_uid;

	infile.close();

	if (seq == last_seq) {
		std::cout << "[下载完成] 文件: " << download_file << std::endl;
		LogicSystem::getInstance()->DeleteDownloadFileInfo(download_file);
	}
	else {
		//更新信息
		std::shared_ptr<DownloadFileInfo> file_info = std::make_shared<DownloadFileInfo>(session->getUserId(),download_file,
			seq,last_seq,trans_size,total_size,client_save_path, type);
		//更新redis
		LogicSystem::getInstance()->addDownloadFileInfo(download_file, file_info);
	}
}

std::string DownloadWorker::base64_encode(const std::string& data)
{
	// 计算编码后所需空间
	std::string out;
	out.resize(boost::beast::detail::base64::encoded_size(data.size()));

	// 执行编码
	size_t len = boost::beast::detail::base64::encode(
		(void*)out.data(),
		data.data(),
		data.size()
	);

	out.resize(len);
	return out;
}


void DownloadWorker::postTaskToQue(std::shared_ptr<DownloadTask> task)
{
	// 将文件结点 根据 文件名 放入FileSystem的某个FileWorker中
	std::lock_guard<std::mutex> locket(mtx_);
	que_.push(task);
	cond_.notify_one();
}