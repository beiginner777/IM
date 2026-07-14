/**
 * MySQL Batch INSERT vs Single INSERT benchmark
 *
 * Compares QPS and latency between:
 *   - Batch:  INSERT INTO ... VALUES (r1), (r2), ... (rN)  (current implementation)
 *   - Single: N separate INSERT statements (old approach)
 *
 * Prerequisites: MySQL running at 127.0.0.1:3306, database JerryChat with chatmessage table
 */
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include "MysqlDao.h"
#include "MysqlManager.h"
#include "data.h"
#include "ConfigManager.h"
// Benchmark config
static const int  MSG_COUNT    = 1000;  // total messages per run
static const int  BATCH_SIZE   = 100;   // batch INSERT size
static const int  WARMUP_RUNS  = 3;     // warmup iterations
static const int  MEASURE_RUNS = 5;     // measured iterations
struct BenchResult {
    double totalMs;
    double qps;
    double avgLatencyUs;
};
// ==== Batch INSERT: single SQL with N rows (current AddChatMsg) ====
BenchResult benchBatchInsert(int count) {
    std::vector<std::shared_ptr<ChatMessage>> batch;
    batch.reserve(count);
    // Generate test messages
    for (int i = 0; i < count; i++) {
        auto msg = std::make_shared<ChatMessage>();
        msg->message_id = 9000000 + rand() % 9000000; // avoid PK conflict
        msg->thread_id  = 1;
        msg->sender_id  = 1;
        msg->recv_id    = 2;
        msg->content    = "benchmark batch test message number " + std::to_string(i);
        msg->chat_time  = "2026-01-01 00:00:00";
        msg->status     = 2;
        msg->type       = CHAT_MSG_TYPE::TEXT_MSG;
        msg->unique_id  = "bench-batch-" + std::to_string(rand());
        batch.push_back(msg);
    }
    auto start = std::chrono::high_resolution_clock::now();
    // Split into sub-batches of BATCH_SIZE and insert
    size_t totalWritten = 0;
    for (size_t i = 0; i < batch.size(); i += BATCH_SIZE) {
        size_t end = std::min(i + BATCH_SIZE, batch.size());
        std::vector<std::shared_ptr<ChatMessage>> sub(
            batch.begin() + i, batch.begin() + end);
        auto mutableSub = const_cast<std::vector<std::shared_ptr<ChatMessage>>&>(sub);
        MysqlManager::getInstance()->AddChatMsg(mutableSub);
        totalWritten += sub.size();
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    BenchResult r;
    r.totalMs      = ms;
    r.qps          = (count * 1000.0) / ms;
    r.avgLatencyUs = (ms * 1000.0) / count;
    return r;
}
// ==== Single INSERT: one row per SQL (simulates old approach) ====
BenchResult benchSingleInsert(int count) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < count; i++) {
        auto msg = std::make_shared<ChatMessage>();
        msg->message_id = 8000000 + rand() % 9000000; // avoid PK conflict
        msg->thread_id  = 2;
        msg->sender_id  = 1;
        msg->recv_id    = 2;
        msg->content    = "benchmark single test message number " + std::to_string(i);
        msg->chat_time  = "2026-01-01 00:00:00";
        msg->status     = 2;
        msg->type       = CHAT_MSG_TYPE::TEXT_MSG;
        msg->unique_id  = "bench-single-" + std::to_string(rand());
        std::vector<std::shared_ptr<ChatMessage>> single;
        single.push_back(msg);
        auto mutableSingle = const_cast<std::vector<std::shared_ptr<ChatMessage>>&>(single);
        MysqlManager::getInstance()->AddChatMsg(mutableSingle);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    BenchResult r;
    r.totalMs      = ms;
    r.qps          = (count * 1000.0) / ms;
    r.avgLatencyUs = (ms * 1000.0) / count;
    return r;
}
// ==== Warmup: run a few times before measuring to warm caches ====
void warmup(int count) {
    std::cout << "  Warming up (" << WARMUP_RUNS << " runs)..." << std::endl;
    for (int i = 0; i < WARMUP_RUNS; i++) {
        benchBatchInsert(count / 10);
        benchSingleInsert(count / 10);
    }
    std::cout << "  Warmup done." << std::endl;
}
// ==== Main ====
int main() {
    srand((unsigned)time(nullptr));
    try {
        ConfigManager::getInstance();
        std::cout << "\n================================================================" << std::endl;
        std::cout << "  MySQL Batch vs Single INSERT Benchmark" << std::endl;
        std::cout << "  Messages: " << MSG_COUNT << " | Batch size: " << BATCH_SIZE
                  << " | Runs: " << MEASURE_RUNS << std::endl;
        std::cout << "================================================================\n" << std::endl;
        warmup(MSG_COUNT);
        // ==== Batch benchmark ====
        std::cout << "--- Batch INSERT (" << MSG_COUNT << " msgs, "
                  << (MSG_COUNT / BATCH_SIZE) << " statements) ---" << std::endl;
        double batchTotalMs = 0, batchTotalQps = 0, batchTotalLat = 0;
        for (int run = 1; run <= MEASURE_RUNS; run++) {
            auto r = benchBatchInsert(MSG_COUNT);
            batchTotalMs  += r.totalMs;
            batchTotalQps += r.qps;
            batchTotalLat += r.avgLatencyUs;
            std::cout << "  Run " << run << ": " << std::fixed << std::setprecision(1)
                      << r.totalMs << "ms | QPS=" << std::setprecision(0) << r.qps
                      << " | avg=" << std::setprecision(1) << r.avgLatencyUs << "us/msg" << std::endl;
        }
        double batchAvgMs  = batchTotalMs  / MEASURE_RUNS;
        double batchAvgQps = batchTotalQps / MEASURE_RUNS;
        double batchAvgLat = batchTotalLat / MEASURE_RUNS;
        // ==== Single INSERT benchmark ====
        std::cout << "\n--- Single INSERT (" << MSG_COUNT << " msgs, "
                  << MSG_COUNT << " statements) ---" << std::endl;
        double singleTotalMs = 0, singleTotalQps = 0, singleTotalLat = 0;
        for (int run = 1; run <= MEASURE_RUNS; run++) {
            auto r = benchSingleInsert(MSG_COUNT);
            singleTotalMs  += r.totalMs;
            singleTotalQps += r.qps;
            singleTotalLat += r.avgLatencyUs;
            std::cout << "  Run " << run << ": " << std::fixed << std::setprecision(1)
                      << r.totalMs << "ms | QPS=" << std::setprecision(0) << r.qps
                      << " | avg=" << std::setprecision(1) << r.avgLatencyUs << "us/msg" << std::endl;
        }
        double singleAvgMs  = singleTotalMs  / MEASURE_RUNS;
        double singleAvgQps = singleTotalQps / MEASURE_RUNS;
        double singleAvgLat = singleTotalLat / MEASURE_RUNS;
        // ==== Summary ====
        double speedup    = batchAvgQps / singleAvgQps;
        double latencyRatio = singleAvgLat / batchAvgLat;
        std::cout << "\n================================================================\n";
        std::cout << "  RESULTS (average of " << MEASURE_RUNS << " runs)\n";
        std::cout << "----------------------------------------------------------------\n";
        std::cout << std::left << std::setw(20) << "  Metric"
                  << std::right << std::setw(15) << "Batch"
                  << std::setw(15) << "Single"
                  << std::setw(15) << "Improvement" << std::endl;
        std::cout << "  " << std::string(60, '-') << std::endl;
        std::cout << std::left << std::setw(20) << "  Total time"
                  << std::right << std::setw(12) << std::fixed << std::setprecision(0) << batchAvgMs << "ms"
                  << std::setw(12) << std::setprecision(0) << singleAvgMs << "ms"
                  << std::setw(12) << std::setprecision(1) << speedup << "x faster" << std::endl;
        std::cout << std::left << std::setw(20) << "  Throughput"
                  << std::right << std::setw(12) << std::setprecision(0) << batchAvgQps << "/s"
                  << std::setw(12) << std::setprecision(0) << singleAvgQps << "/s"
                  << std::setw(12) << std::setprecision(1) << speedup << "x" << std::endl;
        std::cout << std::left << std::setw(20) << "  Avg latency"
                  << std::right << std::setw(11) << std::setprecision(0) << batchAvgLat << "us"
                  << std::setw(11) << std::setprecision(0) << singleAvgLat << "us"
                  << std::setw(12) << std::setprecision(1) << latencyRatio << "x lower" << std::endl;
        std::cout << "================================================================\n";
        std::cout << "\n  Speedup: " << std::setprecision(1) << speedup
                  << "x | Latency reduction: " << latencyRatio << "x" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << std::endl;
        return 1;
    }
}
