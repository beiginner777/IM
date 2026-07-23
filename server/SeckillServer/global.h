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
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include <cstdio>

#define _CRT_SECURE_NO_WARNINGS

// TCP 消息帧头：[2字节 msg_id][2字节 msg_len]（网络字节序），与 StatusServer 一致
#define HEAD_TOTOL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECV_LENGTH 1024
#define SECKILLSERVERS "SeckillServers" // 与 StatusServer global.h 中的 key 一致，用于 Redis HSet 存储连接数
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

enum REQUEST_ID
{
	ID_GET_VERIFY_CODE = 1001, // 获取验证码
	ID_REG_USER = 1002, // 注册用户
	ID_LOGIN, // 登录
	ID_CHAT_LOGIN, // 发送长连接
	ID_CHAT_LOGIN_RSP, // tcpMsg接收到服务器的消息
	ID_SEARCH_USER_REQ, // 用户搜索请求
	ID_SEARCH_USER_RSP, // 搜索用户回包
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
	ID_LOAD_CHAT_THREAD_REQ, // 加载聊天列表请求
	ID_LOAD_CHAT_THREAD_RSP, // 加载聊天列表回包
	ID_CREATE_PRIVATE_CHAT_THREAD_REQ, // 创建私聊会话请求
	ID_CREATE_PRIVATE_CHAT_THREAD_RSP, // 创建私聊会话回包
	ID_LOAD_MORE_FRIEND_REQ, // 加载好友列表的请求
	ID_LOAD_MORE_FRIEND_RSP, // 加载好友列的回包
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
	ID_DOWN_LOAD_FILE_REQ, // 下载文件请求
	ID_DOWN_LOAD_FILE_RSP, // 下载文件回复
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
	ID_REGISTER_RSP // 服务注册回包（StatusServer）
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
