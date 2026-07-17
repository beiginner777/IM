#include "BloomFilter.h"
#include "RedisManager.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <cstring>
// 计算位数组大小 m = -n * ln(p) / (ln2)^2
static size_t calcBitSize(size_t n, double p)
{
	double m = -((double)n * std::log(p)) / (std::log(2.0) * std::log(2.0));
	return (size_t)std::ceil(m);
}
// 计算哈希函数个数 k = (m / n) * ln2
static size_t calcHashCount(size_t m, size_t n)
{
	double k = ((double)m / (double)n) * std::log(2.0);
	return (size_t)std::ceil(k);
}
BloomFilter::BloomFilter(size_t n, double p)
	: k_(calcHashCount(calcBitSize(n, p), n))
	, bits_(calcBitSize(n, p))
{
	std::cout << "[BloomFilter] Init: n=" << n << " p=" << p
	          << " -> m=" << bits_.size() << " bits ("
	          << (bits_.size() / 8 / 1024) << "KB) k=" << k_ << std::endl;
}
// 双哈希法：用两个哈希函数生成 k 个"独立"位置
// pos[i] = (h1 + i * h2) % m
std::vector<size_t> BloomFilter::hashPositions(const std::string& key) const
{
	std::vector<size_t> positions;
	positions.reserve(k_);
	std::hash<std::string> hasher;
	size_t h1 = hasher(key);
	size_t h2 = hasher(key + "_salt");
	for (size_t i = 0; i < k_; i++) {
		positions.push_back((h1 + i * h2) % bits_.size());
	}
	return positions;
}
void BloomFilter::add(const std::string& key)
{
	for (size_t pos : hashPositions(key)) {
		bits_.set(pos);
	}
}
void BloomFilter::add(uint64_t uid)
{
	add(std::to_string(uid));
}
// contains: 先查本地内存 → false 则查 Redis BITMAP（防止其他 server 新写入的数据无法感知）
bool BloomFilter::contains(const std::string& key) const
{
	bool localResult = true;
	for (size_t pos : hashPositions(key)) {
		if (!bits_.test(pos)) {
			localResult = false;
			break;
		}
	}
	if (localResult) return true;
	// 本地 miss → 查 Redis BITMAP（GateServer 可能刚写了新用户）
	auto redis = RedisManager::getInstance();
	if (redis) {
		bool redisResult = true;
		for (size_t pos : hashPositions(key)) {
			int val = redis->GetBit("bloom:user_search", pos);
			if (val <= 0) {
				redisResult = false;
				break;
			}
		}
		if (redisResult) {
			// Redis 里有 → 更新本地内存，下次直接命中
			const_cast<BloomFilter*>(this)->add(key);
			return true;
		}
	}
	return false;
}
bool BloomFilter::contains(uint64_t uid) const
{
	return contains(std::to_string(uid));
}
// 持久化到 Redis（一次 SET 批量写入，避免逐位 SETBIT 的 958 万次网络往返）
bool BloomFilter::saveToRedis(const std::string& bitmapKey)
{
	auto redis = RedisManager::getInstance();
	if (!redis) return false;
	// 将 boost::dynamic_bitset 序列化为二进制 block 数组
	std::vector<boost::dynamic_bitset<>::block_type> blocks;
	boost::to_block_range(bits_, std::back_inserter(blocks));
	std::string binary(reinterpret_cast<const char*>(blocks.data()),
	                   blocks.size() * sizeof(boost::dynamic_bitset<>::block_type));
	if (!redis->SetBinary(bitmapKey, binary)) {
		std::cerr << "[BloomFilter] Failed to save to Redis: " << bitmapKey << std::endl;
		return false;
	}
	std::cout << "[BloomFilter] Saved to Redis: " << bitmapKey
	          << " (" << bits_.count() << " bits set of " << bits_.size()
	          << ", " << binary.size() << " bytes)" << std::endl;
	return true;
}
// 从 Redis 恢复（一次 EXISTS + GET 批量读取）
bool BloomFilter::loadFromRedis(const std::string& bitmapKey)
{
	auto redis = RedisManager::getInstance();
	if (!redis) return false;
	// 用 EXISTS 判断 key 是否存在（旧代码 GetBit(0) < 0 永远失效）
	if (!redis->ExistsKey(bitmapKey)) {
		std::cout << "[BloomFilter] No Redis bitmap: " << bitmapKey << std::endl;
		return false;
	}
	std::string binary = redis->GetBinary(bitmapKey);
	if (binary.empty()) {
		std::cout << "[BloomFilter] Redis bitmap is empty: " << bitmapKey << std::endl;
		return false;
	}
	// 反序列化：从二进制 block 数组恢复 bitset
	size_t blockSize = sizeof(boost::dynamic_bitset<>::block_type);
	if (binary.size() % blockSize != 0) {
		std::cerr << "[BloomFilter] Corrupted Redis data: size=" << binary.size() << std::endl;
		return false;
	}
	std::vector<boost::dynamic_bitset<>::block_type> blocks(
		binary.size() / blockSize);
	std::memcpy(blocks.data(), binary.data(), binary.size());
	boost::from_block_range(blocks.begin(), blocks.end(), bits_);
	std::cout << "[BloomFilter] Loaded from Redis: " << bitmapKey
	          << " (" << bits_.count() << " bits set of " << bits_.size()
	          << ", " << binary.size() << " bytes)" << std::endl;
	return true;
}
