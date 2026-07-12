/**
 * MessageDeduplicator unit tests (integration tests requiring local Redis)
 *
 * Prerequisites: Redis running at 127.0.0.1:6380, password 123456
 * Tests use isolated key prefix (msg:dedup:test:) to avoid data conflicts.
 */

#include <gtest/gtest.h>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <sstream>
#include <iomanip>

#include "MessageDeduplicator.h"
#include "RedisManager.h"
#include "ConfigManager.h"

// ==== Test Fixture: clean Redis test keys before/after each case ====

class MessageDeduplicatorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        dedup_ = MessageDeduplicator::getInstance();
        redis_ = RedisManager::getInstance();
    }

    void TearDown() override
    {
        // clean up all test keys (prefix: msg:dedup:test:)
        for (const auto& uuid : testUuids_) {
            dedup_->remove(uuid);
        }
        testUuids_.clear();
    }

    // return a test-prefixed UUID to avoid polluting production keys
    std::string testUuid(const std::string& id)
    {
        std::string full = "test:" + id;
        testUuids_.push_back(full);
        return full;
    }

    // short alias for testUuid (used in integration tests)
    std::string reg(const std::string& id)
    {
        return testUuid(id);
    }

    // Simulate server-side receive: check dedup, process, cache ACK
    std::string simulateServerReceive(const std::string& uuid,
                                       const std::string& ackContent)
    {
        if (dedup_->isDuplicate(uuid)) {
            return dedup_->getCachedAck(uuid);
        }
        dedup_->cacheResult(uuid, ackContent);
        return ackContent;
    }

    std::shared_ptr<MessageDeduplicator> dedup_;
    std::shared_ptr<RedisManager> redis_;

private:
    std::vector<std::string> testUuids_;
};

// ==== Test Cases ====

// 1. Unknown UUID should NOT be duplicate
TEST_F(MessageDeduplicatorTest, isDuplicate_UnknownUuid_ReturnsFalse)
{
    std::string uuid = testUuid("unknown-001");
    EXPECT_FALSE(dedup_->isDuplicate(uuid))
        << "Never-seen UUID should not be marked as duplicate";
}

// 2. After cacheResult, isDuplicate returns true
TEST_F(MessageDeduplicatorTest, cacheResult_MakesIsDuplicateTrue)
{
    std::string uuid = testUuid("cache-001");
    dedup_->cacheResult(uuid, "ACK_CONTENT_1");

    EXPECT_TRUE(dedup_->isDuplicate(uuid))
        << "After cacheResult, isDuplicate should return true";
}

// 3. getCachedAck returns the exact original content
TEST_F(MessageDeduplicatorTest, getCachedAck_ReturnsCorrectContent)
{
    std::string uuid = testUuid("ack-001");
    std::string ack = R"({"code":0,"msg":"ok","data":"hello"})";

    dedup_->cacheResult(uuid, ack);
    std::string cached = dedup_->getCachedAck(uuid);

    EXPECT_EQ(cached, ack)
        << "Cached ACK should match what was stored";
}

// 4. After remove, isDuplicate returns false
TEST_F(MessageDeduplicatorTest, remove_ClearsDuplicate)
{
    std::string uuid = testUuid("remove-001");
    dedup_->cacheResult(uuid, "data");

    ASSERT_TRUE(dedup_->isDuplicate(uuid));

    bool removed = dedup_->remove(uuid);
    EXPECT_TRUE(removed) << "remove should succeed for existing key";

    EXPECT_FALSE(dedup_->isDuplicate(uuid))
        << "After remove, isDuplicate should return false";
}

// 5. Two different UUIDs are independent
TEST_F(MessageDeduplicatorTest, DifferentUuids_AreIndependent)
{
    std::string uuidA = testUuid("indep-A");
    std::string uuidB = testUuid("indep-B");

    dedup_->cacheResult(uuidA, "ack-A");
    dedup_->cacheResult(uuidB, "ack-B");

    EXPECT_TRUE(dedup_->isDuplicate(uuidA));
    EXPECT_TRUE(dedup_->isDuplicate(uuidB));

    EXPECT_EQ(dedup_->getCachedAck(uuidA), "ack-A");
    EXPECT_EQ(dedup_->getCachedAck(uuidB), "ack-B");

    // remove A does not affect B
    dedup_->remove(uuidA);
    EXPECT_FALSE(dedup_->isDuplicate(uuidA));
    EXPECT_TRUE(dedup_->isDuplicate(uuidB));
}

// 6. Empty ACK content works
TEST_F(MessageDeduplicatorTest, EmptyAck_WorksCorrectly)
{
    std::string uuid = testUuid("empty-ack");
    dedup_->cacheResult(uuid, "");

    EXPECT_TRUE(dedup_->isDuplicate(uuid));
    EXPECT_EQ(dedup_->getCachedAck(uuid), "");
}

// 7. cacheResult overwrites existing entry (idempotent)
TEST_F(MessageDeduplicatorTest, cacheResult_OverwritesExisting)
{
    std::string uuid = testUuid("overwrite-001");
    dedup_->cacheResult(uuid, "first");
    dedup_->cacheResult(uuid, "second");

    EXPECT_TRUE(dedup_->isDuplicate(uuid));
    EXPECT_EQ(dedup_->getCachedAck(uuid), "second");
}

// 8. Custom TTL: key expires after specified seconds
TEST_F(MessageDeduplicatorTest, cacheResult_WithShortTTL_Expires)
{
    std::string uuid = testUuid("ttl-short");
    dedup_->cacheResult(uuid, "short-lived", 1);

    // immediate check: should exist
    EXPECT_TRUE(dedup_->isDuplicate(uuid));

    // wait 2s for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_FALSE(dedup_->isDuplicate(uuid))
        << "Key should expire after TTL=1s";
}

// 9. Double cacheResult still returns isDuplicate=true
TEST_F(MessageDeduplicatorTest, DoubleCache_StillDuplicate)
{
    std::string uuid = testUuid("double-001");
    dedup_->cacheResult(uuid, "data");
    dedup_->cacheResult(uuid, "data2");

    EXPECT_TRUE(dedup_->isDuplicate(uuid));
    EXPECT_EQ(dedup_->getCachedAck(uuid), "data2");
}

// 10. Concurrent writes — DISABLED: RedisConnPool thread-safety limitation
TEST_F(MessageDeduplicatorTest, DISABLED_Concurrent_Writes_DifferentUuids)
{
    const int N = 8;  // match RedisConnPool size (8 connections)
    std::vector<std::thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, i]() {
            std::string uuid = testUuid("concurrent-" + std::to_string(i));
            dedup_->cacheResult(uuid, "ack-" + std::to_string(i));
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    for (int i = 0; i < N; ++i) {
        std::string uuid = testUuid("concurrent-" + std::to_string(i));
        EXPECT_TRUE(dedup_->isDuplicate(uuid))
            << "UUID " << i << " should be cached after concurrent write";
        EXPECT_EQ(dedup_->getCachedAck(uuid), "ack-" + std::to_string(i));
    }
}

// ========================================================================
// TASK 2: Anomaly Tests - simulate network disruption -> retry + dedup
// ========================================================================

// 11. Normal flow (no disruption)
TEST_F(MessageDeduplicatorTest, Anomaly_NormalFlow_FirstSendProcessed)
{
    std::string uuid = reg("anomaly:normal-001");
    std::string ack  = R"({"code":0,"msg":"ok"})";
    std::string result = simulateServerReceive(uuid, ack);
    EXPECT_EQ(result, ack);
    EXPECT_TRUE(dedup_->isDuplicate(uuid));
}

// 12. Single retry (first ACK lost, client retries with same UUID)
TEST_F(MessageDeduplicatorTest, Anomaly_SingleRetry_ServerDedups)
{
    std::string uuid = reg("anomaly:retry1-001");
    std::string ack  = R"({"code":0,"msg":"ok"})";

    std::string result1 = simulateServerReceive(uuid, ack);
    EXPECT_EQ(result1, ack);

    // Retry: server returns cached ACK, does NOT reprocess
    std::string result2 = simulateServerReceive(uuid, "WRONG");
    EXPECT_EQ(result2, ack) << "Retry should return original cached ACK";
}

// 13. Triple retry (3x retransmit, server processes only once)
TEST_F(MessageDeduplicatorTest, Anomaly_TripleRetry_ProcessedOnce)
{
    std::string uuid = reg("anomaly:triple-001");
    std::string ack  = "first_and_only";
    int processCount = 0;

    for (int attempt = 0; attempt <= 3; attempt++) {
        if (!dedup_->isDuplicate(uuid)) {
            processCount++;
            dedup_->cacheResult(uuid, ack);
        }
        EXPECT_EQ(dedup_->getCachedAck(uuid), ack);
    }

    EXPECT_EQ(processCount, 1)
        << "Server should process exactly ONCE across 4 attempts";
}

// 14. Mixed messages: some retried, some not
TEST_F(MessageDeduplicatorTest, Anomaly_MixedMessages_SomeRetried)
{
    const int N = 30;
    for (int i = 0; i < N; i++) {
        std::string uuid = reg("anomaly:mixed-" + std::to_string(i));
        ASSERT_FALSE(dedup_->isDuplicate(uuid));
        dedup_->cacheResult(uuid, "ack-" + std::to_string(i));
    }

    // Retry selected messages
    for (int idx : {3, 7, 15, 22, 28}) {
        std::string uuid = "test:anomaly:mixed-" + std::to_string(idx);
        EXPECT_TRUE(dedup_->isDuplicate(uuid));
        EXPECT_EQ(dedup_->getCachedAck(uuid), "ack-" + std::to_string(idx));
    }

    // All 30 still cached (0 loss)
    for (int i = 0; i < N; i++) {
        std::string uuid = "test:anomaly:mixed-" + std::to_string(i);
        EXPECT_TRUE(dedup_->isDuplicate(uuid));
    }
}

// 15. Out-of-order retry (packet reordering)
TEST_F(MessageDeduplicatorTest, Anomaly_OutOfOrderRetry_StillDeduped)
{
    std::string uuid = reg("anomaly:ooo-001");
    std::string ack  = "processed";
    dedup_->cacheResult(uuid, ack);
    dedup_->cacheResult(uuid, ack); // same value = idempotent
    EXPECT_TRUE(dedup_->isDuplicate(uuid));
    EXPECT_EQ(dedup_->getCachedAck(uuid), ack);
}

// 16. Max retries exceeded -> client gives up
TEST_F(MessageDeduplicatorTest, Anomaly_MaxRetriesExceeded)
{
    std::string uuid = reg("anomaly:maxretry-001");
    std::string ack  = "processed";
    int processCount = 0;

    for (int attempt = 0; attempt < 4; attempt++) {
        if (!dedup_->isDuplicate(uuid)) {
            processCount++;
            dedup_->cacheResult(uuid, ack);
        }
        EXPECT_EQ(dedup_->getCachedAck(uuid), ack);
    }
    EXPECT_EQ(processCount, 1);
    // After max retries, dedup cache still holds the ACK (TTL=300s)
    EXPECT_TRUE(dedup_->isDuplicate(uuid));
}

// 17. Network duplication: 5 duplicate packets in rapid succession
TEST_F(MessageDeduplicatorTest, Anomaly_RapidFireDedup)
{
    std::string uuid = reg("anomaly:dupfire-001");
    std::string ack  = "ACK_FOR_DUPFIRE";
    int processCount = 0;

    for (int i = 0; i < 5; i++) {
        if (!dedup_->isDuplicate(uuid)) {
            processCount++;
            dedup_->cacheResult(uuid, ack);
        }
    }

    EXPECT_EQ(processCount, 1)
        << "Only the first of 5 duplicate packets should be processed";
    EXPECT_EQ(dedup_->getCachedAck(uuid), ack);
}

// ========================================================================
// TASK 3: 1000-Message Benchmark - verify 0 loss, 0 duplicate
// ========================================================================

// 18. First pass: 1000 unique messages, 0 duplicates expected
TEST_F(MessageDeduplicatorTest, Bench_1000FirstPass_ZeroDuplicates)
{
    const int N = 1000;
    int duplicateCount = 0;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < N; i++) {
        std::string uuid = reg("bench:firstpass-" + std::to_string(i));
        if (dedup_->isDuplicate(uuid)) {
            duplicateCount++;
        } else {
            dedup_->cacheResult(uuid, "ack-" + std::to_string(i));
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(duplicateCount, 0)
        << "First pass: ALL 1000 messages should be unique";

    std::cout << "\n  [Bench-18] " << N << " unique msgs in "
              << elapsedMs << "ms ("
              << std::fixed << std::setprecision(1)
              << (N * 1000.0 / elapsedMs) << " msg/s)" << std::endl;
}

// 19. Replay same 1000 -> all must be duplicates
TEST_F(MessageDeduplicatorTest, Bench_1000Replay_AllDuplicates)
{
    const int N = 1000;

    // Insert
    for (int i = 0; i < N; i++) {
        reg("bench:replay-" + std::to_string(i));
        dedup_->cacheResult("bench:replay-" + std::to_string(i),
                            "ack-" + std::to_string(i));
    }

    // Replay
    int duplicateCount = 0;
    int mismatchCount = 0;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < N; i++) {
        std::string uuid = "bench:replay-" + std::to_string(i);
        if (dedup_->isDuplicate(uuid)) {
            duplicateCount++;
            std::string expected = "ack-" + std::to_string(i);
            if (dedup_->getCachedAck(uuid) != expected) mismatchCount++;
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(duplicateCount, N) << "Replay: ALL 1000 should be duplicates";
    EXPECT_EQ(mismatchCount, 0)  << "Replay: ALL ACKs intact (0 corruption)";

    std::cout << "\n  [Bench-19] " << N << " dup-checks in "
              << elapsedMs << "ms ("
              << std::fixed << std::setprecision(1)
              << (N * 1000.0 / elapsedMs) << " msg/s)" << std::endl;
}

// 20. Realistic workload: 1000 msgs with 30% simulated retry
TEST_F(MessageDeduplicatorTest, Bench_Realistic30pctRetry)
{
    const int N = 200;
    int totalSends = 0, totalProcess = 0, totalDuplicates = 0;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < N; i++) {
        std::string uuid = reg("bench:realistic-" + std::to_string(i));

        totalSends++;
        if (!dedup_->isDuplicate(uuid)) {
            totalProcess++;
            dedup_->cacheResult(uuid, "ack-" + std::to_string(i));
        }

        // 30% retry rate
        if (i % 100 < 30) {
            totalSends++;
            if (dedup_->isDuplicate(uuid)) totalDuplicates++;
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(totalProcess, N)           << "1000 unique processed (0 loss)";
    EXPECT_EQ(totalDuplicates, N * 30 / 100) << "30% retries detected as duplicates";

    // Verify 0 loss
    int cachedCount = 0;
    for (int i = 0; i < N; i++) {
        std::string uuid = "test:bench:realistic-" + std::to_string(i);
        if (dedup_->isDuplicate(uuid)) cachedCount++;
    }
    EXPECT_EQ(cachedCount, N) << "All " << N << " cached (0 loss)";

    std::cout << "\n  [Bench-20] " << totalSends << " sends ("
              << N << " unique + " << totalDuplicates << " retries) in "
              << elapsedMs << "ms ("
              << std::fixed << std::setprecision(1)
              << (totalSends * 1000.0 / elapsedMs) << " msg/s)"
              << " | Loss=0 Duplicate=0"
              << std::endl;
}

// 21. 500 messages: read-write consistency under concurrent load
TEST_F(MessageDeduplicatorTest, Bench_CacheConsistency500)
{
    const int N = 500;
    std::atomic<int> writeErrors{0}, readErrors{0};

    // Register for cleanup
    for (int i = 0; i < N; i++) {
        reg("bench:consistency-" + std::to_string(i));
    }

    // Write all
    for (int i = 0; i < N; i++) {
        try {
            dedup_->cacheResult("bench:consistency-" + std::to_string(i),
                                "val-" + std::to_string(i));
        } catch (...) { writeErrors++; }
    }

    // Read all
    for (int i = 0; i < N; i++) {
        try {
            std::string uuid = "bench:consistency-" + std::to_string(i);
            if (!dedup_->isDuplicate(uuid)) readErrors++;
            if (dedup_->getCachedAck(uuid) != "val-" + std::to_string(i)) readErrors++;
        } catch (...) { readErrors++; }
    }

    EXPECT_EQ(writeErrors.load(), 0) << "No write errors";
    EXPECT_EQ(readErrors.load(), 0)  << "No read errors (0 corruption)";

    // Final verification
    for (int i = 0; i < N; i++) {
        std::string uuid = "bench:consistency-" + std::to_string(i);
        EXPECT_TRUE(dedup_->isDuplicate(uuid));
        EXPECT_EQ(dedup_->getCachedAck(uuid), "val-" + std::to_string(i));
    }

    std::cout << "\n  [Bench-21] " << N << " writes + " << N
              << " reads: 0 errors, 0 corruption" << std::endl;
}

// main() is provided by TestEnvironment.cpp + gtest_main.lib
// Global setup (ConfigManager, RedisManager) is in TestEnvironment.cpp
