#ifndef LOGICWORKER_H
#define LOGICWORKER_H

#include "global.h"

class CSession;
class LogicNode;

class LogicWorker
{
	using functionCallback = std::function<void(std::shared_ptr<CSession>, short msgId, std::string msgData)>;
public:
	LogicWorker();
	~LogicWorker();
	// 向LogicSystem提供接口，向当前逻辑线程的队列中投递消息
	void postMsgToQue(std::shared_ptr<LogicNode> logicNode);
private:
	// 注册逻辑层的回调函数
	void registerFunctionCallbacks();
	// 工作线程
	void dealTask();
	// 处理上传头像的请求
	void uploadHeadIcon(std::shared_ptr<CSession> session, short msgId, std::string msgData);
	// 处理上传文件的请求
	void uploadFile(std::shared_ptr<CSession> session, short msgId, std::string msgData);
	// 处理继续同步文件的请求
	void syncFile(std::shared_ptr<CSession> session, short msgId, std::string msgData);
	// 处理下载文件的请求
	void downloadFile(std::shared_ptr<CSession> session, short msgId, std::string msgData);
	// 处理聊天图片续传的请求
	void imgChatContinueUpload(std::shared_ptr<CSession> session, short msgId, std::string msgData);
	// 处理文件继续下载的请求
	void fileContinueDownload(std::shared_ptr<CSession> session, short msgId, std::string msgData);
t// 处理 StatusServer 注册响应
	void handleRegisterRsp(std::shared_ptr<CSession> session, short msgId, std::string msgData);
	// 处理 StatusServer 心跳响应
	void handleHeartCheckRsp(std::shared_ptr<CSession> session, short msgId, std::string msgData);
	// 解密base64编码的数据
	std::string base64_decode(const std::string& in);

private:
	std::queue<std::shared_ptr<LogicNode>> que_;
	std::map<int, functionCallback> handlers_;
	// 子线程来执行任务
	std::thread work_thread_;
	// 逻辑队列是共享资源
	std::mutex mtx_;
	std::condition_variable cond_;
	// 是否停止工作
	std::atomic_bool b_stop_;
};

#endif

