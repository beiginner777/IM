#include "BloomFilter.h"
#include <cmath>
#include <functional>
#include <iostream>

// ============================================================================
// 内部辅助函数：根据 n 和 p 计算最优参数
// ============================================================================

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

// ============================================================================
// 构造函数
// ============================================================================

BloomFilter::BloomFilter(size_t n, double p)
	: k_(calcHashCount(calcBitSize(n, p), n))        // 先算 m，再用 m/n 算 k
	, bits_(calcBitSize(n, p))                        // 用 m 初始化位数组
{
	std::cout << "[BloomFilter] Init: n=" << n << " p=" << p
	          << " -> m=" << bits_.size() << " bits ("
	          << (bits_.size() / 8 / 1024) << "KB) k=" << k_ << std::endl;
}

// ============================================================================
// 双哈希法：用两个 hash 函数生成 k 个"独立"位置
// ============================================================================
//
// 原理：只需要两个基础哈希 h1 和 h2，第 i 个位置为 (h1 + i * h2) % m
//   这 k 个位置在数学上等价于 k 个独立哈希函数的效果
//   比真的写 7 个不同哈希函数更简洁，效果一样
//
// 使用 std::hash<std::string>（C++ 标准库内置），够用且不需要第三方库

std::vector<size_t> BloomFilter::hashPositions(const std::string& key) const
{
	std::vector<size_t> positions;
	positions.reserve(k_);

	std::hash<std::string> hasher;
	size_t h1 = hasher(key);             // 基础哈希 1
	size_t h2 = hasher(key + "_salt");   // 基础哈希 2（加盐避免与 h1 碰撞）

	for (size_t i = 0; i < k_; i++) {
		positions.push_back((h1 + i * h2) % bits_.size());
	}
	return positions;
}

// ============================================================================
// 写入：把 k 个位置全部置 1
// ============================================================================

void BloomFilter::add(const std::string& key)
{
	for (size_t pos : hashPositions(key)) {
		bits_.set(pos);    // dynamic_bitset 的 set(pos)：把第 pos 位置为 1
	}
}

void BloomFilter::add(uint64_t uid)
{
	add(std::to_string(uid));
}

// ============================================================================
// 查询：查 k 个位置是否全为 1
//   - 全是 1 → 元素"可能存在"（其他元素也可能把这些位置置 1，所以有误判）
//   - 有 0    → 元素"一定不存在"（布隆不会漏判，这是核心优势）
// ============================================================================

bool BloomFilter::contains(const std::string& key) const
{
	for (size_t pos : hashPositions(key)) {
		if (!bits_.test(pos)) {    // dynamic_bitset 的 test(pos)：检查第 pos 位是否为 1
			return false;          // 有一个 0 就能确定：这个 key 从来没被 add 过
		}
	}
	return true;                   // 全 1：可能在里面（需要进一步确认）
}

bool BloomFilter::contains(uint64_t uid) const
{
	return contains(std::to_string(uid));
}

// ============================================================================
// Redis BITMAP 持久化（Day 14 实现）
// ============================================================================
//
// 思路：
//   saveToRedis:  遍历 bits_，对每个 1 发 SETBIT bitmapKey pos 1
//   loadFromRedis: 发 GETBIT bitmapKey 0..m-1，逐位恢复到 bits_
// 或者用 BITFIELD 批量操作更快。Day 14 实现。

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
