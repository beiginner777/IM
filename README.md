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


# 三、秒杀系统流程图

## API 时序图

```mermaid
sequenceDiagram
    participant U as 用户(浏览器)
    participant GS as GateServer
    participant S as SeckillServer
    participant R as Redis
    participant M as MySQL

    Note over U,S: ═══════ ① 登录 ═══════
    U->>GS: POST /api/login {username,password}
    GS->>M: bcrypt 验证
    GS->>R: SETEX token:{uuid} {uid,username,exp} 86400
    GS-->>U: {token,balance,host,port,username}
    Note over U: 存 token → localStorage<br/>setBaseURL → SeckillServer

    Note over U,S: ═══════ ② 充值 ═══════
    U->>S: POST /recharge {amount,password}<br/>Header: Bearer token
    S->>R: GET token:{uuid} → 提取uid
    S->>M: SELECT password → bcrypt验证
    S->>M: UPDATE balance += amount
    S->>R: DEL balance_cache:{uid}
    S-->>U: {code:0, balance:新余额}

    Note over U,S: ═══════ ③ 抢购(创建订单) ═══════
    U->>S: POST /buy/{id}<br/>Header: Bearer token
    S->>R: GET token:{uuid} → 提取uid
    S->>M: SELECT balance → 检查余额
    S->>M: INSERT seckill_order(status='unpaid')
    S-->>U: {success:true, orderId:123}

    Note over U,S: ═══════ ④ 订单详情 + 倒计时 ═══════
    U->>S: GET /order/123<br/>Header: Bearer token
    S->>R: GET token:{uuid} → 提取uid
    S->>M: SELECT seckill_order WHERE id=123
    Note over S: 计算 remainSeconds = 1800 - elapsed
    S-->>U: {id,productName,price,status:'unpaid',remainSeconds:1735}
    Note over U: 倒计时 28:55...28:54...

    Note over U,S: ═══════ ⑤a 支付 ═══════
    U->>S: POST /order/123/pay {password:'123456'}
    S->>R: GET token:{uuid} → 提取uid
    S->>M: SELECT password → bcrypt验证
    S->>M: SELECT balance → 检查 >= price
    S->>M: UPDATE balance -= price
    S->>M: UPDATE seckill_order SET status='paid'
    S->>M: UPDATE seckill_product SET stock=stock-1
    S->>R: DEL balance_cache:{uid}
    S-->>U: {success:true, balance:新余额}

    Note over U,S: ═══════ ⑤b 取消订单 ═══════
    U->>S: POST /order/123/cancel
    S->>R: GET token:{uuid} → 提取uid
    S->>M: UPDATE seckill_order SET status='cancelled',cancelled_at=NOW()
    S-->>U: {success:true, message:'订单已取消'}
```

## 个人中心 + 商品 + 排行

```mermaid
sequenceDiagram
    participant U as 用户
    participant S as SeckillServer
    participant M as MySQL
    participant R as Redis

    Note over U,S: ═══════ ⑥ 个人中心 ═══════
    U->>S: GET /profile
    S->>R: GET token:{uuid} → uid
    S->>M: SELECT name,balance FROM user
    S->>M: SELECT * FROM seckill_order WHERE uid=?
    S-->>U: {username,balance,orders:[{orderId,productName,price,status,time}]}

    Note over U,S: ═══════ ⑦ 商品列表 ═══════
    U->>S: GET /products
    S->>M: SELECT * FROM seckill_product
    S-->>U: [{id,name,price,stock,imageUrl}]

    Note over U,S: ═══════ ⑧ 排行榜 ═══════
    U->>S: GET /rank
    S->>M: SELECT product_id,COUNT(*) GROUP BY product_id
    S-->>U: [{productId,productName,count}]
```

## 用户操作流程

```mermaid
graph TB
    subgraph 主流程
        A[登录] --> B[浏览商品]
        B --> C{选择商品}
        C --> D[创建订单<br/>status=unpaid]
        D --> E[订单详情页<br/>30分钟倒计时]
        E --> F{操作}
        F -->|支付| G[输入密码]
        G --> H{验证}
        H -->|通过| I[扣款 + 扣库存<br/>status=paid]
        H -->|失败| G
        F -->|取消| J[确认取消]
        J --> K[status=cancelled]
        F -->|超时| L[按钮消失<br/>无法支付]
    end

    subgraph 个人中心
        P[我的] --> Q[余额展示]
        P --> R[我的订单Tab]
        P --> S[我的宝贝Tab]
        R --> T[点击订单]
        T --> E
    end

    subgraph 充值流程
        RC[充值] --> RP[输入金额+密码]
        RP --> RM[(MySQL UPDATE balance)]
        RM --> RB[余额更新]
    end
```

## 订单状态流转

```
         ┌─── 抢购 ──→ unpaid ──┬── 支付(密码) ──→ paid
         │                      ├── 取消 ──→ cancelled
         │                      └── 30分钟超时 ──→ 无法支付(前端隐藏按钮)
```

## API 汇总

| 方法 | 路径 | 认证 | 功能 |
|---|---|---|---|
| POST | `/api/login`(GateServer) | 密码 | 登录，返回JWT |
| POST | `/recharge` | JWT+密码 | 充值 |
| GET | `/balance` | JWT | 查余额 |
| GET | `/products` | 无 | 商品列表 |
| POST | `/buy/{id}` | JWT | 创建未支付订单 |
| GET | `/order/{id}` | JWT | 订单详情+剩余时间 |
| POST | `/order/{id}/pay` | JWT+密码 | 支付(扣款+扣库存) |
| POST | `/order/{id}/cancel` | JWT | 取消订单 |
| GET | `/profile` | JWT | 个人中心(余额+订单) |
| GET | `/rank` | 无 | 排行榜 |

## 数据存储

| 数据 | 存储位置 | 说明 |
|---|---|---|
| 用户余额 | MySQL `user.balance` | 充值/购买时更新 |
| 商品库存 | MySQL `seckill_product.stock` | 支付后才扣库存 |
| 订单记录 | MySQL `seckill_order` | unpaid→paid→cancelled |
| JWT token | Redis `token:{uuid}` | TTL 24h，登录签发 |
| 余额缓存 | Redis `balance_cache:{uid}` | TTL 60s，充值/购买后清除 |
