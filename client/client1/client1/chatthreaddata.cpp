#include "chatthreaddata.h"
#include "userdata.h"

void ChatThreadData::AddMsg(std::shared_ptr<ChatDataBase> msg)
{
    msg_map_.insert(msg->GetMsgId(), msg);
    if(_last_msg == nullptr || msg->GetMsgId() > _last_msg->GetMsgId()){
        _last_msg = msg;
    }
    //qDebug() << "Add Msg = " << msg->GetMsgContent();
}

std::shared_ptr<ChatDataBase> ChatThreadData::GetLastMsg()
{
    return _last_msg;
}

void ChatThreadData::SetOtherId(int other_id)
{
    _other_id = other_id;
}

int ChatThreadData::GetOtherId() {
    return _other_id;
}

QString ChatThreadData::GetGroupName()
{
    return _group_name;
}

QMap<int, std::shared_ptr<ChatDataBase>> ChatThreadData::GetMsgMap() {
    return msg_map_;
}

int ChatThreadData::GetThreadId()
{
    return _thread_id;
}

QMap<int, std::shared_ptr<ChatDataBase>>& ChatThreadData::GetMsgMapRef()
{
    return msg_map_;
}

void ChatThreadData::AppendMsg(int msg_id, std::shared_ptr<ChatDataBase> base_msg) {
    msg_map_.insert(msg_id, base_msg);
    _last_msg = base_msg;
}

// 存储服务器还未发送的消息
void ChatThreadData::AppenUnrspMsg(QString uuid,std::shared_ptr<ChatDataBase> base_msg)
{
    unrsp_msg_map_.insert(uuid,base_msg);
}

void ChatThreadData::UpdateMsgStatus(std::shared_ptr<ChatDataBase> base_msg)
{
    auto iter = unrsp_msg_map_.find(base_msg->GetUniqueId());
    if(iter == unrsp_msg_map_.end()){
        if(base_msg->GetMsgType() == CHAT_MSG_TYPE::TEXT_MSG){
            qDebug() << "[FATAL] find msg through unique_id failed: " << base_msg->GetUniqueId();
        }
    }else{
        //qDebug() << "[DEBUG] find msg through unique_id success: " << base_msg->GetUniqueId();
        unrsp_msg_map_.erase(iter);
    }
    // 添加到消息记录
    AddMsg(base_msg);
}
