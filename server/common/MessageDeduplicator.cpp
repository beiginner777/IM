#include "MessageDeduplicator.h"
#include "RedisManager.h"
const char* MessageDeduplicator::KEY_PREFIX = "msg:dedup:";
const int   MessageDeduplicator::DEFAULT_TTL = 300;
std::string MessageDeduplicator::makeKey(const std::string& uuid)
{
	return std::string(KEY_PREFIX) + uuid;
}
bool MessageDeduplicator::isDuplicate(const std::string& uuid)
{
	auto* redis = RedisManager::getInstance().get();
	return redis->ExistsKey(makeKey(uuid));
}
void MessageDeduplicator::cacheResult(const std::string& uuid, const std::string& ackContent, int ttlSeconds)
{
	auto* redis = RedisManager::getInstance().get();
	bool ok = redis->SetExp(makeKey(uuid), ackContent, ttlSeconds);
	if (!ok) {
		std::cerr << "MessageDeduplicator: cacheResult failed for uuid=" << uuid << std::endl;
	}
}
std::string MessageDeduplicator::getCachedAck(const std::string& uuid)
{
	auto* redis = RedisManager::getInstance().get();
	std::string ack = redis->Get(makeKey(uuid));
	return ack;
}
bool MessageDeduplicator::remove(const std::string& uuid)
{
	auto* redis = RedisManager::getInstance().get();
	return redis->Del(makeKey(uuid));
}
