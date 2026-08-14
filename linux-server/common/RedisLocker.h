#ifndef REDISLOCKER_H
#define REDISLOCKER_H

#include <string>
#include <hiredis/hiredis.h>
class RedisLocker
{
public:
	static RedisLocker* GetInstance()
	{
		static RedisLocker instance;
		return &instance;
	}
	~RedisLocker();
	// lockTimeOut: ��ʱʱ�䣬��ֹ���޷��ʣ���ɳ������������� expireTime: ���Ĺ���ʱ�䣬��λ��
	std::string acquireLock(redisContext* context, const std::string& lockName, int lockTimeOut, int expireTime);
	// �ͷ���
	bool releaseLock(redisContext* context, const std::string& lockName, const std::string& lockValue);
private:
	RedisLocker();
};
#endif
