#ifndef USERDATA_H
#define USERDATA_H

#include "global.h"

// 搜索到的用户信息
class SearchInfo {
public:
    SearchInfo() = default;
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex, QString icon);
    int uid_;
    QString name_;
    QString nick_;
    QString desc_;
    int sex_;
    QString icon_;
};
Q_DECLARE_METATYPE(SearchInfo)
Q_DECLARE_METATYPE(std::shared_ptr<SearchInfo>)

// 好友申请申请信息
class AddFriendApply {
public:
    AddFriendApply() = default;
    AddFriendApply(int id,int fromuid_, QString name,QString email, QString desc,
                   QString icon, int sex,QString apply_time_,int status);
    int id_;
    int fromUid_;
    QString name_;
    QString email_;
    QString desc_;
    QString nick_;
    QString icon_;
    int     sex_;
    QString apply_time_;
    int status_;
};
Q_DECLARE_METATYPE(AddFriendApply)
Q_DECLARE_METATYPE(std::shared_ptr<AddFriendApply>)

// 将上面的好友申请信息保存在好友申请界面中
struct ApplyInfo {
    ApplyInfo() = default;
    ApplyInfo(int uid, QString name, QString desc,
        QString icon, QString nick, int sex, int status)
        :uid_(uid),name_(name),desc_(desc),
        icon_(icon),nick_(nick),sex_(sex),status_(status){}

    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo)
        : id_(addinfo->id_),uid_(addinfo->fromUid_),name_(addinfo->name_),
          desc_(addinfo->desc_),icon_(addinfo->icon_),
          nick_(addinfo->nick_),sex_(addinfo->sex_),
          status_(false)
    {}

    ApplyInfo(int id, int uid, QString name, QString email, QString desc,
        QString icon, int sex, QString apply_time, int status)
        : id_(id), uid_(uid), name_(name), email_(email), desc_(desc),
        icon_(icon), sex_(sex), apply_time_(apply_time), status_(status)
    {
    }

    void SetIcon(QString head){
        icon_ = head;
    }

    int id_;
    QString email_;
    QString apply_time_;
    int uid_;
    QString name_;
    QString desc_;
    QString icon_;
    QString nick_;
    int sex_;
    int status_;
};
Q_DECLARE_METATYPE(ApplyInfo)
Q_DECLARE_METATYPE(std::shared_ptr<ApplyInfo>)

struct AuthInfo {
    AuthInfo() = default;
    AuthInfo(int uid, QString name,
             QString nick, QString icon, int sex):
        uid_(uid), name_(name), nick_(nick), icon_(icon),
        sex_(sex){}
    int uid_;
    QString name_;
    QString nick_;
    QString icon_;
    int sex_;
};
Q_DECLARE_METATYPE(AuthInfo)
Q_DECLARE_METATYPE(std::shared_ptr<AuthInfo>)

// 同意好友申请之后，服务器传过来的对方的信息
struct AuthRsp {
    AuthRsp() = default;
    AuthRsp(int peeruid_, QString peername_,
            QString peernick_, QString peericon_, int peersex_)
        :uid_(peeruid_),name_(peername_),nick_(peernick_),
          icon_(peericon_),sex_(peersex_)
    {}

    int uid_;
    QString name_;
    QString nick_;
    QString icon_;
    int sex_;
};
Q_DECLARE_METATYPE(AuthRsp)
Q_DECLARE_METATYPE(std::shared_ptr<AuthRsp>)

struct TextChatData;
struct FriendInfo {
    FriendInfo() = default;

    FriendInfo(int id,int uid, QString name, QString nick, QString icon,
        int sex, QString desc, QString email,QString back = "", QString last_msg=""):id_(id), uid_(uid),
        name_(name),nick_(nick),icon_(icon),sex_(sex),desc_(desc),email_(email),
        back_(back),lastMsg_(last_msg){}

    FriendInfo(int uid, QString name, QString nick, QString icon,
        int sex, QString desc, QString back = "", QString last_msg=""):uid_(uid),
        name_(name),nick_(nick),icon_(icon),sex_(sex),desc_(desc),
        back_(back),lastMsg_(last_msg){}

    FriendInfo(std::shared_ptr<AuthInfo> auth_info):uid_(auth_info->uid_),
    nick_(auth_info->nick_),icon_(auth_info->icon_),name_(auth_info->name_),
      sex_(auth_info->sex_){}

    FriendInfo(std::shared_ptr<AuthRsp> auth_rsp):uid_(auth_rsp->uid_),
    nick_(auth_rsp->nick_),icon_(auth_rsp->icon_),name_(auth_rsp->name_),
      sex_(auth_rsp->sex_){}

    void AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData>> text_vec);

    int id_;
    int uid_;
    QString name_;
    QString nick_;
    QString icon_;
    int sex_;
    QString email_;
    QString desc_;
    QString back_;
    QString lastMsg_;
    std::vector<std::shared_ptr<TextChatData>> chatMsgs_;
};
Q_DECLARE_METATYPE(FriendInfo)
Q_DECLARE_METATYPE(std::shared_ptr<FriendInfo>)

// 消息类的基类
class ChatDataBase {
public:
    ChatDataBase() = default;
    virtual ~ChatDataBase() = default;
    ChatDataBase(int msg_id, QString unique_id, int thread_id, CHAT_THREAD_TYPE form_type, CHAT_MSG_TYPE msg_type, QString content,
                 int send_uid,int recv_id, int status, QDateTime chat_time = QDateTime())
        : _msg_id(msg_id),_unique_id(unique_id),_thread_id(thread_id),_form_type(form_type),_msg_type(msg_type),
          _msg_content(content),_status(status),_send_uid(send_uid),_recv_uid_(recv_id),_chat_time(chat_time)
    {
    }
    int GetMsgId() { return _msg_id; }
    int GetThreadId() { return _thread_id; }
    CHAT_THREAD_TYPE GetFormType() { return _form_type; }
    CHAT_MSG_TYPE GetMsgType() { return _msg_type; }
    QString GetContent() { return _msg_content; }
    int GetSendUid() { return _send_uid; }
    QString GetMsgContent(){return _msg_content;}
    QString GetUniqueId() { return _unique_id; }
    int GetStatus() { return _status; }
    QDateTime GetChatTime() { return _chat_time; }
    void SetUniqueId(int unique_id);
//private:
    // 消息的唯一标识
    QString _unique_id;
    // 消息id
    int _msg_id;
    // 会话id
    int _thread_id;
    // 群聊还是私聊
    CHAT_THREAD_TYPE _form_type;
    // 文本信息为0，图片为1，文件为2
    CHAT_MSG_TYPE _msg_type;
    // 聊天信息
    QString _msg_content;
    // 发送者id
    int _send_uid;
    // 接收者id
    int _recv_uid_;
    // 消息状态
    int _status;
    // 消息发送的时间
    QDateTime _chat_time;
};
Q_DECLARE_METATYPE(ChatDataBase)
Q_DECLARE_METATYPE(std::shared_ptr<ChatDataBase>)
Q_DECLARE_METATYPE(std::vector<std::shared_ptr<ChatDataBase>>)

// 文本消息类
class TextChatData : public ChatDataBase {
public:
    TextChatData() = default;
    TextChatData(int msg_id, QString unique_id, int thread_id, CHAT_THREAD_TYPE form_type, CHAT_MSG_TYPE msg_type, QString content,
        int send_uid, int recv_uid, int status, QDateTime chat_time = QDateTime()) :
        ChatDataBase(msg_id, unique_id, thread_id, form_type, msg_type, content, send_uid, recv_uid, status, chat_time)
    {

    }

};
Q_DECLARE_METATYPE(TextChatData)
Q_DECLARE_METATYPE(std::shared_ptr<TextChatData>)

// 图片消息类
class ImageDataBase : public ChatDataBase
{
public:
    ImageDataBase(std::shared_ptr<MsgInfo> msg_info, int message_id, QString unique_id,
        int thread_id, CHAT_THREAD_TYPE form_type, CHAT_MSG_TYPE msg_type,
        int send_uid, int recv_uid, int status, QDateTime chat_time):
        ChatDataBase(message_id,unique_id,thread_id, form_type, msg_type, msg_info->unique_name_,
            send_uid, recv_uid, status, chat_time)
        ,msg_info_(msg_info)
    {
    }
    std::shared_ptr<MsgInfo> msg_info_;
};
Q_DECLARE_METATYPE(std::shared_ptr<ImageDataBase>)

// 文件消息类
class FileDataBase : public ChatDataBase
{
public:
    FileDataBase(std::shared_ptr<MsgInfo> msg_info, int message_id, QString unique_id,
        int thread_id, CHAT_THREAD_TYPE form_type, CHAT_MSG_TYPE msg_type,
        int send_uid, int recv_uid, int status, QDateTime chat_time):
        ChatDataBase(message_id,unique_id,thread_id, form_type, msg_type, msg_info->text_or_url_,
            send_uid, recv_uid, status, chat_time), msg_info_(msg_info)
    {
    }
    std::shared_ptr<MsgInfo> msg_info_;
};

struct UserInfo {
    UserInfo() = default;
    UserInfo(int uid, QString name, QString nick, QString icon, int sex,QString desc="", QString last_msg = "" ):
        uid_(uid),name_(name),nick_(nick),icon_(icon),sex_(sex),lastMsg_(last_msg),desc_(desc){}

    UserInfo(int uid, QString name, QString email, QString pwd,QString nick, QString icon, int sex,QString desc,QString last_msg = ""):
        uid_(uid),name_(name),email_(email),pwd_(pwd),nick_(nick),icon_(icon),sex_(sex),desc_(desc),lastMsg_(last_msg){}


    UserInfo(std::shared_ptr<AuthInfo> auth):
        uid_(auth->uid_),name_(auth->name_),nick_(auth->nick_),
        icon_(auth->icon_),sex_(auth->sex_),lastMsg_(""),desc_(""){}

    UserInfo(int uid, QString name, QString icon):
    uid_(uid), name_(name), icon_(icon),nick_(name_),
    sex_(0),lastMsg_(""),desc_(""){

    }

    UserInfo(std::shared_ptr<AuthRsp> auth):
        uid_(auth->uid_),name_(auth->name_),nick_(auth->nick_),
        icon_(auth->icon_),sex_(auth->sex_),lastMsg_(""){}

    UserInfo(std::shared_ptr<SearchInfo> search_info):
        uid_(search_info->uid_),name_(search_info->name_),nick_(search_info->nick_),
    icon_(search_info->icon_),sex_(search_info->sex_),lastMsg_(""){

    }

    UserInfo(std::shared_ptr<FriendInfo> friend_info):
        uid_(friend_info->uid_),name_(friend_info->name_),nick_(friend_info->nick_),
        icon_(friend_info->icon_),sex_(friend_info->sex_),lastMsg_(""){
            chatMsgs_ = friend_info->chatMsgs_;
        }

    int uid_;
    QString name_;
    QString email_;
    QString pwd_;
    QString nick_;
    QString icon_;
    int sex_;
    QString desc_;
    QString lastMsg_;
    std::vector<std::shared_ptr<TextChatData>> chatMsgs_;
};
Q_DECLARE_METATYPE(UserInfo)
Q_DECLARE_METATYPE(std::shared_ptr<UserInfo>)

// 聊天线程信息
struct ChatThreadInfo
{
    ChatThreadInfo() = default;
    ChatThreadInfo(int threadId, int type,int user1_id,int user2_id)
        : threadId_(threadId), threadType_(type), user1_id_(user1_id), user2_id_(user2_id){
    }
    int threadId_;
    int threadType_;
    int user1_id_;
    int user2_id_;
};
Q_DECLARE_METATYPE(ChatThreadInfo)
Q_DECLARE_METATYPE(std::shared_ptr<ChatThreadInfo>)

// 下载文件的信息类
class DownloadFileInfo
{
public:
    DownloadFileInfo(QString download_file = "", int seq = 0, int last_seq = 0, int trans_size = 0, int total_size = 0,
        QString client_save_path = "",Download_File_Type type = Download_File_Type::NONE, TRANSFER_STATE state = TRANSFER_STATE::None)
        :download_file_(download_file), seq_(seq), last_seq_(last_seq), trans_size_(trans_size), total_size_(total_size),
        client_save_path_(client_save_path), type_(type), state_(state)
    {
    }
    QString download_file_;
    int seq_;
    int last_seq_;
    int trans_size_;
    int total_size_;
    QString client_save_path_;
    Download_File_Type type_;
    TRANSFER_STATE state_;
};
Q_DECLARE_METATYPE(DownloadFileInfo)
Q_DECLARE_METATYPE(std::shared_ptr<DownloadFileInfo>)

enum Message_Type {
    TEXT = 0,
    IMAGE = 1,
    EMOJI = 2,
};
Q_DECLARE_METATYPE(Message_Type)

struct ChatMessage
{
    ChatMessage() = default;
    ChatMessage(int message_id,int thread_id,int sender_id,int recv_id,QString content,QString chat_time,int status,int message_type)
        : message_id_(message_id),thread_id_(thread_id),sender_id_(sender_id),recv_id_(recv_id),content_(content), chat_time_(chat_time),status_(status),message_type_(message_type)
    {

    }
    int message_id_;
    int thread_id_;
    int sender_id_;
    int recv_id_;
    QString content_;
    QString chat_time_;
    int status_;
    int message_type_;
};
Q_DECLARE_METATYPE(ChatMessage)

#endif

