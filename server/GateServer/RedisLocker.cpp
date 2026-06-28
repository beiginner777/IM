#include "RedisLocker.h"

RedisLocker::RedisLocker()
{
}

RedisLocker::~RedisLocker()
{
}

std::string RedisLocker::acquireLock(redisContext* context, const std::string& lockName, int lockTimeOut, int expireTime)
{
	/*
	NX：确保只有当键不存在时才能成功设置（即，只有一个客户端能够成功设置锁）。
	EX：设置一个超时时间，锁会在超时后自动释放，避免死锁。
	*/
	std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(lockTimeOut);
	while (std::chrono::steady_clock::now() < end) {
		// 尝试获取锁
		redisReply* reply = (redisReply*)redisCommand(context, "SET %s %s NX EX %d", lockName.c_str(), uuid.c_str(), expireTime);
		// 判断是否获取锁成功
		if (reply != nullptr) {
			if(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK") {
				freeReplyObject(reply);
				return uuid; // 获取锁成功，返回锁的标识
			}
			freeReplyObject(reply);
		}
		// 等待一段时间后重试
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return ""; // 获取锁失败，返回空字符串
}

bool RedisLocker::releaseLock(redisContext* context, const std::string& lockName, const std::string& lockValue)
{	
	const char* luaScript =
		"if redis.call('get', KEYS[1]) == ARGV[1] then "
		"   return redis.call('del', KEYS[1]) "
		"else "
		"   return 0 "
		"end";
	// 使用EVAL命令执行Lua脚本，第一个参数是脚本内容，后面的参数依次是键的数量，键和参数
	redisReply* reply = (redisReply*)redisCommand(context, "EVAL %s 1 %s %s", luaScript, lockName.c_str(), lockValue.c_str());
	
	if (reply != nullptr) {
		if(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1) {
			freeReplyObject(reply);
			return true; // 释放锁成功
		}
		freeReplyObject(reply);
	}
	return false;
}
