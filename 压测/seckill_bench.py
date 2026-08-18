#!/usr/bin/env python3
# seckill_bench.py —— 场景 10：秒杀防超卖（两步流程：买 → 支付）
# 用法:
#   python3 seckill_bench.py                                          # 默认：50 线程 / 库存 10 / 3 用户
#   python3 seckill_bench.py --threads 1000 --stock 1000 \
#       --uid-base 2001 --uid-count 1000                              # 1000 并发 0 超卖
# 前置:
#   - 测试用户名格式 test{uid}（name 字段），密码 123456，需先插入 user 表
#   - 商品库存重置为 --stock，清空订单表 + Redis seckill:stock:1 / seckill:paid:*
# 注意:
#   - 秒杀有单用户限流(5/s)，高并发需足够多 uid（--uid-count >= --threads 数量级）

import requests, threading, argparse

HOST = "http://127.0.0.1:8100"   # Nginx 统一入口（auth 8080 / seckill 8101）


def login(uid: int) -> str:
    # 走 /fe_login（Nginx 有该路由 → authserver）；/api/login 在 Nginx 无路由会被转到 seckill 导致 not found
    r = requests.post(f"{HOST}/fe_login", json={
        "username": f"test{uid}", "password": "123456",
    }, headers={"X-Forwarded-For": "10.0.0.1"}, timeout=10)
    d = r.json()
    assert d.get("error_code") == 0, f"login failed for test{uid}: {d}"
    return d["token"]


def main():
    ap = argparse.ArgumentParser(description="秒杀防超卖压测（买 → 支付两步流程）")
    ap.add_argument("--threads", type=int, default=50, help="并发线程数")
    ap.add_argument("--stock", type=int, default=10, help="商品库存（Expected）")
    ap.add_argument("--uid-base", type=int, default=1, help="测试用户名起始 test{uid-base}")
    ap.add_argument("--uid-count", type=int, default=3, help="测试用户数量")
    ap.add_argument("--rounds", type=int, default=10, help="每线程抢购轮数")
    args = ap.parse_args()

    uids = list(range(args.uid_base, args.uid_base + args.uid_count))
    print(f"登录 {len(uids)} 个用户 (test{args.uid_base}~test{uids[-1]})...")

    # 预登录：每个 uid 一个 token（fe_login 每次登录会吊销同 uid 旧 token，故只登录一次共享）
    tokens = {uid: login(uid) for uid in uids}

    paid_count = 0
    lock = threading.Lock()

    def worker(idx: int):
        nonlocal paid_count
        uid = uids[idx % len(uids)]
        token = tokens[uid]
        headers = {"Authorization": f"Bearer {token}"}
        s = requests.Session()
        for _ in range(args.rounds):
            # 1. 买 → 拿 orderId
            r = s.post(f"{HOST}/buy/1", headers=headers, timeout=5)
            if not r.json().get("success"):
                continue   # 售罄 / 限流
            oid = r.json()["orderId"]
            # 2. 支付
            r = s.post(f"{HOST}/order/{oid}/pay", json={"password": "123456"},
                       headers=headers, timeout=5)
            if r.json().get("success"):
                with lock:
                    paid_count += 1

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(args.threads)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    print(f"Total paid: {paid_count}, Expected: {args.stock}, Oversold: {paid_count > args.stock}")


if __name__ == "__main__":
    main()
