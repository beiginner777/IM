-- pay.lua —— 场景 4：秒杀支付接口压测
-- 用法: wrk -t4 -c50 -d30s -s pay.lua http://127.0.0.1:8101
-- 前置: 先手动买一个拿到 orderId，把下面 URL 里的 /order/1/pay 改成真实 orderId
--       并把 <token> 换成 /fe_login 拿到的 token
wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.headers["Authorization"] = "Bearer <token>"
wrk.body = '{"password":"123456"}'
request = function()
    return wrk.format("POST", "/order/1/pay", wrk.headers, wrk.body)
end
