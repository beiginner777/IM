-- MySQL dump 10.13  Distrib 8.0.29, for Win64 (x86_64)
--
-- Host: localhost    Database: JerryChat
-- ------------------------------------------------------
-- Server version	8.0.29

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `chatmessage`
--

DROP TABLE IF EXISTS `chatmessage`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `chatmessage` (
  `message_id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `thread_id` bigint unsigned NOT NULL,
  `sender_id` bigint unsigned NOT NULL,
  `recv_id` bigint unsigned NOT NULL,
  `content` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `status` tinyint NOT NULL DEFAULT '0' COMMENT '0=未读 1=已读 2=撤回',
  `message_type` tinyint NOT NULL DEFAULT '0' COMMENT '消息类型: 0-文本 1-图片 2-表情 3-文件',
  `client_msg_id` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`message_id`),
  UNIQUE KEY `idx_client_msg_id` (`client_msg_id`),
  KEY `idx_thread_created` (`thread_id`,`created_at`),
  KEY `idx_thread_message` (`thread_id`,`message_id`)
) ENGINE=InnoDB AUTO_INCREMENT=9032718 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `chatmessage`
--


--
-- Table structure for table `chatthread`
--

DROP TABLE IF EXISTS `chatthread`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `chatthread` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `type` enum('private','group') NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=61 DEFAULT CHARSET=utf8mb3;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `chatthread`
--


--
-- Table structure for table `friend`
--

DROP TABLE IF EXISTS `friend`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `friend` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `self_id` int NOT NULL,
  `friend_id` int NOT NULL,
  `back` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `self_friend` (`self_id`,`friend_id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=74 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `friend`
--


--
-- Table structure for table `friendapply`
--

DROP TABLE IF EXISTS `friendapply`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `friendapply` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT COMMENT '主键ID',
  `fromuid` bigint unsigned NOT NULL COMMENT '发起申请的用户ID',
  `touid` bigint unsigned NOT NULL COMMENT '接收申请的用户ID',
  `status` tinyint NOT NULL DEFAULT '0' COMMENT '申请状态：0-待处理，1-已同意，2-已拒绝',
  `apply_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '申请时间',
  `update_time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `uk_fromuid_touid` (`fromuid`,`touid`) USING BTREE COMMENT '同一对用户只能有一条未处理的申请',
  KEY `idx_touid` (`touid`) USING BTREE COMMENT '接收方查询索引',
  KEY `idx_status` (`status`) USING BTREE COMMENT '状态查询索引',
  KEY `idx_apply_time` (`apply_time`) USING BTREE COMMENT '申请时间索引'
) ENGINE=InnoDB AUTO_INCREMENT=34 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci COMMENT='好友申请表';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `friendapply`
--


--
-- Table structure for table `groupchat`
--

DROP TABLE IF EXISTS `groupchat`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `groupchat` (
  `thread_id` bigint unsigned NOT NULL COMMENT '引用chat_thread.id',
  `name` varchar(255) DEFAULT NULL COMMENT '群聊名称',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `groupchat`
--


--
-- Table structure for table `groupchatmember`
--

DROP TABLE IF EXISTS `groupchatmember`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `groupchatmember` (
  `thread_id` bigint unsigned NOT NULL COMMENT '引用 group_chat_thread.thread_id',
  `user_id` bigint unsigned NOT NULL COMMENT '引用 user.user_id',
  `role` tinyint NOT NULL DEFAULT '0' COMMENT '0=普通成员,1=管理员,2=创建者',
  `joined_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `muted_until` timestamp NULL DEFAULT NULL COMMENT '如果被禁言，可存到什么时候',
  PRIMARY KEY (`thread_id`,`user_id`),
  KEY `idx_user_threads` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `groupchatmember`
--


--
-- Table structure for table `privatechat`
--

DROP TABLE IF EXISTS `privatechat`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `privatechat` (
  `thread_id` bigint unsigned NOT NULL COMMENT '引用chat_thread.id',
  `user1_id` bigint unsigned NOT NULL,
  `user2_id` bigint unsigned NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`),
  UNIQUE KEY `uniq_private_thread` (`user1_id`,`user2_id`),
  KEY `idx_private_user1_thread` (`user1_id`,`thread_id`),
  KEY `idx_private_user2_thread` (`user2_id`,`thread_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `privatechat`
--


--
-- Table structure for table `user`
--

DROP TABLE IF EXISTS `user`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `user` (
  `id` int NOT NULL AUTO_INCREMENT,
  `uid` int NOT NULL,
  `name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `email` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `password` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `desc` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT 'Hello,world!' COMMENT '用户描述',
  `icon` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT 'jerry.jpg' COMMENT '用户头像',
  `sex` int DEFAULT '0' COMMENT '性别：0-未知 1-男 2-女',
  `nick` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT 'hahaha' COMMENT '用户昵称',
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `email` (`email`),
  UNIQUE KEY `uid` (`uid`),
  KEY `idx_uid` (`uid`),
  KEY `idx_name` (`name`),
  KEY `idx_nick` (`nick`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `user`
--


--
-- Table structure for table `userid`
--

DROP TABLE IF EXISTS `userid`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `userid` (
  `id` int DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `userid`
--


--
-- Dumping routines for database 'JerryChat'
--
/*!50003 DROP PROCEDURE IF EXISTS `reg_user` */;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8mb4 */ ;
/*!50003 SET character_set_results = utf8mb4 */ ;
/*!50003 SET collation_connection  = utf8mb4_0900_ai_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
CREATE DEFINER=`hahaha`@`%` PROCEDURE `reg_user`(

    IN `new_name` VARCHAR(255), 

    IN `new_email` VARCHAR(255), 

    IN `new_pwd` VARCHAR(255), 

    OUT `result` INT

)
BEGIN

    DECLARE new_id INT;

    DECLARE name_exists INT DEFAULT 0;

    DECLARE email_exists INT DEFAULT 0;

    

    -- 错误处理

    DECLARE EXIT HANDLER FOR SQLEXCEPTION

    BEGIN

        ROLLBACK;

        SET result = -1;  -- 表示系统错误

    END;



    -- 开始事务

    START TRANSACTION;



    -- 检查用户名是否已存在

    SELECT COUNT(*) INTO name_exists FROM `user` WHERE `name` = new_name;

    

    -- 检查邮箱是否已存在  

    SELECT COUNT(*) INTO email_exists FROM `user` WHERE `email` = new_email;



    IF name_exists > 0 THEN

        SET result = -1020;  -- 用户名已存在

        ROLLBACK;

    ELSEIF email_exists > 0 THEN

        SET result = -1019;  -- 邮箱已存在

        ROLLBACK;

    ELSE

        -- 更新user_id表获取新ID

        UPDATE `userid` SET `id` = `id` + 1;

        

        -- 获取更新后的id

        SELECT `id` INTO new_id FROM `userid`;

        

        -- 插入新用户

        INSERT INTO `user` (`uid`, `name`, `email`, `password`) 

        VALUES (new_id, new_name, new_email, new_pwd);

        

        SET result = new_id;  -- 成功，返回新用户ID

        COMMIT;

    END IF;



END ;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;


-- ============ 秒杀系统表结构 ============
-- 秒杀系统表结构 + 初始数据
-- MySQL 容器首次初始化时，在 dump.sql 之后执行（字母序 s > d）

-- user 表加余额字段（充值/支付用）
ALTER TABLE user ADD COLUMN balance DECIMAL(10,2) NOT NULL DEFAULT 0.00;

-- user 表加异地登录 IP 字段
ALTER TABLE user ADD COLUMN last_login_ip VARCHAR(45) DEFAULT NULL;

-- 秒杀商品表
CREATE TABLE seckill_product (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(255) NOT NULL,
    price DECIMAL(10,2) NOT NULL,
    stock INT NOT NULL DEFAULT 0,
    image_url VARCHAR(255) DEFAULT ''
);

-- 秒杀订单表
CREATE TABLE seckill_order (
    id INT PRIMARY KEY AUTO_INCREMENT,
    uid INT NOT NULL,
    product_id INT NOT NULL,
    product_name VARCHAR(255) NOT NULL,
    price DECIMAL(10,2) NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'unpaid',
    recipient VARCHAR(255) DEFAULT '',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    cancelled_at DATETIME DEFAULT NULL
);

-- 初始商品数据
INSERT INTO seckill_product (name, price, stock, image_url) VALUES
('iPhone 15 Pro', 7999.00, 100, ''),
('MacBook Pro 14', 14999.00, 50, ''),
('AirPods Pro 2', 1899.00, 200, ''),
('iPad Air', 4399.00, 80, '');

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-07-13  0:34:23
