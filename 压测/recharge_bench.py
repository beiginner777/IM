#!/usr/bin/env python3
# recharge_bench.py —— 场景 6：充值接口三层限流验证
#
# 三层限流（SeckillServer LogicSystem::tryAcquireRateLimit）：
#   第 1 层 全局令牌桶   3000 token/s（容量 5000）→ 超限返回 "server busy"
#   第 2 层 单用户令牌桶 5 token/s（容量 10）      → 超限返回 "发送过于频繁"
#   第 3 层 Redis 限流   100/s/uid                → 超限无回包（客户端超时）
#
# 关键事实（决定验证范围）：
#   - 第 2 层(5/s) 远紧于第 3 层(100/s)：单机单 uid 下第 2 层先触发，第 3 层到不了
#     （除非移除单用户桶或多机部署，让单 uid 总速率突破 100/s）。
#   - 第 1 层(3000/s)：需要总 QPS > 3000，而每 uid 仅 5/s，需 ≥600 个 uid 才能触发，
#     测试环境 uid 有限，一般测不到。
#   => 单机环境本脚本能稳定验证的是「第 2 层单用户限流 5/s」；第 1/3 层仅在海量
#      并发 / 特定部署下才会触发，脚本会据实统计。
#
# 用法:
#   python3 recharge_bench.py [并发线程数] [每线程请求数] [用户名1,用户名2,...]
# 例:
#   python3 recharge_bench.py 20 30 test1,test2,test3
#   python3 recharge_bench.py 50 20 jerry,Bob,David
#
# 前置:
#   - 用户名需在 MySQL user 表存在，密码默认 123456
#     （dump.sql 预置 jerry/Bob/David；若部署库是 test1/test2/test3 则改参数）
#   - 直连 seckillserver(8101) 绕过 Nginx 的 limit_req(10r/s)，专测服务端三层限流

import sys, time, threading, requests
from collections import Counter, defaultdict

AUTH_HOST   = "http://127.0.0.1:8080"   # authserver：登录拿 token（/fe_login）
SECILL_HOST = "http://127.0.0.1:8101"   # seckillserver：直连，绕过 Nginx 限流
PASSWORD    = "123456"

# 响应分类
SUCCESS        = "充值成功"
USER_LIMITED   = "单用户限流(发送过于频繁)"
GLOBAL_LIMITED = "全局限流(server busy)"
TIMEOUT        = "超时(疑似Redis层限流)"
OTHER          = "其他错误"


def login(username: str) -> str:
    # /fe_login 依赖 X-Forwarded-For 做异地登录检测，缺了会拒绝登录
    r = requests.post(f"{AUTH_HOST}/fe_login",
                      json={"username": username, "password": PASSWORD},
                      headers={"X-Forwarded-For": "10.0.0.1"}, timeout=10)
    d = r.json()
    if d.get("error_code") != 0:
        raise RuntimeError(f"登录失败 {username}: {d}")
    return d["token"]


def classify(resp) -> str:
    try:
        d = resp.json()
    except Exception:
        return OTHER
    # 限流响应走 sendAuthError：{"success":false,"message":"..."}
    if d.get("success") is False:
        msg = d.get("message", "")
        if msg == "server busy":
            return GLOBAL_LIMITED
        if msg == "发送过于频繁":
            return USER_LIMITED
        return OTHER
    # 业务响应：{"code":0,...} 成功；{"code":-1,...} 失败
    if d.get("code") == 0:
        return SUCCESS
    return OTHER


def main():
    threads_n = int(sys.argv[1]) if len(sys.argv) > 1 else 20
    req_per_t = int(sys.argv[2]) if len(sys.argv) > 2 else 30
    users = sys.argv[3].split(",") if len(sys.argv) > 3 else ["test1", "test2", "test3"]

    # 预登录：每 uid 只登录一次（fe_login 每次登录会吊销同 uid 旧 token）
    tokens = {}
    for u in users:
        tokens[u] = login(u)
    print(f"登录成功 {len(tokens)} 个用户: {', '.join(tokens)}")

    stats = Counter()
    per_user = defaultdict(Counter)
    lock = threading.Lock()
    start = time.time()

    def worker(idx):
        u = users[idx % len(users)]
        headers = {"Authorization": f"Bearer {tokens[u]}"}
        s = requests.Session()
        for _ in range(req_per_t):
            try:
                r = s.post(f"{SECILL_HOST}/recharge",
                           json={"amount": 1.0, "password": PASSWORD},
                           headers=headers, timeout=5)
                c = classify(r)
            except requests.exceptions.ReadTimeout:
                c = TIMEOUT
            except Exception:
                c = OTHER
            with lock:
                stats[c] += 1
                per_user[u][c] += 1

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(threads_n)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    elapsed = time.time() - start
    total = sum(stats.values())

    print("\n================ 三层限流验证结果 ================")
    print(f"并发线程: {threads_n} | 每线程请求: {req_per_t} | 用户数: {len(users)}")
    print(f"总请求: {total} | 耗时: {elapsed:.2f}s | 平均 QPS: {total / elapsed:.1f}")
    print(f"\n{'分类':<30}{'数量':>10}{'占比':>10}")
    print("-" * 52)
    for c in [SUCCESS, USER_LIMITED, GLOBAL_LIMITED, TIMEOUT, OTHER]:
        n = stats.get(c, 0)
        pct = n / total * 100 if total else 0
        print(f"{c:<30}{n:>10}{pct:>9.1f}%")

    print("\n--- 按用户分组（观察单用户桶 5/s） ---")
    for u in users:
        row = per_user[u]
        succ = row.get(SUCCESS, 0)
        lim = row.get(USER_LIMITED, 0)
        print(f"  {u:<12} 成功 {succ:>4}  单用户限流 {lim:>4}  其他 {sum(row.values()) - succ - lim:>4}")

    print("""
【解读】
  - 「单用户限流」占比高 → 第 2 层单用户桶(5/s)生效，单机环境能稳定观察到的限流层。
  - 「全局限流」出现 → 第 1 层全局桶(3000/s)被击穿，需总 QPS > 3000（≥600 uid）才出现。
  - 「超时」出现 → 第 3 层 Redis 限流(100/s)触发（服务端未回包，客户端超时）；
      单机下被 5/s 单用户桶挡住，一般测不到。
""")


if __name__ == "__main__":
    main()
