# SeckillServer — 秒杀业务服务器

## 职责
秒杀商城的所有业务逻辑。负责：
- 用户充值 / 余额查询
- 商品浏览 / 秒杀下单
- 订单管理（创建 / 支付 / 取消，30 分钟超时）
- 排行榜统计
- 个人中心（我的订单 / 已购宝贝）
- JWT 认证（与 GateServer 共享密钥）

## 文件

| 文件 | 说明 |
|------|------|
| `main.cpp` | 启动入口：io_context + SeckillServer |
| `global.h` | 全局定义：错误码、REQUEST_ID、服务类型、`SECKILLSERVERS` key |
| `SeckillServer.h/cpp` | HTTP acceptor：`startAccept` + 连接计数 + StatusServer 心跳注册 |
| `HttpConnection.h/cpp` | HTTP 连接处理：CORS、keep-alive、JWT authenticate、读写循环 |
| `LogicSystem.h/cpp` | GET/POST 路由分发：`getHandles_` / `postHandles_` 前缀匹配，`GetParams`/`PostParams` 携带解析结果 |
| `MysqlDao.h/cpp` | 数据库访问：余额读写、商品查询、订单 CRUD、密码验证、排行榜 |
| `MsgNode.h/cpp` | 消息节点（协议层，与 StatusServer 心跳通信） |
| `StatusClientSession.h/cpp` | 与 StatusServer 的 TCP 连接（服务注册 + 心跳） |
| `config.ini` | 配置文件：`[SelfServer] Port=8100`、`[Mysql]`、`[Redis]`、`[JWT]` |

## API 列表

| 方法 | 路径 | 认证 | 触发场景 |
|------|------|------|----------|
| GET | `/products` | 无 | 进入商品列表页 |
| GET | `/rank` | 无 | 进入排行榜页，前端定时轮询 |
| GET | `/orders` | 无 | Navbar 点击「订单」，查看全部抢购记录 |
| GET | `/profile` | JWT | Navbar 点击「我的」→ 个人中心 |
| GET | `/balance` | JWT | 充值页加载时显示余额 |
| GET | `/order/{id}` | JWT | 抢购后 / 从个人中心点击订单 → 订单详情 + 倒计时 |
| POST | `/buy/{id}` | JWT | 商品卡片点击「立即抢购」→ 创建 unpaid 订单 |
| POST | `/recharge` | JWT+密码 | 充值页面输入金额+密码 → 确认充值 |
| POST | `/order/{id}/pay` | JWT+密码 | 订单详情页输入密码 → 支付（扣款+扣库存） |
| POST | `/order/{id}/cancel` | JWT | 订单详情页点击「取消订单」 |

## 订单状态流转

```
抢购 → unpaid ──支付──→ paid
              ├─取消──→ cancelled
              └─30分钟超时→ 前端按钮消失，无法支付
```

## 数据库表

| 表 | 用途 |
|----|------|
| `seckill_product` | 商品（id, name, price, stock, image_url） |
| `seckill_order` | 订单（uid, product_id, price, status, recipient, created_at） |
| `user` | 用户余额（balance 字段） |

## 依赖

```
SeckillServer
 ├── common.lib (ConfigManager, RedisManager, JWT, BCryptHasher)
 ├── MySQL（余额读写、商品/订单 CRUD）
 ├── Redis（JWT token 验证、余额缓存）
 └── StatusServer（gRPC 服务注册 + TCP 心跳）
```

## 与其他服务的关系

```
Client (React) ──HTTP──→ SeckillServer (充值/秒杀/订单)
                           │
                           ├── Redis（token 验证 + 库存）
                           └── MySQL（余额 + 订单持久化）

GateServer ──JWT 签发──→  Client 携 token 访问 SeckillServer
```

## 注意
- 端口默认 8100（GateServer 8080，ResourceServer 9090）
- `MysqlDao` 自行管理连接池（`tcp://host:port` 格式），未使用 common 的 MysqlManager
- JWT 为 Redis 存储模式（UUID token → Redis GET 验证），密钥通过 `config.ini [JWT] Secret` 配置
- 排行榜只统计已支付订单（`WHERE status='paid'`），按购买数量降序，同数量按最近购买时间降序
