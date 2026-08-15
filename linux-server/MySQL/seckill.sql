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
