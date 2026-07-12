#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include <string>
#include <vector>
#include <boost/dynamic_bitset.hpp>

// ============================================================================
// 布隆过滤器（Bloom Filter）—— 用户搜索加速
// ============================================================================
//
// 核心原理：
//   一个"可能存在的集合"。问它"这个元素在吗？"
//     - 回答"不在"  → 100% 确定不在（不会有假阴性）
//     - 回答"可能在" → 有 p 的概率是误判（假阳性），需再查 MySQL 确认
//
// 为什么能加速用户搜索？
//   客户端搜索一个名字 → 先问布隆"这个用户存在吗？"
//     → "不在"  → 直接返回"找不到"，省掉 MySQL 查询（97% 的搜索都找不到人）
//     → "可能在" → 再查 MySQL 确认（只有 3% 的搜索需要查数据库）
//
// 参数计算（自动，无需手算）：
//   n = 预期插入的元素数量（默认 100 万）
//   p = 可接受的误判率（默认 1%）
//   m = 位数组大小  = -n * ln(p) / (ln2)^2 → 约 9,585,059 位 ≈ 1.2 MB
//   k = 哈希函数个数 = (m/n) * ln2      → 约 7 个
//
// 用 boost::dynamic_bitset 存放位数组（动态大小，运行时确定）
//
// ============================================================================

class BloomFilter
{
public:
	// n=预期元素数量, p=可接受误判率（默认 1% 即 0.01）
	BloomFilter(size_t n = 1000000, double p = 0.01);
	~BloomFilter() = default;

	// ---- 写入 ----
	void add(const std::string& key);
	void add(uint64_t uid);

	// ---- 查询（不会漏判，可能误判） ----
	bool contains(const std::string& key) const;
	bool contains(uint64_t uid) const;

	// ---- Redis 持久化（Day 14 实现） ----
	bool saveToRedis(const std::string& bitmapKey);
	bool loadFromRedis(const std::string& bitmapKey);

	// ---- 统计 ----
	size_t bitSize() const    { return bits_.size(); }  // 位数组总大小 m
	size_t hashCount() const  { return k_; }            // 哈希函数个数 k
	size_t count() const      { return bits_.count(); } // 已置 1 的位数

private:
	// 双哈希法：用两个哈希函数生成 k 个独立位置
	// pos[i] = (hash1(key) + i * hash2(key)) % m
	std::vector<size_t> hashPositions(const std::string& key) const;

	size_t k_;                       // 哈希函数个数
	boost::dynamic_bitset<> bits_;   // 位数组（boost::dynamic_bitset 支持运行时动态大小）
};

#endif
