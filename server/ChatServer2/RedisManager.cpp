#include "RedisManager.h"
#include "RedisLocker.h"

std::string RedisManager::Get(const std::string& key)
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "GET %s", key.c_str());
	if (reply_ == nullptr)
	{
		std::cout << "[ GET " << key << " ] failed ." << std::endl;
		return std::string();
	}
	if (reply_->type != REDIS_REPLY_STRING)
	{
		std::cout << "[ GET " << key << " ] failed ." << std::endl;
		freeReplyObject(reply_);
		return std::string();
	}

	std::string value = reply_->str;
	freeReplyObject(reply_);

	std::cout << "Succeed to execute command [ GET " << key << " ]" << std::endl;
	return value;
}

bool RedisManager::Set(const std::string& key, const std::string& value)
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "set %s %s", key.c_str(), value.c_str());
	if (reply_ == nullptr)
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		return false;  // 不要 freeReplyObject
	}

	if (reply_->type != REDIS_REPLY_STATUS || strcmp(reply_->str, "OK") != 0)
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	freeReplyObject(reply_);
	std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
	return true;

}

bool RedisManager::Auth(const std::string& password)
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "AUTH %s", password.c_str());
	if (reply_->type == REDIS_REPLY_ERROR)
	{
		std::cout << "AUTH failed: password: " << password << std::endl;
		freeReplyObject(reply_);
		return false;
	}
	freeReplyObject(reply_);
	std::cout << "AUTH Success: password: " << password << std::endl;
	return true;
}

bool RedisManager::LPush(const std::string& key, const std::string& value)
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "LPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply_)
	{
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	if (reply_->type != REDIS_REPLY_INTEGER || reply_->integer <= 0) {
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

std::string RedisManager::LPop(const std::string& key) 
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "LPOP %s ", key.c_str());
	if (reply_ == nullptr || reply_->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return std::string();
	}
	std::string value = reply_->str;
	std::cout << "Execut command [ LPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return value;
}

bool RedisManager::RPush(const std::string& key, const std::string& value) 
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "RPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply_)
	{
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	if (reply_->type != REDIS_REPLY_INTEGER || reply_->integer <= 0) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

std::string RedisManager::RPop(const std::string& key) 
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "RPOP %s ", key.c_str());
	if (reply_ == nullptr || reply_->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return std::string();
	}
	std::string value = reply_->str;
	std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return value;
}

bool RedisManager::HSet(const std::string& key, const std::string& hkey, const std::string& value) 
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
	if (reply_ == nullptr || reply_->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

//bool RedisManager::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
//{
//	const char* argv[4];
//	size_t argvlen[4];
//	argv[0] = "HSET";
//	argvlen[0] = 4;
//	argv[1] = key;
//	argvlen[1] = strlen(key);
//	argv[2] = hkey;
//	argvlen[2] = strlen(hkey);
//	argv[3] = hvalue;
//	argvlen[3] = hvaluelen;
//	reply_ = (redisReply*)redisCommandArgv(connect_, 4, argv, argvlen);
//	if (reply_ == nullptr || reply_->type != REDIS_REPLY_INTEGER) {
//		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
//		freeReplyObject(reply_);
//		return false;
//	}
//	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
//	freeReplyObject(reply_);
//	return true;
//}

std::string RedisManager::HGet(const std::string& key, const std::string& hkey)
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	const char* argv[3];
	size_t argvlen[3];
	argv[0] = "HGET";
	argvlen[0] = 4;
	argv[1] = key.c_str();
	argvlen[1] = key.length();
	argv[2] = hkey.c_str();
	argvlen[2] = hkey.length();
	redisReply* reply_ = (redisReply*)redisCommandArgv(connect_, 3, argv, argvlen);
	if (reply_ == nullptr || reply_->type == REDIS_REPLY_NIL) {
		freeReplyObject(reply_);
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		return "";
	}

	std::string value = reply_->str;
	freeReplyObject(reply_);
	std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
	return value;
}

bool RedisManager::Del(const std::string& key)
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "DEL %s", key.c_str());
	if (reply_ == nullptr || reply_->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}
	std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

bool RedisManager::ExistsKey(const std::string& key)
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "exists %s", key.c_str());
	if (reply_ == nullptr || reply_->type != REDIS_REPLY_INTEGER || reply_->integer == 0) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}
	std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

std::string RedisManager::acqueireLock(const std::string& lockName, int lockTimeOut, int expireTime)
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	return RedisLocker::GetInstance()->acquireLock(connect_, lockName, lockTimeOut, expireTime);
}

bool RedisManager::releaseLock(const std::string& lockName, const std::string& lockValue)
{
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	return RedisLocker::GetInstance()->releaseLock(connect_, lockName, lockValue);
}

bool RedisManager::pushOfflineMessage(int uid, const std::string& message)
{
	// 向 Redis的列表 notify_message:uid 中推送消息 message
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});
	redisReply* reply = (redisReply*)redisCommand(connect_, "LPUSH notify_messages:%d %s", uid, message.c_str());
	if (reply == NULL) {
		std::cerr << "Error executing Redis command" << std::endl;
	}
	else {
		if (reply->type == REDIS_REPLY_ERROR) {
			std::cerr << "Redis error: " << reply->str << std::endl;
		}
		else {
			std::cout << "Offline message pushed to Redis: " << message << std::endl;
		}
	}
	return true;
}

std::vector<std::string> RedisManager::popOfflineMessages(int uid)
{
	// 向 Redis的列表 notify_message:uid 中获取所有的消息
	auto connect_ = pool_->getConnection();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return std::vector<std::string>();
	}
	Defer defer([this, &connect_]() {
		pool_->returnConnection(connect_);
		});

	// 定义一个 Lua 脚本，一次性获取并清空队列
	const char* lua_script =
		"local key = KEYS[1] "
		"local messages = redis.call('LRANGE', key, 0, -1) "
		"redis.call('DEL', key) "
		"return messages";

	redisReply* reply = (redisReply*)redisCommand(connect_,
		"EVAL %s 1 notify_messages:%d", lua_script, uid);

	std::vector<std::string> messages;
	if (reply && reply->type == REDIS_REPLY_ARRAY) {
		for (size_t i = 0; i < reply->elements; ++i) {
			if (reply->element[i]->str) {
				messages.push_back(reply->element[i]->str);
			}
		}
	}
	return messages;
}

RedisManager::RedisManager()
{
	pool_ = std::make_unique<RedisConnPool>();
}

// 需要将连接返回给 redis连接池
RedisManager::~RedisManager()
{
}