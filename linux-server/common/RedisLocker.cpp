#include "RedisLocker.h"
#include <chrono>
#include <thread>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
RedisLocker::RedisLocker()
{
}
RedisLocker::~RedisLocker()
{
}

std::string RedisLocker::acquireLock(redisContext* context, const std::string& lockName, int lockTimeOut, int expireTime)
{
	/*
	NX��ȷ��ֻ�е���������ʱ���ܳɹ����ã�����ֻ��һ���ͻ����ܹ��ɹ�����������
	EX������һ����ʱʱ�䣬�����ڳ�ʱ���Զ��ͷţ�����������
	*/
	std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(lockTimeOut);
	while (std::chrono::steady_clock::now() < end) {
		// ���Ի�ȡ��
		redisReply* reply = (redisReply*)redisCommand(context, "SET %s %s NX EX %d", lockName.c_str(), uuid.c_str(), expireTime);
		// �ж��Ƿ��ȡ���ɹ�
		if (reply != nullptr) {
			if(reply->type == REDIS_REPLY_STATUS && std::string(reply->str) == "OK") {
				freeReplyObject(reply);
				return uuid; // ��ȡ���ɹ����������ı�ʶ
			}
			freeReplyObject(reply);
		}
		// �ȴ�һ��ʱ�������
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return ""; // ��ȡ��ʧ�ܣ����ؿ��ַ���
}

bool RedisLocker::releaseLock(redisContext* context, const std::string& lockName, const std::string& lockValue)
{	
	const char* luaScript =
		"if redis.call('get', KEYS[1]) == ARGV[1] then "
		"   return redis.call('del', KEYS[1]) "
		"else "
		"   return 0 "
		"end";
	// ʹ��EVAL����ִ��Lua�ű�����һ�������ǽű����ݣ�����Ĳ��������Ǽ������������Ͳ���
	redisReply* reply = (redisReply*)redisCommand(context, "EVAL %s 1 %s %s", luaScript, lockName.c_str(), lockValue.c_str());
	
	if (reply != nullptr) {
		if(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1) {
			freeReplyObject(reply);
			return true; // �ͷ����ɹ�
		}
		freeReplyObject(reply);
	}
	return false;
}
