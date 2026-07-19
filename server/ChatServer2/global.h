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
#include <fstream>
#include <json/json.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <queue>
#include <hiredis/hiredis.h>
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
#define HEAD_UUID_LEN 36
#define HEAD_TOTOL_LEN_WITH_UUID 40
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECV_LENGTH 4096
#define MAX_MSG_ID 2048
#define MAX_MSG_LEN 4096
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
#define HEART_CHRCK_INTERVAL 10
#define HEART_CHECK_OVERTIME 30
#define MYSQL_CONN_OVERTIME 5
#define REDIS_CONN_OVERTIME 5
namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
enum ERROE_CODR
{
    ERROR_JSON = -1024,
    ERROR_RPC,
    ERROR_VERIFYCODE,
    ERROR_REGISTER,
    ERROR_NAME_EXIST,
    ERROR_EMAIL_EXIST,
    ERROR_CONNECT_GRPC,
    ERROR_USER_NOT_EXIST,
    ERROR_PASSWORD,
    ERROR_LOGIN,
    ERROR_RPC_CON_STATUSSERVER,
    REDISCONNPOOL_BUSY,
    ERROR_INVALIDUID,
    ERROR_INVALIDTOKEN,
    ERROR_SEARCH_INVALIDNAME,
    ERROR_SEARCH_INVALIDUID,
    ERROR_MULTIPLE_FRIEND_APPLY,
    ERROR_FRIEND_APPLY,
    ERROR_FIND_PEER_IP,
    ERROR_RPC_VISIT_CHATSERVER,
    ERROR_GET_FRIEND_APPLY_LIST,
    ERROR_AUTH_APPLY,
    ERROR_GET_FRIEND_LIST,
    ERROR_FIND_CONN_IN_RPCPOOLS,
    ERROR_ADD_FRIEND_RELATION,
    ERROR_MODIFLY_APPLY_STATUS_FAILED,
    ERROR_LOAD_CHAT_THREAD,
    ERROR_CREATE_PRIVATE_THREAD,
    ERROR_SEND_MSG_FAILED,
	ERROR_LOAD_MORE_FRIEND,
	ERROR_PUSH_NOTIFY_MESSAGE,
	ERROR_GET_NOTIFY_MESSAGE,
	ERROR_LOAD_FRIEND_APPLY,
    ERROR_USER_NOT_EXIST_IN_CHATSERVER,
    ERROR_CHATIMG_NOT_EXIST_IN_CHATSERVER,
    ERROR_MODIFY_MSG_STATUS,
    ERROR_LOAD_CHAT_MESSAGE,
ERROR_RATE_LIMITED,   // 发送频率超限
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
// 请求 msgId → 回包 msgId 显式映射表
// 不再依赖枚举顺序 REQ+1=RSP 的隐式约定
inline short getRspMsgId(short reqId)
{
	switch (reqId) {
	case ID_GET_VERIFY_CODE:        return ID_GET_VERIFY_CODE;        // 无单独 RSP
	case ID_REG_USER:                return ID_REG_USER;               // 无单独 RSP
	case ID_CHAT_LOGIN:              return ID_CHAT_LOGIN_RSP;
	case ID_SEARCH_USER_REQ:         return ID_SEARCH_USER_RSP;
	case ID_APPLY_FRIEND_REQ:        return ID_APPLY_FRIEND_RSP;
	case ID_AUTH_FRIEND_REQ:         return ID_AUTH_FRIEND_RSP;
	case ID_TEXT_CHAT_MSG_REQ:       return ID_TEXT_CHAT_MSG_RSP;
	case ID_LOAD_CHAT_THREAD_REQ:    return ID_LOAD_CHAT_THREAD_RSP;
	case ID_CREATE_PRIVATE_CHAT_THREAD_REQ: return ID_CREATE_PRIVATE_CHAT_THREAD_RSP;
	case ID_LOAD_MORE_FRIEND_REQ:    return ID_LOAD_MORE_FRIEND_RSP;
	case ID_UPLOAD_HEAD_ICON_REQ:    return ID_UPLOAD_HEAD_ICON_RSP;
	case ID_UPLOAD_FILE_REQ:         return ID_UPLOAD_FILE_RSP;
	case ID_SYNC_FILE_REQ:           return ID_SYNC_FILE_RSP;
	case ID_GET_NOTIFY_MESSAGE_REQ:  return ID_GET_NOTIFY_MESSAGE_RSP;
	case ID_IMAGE_CHAT_MSG_REQ:      return ID_IMAGE_CHAT_MSG_RSP;
	case ID_DOWN_LOAD_FILE_REQ:      return ID_DOWN_LOAD_FILE_RSP;
	case ID_LOAD_FRIEND_APPLY_REQ:   return ID_LOAD_FRIEND_APPLY_RSP;
	case ID_IMG_CHAT_CONTINUE_UPLOAD_REQ: return ID_IMG_CHAT_CONTINUE_UPLOAD_RSP;
	case ID_LOAD_CHAT_MSG_REQ:       return ID_LOAD_CHAT_MSG_RSP;
	case ID_FILE_CONTINUE_DOWNLOAD_REQ: return ID_FILE_CONTINUE_DOWNLOAD_RSP;
	case ID_HEADT_CHECK_REQ:         return ID_HEADT_CHECK_RSP;
	default:                         return reqId;  // 通知类消息不需要回包，返回自身
	}
}
enum REDIS_ID
{
	REDIS_ID_FRIEND_ICON_CHANGE,
};
enum CHAT_THREAD_TYPE
{
    CHAT_THREAD_TYPE_PRIVATE = 1,
    CHAT_THREAD_TYPE_GROUP = 2,
};
enum  FRIEND_APPLY
{
    UN_HANDLE,
    ACCEPTED,
    REFUSED
};
class Defer {
public:
    Defer(std::function<void()> func) : func_(func) {}
    ~Defer() {
        if (func_) func_();
    }
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
#endif
