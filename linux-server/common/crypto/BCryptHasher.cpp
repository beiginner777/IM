#include "BCryptHasher.h"
#include "bcrypt_impl.h"   // 仅用于 bcrypt_encode64（生成 salt）
#include <crypt.h>          // 系统 libxcrypt：crypt_r（标准 bcrypt 实现）
#include <random>
#include <cstring>

std::string BCryptHasher::generateSalt(unsigned int cost)
{
    // 生成 16 随机字节
    unsigned char randomBytes[16];
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<unsigned int> dist(0, 255);
    for (int i = 0; i < 16; i++) {
        randomBytes[i] = (unsigned char)dist(gen);
    }
    // 编码 16 字节 → 22 字符 Radix-64
    char encoded[32] = {0};
    bcrypt_encode64(encoded, randomBytes, 16);
    // 格式: $2b$<cost>$<22-char-salt>
    std::string costStr = std::to_string(cost);
    if (costStr.length() < 2) costStr = "0" + costStr;
    return "$2b$" + costStr + "$" + std::string(encoded, 22);
}

std::string BCryptHasher::generateHash(const std::string& password, unsigned int cost)
{
    if (password.empty() || cost < 4 || cost > 31) return "";
    if (password.length() > 72) return "";
    std::string salt = generateSalt(cost);
    struct crypt_data data;
    memset(&data, 0, sizeof(data));
    char* result = crypt_r(password.c_str(), salt.c_str(), &data);
    if (!result) return "";
    return std::string(result);
}

bool BCryptHasher::verifyPassword(const std::string& password, const std::string& hash)
{
    if (password.empty() || hash.empty()) return false;
    if (password.length() > 72) return false;
    struct crypt_data data;
    memset(&data, 0, sizeof(data));
    char* result = crypt_r(password.c_str(), hash.c_str(), &data);
    if (!result) return false;
    return std::string(result) == hash;
}
