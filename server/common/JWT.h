#ifndef JWT_H
#define JWT_H
#include <string>
#include <cstdint>

// 真实 JWT：HMAC-SHA256 签名，自包含 claims
// 验签纯 CPU 运算，不查 Redis。吊销通过 Redis 黑名单（jwt:revoked:{uid}）
class JWT
{
public:
    // 签发 JWT token（header.payload.signature），exp = 当前时间 + TTL
    static std::string generateToken(int uid, const std::string& username);

    // 验签 + 提取 uid。不查 Redis（除非走黑名单检查）
    static bool verify(const std::string& token, int& uid);

    // 吊销某个 uid 的所有旧 token（改密码/封号时调用）
    // Redis: SET "jwt:revoked:{uid}" = now_epoch
    static void revoke(int uid);

    // 检查 uid 是否已被吊销（iat < revoked_at → token 在吊销前签发 → 失效）
    static bool isRevoked(int uid, int64_t iat);

    static constexpr int TOKEN_TTL = 86400; // 24h
};
#endif
