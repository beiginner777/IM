#include "userdata.h"

SearchInfo::SearchInfo(int uid, QString name,
    QString nick, QString desc, int sex, QString icon):uid_(uid)
  ,name_(name), nick_(nick),desc_(desc),sex_(sex),icon_(icon){
}

AddFriendApply::AddFriendApply(int id,int fromuid, QString name,QString email, QString desc,
               QString icon, int sex,QString apply_time,int status)
    :id_(id), fromUid_(fromuid),name_(name),email_(email),desc_(desc),icon_(icon),sex_(sex),apply_time_(apply_time),status_(status)
{
}


void FriendInfo::AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData> > text_vec)
{
    for(const auto & text: text_vec){
      chatMsgs_.push_back(text);
    }
}
