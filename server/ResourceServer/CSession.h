#ifndef CSESSION_H
#define CSESSION_H

#include "global.h"
#include "MsgNode.h"

class CServer;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& ioc,CServer* server);
	~CSession();

	void start();
	void Close();

	void setUserId(int userId) { userId_ = userId; }

	tcp::socket& getSocket() { return socket_; }
	std::string& getUuid() { return uuid_;  }
	int getUserId() { return userId_; }

	// 发送数据
	void Send(const char* msg, int max_length, short msgid);
	void Send(std::string msg, short msgid);
	// 客户端心跳检测（文件上传/下载客户端）
	bool isClientOverTime();
	void setClientHeartCheckTime(time_t tm);

	// 设置自定义头部长度（StatusServer 连接用 4 字节头）
	void setHeaderLen(int len) { headerLen_ = len; }
	int getHeaderLen() const { return headerLen_; }

public:
	// 当前 CSession 对应的 ChatServer
	CServer* server_;

private:
	// 读取头部消息
	void AsyncReadHead(std::size_t len);
	// 出现错误 或者 读取指定长度之后，触发回调函数
	void AsyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code, std::size_t)>handler);
	// 继续读取 长度为 totolLen - readLen 的剩余数据，出现错误 或者 读取完成之后触发回调函数
	void AsyncReadLen(std::size_t readLen, std::size_t totolLen, std::function<void(const boost::system::error_code, std::size_t)> handler);
	// 读取消息主体
	void AsyncReadBody(std::size_t len);
	// 发送消息的回调函数
	void handleWrite(boost::system::error_code ec,std::shared_ptr<CSession> session);
	// 带格式的十六进制打印
	void printHexFormatted(const char* data, size_t length, const std::string& label = "") {
		std::cout << label << "(" << length << " bytes): ";
		for (size_t i = 0; i < length; ++i) {
			std::cout << std::hex << std::setw(2) << std::setfill('0')
				<< (static_cast<unsigned int>(data[i]) & 0xFF) << " ";
		}
		std::cout << std::dec << std::endl;
	}
private:
	// 连接是否关闭
	bool b_close_;

	boost::asio::io_context& ioc_; 
	// 当前 CSession 对应的socket，用于与客户端通信
	boost::asio::ip::tcp::socket socket_; 
	// uuid标识符,存放在CServer的map中
	std::string uuid_; 
	// 客户端的uid
	int userId_; 

	// 客户端心跳时间戳
	std::time_t clientHeartCheckTime_;
	// 可配置的头部长度（默认 HEAD_TOTOL_LEN=6, StatusServer 连接设为 4）
	int headerLen_ = HEAD_TOTOL_LEN;
	std::mutex clientTimeMtx_;
	// 存储接收到的消息
	char data_[MAX_RECV_LENGTH];

	// 存放SendNode消息结点的队列是共享资源
	std::queue<std::shared_ptr<SendNode>> que_;
	std::mutex send_mtx_;

	// 接收消息头部的结点
	std::shared_ptr<RecvNode> recv_head_node_;
	// 接收消息主体的结点
	std::shared_ptr<RecvNode> recv_msg_node_;

	// socket_ 是共享资源
	std::mutex mtx_;
};

#endif

