#include "BloomFilter.h"
#include <cmath>
#include <functional>
#include <iostream>

// m = -n * ln(p) / (ln(2)^2)
static size_t calcBitSize(size_t n, double p)
{
	double m = -((double)n * std::log(p)) / (std::log(2.0) * std::log(2.0));
	return (size_t)std::ceil(m);
}

// k = (m / n) * ln(2)
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

// Double hashing: h(i, key) = (h1(key) + i * h2(key)) % m
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

bool BloomFilter::contains(const std::string& key) const
{
	for (size_t pos : hashPositions(key)) {
		if (!bits_.test(pos)) {
			return false;
		}
	}
	return true;
}

bool BloomFilter::contains(uint64_t uid) const
{
	return contains(std::to_string(uid));
}

// TODO: Day 14 — Redis BITMAP persistence
bool BloomFilter::saveToRedis(const std::string& bitmapKey)
{
	(void)bitmapKey;
	return false;
}

bool BloomFilter::loadFromRedis(const std::string& bitmapKey)
{
	(void)bitmapKey;
	return false;
}
