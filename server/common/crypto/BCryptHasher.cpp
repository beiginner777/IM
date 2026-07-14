#include "BCryptHasher.h"
#include "bcrypt_impl.h"
#include <random>
#include <cstring>
#include <iostream>
std::string BCryptHasher::generateSalt(unsigned int cost)
{
    // Generate 16 random bytes
    unsigned char randomBytes[16];
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<unsigned int> dist(0, 255);
    for (int i = 0; i < 16; i++) {
        randomBytes[i] = (unsigned char)dist(gen);
    }
    // Encode to 22 Radix-64 chars
    char encoded[32] = {0};
    bcrypt_encode64(encoded, randomBytes, 16);
    // Format: $2b$<cost>$<22-char-salt>
    std::string costStr = std::to_string(cost);
    if (costStr.length() < 2) costStr = "0" + costStr;
    return "$2b$" + costStr + "$" + std::string(encoded, 22);
}
std::string BCryptHasher::generateHash(const std::string& password, unsigned int cost)
{
    if (password.empty() || cost < 4 || cost > 31) return "";
    if (password.length() > 72) return "";
    std::string salt = generateSalt(cost);
    // Decode the 22-char salt back into 16 raw bytes
    // Use the same bcrypt_encode64 on raw bytes that were just generated
    unsigned char saltBytes[16];
    // Reconstruct from the encoded salt string
    // The salt is $2b$XX$<22-char-radix-64>
    const char* saltChars = salt.c_str() + 7;  // skip "$2b$XX$"
    // Decode Radix-64: manually do the inverse
    {
        const char* Base64Code = "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        int di = 0, si = 0;
        while (si < 22) {
            int c[4] = {0, 0, 0, 0};
            int n = 0;
            for (; n < 4 && si < 22; n++, si++) {
                char ch = saltChars[si];
                const char* pos = (const char*)memchr(Base64Code, ch, 64);
                c[n] = pos ? (int)(pos - Base64Code) : 0;
            }
            if (n >= 2) saltBytes[di++] = (unsigned char)((c[0] << 2) | (c[1] >> 4));
            if (n >= 3) saltBytes[di++] = (unsigned char)(((c[1] & 0x0F) << 4) | (c[2] >> 2));
            if (n >= 4) saltBytes[di++] = (unsigned char)(((c[2] & 0x03) << 6) | c[3]);
        }
    }
    // Compute bcrypt hash using OpenBSD reference
    std::string pwdWithNull = password + '\0';
    unsigned char ciphertext[24];
    bcrypt_hash(
        (const unsigned char*)pwdWithNull.c_str(), pwdWithNull.size(),
        saltBytes, 16,
        cost,
        ciphertext);
    // Encode 23 bytes of ciphertext to 31 Radix-64 chars
    char hashEncoded[32] = {0};
    bcrypt_encode64(hashEncoded, ciphertext, 23);
    // Construct full hash: $2b$<cost>$<22-char-salt><31-char-hash>
    std::string saltOnly = salt.substr(7, 22);
    std::string costStr = std::to_string(cost);
    if (costStr.length() < 2) costStr = "0" + costStr;
    return "$2b$" + costStr + "$" + saltOnly + std::string(hashEncoded, 31);
}
bool BCryptHasher::verifyPassword(const std::string& password, const std::string& hash)
{
    std::cout << "===========================================================================" << std::endl;
    std::cout << "Verifying password: " << password << " against hash: " << hash << std::endl;
    if (password.empty() || hash.empty())
        return false;
    if (hash.length() < 28 || hash[0] != '$')
        return false;
    if (password.length() > 72)
        return false;
    unsigned int cost = (unsigned int)std::stoi(hash.substr(4, 2));
    // Extract and decode the 22-char salt
    std::string saltPrefix = hash.substr(0, 29);  // "$2b$XX$<22-char-salt>"
    const char* saltChars = saltPrefix.c_str() + 7;
    unsigned char saltBytes[16];
    {
        const char* Base64Code = "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        int di = 0, si = 0;
        while (si < 22) {
            int c[4] = {0, 0, 0, 0};
            int n = 0;
            for (; n < 4 && si < 22; n++, si++) {
                char ch = saltChars[si];
                const char* pos = (const char*)memchr(Base64Code, ch, 64);
                c[n] = pos ? (int)(pos - Base64Code) : 0;
            }
            if (n >= 2) saltBytes[di++] = (unsigned char)((c[0] << 2) | (c[1] >> 4));
            if (n >= 3) saltBytes[di++] = (unsigned char)(((c[1] & 0x0F) << 4) | (c[2] >> 2));
            if (n >= 4) saltBytes[di++] = (unsigned char)(((c[2] & 0x03) << 6) | c[3]);
        }
    }
    // Compute
    std::string pwdWithNull = password + '\0';
    unsigned char ciphertext[24];
    bcrypt_hash(
        (const unsigned char*)pwdWithNull.c_str(), pwdWithNull.size(),
        saltBytes, 16,
        cost,
        ciphertext);
    // Encode
    char hashEncoded[32] = {0};
    bcrypt_encode64(hashEncoded, ciphertext, 23);
    // Construct
    std::string saltOnly = hash.substr(7, 22);
    std::string costStr = std::to_string(cost);
    if (costStr.length() < 2) costStr = "0" + costStr;
    std::string computed = "$2b$" + costStr + "$" + saltOnly + std::string(hashEncoded, 31);
    // Compare
    if (computed.length() != hash.length()) {
        std::cout << "BCryptHasher::verify FAILED (length mismatch)" << std::endl;
        std::cout << "  computed: " << computed << std::endl;
        std::cout << "===========================================================================" << std::endl;
        return false;
    }
    int result = 0;
    for (size_t i = 0; i < computed.length(); i++) {
        result |= (computed[i] ^ hash[i]);
    }
    if (result == 0) {
        std::cout << "BCryptHasher::verify SUCCESS" << std::endl;
    } else {
        std::cout << "BCryptHasher::verify FAILED" << std::endl;
        std::cout << "  computed: " << computed << std::endl;
    }
    std::cout << "===========================================================================" << std::endl;
    return result == 0;
}
