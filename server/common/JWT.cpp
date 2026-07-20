#include "JWT.h"
#include "RedisManager.h"
#include "ConfigManager.h"
#include <json/json.h>

bool JWT::verify(const std::string& token, int& uid, const std::string& username)
{
	// 查 Redis: "token:{token}" → JSON {uid, username, exp}
	std::string key = "token:" + token;
	std::string jsonStr = RedisManager::getInstance()->Get(key, true);
	if (jsonStr.empty()) return false;

	Json::Value payload;
	Json::Reader reader;
	if (!reader.parse(jsonStr, payload)) return false;

	long long exp = payload["exp"].asInt();
	auto now = std::chrono::system_clock::now();
	auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
	if (nowSec > exp) {
		RedisManager::getInstance()->Del(key);
		return false;
	}

	uid = payload["uid"].asInt();
	if (!username.empty() && payload["username"].asString() != username) return false;
	return true;
}
