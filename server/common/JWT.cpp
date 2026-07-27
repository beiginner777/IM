#include "JWT.h"
#include "ConfigManager.h"
#include "RedisManager.h"
#include <json/json.h>
#include <windows.h>
#include <bcrypt.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

// ==================== Base64 URL-safe (RFC 7515) ====================
static const char BASE64URL_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static std::string base64UrlEncode(const std::vector<unsigned char>& data)
{
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        uint32_t val = (uint32_t)data[i] << 16;
        if (i + 1 < data.size()) val |= (uint32_t)data[i + 1] << 8;
        if (i + 2 < data.size()) val |= (uint32_t)data[i + 2];
        out += BASE64URL_CHARS[(val >> 18) & 0x3F];
        out += BASE64URL_CHARS[(val >> 12) & 0x3F];
        out += (i + 1 < data.size()) ? BASE64URL_CHARS[(val >> 6) & 0x3F] : '=';
        out += (i + 2 < data.size()) ? BASE64URL_CHARS[val & 0x3F] : '=';
    }
    // Remove padding (URL-safe base64 for JWT)
    while (!out.empty() && out.back() == '=') out.pop_back();
    return out;
}

static std::vector<unsigned char> base64UrlDecode(const std::string& str)
{
    std::string s = str;
    // Add padding back
    while (s.size() % 4 != 0) s += '=';
    std::vector<unsigned char> out;
    out.reserve(s.size() * 3 / 4);

    // Build decode table
    static int decodeTable[256] = {};
    static bool tableBuilt = false;
    if (!tableBuilt) {
        for (int i = 0; i < 256; i++) decodeTable[i] = -1;
        for (int i = 0; i < 64; i++) decodeTable[(unsigned char)BASE64URL_CHARS[i]] = i;
        decodeTable['+'] = 62;  // Also support standard base64
        decodeTable['/'] = 63;
        decodeTable['='] = 0;
        tableBuilt = true;
    }

    for (size_t i = 0; i < s.size(); i += 4) {
        int a = decodeTable[(unsigned char)s[i]];
        int b = decodeTable[(unsigned char)s[i + 1]];
        int c = decodeTable[(unsigned char)s[i + 2]];
        int d = decodeTable[(unsigned char)s[i + 3]];
        uint32_t val = (a << 18) | (b << 12) | (c << 6) | d;
        out.push_back((val >> 16) & 0xFF);
        if (s[i + 2] != '=') out.push_back((val >> 8) & 0xFF);
        if (s[i + 3] != '=') out.push_back(val & 0xFF);
    }
    return out;
}

// ==================== HMAC-SHA256 (Windows BCrypt API) ====================
static std::vector<unsigned char> hmacSha256(const std::string& key, const std::string& data)
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    std::vector<unsigned char> hash(32);

    // 打开 SHA-256 HMAC provider
    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL,
                                    BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0)
        return hash;

    // 创建 hash 对象（传入密钥）
    if (BCryptCreateHash(hAlg, &hHash, NULL, 0,
                         (PUCHAR)key.data(), (ULONG)key.size(), 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return hash;
    }

    // 哈希数据
    BCryptHashData(hHash, (PUCHAR)data.data(), (ULONG)data.size(), 0);

    // 获取结果
    BCryptFinishHash(hHash, hash.data(), 32, 0);

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return hash;
}

// ==================== 签名用二进制转 Hex ====================
static std::string binToHex(const std::vector<unsigned char>& data)
{
    std::ostringstream oss;
    for (unsigned char c : data)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    return oss.str();
}

// ==================== 生成紧凑 JSON（无换行） ====================
static std::string compactJson(const Json::Value& v)
{
    Json::FastWriter writer;
    std::string s = writer.write(v);
    // FastWriter 末尾有一个换行，去掉
    if (!s.empty() && s.back() == '\n') s.pop_back();
    return s;
}

// ==================== Public API ====================
std::string JWT::generateToken(int uid, const std::string& username,
                                 const std::string& clientType)
{
    auto now = std::chrono::system_clock::now();
    auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    // 1. Header
    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";
    std::string headerB64 = base64UrlEncode(
        std::vector<unsigned char>(compactJson(header).begin(), compactJson(header).end()));

    // 2. Payload
    Json::Value payload;
    payload["uid"] = uid;
    payload["username"] = username;
    payload["client_type"] = clientType;
    payload["iat"] = (int)nowSec;
    payload["exp"] = (int)(nowSec + TOKEN_TTL);
    std::string payloadB64 = base64UrlEncode(
        std::vector<unsigned char>(compactJson(payload).begin(), compactJson(payload).end()));

    // 3. Sign
    std::string toSign = headerB64 + "." + payloadB64;
    std::string secret = ConfigManager::getInstance()["JWT"]["Secret"];
    auto hash = hmacSha256(secret, toSign);
    std::string sigHex = binToHex(hash);

    return toSign + "." + sigHex;
}

bool JWT::verify(const std::string& token, int& uid)
{
    // 1. 拆三段
    auto p1 = token.find('.');
    auto p2 = token.rfind('.');
    if (p1 == std::string::npos || p2 == std::string::npos || p1 == p2)
        return false;

    std::string headerB64  = token.substr(0, p1);
    std::string payloadB64 = token.substr(p1 + 1, p2 - p1 - 1);
    std::string sigReceived = token.substr(p2 + 1);
    std::string toSign = headerB64 + "." + payloadB64;

    // 2. 验签
    std::string secret = ConfigManager::getInstance()["JWT"]["Secret"];
    auto hash = hmacSha256(secret, toSign);
    std::string sigComputed = binToHex(hash);
    if (sigComputed != sigReceived)
        return false;

    // 3. 解码 payload
    auto payloadBytes = base64UrlDecode(payloadB64);
    std::string payloadJson(payloadBytes.begin(), payloadBytes.end());
    Json::Value payload;
    Json::Reader reader;
    if (!reader.parse(payloadJson, payload))
        return false;

    // 4. 检查过期
    auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (nowSec > payload["exp"].asInt())
        return false;

    // 5. 检查黑名单（key 由 payload 里的 client_type 决定）
    int tokenUid = payload["uid"].asInt();
    int64_t iat = payload["iat"].asInt();
    std::string clientType = payload.get("client_type", "").asString();
    if (clientType.empty()) clientType = CLIENT_WEB; // 兼容旧 token
    if (isRevoked(tokenUid, iat, clientType))
        return false;

    uid = tokenUid;
    return true;
}

void JWT::revoke(int uid, const std::string& clientType)
{
    auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    RedisManager::getInstance()->Set(
        "jwt:revoked:" + clientType + ":" + std::to_string(uid),
        std::to_string(nowSec));
}

bool JWT::isRevoked(int uid, int64_t iat, const std::string& clientType)
{
    // 强制读 Master，保证读到最新的黑名单（刚 revoke 的 key 可能还没同步到 Slave）
    std::string val = RedisManager::getInstance()->Get(
        "jwt:revoked:" + clientType + ":" + std::to_string(uid), true);
    if (val.empty()) return false;
    int64_t revokedAt = std::stoll(val);
    return iat < revokedAt;
}
