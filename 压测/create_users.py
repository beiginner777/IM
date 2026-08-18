#!/usr/bin/env python3
# create_users.py —— 预置压测用户（用户名 test{uid}，密码统一）
#
# 用法:
#   python3 create_users.py <数量> <起始uid> <密码>
#
# 例:
#   python3 create_users.py 200 1000 123456    # uid 1000~1199（tcp_bench 场景 1）
#   python3 create_users.py 3 1 123456         # uid 1/2/3（seckill_bench 场景 10 默认）
#   python3 create_users.py 1000 2001 123456   # uid 2001~3000（秒杀 1000 并发）
#
# 前置:
#   pip install pymysql bcrypt
#
# 说明:
#   - 用户名 = test{uid}，email = test{uid}@im.local，密码 bcrypt(cost=10)
#   - INSERT IGNORE 幂等，可重复执行
#   - 直连宿主机 127.0.0.1:3307（docker-compose 里 mysql-master 的映射端口）
#   - ⚠️ 建议「先建用户，再启动整套服务」：AuthServer 启动时会把 MySQL 用户名
#     构建成布隆过滤器，后补的用户不在布隆里会被登录误判「用户不存在」

import sys
import pymysql
import bcrypt

HOST = "127.0.0.1"
PORT = 3307          # mysql-master 宿主机映射端口
USER = "root"
PASSWORD = "123456"
DB = "JerryChat"


def main():
    if len(sys.argv) < 4:
        print("usage: python3 create_users.py <count> <uid_base> <password>")
        sys.exit(1)

    count = int(sys.argv[1])
    uid_base = int(sys.argv[2])
    password = sys.argv[3]

    conn = pymysql.connect(
        host=HOST, port=PORT, user=USER, password=PASSWORD,
        database=DB, charset='utf8mb4')
    try:
        cur = conn.cursor()
        inserted = 0
        skipped = 0
        for i in range(count):
            uid = uid_base + i
            name = f"test{uid}"
            email = f"test{uid}@im.local"
            # 每个用户独立 salt 的 bcrypt 哈希
            pwd_hash = bcrypt.hashpw(
                password.encode(), bcrypt.gensalt(rounds=10, prefix=b'2b')).decode()
            cur.execute(
                "INSERT IGNORE INTO user (uid, name, email, password) "
                "VALUES (%s, %s, %s, %s)",
                (uid, name, email, pwd_hash))
            if cur.rowcount > 0:
                inserted += 1
            else:
                skipped += 1
        conn.commit()
        print(f"完成：新增 {inserted} 个，已存在跳过 {skipped} 个")
        print(f"范围：uid {uid_base}~{uid_base + count - 1}，name test{uid_base}~test{uid_base + count - 1}")
    finally:
        conn.close()


if __name__ == "__main__":
    main()
