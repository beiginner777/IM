#include "BloomFilter.h"
#include "RedisManager.h"
#include <cmath>
#include <functional>
#include <iostream>

// Calculate optimal bit array size: m = -n * ln(p) / (ln(2)^2)
static size_t calcBitSize(size_t n, double p)
{
	double m = -((double)n * std::log(p)) / (std::log(2.0) * std::log(2.0));
	return (size_t)std::ceil(m);
}

// Calculate optimal hash count: k = (m / n) * ln(2)
static size_t calcHashCount(size_t m, size_t n)
{
	double k = ((double)m / (double)n) * std::log(2.0);
	return (size_t)std::ceil(k);
}

BloomFilter::BloomFilter(size_t n, double p)
	: m_(calcBitSize(n, p))
	, k_(calcHashCount(m_, n))
{
	// Allocate bits_ as vector of uint64_t (each holding 64 bits)
	size_t words = (m_ + 63) / 64;
	bits_.resize(words, 0);

	std::cout << "[BloomFilter] Init: n=" << n << " p=" << p
	          << " -> m=" << m_ << " bits (" << (m_ / 8 / 1024) << "KB)"
	          << " k=" << k_ << std::endl;
}

// Double hashing: h(i, key) = (h1(key) + i * h2(key)) % m
// where h1/h2 are std::hash specializations
std::vector<size_t> BloomFilter::hashPositions(const std::string& key) const
{
	std::vector<size_t> positions;
	positions.reserve(k_);

	std::hash<std::string> hasher;
	size_t h1 = hasher(key);
	size_t h2 = hasher(key + "_salt");  // different seed for second hash

	for (size_t i = 0; i < k_; i++) {
		size_t pos = (h1 + i * h2) % m_;
		positions.push_back(pos);
	}
	return positions;
}

void BloomFilter::add(const std::string& key)
{
	for (size_t pos : hashPositions(key)) {
		size_t word = pos / 64;
		size_t bit  = pos % 64;
		bits_[word] |= (1ULL << bit);
	}
}

void BloomFilter::add(uint64_t uid)
{
	add(std::to_string(uid));
}

bool BloomFilter::contains(const std::string& key) const
{
	for (size_t pos : hashPositions(key)) {
		size_t word = pos / 64;
		size_t bit  = pos % 64;
		if (!(bits_[word] & (1ULL << bit))) {
			return false;  // definitely not present
		}
	}
	return true;  // might be present
}

bool BloomFilter::contains(uint64_t uid) const
{
	return contains(std::to_string(uid));
}

bool BloomFilter::saveToRedis(const std::string& bitmapKey)
{
	// TODO: 使用 Redis SETBIT 逐位写入（Day 14 实现）
	(void)bitmapKey;
	return false;
}

bool BloomFilter::loadFromRedis(const std::string& bitmapKey)
{
	// TODO: 使用 Redis GETBIT 逐位恢复（Day 14 实现）
	(void)bitmapKey;
	return false;
}
