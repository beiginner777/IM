#ifndef LOGINCSYSTEM_H
#define LOGINCSYSTEM_H

#include "global.h"
#include "SingleTon.h"
#include "HttpConnection.h"
#include "MysqlDao.h"
struct GetParams { std::string url; std::map<std::string,std::string> query; int pathId = 0; };
struct PostParams { std::string body; int pathId = 0; };

class LogicSystem : public SingleTon<LogicSystem>
{
	friend class SingleTon<LogicSystem>;
public:
	LogicSystem();
	~LogicSystem();
	void handleGetRequest(std::shared_ptr<HttpConnection> conn);
	void handlePostRequest(std::shared_ptr<HttpConnection> conn);
private:
	void registerGetHandler();
	void registerPostHandler();
	void sendJson(std::shared_ptr<HttpConnection> conn, const Json::Value& value);
	void sendAuthError(std::shared_ptr<HttpConnection> conn, const std::string& msg);

	using getRequestHandler  = std::function<void(std::shared_ptr<HttpConnection>, const GetParams&)>;
	using postRequestHandler = std::function<void(std::shared_ptr<HttpConnection>, const PostParams&)>;
	std::map<std::string, getRequestHandler> getHandles_;
	std::map<std::string, postRequestHandler> postHandles_;

	std::string urlEncode(std::string url), urlDecode(std::string url);
	unsigned char toHex(unsigned char ch), fromHex(unsigned char ch);
	void prase_get_request(std::string& url);

	MysqlDao* mysqlDao_{nullptr};
};
#endif
