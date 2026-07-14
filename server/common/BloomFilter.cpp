#include "BloomFilter.h"
#include "RedisManager.h"
#include <cmath>
#include <functional>
#include <iostream>
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
// 持久化到 Redis BITMAP
bool BloomFilter::saveToRedis(const std::string& bitmapKey)
{
	auto redis = RedisManager::getInstance();
	if (!redis) return false;
	size_t written = 0;
	for (size_t pos = 0; pos < bits_.size(); pos++) {
		if (bits_.test(pos)) {
			redis->SetBit(bitmapKey, pos, 1);
			written++;
		}
	}
	std::cout << "[BloomFilter] Saved to Redis: " << bitmapKey
	          << " (" << written << " bits set of " << bits_.size() << ")" << std::endl;
	return true;
}
// 从 Redis BITMAP 恢复
bool BloomFilter::loadFromRedis(const std::string& bitmapKey)
{
	auto redis = RedisManager::getInstance();
	if (!redis) return false;
	if (redis->GetBit(bitmapKey, 0) < 0) {
		std::cout << "[BloomFilter] No Redis bitmap: " << bitmapKey << std::endl;
		return false;
	}
	size_t loaded = 0;
	for (size_t pos = 0; pos < bits_.size(); pos++) {
		if (redis->GetBit(bitmapKey, pos) > 0) {
			bits_.set(pos);
			loaded++;
		}
	}
	std::cout << "[BloomFilter] Loaded from Redis: " << bitmapKey
	          << " (" << loaded << " bits set of " << bits_.size() << ")" << std::endl;
	return true;
}
