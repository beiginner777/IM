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

    ID_GET_VERIFY_CODE = 1001,
    ID_REG_USER = 1002,
    ID_LOGIN,
    ID_CHAT_LOGIN,
    ID_CHAT_LOGIN_RSP,
    ID_SEARCH_USER_REQ,
    ID_SEARCH_USER_RSP,
    ID_APPLY_FRIEND_REQ,
    ID_APPLY_FRIEND_RSP,
    ID_NOTIFY_ADD_FRIEND_REQ,

    ID_AUTH_FRIEND_REQ,

    ID_AUTH_FRIEND_RSP,

    ID_NOTIFY_ACCESS_VERIFY,



    ID_TEXT_CHAT_MSG_REQ,
    ID_TEXT_CHAT_MSG_RSP,


    ID_NOTIFY_TEXT_CHAT_MSG_REQ,



    ID_NOTIFY_OFFLINE,


    ID_HEADT_CHECK_REQ,
    ID_HEADT_CHECK_RSP,


    ID_LOAD_CHAT_THREAD_REQ,
    ID_LOAD_CHAT_THREAD_RSP,


    ID_CREATE_PRIVATE_CHAT_THREAD_REQ,
    ID_CREATE_PRIVATE_CHAT_THREAD_RSP,


    ID_LOAD_MORE_FRIEND_REQ,
    ID_LOAD_MORE_FRIEND_RSP,


    ID_UPLOAD_HEAD_ICON_REQ,
    ID_UPLOAD_HEAD_ICON_RSP,


    ID_UPLOAD_FILE_REQ,
    ID_UPLOAD_FILE_RSP,


    ID_SYNC_FILE_REQ,
    ID_SYNC_FILE_RSP,


    ID_GET_NOTIFY_MESSAGE_REQ,
    ID_GET_NOTIFY_MESSAGE_RSP,


    ID_IMAGE_CHAT_MSG_REQ,
    ID_IMAGE_CHAT_MSG_RSP,


    ID_DOWN_LOAD_FILE_REQ,
    ID_DOWN_LOAD_FILE_RSP,


    ID_LOAD_FRIEND_APPLY_REQ,
    ID_LOAD_FRIEND_APPLY_RSP,


    ID_IMG_CHAT_CONTINUE_UPLOAD_REQ,
    ID_IMG_CHAT_CONTINUE_UPLOAD_RSP,


    ID_NOTIFY_FRIEND_ICON_CHANGE,


    ID_NOTIFY_CHAT_IMAGE_MSG,


    ID_LOAD_CHAT_MSG_REQ,
    ID_LOAD_CHAT_MSG_RSP,


	ID_REGISTER_REQ,
	ID_REGISTER_RSP
};



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
