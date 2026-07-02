#ifndef REDISLOCKER_H
#define REDISLOCKER_H

#include "global.h"

class RedisLocker
{
public:
	static RedisLocker* GetInstance()
	{
		static RedisLocker instance;
		return &instance;
	}
	~RedisLocker();
	// lockTimeOut: 超时时间，防止无限访问，造成程序严重阻塞。 expireTime: 锁的过期时间，单位秒
	std::string acquireLock(redisContext* context, const std::string& lockName, int lockTimeOut, int expireTime);
	// 释放锁
	bool releaseLock(redisContext* context, const std::string& lockName, const std::string& lockValue);
private:
	RedisLocker();
};

#endif

