/**
 * Global test environment: initializes ConfigManager & RedisManager once
 * before all tests, cleans up after all tests complete.
 *
 * Used with gtest_main.lib (which provides main()).
 */

#include <gtest/gtest.h>
#include <iostream>
#include "ConfigManager.h"
#include "RedisManager.h"

class IMTestEnvironment : public ::testing::Environment
{
public:
    ~IMTestEnvironment() override {}

    void SetUp() override
    {
        std::cout << "\n====================================================" << std::endl;
        std::cout << "  IM Dedup/Retry Integration & Benchmark Tests" << std::endl;
        std::cout << "====================================================" << std::endl;

        try {
            ConfigManager::getInstance();
            RedisManager::getInstance();
            std::cout << "  Redis connected OK at 127.0.0.1:6380" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "  FATAL: " << e.what() << std::endl;
            std::cerr << "  Make sure Redis is running at 127.0.0.1:6380" << std::endl;
            exit(1);
        }

        std::cout << "====================================================\n" << std::endl;
    }

    void TearDown() override
    {
        std::cout << "\n====================================================" << std::endl;
        std::cout << "  All tests complete." << std::endl;
        std::cout << "====================================================" << std::endl;
    }
};

// Register the environment globally — runs before all test cases
::testing::Environment* const imTestEnv =
    ::testing::AddGlobalTestEnvironment(new IMTestEnvironment);
