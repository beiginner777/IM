# 一、总体架构图


![](drawio/1782684161650.drawio.svg)

---

# 二、各服务架构图

## 2.1 GateServer
![1782687112256.drawio](drawio/1782687112256.drawio.svg)

---

## 2.2 StatusServer
![1782687211724.drawio](drawio/1782687211724.drawio.svg)

---

## 2.3 ChatServer

![1782687278041.drawio](drawio/1782687278041.drawio.svg)

--- 

## 2.4 ResourceServer

![1782687310607.drawio](drawio/1782687310607.drawio.svg)

---

# 三、存储层

![](drawio/1782684256881.drawio.svg)

---

# 四、核心时序

## 4.1 消息发送 + ACK
![](drawio/1782684360406.drawio.svg)

---

## 4.2 抢红包 (Redis Lua 原子操作)
![](drawio/1782684406499.drawio.svg)

---

## 4.3 通讯录匹配 + 布隆过滤器
![](drawio/1782684446469.drawio.svg)

---

## 4.4 分布式限流（令牌桶）

![](drawio/1782688803442.drawio.svg)

---

## 4.5 消息入库（批量写入 + 异步重试）

![](drawio/1782689043975.drawio.svg)

---

## 4.6 用户登录 + 服务发现

![](drawio/1782689075187.drawio.svg)

---

## 4.7 服务注册与心跳摘除

![](drawio/1782689115635.drawio.svg)

---

## 4.8 文件分片上传 + 断点续传

![](drawio/1782689152458.drawio.svg)

---

## 4.9 Client→ChatServer（一致性哈希路由 + 节点扩缩容）

![1782690636108.drawio](drawio/1782690636108.drawio.svg)

---

## 4.10 ChatServer→ResourceServer（一致性哈希文件路由）

![1782690658441.drawio](drawio/1782690658441.drawio.svg)

---

## 4.11 MySQL 分表（一致性哈希分片 + 扩容）

![1782690690788.drawio](drawio/1782690690788.drawio.svg)

---

## 4.12 SeckillServer 秒杀服务

### API 列表

| 方法 | 路径 | 触发场景 |
|---|---|---|
| GET | `/products` | 用户进入商品列表页（ProductListPage） |
| GET | `/rank` | 用户进入排行榜页，前端定时轮询刷新 |
| GET | `/profile` | 用户进入个人中心（Navbar 点「我的」） |
| GET | `/balance` | 充值页加载时显示余额 |
| GET | `/order/{id}` | 抢购后跳转订单详情页，或从个人中心点击订单 |
| POST | `/buy/{id}` | 商品卡片点击「立即抢购」→ 创建 unpaid 订单 |
| POST | `/recharge` | 充值页面输入金额+密码 → 确认充值 |
| POST | `/order/{id}/pay` | 订单详情页输入密码 → 支付（扣款+扣库存） |
| POST | `/order/{id}/cancel` | 订单详情页点击「取消订单」 |

### 订单状态流转

```
抢购 → unpaid ──支付──→ paid
              ├─取消──→ cancelled
              └─30分钟超时→ 前端按钮消失,无法支付
```

### 认证

所有写操作（/recharge、/buy/*、/order/*/pay、/order/*/cancel）和读操作（/profile、/balance、/order/*）通过 `Authorization: Bearer <token>` 携带 JWT。token 由 GateServer `/fe_login` 签发，Redis 存储，TTL 24h。
