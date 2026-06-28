#ifndef USERMANAGER_H
#define USERMANAGER_H

#include "global.h"
#include "userdata.h"
#include "chatthreaddata.h"

class UserManager : public QObject,
                    public std::enable_shared_from_this<UserManager>,
                    public SingleTon<UserManager>
{
    Q_OBJECT
public:
    friend class SingleTon<UserManager>;
    ~ UserManager();
    // 设置当前用户信息
    void setUserInfo(std::shared_ptr<UserInfo> user_info);
    void setToken(QString token);
    // 获取用户信息
    int getUid();
    QString getName();
    QString getNick();
    QString getIcon();
    QString getDesc();
    std::shared_ptr<UserInfo> getUserInfo();
    QString GetToken() { return token_; }
    // 设置当前登录用户的token
    void SetToken(QString token) { token_ = token; }
    // 初始化好友申请列表
    void appendApplyList(QJsonArray array);
    void appendApplyList(std::vector<std::shared_ptr<ApplyInfo>> apply_list);
    // 初始化好友列表
    void appendFriendList(std::vector<std::shared_ptr<FriendInfo>> friends);
    void appendFriendList(QJsonArray array);
    // 获取好友申请列表
    std::vector<std::shared_ptr<ApplyInfo>> getApplyList();
    // 获得好友列表
    std::vector<std::shared_ptr<FriendInfo>> getFriendList();
    // 添加好友申请到列表
    void addApplyList(std::shared_ptr<ApplyInfo> app);
    //
    bool alreadyApply(int uid);
    // 获取每个聊天界面
    std::vector<std::shared_ptr<FriendInfo>> getChatListPerPage();
    //
    bool isLoadChatFin();
    //
    void updateChatLoadedCount();
    //
    std::vector<std::shared_ptr<FriendInfo>> getConListPerPage();

    void updateContactLoadedCount();

    bool isLoadConFin();

    bool checkFriendById(int uid);

    void addFriend(std::shared_ptr<AuthRsp> auth_rsp);

    void addFriend(std::shared_ptr<AuthInfo> auth_info);

    std::shared_ptr<FriendInfo> getFriendById(int uid);

    void appendFriendChatMsg(int friend_id,std::vector<std::shared_ptr<TextChatData>>);

    // 通过thread_id获取对应的Chat_thread_data
    std::shared_ptr<ChatThreadData> GetChatThreadDataBuThreadID(int thread_id);

    // 通过好友的uid找到对应的thread_id
    int GetThreadIdByUid(int uid);
    // 通过好友的thread_id找到uid
    int GetUidByThreadId(int thread_id);
    // 添加ChatThreadData
    void addChatThreadData(std::shared_ptr<ChatThreadData> chatThreadData);
    std::vector<int> GetAllThreadID();
    // 设置上一次请求的最大的thread_id
    void setLastChatThreadID(int lastChatThreadID) { lastChatThreadID_ = lastChatThreadID; }
    int GetLastChatThreadID() { return lastChatThreadID_; }
    // 设置每一个thread_id对应的上次获取的msg_id
    void set_last_msg_id_for_thread_id(int thread_id,int msg_id);
    int getLastMsgIdByThreadId(int thread_id);
    // 设置上次获取的好友id
    void set_last_friend_id(int friend_id) { last_friend_id_ = friend_id; }
    int get_last_friend_id() { return last_friend_id_; }
    // 设置上次获取的好友请求id
    void set_last_friend_apply_id(int friend_apply_id) { last_friend_apply_id_ = friend_apply_id; }
    int get_last_friend_apply_id() { return last_friend_apply_id_; }

    // 设置上次获取的通知消息id
    //void set_last_notify_id(int notify_id) { last_notify_id_ = notify_id; }
    //int get_last_notify_id() { return last_notify_id_; }

    // 添加头像上传中 文件名字和文件路径的映射关系
    void add_file_path(QString name,QString file_path) { files_path_[name] = file_path; }
    QString get_file_path(QString md5);

    // 添加文件传输中 映射关系
    void add_trans_file(QString name,std::shared_ptr<MsgInfo> image) { trans_files_[name] = image; }
    std::shared_ptr<MsgInfo> get_trans_file(QString name);

    // 下载文件的信息
    void add_download_file(QString file, std::shared_ptr<DownloadFileInfo> file_info);
    std::shared_ptr<DownloadFileInfo> get_download_file(QString file);
    bool remove_download_file(QString file);

    // 添加本地文件和unique_name的映射关系
    void AddUniqueNameByLocalPath(QString local_path, QString unique_name);
    QString GetUniqueNameByLocalPath(QString local_path);


    // 将文件上传设置为暂停状态
    bool PauseTransFileByName(QString unique_name,  TRANSFER_STATE transfer_state);
    bool ResumeTransFileByName(QString unique_name);

    // 确认对方是否是我的好友
    bool isFriendByUid(int uid);
    // 确认对方的聊天线程是否创建了
    bool isCreateThreadByThreadID(int thread_id);

    // 修改好友的头像
    bool changeFriendIconByuid(int uid, QString icon);

    bool add_chat_image(QString unique_name, std::shared_ptr<ImageDataBase> image);
    std::shared_ptr<ImageDataBase> get_chat_image(QString unique_name);
    bool rm_chat_image(QString unique_name);

    void setSelfIcon(QString icon) { userInfo_->icon_ = icon; }

    void addCachedMsg(int thread_id, std::shared_ptr<ChatDataBase> data);
    void rmCacheMsg(int thread_id);
    std::vector<std::shared_ptr<ChatDataBase>> GetCacheMsg(int thread_id);

    int get_threadId_min_message_id(int thread_id);
    int get_threadId_max_message_id(int thread_id);
    void set_threadId_min_message_id(int thread_id,int min_message_id);
    void set_threadId_max_message_id(int thread_id,int max_message_id);

private:
    UserManager();
    // 当前用户信息
    std::shared_ptr<UserInfo> userInfo_;
    // 好友申请信息
    std::vector<std::shared_ptr<ApplyInfo>> applyList_;
    // 好友消息
    std::vector<std::shared_ptr<FriendInfo>> friendList_;
    QMap<int, std::shared_ptr<FriendInfo>> friendMap_;

    QString token_;
    int chatLoaded_;
    int contactLoaded_;
    int lastChatThreadID_;

    // 所有的thread_id
    std::vector<int> threadIds_;
    // thread_id 和 uid 的映射
    QMap<int,int> thread_ids_;
    // uid 和 好友 thread_id 的映射关系
    QMap<int,int> uid_to_thread_id_;
    //建立thread_id 到 聊天数据ChatThreadData的映射关系
    QMap<int, std::shared_ptr<ChatThreadData>> _chat_map;

    // 每一个thread_id对应的上次获取的msg_id
    QMap<int,int> thread_id_to_msg_id_;
    // 好友id
    int last_friend_apply_id_;
    // 好友申请id
    int last_friend_id_;
    // 头像文件的md5和文件路径
    std::map<QString,QString> files_path_;
    // 聊天图片的映射关系
    std::map<QString,std::shared_ptr<MsgInfo>> trans_files_;
    // 下载文件的名 和 文件信息的 映射关系
    std::map<QString,std::shared_ptr<DownloadFileInfo>> download_files_;
    // 聊天图片
    std::map<QString,std::shared_ptr<ImageDataBase>> chat_images_;
    // 缓存的消息
    QMultiMap<int,std::shared_ptr<ChatDataBase>> cache_;

    //
    QMap<int,int> min_message_ids_;
    QMap<int,int> max_message_ids_;

    QMap<QDateTime,int> time_chatthread_;

    QSet<QString> downloaded_files_;
    QSet<QString> uploaded_files_;

    QHash<QString,QString> unqiue_name_by_local_path_;

public slots:
    void slotAddFriendRsp(std::shared_ptr<AuthRsp> rsp);
    void slotAddFriendAuth(std::shared_ptr<AuthInfo> auth);
};

#endif // USERMANAGER_H
