#ifndef JWT_H
#define JWT_H
#include <string>
#include <cstdint>

// 真实 JWT：HMAC-SHA256 签名，自包含 claims
// 验签纯 CPU 运算，不查 Redis。吊销通过 Redis 黑名单
// web 和 desktop 的吊销 key 独立，互不干扰
class JWT
{
public:
    static constexpr const char* CLIENT_WEB     = "web";
    static constexpr const char* CLIENT_DESKTOP = "desktop";

    // 签发 JWT token（header.payload.signature），exp = 当前时间 + TTL
    // clientType: "web" 或 "desktop"，决定吊销时查哪个黑名单
    static std::string generateToken(int uid, const std::string& username,
                                     const std::string& clientType = CLIENT_WEB);

    // 验签 + 提取 uid。不查 Redis（除非走黑名单检查）
    // 黑名单 key 由 payload 里的 client_type 决定
    static bool verify(const std::string& token, int& uid);

    // 吊销某个 uid + clientType 的所有旧 token
    // Redis: SET "jwt:revoked:{clientType}:{uid}" = now_epoch
    static void revoke(int uid, const std::string& clientType);

    // 检查 uid 是否已被吊销
    static bool isRevoked(int uid, int64_t iat, const std::string& clientType);

    static constexpr int TOKEN_TTL = 86400; // 24h
};
#endif
