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
#define MAX_MSG_ID 2048
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
#define HEART_CHRCK_INTERVAL 10 // 10s检测一次
#define HEART_CHECK_OVERTIME 30 // 超过30s没有收到心跳消息，就认为连接过期了
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
    ID_NOTIFY_ADD_FRIEND_REQ, // 服务器通知有好友请求
    ID_AUTH_FRIEND_REQ, // 验证好友申请的请求
    ID_AUTH_FRIEND_RSP, // 验证好友申请的回包
    ID_NOTIFY_ACCESS_VERIFY, // 服务器通知好友申请被对方通过了
    ID_TEXT_CHAT_MSG_REQ, // 发送聊天消息
    ID_TEXT_CHAT_MSG_RSP, // 回包
    ID_NOTIFY_TEXT_CHAT_MSG_REQ, // 有新的聊天消息
    ID_NOTIFY_OFFLINE, // 通知下线
    ID_HEADT_CHECK_REQ, // 心跳检测的请求
    ID_HEADT_CHECK_RSP, // 心跳检测的回包
    ID_LOAD_CHAT_THREAD_REQ,// 加载聊天列表请求
    ID_LOAD_CHAT_THREAD_RSP, // 加载聊天列表回包
    ID_CREATE_PRIVATE_CHAT_THREAD_REQ, // 创建私聊会话请求
    ID_CREATE_PRIVATE_CHAT_THREAD_RSP, // 创建私聊会话回包
    ID_LOAD_MORE_FRIEND_REQ, // 加载好友列表的请求
    ID_LOAD_MORE_FRIEND_RSP,  // 加载好友列的回包
    ID_UPLOAD_HEAD_ICON_REQ, // 上传头像请求
    ID_UPLOAD_HEAD_ICON_RSP, // 上传头像回包
    ID_UPLOAD_FILE_REQ, // 上传文件请求
    ID_UPLOAD_FILE_RSP, // 上传文件回包
    ID_SYNC_FILE_REQ, // 同步文件请求（续传）
    ID_SYNC_FILE_RSP, // 同步文件回包（续传）
    ID_GET_NOTIFY_MESSAGE_REQ, // 获取通知消息的请求
    ID_GET_NOTIFY_MESSAGE_RSP, // 获取通知消息的回包
    ID_IMAGE_CHAT_MSG_REQ, // 发送图片的请求
    ID_IMAGE_CHAT_MSG_RSP, // 发送图片的回包
    ID_DOWN_LOAD_FILE_REQ, //下载文件请求
    ID_DOWN_LOAD_FILE_RSP, //下载文件回复
    ID_LOAD_FRIEND_APPLY_REQ, // 加载好友申请请求
    ID_LOAD_FRIEND_APPLY_RSP, // 加载好友申请回包
    ID_IMG_CHAT_CONTINUE_UPLOAD_REQ, // 聊天图片续传请求
    ID_IMG_CHAT_CONTINUE_UPLOAD_RSP, // 聊天图片续传回包
    ID_NOTIFY_FRIEND_ICON_CHANGE, // 通知有好友的头像变更
    ID_NOTIFY_CHAT_IMAGE_MSG, // 收到聊天图片消息
    ID_LOAD_CHAT_MSG_REQ, // 加载聊天消息的请求
    ID_LOAD_CHAT_MSG_RSP, // 加载聊天消息的回包
    ID_FILE_CONTINUE_DOWNLOAD_REQ, // 文件续传下载请求
    ID_FILE_CONTINUE_DOWNLOAD_RSP, // 文件续传下载回包
    ID_RESUME_UPLOAD_REQ, // 断点续传查询进度
    ID_RESUME_UPLOAD_RSP, // 断点续传返回进度
    ID_REGISTER_REQ, // 服务注册请求（StatusServer）
    ID_REGISTER_RSP  // 服务注册回包（StatusServer）
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
enum ServerType
{
    None = 0,
    CHAT_SERVER,
    RESOURCE_SERVER
};
struct Server_Info
{
	Server_Info() : host(""), port(""), name(""), con_count(0), server_type(ServerType::None)
    {
    }
	ServerType server_type;
    std::string host;
    std::string port;
    std::string name;
    int con_count;
};
struct ChatServer {
public:
    ChatServer() :host(""), port(""), name(""), con_count(0) 
    {
    }
    ChatServer(const ChatServer& cs) 
        : host(cs.host)
        , port(cs.port)
        , name(cs.name)
        , con_count(cs.con_count) 
        , server_type(ServerType::CHAT_SERVER)
    {
    }
    ChatServer operator=(const ChatServer& cs) {
        if (&cs == this) {
            return *this;
        }
        host = cs.host;
        name = cs.name;
        port = cs.port;
        con_count = cs.con_count;
        return *this;
    }
    void printInfo(){
        std::cout << "[" << name << "]: " << "host: " << host << " " << "port: " << port << std::endl;
    }
    ServerType server_type;
    std::string host;
    std::string port;
    std::string name;
    int con_count;
};
std::string generate_unique_string();
#endif