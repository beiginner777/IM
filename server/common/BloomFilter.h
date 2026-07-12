#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <string>
#include <vector>
#include <cstdint>

// Bloom filter for user search acceleration.
// Default: n=1,000,000, p=0.01 -> m=9,585,059 bits (~1.2MB), k=7 hashes
//
// Persistence: Redis BITMAP via saveToRedis(bitmapKey) / loadFromRedis(bitmapKey)
//
// Usage:
//   BloomFilter bf(1000000, 0.01);
//   bf.add("user_123");
//   bf.contains("user_123");  // true
//   bf.contains("user_999");  // false (no false negatives)
class BloomFilter
{
public:
	// n = expected element count, p = acceptable false positive rate
	BloomFilter(size_t n = 1000000, double p = 0.01);
	~BloomFilter() = default;

	// Add an element to the filter
	void add(const std::string& key);
	void add(uint64_t uid);

	// Check if element MAY exist (false positives possible, false negatives impossible)
	bool contains(const std::string& key) const;
	bool contains(uint64_t uid) const;

	// Persistence: save/load to Redis BITMAP
	bool saveToRedis(const std::string& bitmapKey);
	bool loadFromRedis(const std::string& bitmapKey);

	// Stats
	size_t bitSize() const { return m_; }
	size_t hashCount() const { return k_; }

private:
	// k hash positions for a given key
	std::vector<size_t> hashPositions(const std::string& key) const;

	size_t m_;  // bit array size
	size_t k_;  // number of hash functions
	std::vector<uint64_t> bits_;  // bit array (packed as uint64_t)
};

#endif
