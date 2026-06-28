#include "LogicSystem.h"
#include "MsgNode.h"
#include "CSession.h"
#include "RedisManager.h"
#include "CServer.h"

LogicSystem::LogicSystem() : b_stop_(false)
{
	registerFunctionCallbacks();
	// 开启子线程
	work_thread_ = std::thread(&LogicSystem::dealTask, this);
}

LogicSystem::~LogicSystem()
{
	std::cout << "LogicSystem is destructed." << std::endl;
}

void LogicSystem::postMsgToQue(std::shared_ptr<LogicNode> logicNode)
{
	std::lock_guard<std::mutex> locker(mtx_);
	que_.push(logicNode);
	cond_.notify_all();
}

void LogicSystem::dealTask()
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

void LogicSystem::registerFunctionCallbacks()
{
	// 心跳检测 -- 更新Session的时间戳
	handlers_[ID_HEADT_CHECK_REQ] = std::bind(&LogicSystem::heartCheck, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	// 其它服务的注册
	handlers_[ID_REGISTER_REQ] = std::bind(&LogicSystem::registerService, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);	
}

void LogicSystem::registerService(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	std::cout << "handle id = " << msgId << std::endl;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msgData, root);
	int server_type = root["server_type"].asInt();
	std::string serviceName = root["my_name"].asString();
	std::string host = root["my_ip"].asString();
	std::string port = root["my_port"].asString();
	std::cout << "serviceName = " << serviceName << ", host = " << host << ", port = " << port << ", type = " << server_type << std::endl;
	Server_Info info;
	info.server_type = ServerType(server_type);
	info.name = serviceName;
	info.host = host;
	info.port = port;
	info.con_count = 0;
	session->SetServerInfo(info);
	Json::Value rtvalue;
	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "register service success.\n";
	session->Send(rtvalue.toStyledString(), ID_REGISTER_RSP);

	// 将ChatServer信息存储在 Redis 中 TODO
	storeServerInfoInRedis(info);
}

void LogicSystem::storeServerInfoInRedis(const Server_Info& info)
{
	RedisManager::getInstance()->HSet(LOGINCOUNT, info.name, std::to_string(info.con_count));
}

void LogicSystem::heartCheck(std::shared_ptr<CSession> session, short msgId, std::string msgData)
{
	std::cout << "handle id = " << msgId << std::endl;

	Json::Reader reader;
	Json::Value root;
	reader.parse(msgData, root);

	std::string uuid = root["uuid"].asString();
	std::cout << "uuid = " << uuid << " request to update heartCheckTime." << std::endl;

	session->setHeartCheckTime(time(NULL));

	Json::Value rtvalue;
	rtvalue["code"] = SUCCESS;
	rtvalue["message"] = "update HeartCheckTime In StatusServer Success.\n";
	rtvalue["uuid"] = uuid;
	session->Send(rtvalue.toStyledString(), ID_HEADT_CHECK_RSP);
}