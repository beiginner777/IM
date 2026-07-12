#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <string>
#include <vector>
#include <boost/dynamic_bitset.hpp>

// Bloom filter for user search acceleration.
// Default: n=1,000,000, p=0.01 -> m=9,585,059 bits (~1.2MB), k=7 hashes
class BloomFilter
{
public:
	// n = expected element count, p = acceptable false positive rate
	BloomFilter(size_t n = 1000000, double p = 0.01);
	~BloomFilter() = default;

	void add(const std::string& key);
	void add(uint64_t uid);

	// Check if element MAY exist (false positives possible, false negatives impossible)
	bool contains(const std::string& key) const;
	bool contains(uint64_t uid) const;

	// Persistence: save/load to Redis BITMAP (Day 14)
	bool saveToRedis(const std::string& bitmapKey);
	bool loadFromRedis(const std::string& bitmapKey);

	size_t bitSize() const    { return bits_.size(); }
	size_t hashCount() const  { return k_; }
	size_t count() const      { return bits_.count(); }

private:
	std::vector<size_t> hashPositions(const std::string& key) const;

	size_t k_;  // number of hash functions
	boost::dynamic_bitset<> bits_;
};

#endif
