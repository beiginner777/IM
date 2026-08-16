-- recharge.lua —— 场景 6：充值接口压测（三层限流验证）
-- 用法: wrk -t8 -c200 -d20s -s recharge.lua http://127.0.0.1:8101
-- 前置: 把 <token> 换成 /fe_login 拿到的 token
-- 观察: 超限时返回 {"success":false,"message":"发送过于频繁"} 或 "server busy"
wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.headers["Authorization"] = "Bearer <token>"
wrk.body = '{"amount":1.0,"password":"123456"}'
request = function()
    return wrk.format("POST", "/recharge", wrk.headers, wrk.body)
end
