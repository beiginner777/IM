#!/usr/bin/env python3
# create_users.py —— 预置压测用户（ChatServer 登录 / 秒杀登录都要 MySQL 里有对应用户）
# 依赖: pip install pymysql bcrypt
# 用法: python3 create_users.py [数量] [起始uid] [密码]
#   例: python3 create_users.py 200 1000 123456   # 场景 1（tcp_bench 起始 uid 1000）
#       python3 create_users.py 3 1 123456        # 场景 10（seckill 用 uid 1/2/3）
# 说明: 密码用 bcrypt(cost=10) 哈希，与 AuthServer 注册时一致；name 固定为 test{uid}

import sys
import bcrypt
import pymysql

COUNT    = int(sys.argv[1]) if len(sys.argv) > 1 else 200
UID_BASE = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
PASSWORD = sys.argv[3] if len(sys.argv) > 3 else "123456"

conn = pymysql.connect(
    host="127.0.0.1", port=3307, user="root", password="123456",
    database="JerryChat", charset="utf8mb4",
)
cur = conn.cursor()

# 清理旧的压测用户（name=test* 且邮箱为 @bench.com）
cur.execute("DELETE FROM user WHERE name LIKE 'test%' AND email LIKE '%@bench.com'")

# 生成一次 bcrypt 哈希（所有用户同密码，可复用；盐内嵌在哈希里）
hashed = bcrypt.hashpw(PASSWORD.encode(), bcrypt.gensalt(rounds=10)).decode()

for i in range(COUNT):
    uid = UID_BASE + i
    cur.execute(
        "INSERT INTO user(uid, name, email, password, balance, last_login_ip) "
        "VALUES(%s, %s, %s, %s, 0.00, NULL)",
        (uid, f"test{uid}", f"test{uid}@bench.com", hashed),
    )

conn.commit()
cur.close()
conn.close()
print(f"完成：预置 {COUNT} 个用户 (uid {UID_BASE}~{UID_BASE + COUNT - 1}，name=test{{uid}}，密码={PASSWORD})")
