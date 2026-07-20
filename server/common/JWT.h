#ifndef JWT_H
#define JWT_H
#include <string>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <chrono>

// 简化 JWT：UUID token + Redis 存 payload（无 OpenSSL 依赖）
// token = random UUID, Redis: "token:{uuid}" → {uid, username, exp}
class JWT
{
public:
	// 生成 token（返回 UUID 字符串）
	static std::string generateToken(int uid, const std::string& username)
	{
		auto a_uuid = boost::uuids::random_generator()();
		return boost::uuids::to_string(a_uuid);
	}

	// 验证 token（查 Redis），成功则提取 uid
	static bool verify(const std::string& token, int& uid, const std::string& username);

	// Redis 存储的 TTL（秒）
	static constexpr int TOKEN_TTL = 86400; // 24h
};

#endif
