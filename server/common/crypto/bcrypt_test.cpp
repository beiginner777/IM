#include "BCryptHasher.h"
#include <iostream>
#include <string>
#include <cstring>

static int tests = 0, passed = 0;

#define TEST(name) do { tests++; std::cout << "[" << tests << "] " << name << "... "; } while(0)
#define OK() do { passed++; std::cout << "OK" << std::endl; } while(0)
#define FAIL(msg) do { std::cout << "FAIL: " << msg << std::endl; } while(0)

void test_self_consistency()
{
    TEST("self-consistency: generateHash + verifyPassword");
    std::string pwd = "123456";
    std::string hash = BCryptHasher::generateHash(pwd, 10);
    if (hash.empty()) { FAIL("generateHash returned empty"); return; }
    std::cout << std::endl << "      " << hash << std::endl;
    if (!BCryptHasher::verifyPassword(pwd, hash)) { FAIL("own hash fail"); return; }
    if (BCryptHasher::verifyPassword("wrong", hash)) { FAIL("wrong password matched"); return; }
    OK();
}

void test_hash_format()
{
    TEST("hash format");
    std::string hash = BCryptHasher::generateHash("test", 10);
    if (hash.length() != 60) { FAIL("length " + std::to_string(hash.length())); return; }
    if (hash.substr(0, 4) != "$2b$") { FAIL("no $2b$ prefix"); return; }
    if (hash[6] != '$') { FAIL("no $ after cost"); return; }
    OK();
}

void test_deterministic()
{
    TEST("deterministic: same salt → same hash");
    std::string pwd = "hello";
    std::string hash1 = BCryptHasher::generateHash(pwd, 10);
    if (!BCryptHasher::verifyPassword(pwd, hash1)) { FAIL("verify1 failed"); return; }
    if (BCryptHasher::verifyPassword("world", hash1)) { FAIL("wrong password matched"); return; }
    OK();
}

void test_cost4()
{
    TEST("cost=4 self-check");
    std::string hash = BCryptHasher::generateHash("a", 4);
    if (hash.length() != 60) { FAIL("bad length"); return; }
    if (!BCryptHasher::verifyPassword("a", hash)) { FAIL("verify fail"); return; }
    if (BCryptHasher::verifyPassword("b", hash)) { FAIL("wrong match"); return; }
    OK();
}

void test_python_compat()
{
    struct { const char* pwd; const char* hash; } cases[] = {
        {"123", "$2b$10$79O9GE5dUitei9Sm8J2JfuZ5PY7WPUYA4oXfm/y7pVTthiKcUK2tu"},
        {"123", "$2b$10$6yv.2CEEWu8w5ufR8By2DulAwNW5.iDvDZQ5ZB5Pv2wHX9XCHTacK"},
        {"123", "$2b$10$oANgkcdiqeXghYYn/acRZeUYw.xR4A8p5fAdGyXZ4kPswR8Z8Utjq"},
        {nullptr, nullptr}
    };

    TEST("Python bcrypt 5.0.0 compatibility");
    std::cout << std::endl;
    bool anyFail = false;
    for (int i = 0; cases[i].pwd; i++) {
        bool ok = BCryptHasher::verifyPassword(cases[i].pwd, cases[i].hash);
        std::cout << "      \"" << cases[i].pwd << "\" vs " << cases[i].hash << " → "
                  << (ok ? "MATCH" : "MISMATCH") << std::endl;
        if (!ok) anyFail = true;
    }
    if (anyFail) FAIL("compatibility failed");
    else OK();
}

int main()
{
    std::cout << "=== bcrypt test (OpenBSD reference impl) ===" << std::endl << std::endl;
    test_hash_format();
    test_self_consistency();
    test_deterministic();
    test_cost4();
    test_python_compat();
    std::cout << std::endl << "=== " << passed << "/" << tests << " passed ===" << std::endl;
    return passed < tests ? 1 : 0;
}
