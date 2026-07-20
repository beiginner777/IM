#include "LogicSystem.h"
#include "VerifyGrpcClient.h"
#include "RedisManager.h"
#include "MysqlManager.h"
#include "StatusGrpcClient.h"
#include "crypto/BCryptHasher.h"
#include "JWT.h"
LogicSystem::LogicSystem()
{
    registerGetHandler();
    registerPostHandler();
	//work_thread_ = std::thread(&LogicSystem::work, this);
}
//void LogicSystem::work()
//{
//	while (true)
//	{
//		std::unique_lock<std::mutex> locker_(mtx_);
//		while (!b_stop_ && que_.empty())
//		{
//			cond_.wait(locker_);
//		}
//		if (b_stop_)
//		{
//			while (!que_.empty())
//			{
//				auto& task = que_.front();
//				task();
//				que_.pop();
//			}
//			break;
//		}
//		if (!que_.empty())
//		{
//			auto& task = que_.front();
//			task();
//			que_.pop();
//		}
//	}
//}
LogicSystem::~LogicSystem()
{
    std::cout << "LogicSystem destructed." << std::endl;
}

void LogicSystem::registerGetHandler()
{
//	// 前端静态文件目录（相对 GateServer 项目目录: server/GateServer/）
//	static const std::string kFeDist = "../../client/React/dist";
//
//	// 响应 index.html 的通用 lambda
//	auto serveFeApp = [this](std::shared_ptr<HttpConnection> conn) {
//		auto& response = conn->response_;
//		std::ifstream file(kFeDist + "/index.html", std::ios::binary);
//		if (file.is_open()) {
//			response.result(http::status::ok);
//			std::stringstream buffer;
//			buffer << file.rdbuf();
//			response.set(http::field::content_type, "text/html");
//			beast::ostream(response.body()) << buffer.str();
//			response.content_length(response.body().size());
//		} else {
//			response.result(http::status::not_found);
//			response.set(http::field::content_type, "text/plain");
//			beast::ostream(response.body()) << "index.html not found\n";
//		}
//	};
//
//	getHandles_["/"]            = serveFeApp;
//	getHandles_["/index"] = serveFeApp;

}

void LogicSystem::registerPostHandler()
{
	// 注册 获取验证码 的回调函数
	postHandles_["/getVerifyCode"] = [this](std::shared_ptr<HttpConnection> conn) {
		auto& request = conn->request_;
		auto& response = conn->response_;
		auto& body = request.body();
		auto body_str = beast::buffers_to_string(body.data());
		std::cout << "receive post body = " << body_str << std::endl;
		response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		if (!reader.parse(body_str, root)) {
			response.result(http::status::not_found);
			Json::Value value;
			value["code"] = ERROE_CODR::ERROR_JSON;
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		// 因为在客户端的注册逻辑上 已经处理了Email的输入格式，所以发送过来的数据一定是合法的
		/*if (!root.isMember("email"))
		{
			response.result(http::status::ok);
			Json::Value value;
			value["code"] = ERROE_CODR::ERROR_EMAIL;
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}*/
		std::string email = root["email"].asString();
		VerifyGrpcClient grpcConn;
		auto reply = grpcConn.GetVerifyCode(email);
		std::cout << "receive from Grpc server: " << reply.DebugString() << std::endl;
		Json::Value value;
		value["code"] = reply.error();
		value["email"] = root["email"];
		
		beast::ostream(response.body()) << value.toStyledString();
	};
	// 注册 用户注册的函数
	postHandles_["/registerUserAddr"] = [this](std::shared_ptr<HttpConnection> conn) {
		auto& request = conn->request_;
		auto& response = conn->response_;
		auto& body = request.body();
		std::string bodyStr = boost::beast::buffers_to_string(body.data());
		std::cout << "receive post body = " << bodyStr << std::endl;
		Json::Value root;
		Json::Reader reader;
		// json解析失败
		if (!reader.parse(bodyStr, root))
		{
			Json::Value value;
			value["code"] = ERROE_CODR::ERROR_JSON;
			value["message"] = "json prase failed !";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		// 用户的注册信息
		std::string name = root["name"].asString();
		std::string password = root["password"].asString();
		std::string email = root["email"].asString();
		std::string code = root["code"].asString();
		// 转化为在redis中存储的数据格式
		email.insert(0, "code_");
		
		std::string code_ = RedisManager::getInstance()->Get(email);
		if (code_ != code || code_ == "")
		{
			if (code_ == ""){
				std::cout << "Falied to find key:code_" << email << " in redis." << std::endl;
			}
			else {
				std::cout << "Error verifyCode." << std::endl;
			}
			Json::Value value;
			value["code"] = ERROR_VERIFYCODE;
			value["message"] = "Error verifyCode";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		// bcrypt 哈希后再存储
		std::string hashedPwd = BCryptHasher::generateHash(password, 10);
		int returnCode = MysqlManager::getInstance()->registerUser(name, email, hashedPwd);
		std::cout << "returnCode = " << returnCode << std::endl;
		// 注册成功
		if (returnCode >= 0)
		{
			std::cout << "User " << name << "register success." << std::endl;
			Json::Value value;
			value["code"] = SUCCESS;
			value["message"] = "register success.";
			beast::ostream(response.body()) << value.toStyledString();
		}
		// 注册失败
		else
		{
			std::cout << "User " << name << " register failed." << std::endl;
			Json::Value value;
			if (returnCode == ERROR_REGISTER)
			{
				value["code"] = ERROR_REGISTER;
				value["message"] = "unknown error!";
			}
			else if (returnCode == ERROR_NAME_EXIST)
			{
				value["code"] = ERROR_NAME_EXIST;
				value["message"] = "user_name exists!";
			}
			else if (returnCode == ERROR_EMAIL_EXIST)
			{
				value["code"] = ERROR_EMAIL_EXIST;
				value["message"] = "user_email exists!";
			}
			beast::ostream(response.body()) << value.toStyledString();
		}
		// 因为手动 mysql_close的话，那么就会导致智能指针管理的内存为 nullptr
		// 然后在智能指针释放的时候，又会释放一次，这样的话就是 double free.
		// 所以导致了内存泄漏
		// mysql_close(mysqlconn.get());
	};
	// 用户登录的函数
	postHandles_["/loginAddr"] = [this](std::shared_ptr<HttpConnection> conn) {
		auto& request = conn->request_;
		auto& response = conn->response_;
		auto& body = request.body();
		std::string bodyStr = boost::beast::buffers_to_string(body.data());
		std::cout << "receive post body = " << bodyStr << std::endl;
		Json::Value root;
		Json::Reader reader;
		Json::Value value;
		// json解析失败
		if (!reader.parse(bodyStr, root))
		{
			value["code"] = ERROE_CODR::ERROR_LOGIN;
			value["message"] = "json prase failed !";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		// 用户的登录信息
		std::string name = root["username"].asString();
		std::string password = root["password"].asString();
		std::cout << "name = " << name << std::endl;
		std::cout << "password = " << password << std::endl;
		std::shared_ptr<UserInfo> userInfo = std::make_shared<UserInfo>();
		static BloomFilter bloomLogin(1000000, 0.01);
		static bool bloomLoginLoaded = false;
		if (!bloomLoginLoaded) {
			bloomLogin.loadFromRedis("bloom:user_search");
			bloomLoginLoaded = true;
		}
		if (!bloomLogin.contains(name)) {
			value["code"] = ERROR_USER_NOT_EXIST;
			value["message"] = "User don't exist.";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		int returnCode = MysqlManager::getInstance()->userLogin(name, password, userInfo);
		
		// 查询状态服务器Status找到合适的连接
		StatusGrpcClient client;
		auto reply = client.GetChatServer(userInfo->uid_);
		if (reply.error()) {
			std::cout << " grpc failed to connect StatusServer: get ChatServer failed, error is " << reply.error() << std::endl;
			value["code"] = ERROR_RPC_CON_STATUSSERVER;
			value["message"] = "Can not find suitable ChatServer.";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		else if (returnCode == ERROR_USER_NOT_EXIST)
		{
			value["code"] = ERROR_USER_NOT_EXIST;
			value["message"] = "User don't exist.";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		else if (returnCode == ERROR_PASSWORD) {
			value["code"] = ERROR_PASSWORD;
			value["message"] = "Error password.";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		value["code"] = SUCCESS;
		value["message"] = "Succeed to find suitable ChatServer";
		value["uid"] = userInfo->uid_;
		value["token"] = reply.token();
		// ChatServer
		value["host"] = reply.host();
		value["port"] = reply.port();
		// ResourceServer
		ConfigManager cfg;
		value["res_host"] = cfg["ResourceServer"]["Host"];
		value["res_port"] = cfg["ResourceServer"]["Port"];
		beast::ostream(response.body()) << value.toStyledString();
		//std::cout << "return message1 = " << boost::beast::buffers_to_string(response.body().data()) << std::endl;
	};
	// Web秒杀前端 登录的函数（与桌面端 /loginAddr 区分）
	// 响应协议与前端约定：成功 {error_code:0, username, host, port}；失败 {error_code, error_msg}
	postHandles_["/fe_login"] = [this](std::shared_ptr<HttpConnection> conn) {
		auto& request = conn->request_;
		auto& response = conn->response_;
		auto& body = request.body();
		std::string bodyStr = boost::beast::buffers_to_string(body.data());
		std::cout << "receive post body = " << bodyStr << std::endl;
		response.set(http::field::content_type, "application/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value value;
		// json解析失败
		if (!reader.parse(bodyStr, root))
		{
			value["error_code"] = ERROE_CODR::ERROR_JSON;
			value["error_msg"] = "json prase failed !";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		// 用户的登录信息
		std::string name = root["username"].asString();
		std::string password = root["password"].asString();
		std::cout << "[fe_login] name = " << name << std::endl;
		// 数据库校验用户名密码（bcrypt）
		std::shared_ptr<UserInfo> userInfo = std::make_shared<UserInfo>();
		int returnCode = MysqlManager::getInstance()->userLogin(name, password, userInfo);
		if (returnCode == ERROR_USER_NOT_EXIST)
		{
			value["error_code"] = ERROR_USER_NOT_EXIST;
			value["error_msg"] = "用户不存在";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		else if (returnCode == ERROR_PASSWORD)
		{
			value["error_code"] = ERROR_PASSWORD;
			value["error_msg"] = "密码错误";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		else if (returnCode == ERROR_LOGIN)
		{
			// SQL 执行异常
			value["error_code"] = ERROR_LOGIN;
			value["error_msg"] = "服务繁忙，请稍后重试";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		else if (returnCode != SUCCESS)
		{
			// 其他非用户侧错误（如 MySQL 连接池拿不到连接返回的 ERROR_REGISTER），
			// 统一按服务繁忙处理，防止穿透到成功分支
			value["error_code"] = returnCode;
			value["error_msg"] = "服务繁忙，请稍后重试";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		// 查询状态服务器Status分配一个SeckillServer
		StatusGrpcClient client;
		auto reply = client.GetSeckillServer(userInfo->uid_);
		if (reply.error()) {
			std::cout << " grpc failed to connect StatusServer: get SeckillServer failed, error is " << reply.error() << std::endl;
			value["error_code"] = ERROR_RPC_CON_STATUSSERVER;
			value["error_msg"] = "无法分配秒杀服务器，请稍后重试";
			beast::ostream(response.body()) << value.toStyledString();
			return;
		}
		value["error_code"] = SUCCESS;
		value["username"] = name;
		// JWT token（UUID，Redis 存 payload）
		std::string token = JWT::generateToken(userInfo->uid_, name);
		value["token"] = token;
		// 存 Redis: token:{uuid} → {uid, username, exp}
		Json::Value tokenPayload;
		tokenPayload["uid"] = userInfo->uid_;
		tokenPayload["username"] = name;
		tokenPayload["exp"] = (Json::Int64)std::chrono::duration_cast<std::chrono::seconds>(
			(std::chrono::system_clock::now() + std::chrono::hours(24)).time_since_epoch()).count();
		RedisManager::getInstance()->SetExp("token:" + token, tokenPayload.toStyledString(), JWT::TOKEN_TTL);
		// 用户余额（前端充值页面显示）
		value["balance"] = userInfo->balance_;
		// SeckillServer 地址（前端 setBaseURL 使用，port 需为数字类型）
		value["host"] = reply.host();
		value["port"] = std::atoi(reply.port().c_str());
		beast::ostream(response.body()) << value.toStyledString();
	};
}

void LogicSystem::handleGetRequest(std::shared_ptr<HttpConnection> conn)
{
	std::string target = conn->request_.target();
	// 解码出来 URL放在 url_变量当中;  然后将 键值对 放在getPrama_中
	prase_get_request(target);
	std::cout << "prase url = " << url_ << std::endl;
	// 作为静态文件返回
	static const std::string kFeDist = "../../client/React/dist";
	std::string filePath = kFeDist + url_;
	std::cout << "[GateServer] filePath = " << filePath << std::endl;
	std::ifstream file(filePath, std::ios::binary);
	auto& response = conn->response_;
	auto isSuffix = [](const std::string& s, const std::string& sfx) {
		return s.size() >= sfx.size() && s.rfind(sfx) == s.size() - sfx.size();
	};

	if (file.is_open()) {
		response.result(http::status::ok);
		std::stringstream buffer;
		buffer << file.rdbuf();
		if (isSuffix(filePath, ".css"))       response.set(http::field::content_type, "text/css");
		else if (isSuffix(filePath, ".js"))   response.set(http::field::content_type, "application/javascript");
		else if (isSuffix(filePath, ".svg"))  response.set(http::field::content_type, "image/svg+xml");
		else if (isSuffix(filePath, ".png"))  response.set(http::field::content_type, "image/png");
		else                              response.set(http::field::content_type, "application/octet-stream");
		beast::ostream(response.body()) << buffer.str();
		response.content_length(response.body().size());
	} else {
		// 静态文件也不存在 → SPA 回退
		std::ifstream idx(kFeDist + "/index.html");
		if (idx.is_open()) {
			response.result(http::status::ok);
			std::stringstream buf;
			buf << idx.rdbuf();
			response.set(http::field::content_type, "text/html");
			beast::ostream(response.body()) << buf.str();
			response.content_length(response.body().size());
		} else {
			response.result(http::status::not_found);
			response.set(http::field::content_type, "text/plain");
			beast::ostream(response.body()) << "404 not found\n";
		}
	}
	url_ = "";
	getPrama_.clear();
}

void LogicSystem::handlePostRequest(std::shared_ptr<HttpConnection> conn)
{
	std::string target = conn->request_.target();

	// /api/* 路由映射到 GateServer 内部 handler
	if (target.find("/api/") == 0) {
		if (target == "/api/login")    target = "/fe_login"; // Web前端登录，与桌面端 /loginAddr 区分
		if (target == "/api/register") target = "/registerUserAddr";
		if (target == "/api/verify")   target = "/getVerifyCode";
	}

	std::cout << "prase url = " << target << std::endl;
	if (postHandles_.count(target))
	{
		postHandles_[target](conn);
		// auto body_str = beast::buffers_to_string(conn->request_.body().data());
		// std::cout << "after  request body = " << body_str << std::endl;
	}
	else
	{
		std::cout << "Can' not find PostHandler of " << target << std::endl;
	}
}
//void LogicSystem::postTaskToQue(Task task)
//{
//	std::unique_lock<std::mutex> locket_;
//	que_.push(task);
//	cond_.notify_one();
//}
unsigned char LogicSystem::toHex(unsigned char ch)
{
	if(ch > 9) return ch  + 55;
	else return ch + 48;
}

unsigned char LogicSystem::fromHex(unsigned char ch)
{
	if(ch >= 'A' && ch <= 'Z') return ch - 'A' + 10;
	else if(ch >= 'a' && ch <= 'z') return ch - 'a' + 10;
	else if(ch >= '0' && ch <= '9') return ch - '0';
	else assert(0);
	return '0';
}
// 将十进制转化为十六进制(eg: 将中文转化为十六进制字符)
std::string LogicSystem::urlEncode(std::string url)
{
	std::string result = "";
	for(int i = 0;i < url.size(); ++i)
	{
		if(url[i] == ' ') result += '+';
		else if(url[i] == '-' || url[i] == '_' || url[i] == '.' || url[i] == '~' || isalnum(url[i])) result += url[i];
		else{
			result += "%";
			result += toHex((unsigned char)url[i] >> 4);
            result += toHex((unsigned char)url[i] & 0x0F);
		}
	}
	return result;
}
// 将十六进制转化为十进制(eg: 将十六进制字符转化为中文)
std::string LogicSystem::urlDecode(std::string url)
{
	std::string result = "";
	for(int i = 0; i < url.size(); ++i)
	{
		if (url[i] == '+') result += ' ';
        //遇到%将后面的两个字符从16进制转为char再拼接
        else if (url[i] == '%')
		{
            unsigned char high = fromHex((unsigned char)url[++i]);
            unsigned char low = fromHex((unsigned char)url[++i]);
            result += high >> 4 + low;
        }
        else result += url[i];
	}
	return result;
}

void LogicSystem::prase_get_request(std::string& url)
{
	std::cout << "prase get request ... " << std::endl;
    // 查找查询字符串的开始位置（即 '?' 的位置）  
    auto query_pos = url.find('?');
    if (query_pos == std::string::npos) {
        url_ = url;
        return;
    }
    url_ = url.substr(0, query_pos);
    std::string query_string = url.substr(query_pos + 1);
    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = urlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
            value = urlDecode(pair.substr(eq_pos + 1));
            getPrama_[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    // 处理最后一个参数对（如果没有 & 分隔符）  
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = urlDecode(query_string.substr(0, eq_pos));
            value = urlDecode(query_string.substr(eq_pos + 1));
            getPrama_[key] = value;
        }
    }
}