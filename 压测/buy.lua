-- buy.lua —— 场景 5：秒杀购买接口压测
-- 用法: wrk -t4 -c100 -d60s -s buy.lua http://127.0.0.1:8101
-- 前置: 把 <token> 换成 /fe_login 拿到的 token
-- 注意: 单 token=单 uid，SeckillServer 单用户令牌桶 ~5/s，会触发限流；
--       测原始写 QPS 需多 uid 多 token（见 seckill_bench.py）
wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.headers["Authorization"] = "Bearer <token>"
wrk.body = ""
request = function()
    return wrk.format("POST", "/buy/1", wrk.headers, wrk.body)
end
