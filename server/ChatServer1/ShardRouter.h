#ifndef SHARDROUTER_H
#define SHARDROUTER_H

#include <string>
/// 水平分表路由：thread_id 取模 → 分片 0~3
///
/// 路由规则：shard_index = thread_id % SHARD_COUNT
/// 表名规则：chatmessage_0, chatmessage_1, chatmessage_2, chatmessage_3
///
/// 复合主键设计（每张分片表）：
///   PRIMARY KEY (thread_id, message_id)
///   - 同一 thread 的消息在磁盘上连续存储，顺序写入
///   - loadChatMessage 直接走聚簇索引范围扫描，无需回表
///   - message_id 由 Redis INCR 全局生成，UNIQUE KEY 保证唯一性
///
class ShardRouter
{
public:
    static constexpr int SHARD_COUNT = 4;
    /// thread_id → 分片索引 0~3
    static int getShardIndex(int thread_id)
    {
        return thread_id % SHARD_COUNT;
    }
    /// 分片索引 → 表名
    static std::string getTableName(int shardIndex)
    {
        return "chatmessage_" + std::to_string(shardIndex);
    }
    /// thread_id → 表名（便捷方法）
    static std::string getTableNameByThread(int thread_id)
    {
        return getTableName(getShardIndex(thread_id));
    }
};
#endif
