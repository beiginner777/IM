#ifndef GLOBAL_H
#define GLOBAL_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <mutex>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <chrono>
#include <map>
#include <string>
#include <fstream>e
#include <json/json.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <queue>
#include <hiredis/hiredis.h> // hiredis 头文件
#include <exception>
#include <json/json.h>
#include <grpcpp/grpcpp.h>
#include <unordered_map>
#include <ctime>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include "SingleTon.h"
#include "ConfigManager.h"
#include "data.h"
#define HOST "0.0.0.0"
#define PORT 8080
#define DEFAULT_GRPC_CONN_SIZE 5
#define DEFAULT_REDIS_CONN_SIZE 8
#define DEFAULT_MYSQL_CONN_SIZE 5
#define DEFAULT_STATUSGRPCCLIENT_CONN_SIZE 8
#define DEFAULT_CHATCONNPOOL_SIZE 8
#define HEAD_TOTOL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECV_LENGTH 1024
#define MAX_MSG_ID 1024
#define MAX_MSG_LEN 1024
#define MAX_SENDQUEUE_SIZE 1024
#define USERTOKENPREFIX  "token_"
#define USERUIDPREFIX "uid_"
#define CHATSERVERS "ChatServers"
#define RESOURCESERVERS "ResourceServers"
#define USERBASEINFO "userbaseinfo_"
#define USERIPPREFIX  "uip_"
#define LOCKPREFIX "lock_"
#define LOCK_TIMEOUT 10
#define ACQUIRE_TIMEOUT 5
#define HEART_CHRCK_INTERVAL 60
#define HEART_CHECK_OVERTIME 40
#define MYSQL_CONN_OVERTIME 5
#define REDIS_CONN_OVERTIME 5
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = boost::beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
enum ERROE_CODR
{
    ERROR_JSON = -1024, // json 解析失败    
    ERROR_RPC, // 调用邮箱认证的时候，Rpc调用失败
    ERROR_VERIFYCODE, // 注册时，输入了错误的验证码
    ERROR_REGISTER, // 注册失败
    ERROR_NAME_EXIST, // 注册时，用户名已存在
    ERROR_EMAIL_EXIST, // 注册时，邮箱已存在
    ERROR_CONNECT_GRPC, // 连接gRpc失败
    ERROR_USER_NOT_EXIST, // 登陆时用户不存在
    ERROR_PASSWORD, // 登陆时密码错误
    ERROR_LOGIN, // 登录失败
    ERROR_RPC_CON_STATUSSERVER, // 登陆时，访问StatusServer失败
    REDISCONNPOOL_BUSY, // 没有空闲的 redis 连接
    ERROR_INVALIDUID,// 登陆验证时 uid 不存在
    ERROR_INVALIDTOKEN,// 登陆验证是 token 不匹配
    ERROR_SEARCH_INVALIDNAME, // 查找的用户名不存在
    ERROR_SEARCH_INVALIDUID, // 查找的用户的uid不存在
    ERROR_MULTIPLE_FRIEND_APPLY, // 好友申请已经存在
    ERROR_FRIEND_APPLY, // 执行好友申请的逻辑时，在数据库的层面出现错误
    ERROR_FIND_PEER_IP, // 在reids查找对方所在的ChatServer时，出现错误
    ERROR_RPC_VISIT_CHATSERVER, // 在使用rpc访问别的ChatServer的时候出现错误
    ERROR_GET_FRIEND_APPLY_LIST,// 在获取用户的好友申请列表的时候出现错误
    ERROR_AUTH_APPLY, // 更新好友申请状态的时候出现错误
    ERROR_GET_FRIEND_LIST,// 获取好友列表出现错误
    ERROR_FIND_CONN_IN_RPCPOOLS, // 
    ERROR_ADD_FRIEND_RELATION,
    ERROR_MODIFLY_APPLY_STATUS_FAILED, // 修改好友状态失败
    SUCCESS = 0,
};
enum REQUEST_ID
{
    ID_GET_VERIFY_CODE = 1001, //获取验证码
    ID_REG_USER = 1002, //注册用户
    ID_LOGIN, // 登录
    ID_CHAT_LOGIN,// 发送长连接
    ID_CHAT_LOGIN_RSP, // tcpMsg接收到服务器的消息
    ID_SEARCH_USER_REQ, //用户搜索请求
    ID_SEARCH_USER_RSP, //搜索用户回包
    ID_APPLY_FRIEND_REQ, // 申请好友
    ID_APPLY_FRIEND_RSP, // 申请好友的回包
    ID_NOTIFY_ADD_FRIEND_REQ,
    ID_AUTH_FRIEND_REQ,
    ID_AUTH_FRIEND_RSP,
    ID_NOTIFY_ACCESS_VERIFY,
    ID_TEXT_CHAT_MSG_REQ, // 发送聊天消息
    ID_TEXT_CHAT_MSG_RSP, // 回包
    ID_NOTIFY_TEXT_CHAT_MSG_REQ,
    // 有新的聊天消息
    ID_NOTIFY_OFFLINE, // 通知下线
    ID_HEADT_CHECK_REQ, // 心跳检测的请求
    ID_HEADT_CHECK_RSP // 心跳检测的回包
};
class Defer {
public:
    Defer(std::function<void()> func) : func_(func) {}
    ~Defer() {
        if (func_) func_();
    }
    // 禁止拷贝
    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;
private:
    std::function<void()> func_;
};
#endif