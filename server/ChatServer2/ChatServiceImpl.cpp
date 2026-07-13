#include "ChatServiceImpl.h"
#include "UserManager.h"
#include "CSession.h"
#include "RedisManager.h"
#include "MysqlManager.h"
#include "LogicSystem.h"

ChatServiceImpl::ChatServiceImpl()
{

}

ChatServiceImpl::~ChatServiceImpl()
{
}

Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply)
{
    //查找用户是否在本服务器
    auto touid = request->touid();

    auto session = UserManager::getInstance()->GetSession(touid);

    /*
    reply->set_error(ErrorCodes::Success);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
    */

    //用户不在内存中则直接返回
    if (session == nullptr) {
        std::cout << "找不到 uid = " << touid << " 的连接." << std::endl;
        return Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value  rtvalue;
    rtvalue["code"] = SUCCESS;
    rtvalue["fromuid"] = request->applyuid(); // 对方uid
    rtvalue["name"] = request->name(); // 对方申请name，不是真正的name
    rtvalue["desc"] = request->desc(); // 对方的desc
    rtvalue["icon"] = request->icon(); // 对方的icon
    rtvalue["sex"] = request->sex(); // 对方的sex
    rtvalue["nick"] = request->nick(); // 对方的nick

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);

    return Status::OK;
}

bool ChatServiceImpl::GetBaseInfo(int uid, std::shared_ptr<UserInfo>& userinfo)
{
    std::string base_key = USERBASEINFO + uid;

    //优先查redis中查询用户信息
    std::string info_str = RedisManager::getInstance()->Get(base_key);
    if (!info_str.empty()) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
        auto name = root["name"].asString();
        auto pwd = root["password"].asString();
        auto email = root["email"].asString();
        auto nick = root["nick"].asString();
        auto desc = root["desc"].asString();
        auto sex = root["sex"].asInt();
        auto icon = root["icon"].asString();
        std::cout << "user  uid is  " << uid << " name  is "
            << name << " pwd is " << pwd << " email is " << email << " icon is " << icon << std::endl;
        userinfo->uid_ = uid;
        userinfo->name_ = name;
        userinfo->nick_ = nick;
        userinfo->desc_ = desc;
        userinfo->sex_ = sex;
        userinfo->icon_ = icon;
        return true;
    }

    //查询mysql数据库
    std::shared_ptr<UserInfo> userInfo = MysqlManager::getInstance()->getUserByUid(uid);

    if (userInfo == nullptr) {
        std::cout << "Can't find uid = " << uid << " in Mysql.";
        return false;
    }

    //将数据库内容写入redis缓存
    Json::Value redis_root;
    redis_root["uid"] = userInfo->uid_;
    redis_root["password"] = userInfo->pwd_;
    redis_root["name"] = userInfo->name_;
    redis_root["email"] = userInfo->email_;
    redis_root["nick"] = userInfo->nick_;
    redis_root["desc"] = userInfo->desc_;
    redis_root["sex"] = userInfo->sex_;
    redis_root["icon"] = userInfo->icon_;

    if (!RedisManager::getInstance()->Set(base_key, redis_root.toStyledString())) {
        std::cout << "read user(uid) to redis failed." << std::endl;
    }

    //返回数据
    userinfo->uid_ = userInfo->uid_;
    userinfo->name_ = userInfo->name_;
    userinfo->nick_ = userInfo->nick_;
    userinfo->desc_ = userInfo->desc_;
    userinfo->sex_ = userInfo->sex_;
    userinfo->icon_ = userInfo->icon_;
    return true;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* response)
{
    // 
    int fromuid = request->fromuid();
    int touid = request->touid();

    auto session = UserManager::getInstance()->GetSession(fromuid);
    if (session == nullptr) {
        std::cout << "找不到 uid = " << fromuid << " 的连接." << std::endl;
        return Status::OK;
    }

    Json::Value rtvalue;
    rtvalue["fromuid"] = fromuid;
    rtvalue["touid"] = touid;

    std::shared_ptr<UserInfo> userinfo = std::make_shared<UserInfo>();
    GetBaseInfo(touid, userinfo);
    rtvalue["uid"] = userinfo->uid_;
    rtvalue["name"] = userinfo->name_;
    rtvalue["desc"] = userinfo->desc_;
    rtvalue["nick"] = userinfo->nick_;
    rtvalue["icon"] = userinfo->icon_;
    rtvalue["sex"] = userinfo->sex_;

    session->Send(rtvalue.toStyledString(), ID_NOTIFY_ACCESS_VERIFY);

    return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context, const TextChatMsgReq* request, TextChatMsgRsp* response)
{
    //查找用户是否在本服务器
    auto touid = request->touid();

    std::cout << "RcpService: notify uid = " << touid << " receive TextMsg." << std::endl;

    auto session = UserManager::getInstance()->GetSession(touid);

    response->set_error(ERROE_CODR::SUCCESS);

    //用户不在内存中则直接返回
    if (session == nullptr) {
        return Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value  rtvalue;
    rtvalue["code"] = ERROE_CODR::SUCCESS;
    rtvalue["fromuid"] = request->fromuid();
    rtvalue["touid"] = request->touid();

    //将聊天数据组织为数组
    Json::Value text_array;
    for (auto& msg : request->textmsgs()) {
        Json::Value element;
        element["content"] = msg.msgcontent();
        element["msgid"] = msg.msgid();
        text_array.append(element);
    }
    rtvalue["text_array"] = text_array;

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
    
    return Status::OK;
}

Status ChatServiceImpl::NotifyKickUser(ServerContext* context, const KickUserReq* request, KickUserRsp* response)
{
    std::cout << "receive GrpcClient NotifyKickUser messgae." << std::endl;

    auto cfg = ConfigManager::getInstance();

    //查找用户是否在本服务器
    auto touid = request->uid();
    auto session = UserManager::getInstance()->GetSession(touid);

    response->set_error(ERROE_CODR::SUCCESS);

    //用户不在内存中则直接返回
    if (session == nullptr) {
        std::cout << "recver = " << touid << " is not in current " << cfg["SelfServer"]["Name"] << std::endl;
        return Status::OK;
    }

    session->Send("", ID_NOTIFY_OFFLINE);

    return Status::OK;
}

Status ChatServiceImpl::NotifyChatServerImg(ServerContext* context, const NotifyChatServerImgReq* req, NotifyChatServerImgRsp* response)
{
    std::cout << "recvice Resource request to notify Client ChatImgInfo." << std::endl;

    auto cfg = ConfigManager::getInstance();

    int uid = req->uid();
    std::string unique_name = req->unique_name();

    // 验证uid是否是在当前服务器
    auto session = UserManager::getInstance()->GetSession(uid);
    if (session == nullptr) {
        response->set_error(ERROE_CODR::ERROR_USER_NOT_EXIST_IN_CHATSERVER);
        return Status::OK;
    }
    
    // 向LogicSystem请求相应的信息，并向 Client 回包
    std::shared_ptr<ChatMessage> msg = LogicSystem::getInstance()->GetUserThreadImageMsg(unique_name);
    if (msg == nullptr) {
        response->set_error(ERROE_CODR::ERROR_CHATIMG_NOT_EXIST_IN_CHATSERVER);
        return Status::OK;
    }
    Json::Value rtvalue;
    rtvalue["message_id"] = msg->message_id;
    rtvalue["send_id"] = msg->sender_id;
    rtvalue["recv_id"] = msg->recv_id;
    rtvalue["thread_id"] = msg->thread_id;
    rtvalue["unique_id"] = msg->unique_id;
    rtvalue["chat_time"] = msg->chat_time;
    rtvalue["unique_name"] = msg->content;
    rtvalue["status"] = MsgStatus::READED;
    rtvalue["code"] = SUCCESS;
    rtvalue["message"] = "receive new image message.";
    rtvalue["type"] = msg->type;

    // 修改Mysql的ChatMessage表 message_id = msg->message_id 的 status 为 2
    MysqlManager::getInstance()->updateChatMsgStatus(msg->message_id, MsgStatus::READED);
    // 通知发送方消息发送完成
    session->Send(rtvalue.toStyledString(), ID_IMAGE_CHAT_MSG_RSP);

    // 通知接收方有聊天图片消息
    // to do ... 判断是否是在本服务器，如果不是，还需要使用 grpc 远程调用去通知
    auto peer_session = UserManager::getInstance()->GetSession(msg->recv_id);
    if (peer_session == nullptr) {
        response->set_error(ERROE_CODR::SUCCESS);
        std::cout << "recver = " << msg->recv_id << " is not in current " << cfg["SelfServer"]["Name"] << std::endl;
        return Status::OK;
    }
    else {
        // 通知接收方收到消息
        std::cout << "receicer is " << msg->recv_id << std::endl;
        std::string ans = rtvalue.toStyledString();
        peer_session->Send(ans, ID_NOTIFY_CHAT_IMAGE_MSG);
    }

    response->set_error(SUCCESS);
    return Status::OK;
}

Status ChatServiceImpl::NotifyFriendIconChange(ServerContext* context, const NotifyFriendIconChangeReq* req, NotifyFriendIconChangeRsp* response)
{
    int uid = req->uid();
    int friend_id = req->friend_id();
    int redis_id = req->redis_id();
    std::string friend_icon = req->friend_icon();
    std::string message = req->messgae();
    auto cfg = ConfigManager::getInstance();
    auto session = UserManager::getInstance()->GetSession(uid);
    if (session == nullptr) {
        std::cout << "recver = " << friend_id << " is not in current " << cfg["SelfServer"]["Name"] << std::endl;
        response->set_error(ERROE_CODR::ERROR_USER_NOT_EXIST_IN_CHATSERVER);
        return Status::OK;
    }

    Json::Value rtvalue;
    rtvalue["uid"] = uid;
    rtvalue["friend_id"] = friend_id;
    rtvalue["redis_id"] = redis_id;
    rtvalue["friend_icon"] = friend_icon;
    rtvalue["code"] = SUCCESS;
    rtvalue["message"] = "friend icon change";
    session->Send(rtvalue.toStyledString(), ID_NOTIFY_FRIEND_ICON_CHANGE);

    response->set_error(ERROE_CODR::SUCCESS);
    return Status::OK;
}
