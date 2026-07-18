#ifndef GLOBAL_H
#define GLOBAL_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <mutex>
#include <chrono>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <json/json.h>
#include <vector>
#include <thread>
#include <atomic>
#include <queue>
#include <functional>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include "SingleTon.h"
#include "ConfigManager.h"

// TCP 消息帧头：[2字节 msg_id][2字节 msg_len]（网络字节序），与 StatusServer 一致
#define HEAD_TOTOL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECV_LENGTH 1024
// 心跳间隔：StatusServer 超过 30s（HEART_CHECK_OVERTIME）没收到心跳就会踢掉会话，
// 所以这里每 10s 发一次，与 ChatServer 保持一致
#define HEART_CHRCK_INTERVAL 10

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ERROE_CODR
{
    ERROR_JSON = -1024, // json 解析失败
    ERROR_PARAM,        // 请求参数错误
    ERROR_PRODUCT_NOT_EXIST, // 商品不存在
    SUCCESS = 0,
};

// 消息 ID：必须与 StatusServer/global.h 中的取值一致（StatusServer 按 ID 分发 handler）
enum REQUEST_ID
{
    ID_HEADT_CHECK_REQ = 1018, // 心跳检测的请求
    ID_HEADT_CHECK_RSP = 1019, // 心跳检测的回包
    ID_REGISTER_REQ = 2001,    // 服务注册请求（服务间协议，独立区间避免与客户端消息 ID 冲突）
    ID_REGISTER_RSP = 2002     // 服务注册回包
};

// 服务类型：必须与 StatusServer/global.h 中的 ServerType 一致
enum ServerType
{
    None = 0,
    CHAT_SERVER,
    RESOURCE_SERVER,
    SECKILL_SERVER // 秒杀业务服务器
};
#endif
