#ifndef MESSAGE_DEDUPLICATOR_H
#define MESSAGE_DEDUPLICATOR_H

#include "SingleTon.h"
#include <string>
class MessageDeduplicator : public SingleTon<MessageDeduplicator>
{
	friend class SingleTon<MessageDeduplicator>;
public:
	MessageDeduplicator() = default;
	~MessageDeduplicator() = default;
	bool isDuplicate(const std::string& uuid);
	void cacheResult(const std::string& uuid, const std::string& ackContent, int ttlSeconds = 300);
	std::string getCachedAck(const std::string& uuid);
	bool remove(const std::string& uuid);
private:
	std::string makeKey(const std::string& uuid);
	static const char* KEY_PREFIX;
	static const int DEFAULT_TTL;
};
#endif
