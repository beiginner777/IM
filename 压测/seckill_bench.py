#!/usr/bin/env python3
# seckill_bench.py —— 场景 10：秒杀防超卖（两步流程：买 → 支付）
# 用法: python3 seckill_bench.py
# 前置: 商品 stock 重置为 10，清空订单表 + Redis seckill:stock:1 / seckill:paid:*
# 注意: /fe_login 每登录一次会吊销该 uid 旧 token，所以每个 uid 只登录一次、token 共享

import requests, threading

HOST = "http://127.0.0.1:8100"   # Nginx 统一入口（auth 8080 / seckill 8101）
UIDS = [1, 2, 3]                  # 预置的 3 个测试用户
STOCK = 10
ROUNDS = 10
paid_count = 0
lock = threading.Lock()


def login(uid: int) -> str:
    r = requests.post(f"{HOST}/api/login", json={
        "username": f"test{uid}", "password": "123456",
    }, headers={"X-Forwarded-For": "10.0.0.1"}, timeout=10)
    d = r.json()
    assert d.get("error_code") == 0, f"login failed: {d}"
    return d["token"]


# 预登录：每个 uid 一个 token（避免重复登录互相吊销）
tokens = {uid: login(uid) for uid in UIDS}


def worker(idx: int):
    global paid_count
    uid = UIDS[idx % len(UIDS)]
    token = tokens[uid]
    headers = {"Authorization": f"Bearer {token}"}
    s = requests.Session()
    for _ in range(ROUNDS):
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


if __name__ == "__main__":
    threads = [threading.Thread(target=worker, args=(i,)) for i in range(50)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    print(f"Total paid: {paid_count}, Expected: {STOCK}, Oversold: {paid_count > STOCK}")
