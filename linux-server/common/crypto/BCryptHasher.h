#ifndef BCRYPT_HASHER_H
#define BCRYPT_HASHER_H
#include <string>
/// bcrypt password hasher backed by OpenBSD reference implementation
class BCryptHasher
{
public:
    /// Generate bcrypt hash: $2b$<cost>$<22-char-salt><31-char-hash>
    static std::string generateHash(const std::string& password, unsigned int cost = 10);
    /// Verify password against a bcrypt hash
    static bool verifyPassword(const std::string& password, const std::string& hash);
private:
    static std::string generateSalt(unsigned int cost);
};
#endif
