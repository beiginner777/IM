#ifndef CHATTHREADDATA_H
#define CHATTHREADDATA_H

#include "core/global.h"
#include "core/userdata.h"

//客户端本地存储的聊天线程数据结构
class ChatThreadData {
public:
    ChatThreadData(int other_id, int thread_id):
        _other_id(other_id), _thread_id(thread_id){}
    void AddMsg(std::shared_ptr<ChatDataBase> msg);
    void SetLastMsg(std::shared_ptr<ChatDataBase> base_msg) { _last_msg = base_msg; }
    std::shared_ptr<ChatDataBase> GetLastMsg();
    void SetOtherId(int other_id);
    int  GetOtherId();
    QString GetGroupName();
    QMap<int, std::shared_ptr<ChatDataBase>> GetMsgMap();
    int  GetThreadId();
    QMap<int, std::shared_ptr<ChatDataBase>>&  GetMsgMapRef();
    void AppendMsg(int msg_id, std::shared_ptr<ChatDataBase> base_msg);
    QMap<QString,std::shared_ptr<ChatDataBase>> GetUnrspMsg() { return unrsp_msg_map_; }
    void AppenUnrspMsg(QString uuid,std::shared_ptr<ChatDataBase> base_msg);
    // 将消息从未回复的状态更新
    void UpdateMsgStatus(std::shared_ptr<ChatDataBase> base_msg);

private:
    //如果是私聊，则为对方的id；如果是群聊，则为0
    int _other_id;
    // 对应的thread_id
    int _thread_id;
    //群聊信息,成员列表
    std::vector<int> _group_members;
    //群聊名称
    QString _group_name;
    // 缓存最后一条记录
    std::shared_ptr<ChatDataBase> _last_msg;
    // 缓存服务器已经发送的消息(msgid:value)
    QMap<int,std::shared_ptr<ChatDataBase>> msg_map_;
    // 缓存服务器还未完成发送的消息(unique_id:value)
    QMap<QString,std::shared_ptr<ChatDataBase>> unrsp_msg_map_;
};

#endif // CHATTHREADDATA_H
