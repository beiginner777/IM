-- ============================================================
-- 水平分表迁移脚本：chatmessage → chatmessage_0~3
--
-- 路由规则：thread_id % 4 → 分片索引
-- 复合主键：(thread_id, message_id)
--   - 同一 thread 的消息磁盘连续存储，顺序写入
--   - loadChatMessage 走聚簇索引范围扫描，无需回表
--   - message_id 由 Redis INCR 全局生成
--
-- 执行方式：
--   mysql -u root -p JerryChat < migrate_shard.sql
-- ============================================================

-- 1. 创建 4 张分片表（结构与 chatmessage 相同，复合主键不同）
CREATE TABLE IF NOT EXISTS `chatmessage_0` (
  `message_id` bigint unsigned NOT NULL,
  `thread_id` bigint unsigned NOT NULL,
  `sender_id` bigint unsigned NOT NULL,
  `recv_id` bigint unsigned NOT NULL,
  `content` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `status` tinyint NOT NULL DEFAULT '0' COMMENT '0=未读 1=已读 2=撤回',
  `message_type` tinyint NOT NULL DEFAULT '0' COMMENT '消息类型: 0-文本 1-图片 2-表情 3-文件',
  `client_msg_id` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`thread_id`, `message_id`),
  UNIQUE KEY `idx_message_id` (`message_id`),
  UNIQUE KEY `idx_client_msg_id` (`client_msg_id`),
  KEY `idx_thread_created` (`thread_id`, `created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `chatmessage_1` LIKE `chatmessage_0`;
CREATE TABLE IF NOT EXISTS `chatmessage_2` LIKE `chatmessage_0`;
CREATE TABLE IF NOT EXISTS `chatmessage_3` LIKE `chatmessage_0`;

-- 2. 存量数据迁移（可选，上线时执行一次）
--    将 chatmessage 中已有的数据按 thread_id % 4 分到各分片表
--    注意：message_id 在分片表中不再是 AUTO_INCREMENT，直接使用原值
--
-- INSERT INTO chatmessage_0
--   (message_id, thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type, client_msg_id)
-- SELECT message_id, thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type, client_msg_id
-- FROM chatmessage WHERE thread_id % 4 = 0;
--
-- INSERT INTO chatmessage_1
--   (message_id, thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type, client_msg_id)
-- SELECT message_id, thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type, client_msg_id
-- FROM chatmessage WHERE thread_id % 4 = 1;
--
-- INSERT INTO chatmessage_2
--   (message_id, thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type, client_msg_id)
-- SELECT message_id, thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type, client_msg_id
-- FROM chatmessage WHERE thread_id % 4 = 2;
--
-- INSERT INTO chatmessage_3
--   (message_id, thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type, client_msg_id)
-- SELECT message_id, thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type, client_msg_id
-- FROM chatmessage WHERE thread_id % 4 = 3;

-- 3. 验证数据完整性（迁移后执行）
-- SELECT 'chatmessage' AS tbl, COUNT(*) FROM chatmessage
-- UNION ALL
-- SELECT 'shard_0', COUNT(*) FROM chatmessage_0
-- UNION ALL
-- SELECT 'shard_1', COUNT(*) FROM chatmessage_1
-- UNION ALL
-- SELECT 'shard_2', COUNT(*) FROM chatmessage_2
-- UNION ALL
-- SELECT 'shard_3', COUNT(*) FROM chatmessage_3;
-- -- 期望：chatmessage 的总数 = 四个分片表总数之和
