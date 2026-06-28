#ifndef DATA_H
#define DATA_H

#include "global.h"

struct UserInfo {
	UserInfo() :name_(""), pwd_(""), uid_(), email_(""), nick_(""), desc_(""), sex_(0), icon_(""), back_("") 
	{
	}
	UserInfo(int uid,std::string name, std::string email,
		std::string pwd, std::string desc, std::string icon,
		int sex,std::string nick)
		:uid_(uid), name_(name), email_(email),
		pwd_(pwd), desc_(desc), icon_(icon),
		sex_(sex),nick_(nick)
	{
	}

	std::string name_;
	std::string pwd_;
	int uid_;
	std::string email_;
	std::string nick_;
	std::string desc_;
	int sex_;
	std::string icon_;
	std::string back_;
};

struct ApplyInfo {
	ApplyInfo(int uid, std::string name, std::string desc,
		std::string icon, std::string nick, int sex, int status = 0)
		:uid_(uid), name_(name), desc_(desc),
		icon_(icon), nick_(nick), sex_(sex), status_(status) {
	}

	ApplyInfo(int id, int uid, std::string name, std::string email, std::string desc,
		std::string icon, int sex, std::string apply_time, int status)
		: id_(id), uid_(uid), name_(name), email_(email), desc_(desc),
		icon_(icon), sex_(sex), apply_time_(apply_time), status_(status)
	{
	}

	int id_;
	int uid_;
	std::string name_;
	std::string email_;
	std::string desc_;
	std::string icon_;
	std::string nick_;
	int sex_;
	std::string apply_time_;
	int status_;
};

// 聊天线程信息
struct ChatThreadInfo
{
	ChatThreadInfo() {}
	ChatThreadInfo(int threadId, int type,int user1_id,int user2_id) 
		: threadId_(threadId), threadType_(type), user1_id_(user1_id), user2_id_(user2_id){
	}
	int threadId_;
	int threadType_;
	int user1_id_;
	int user2_id_;
};

enum CHAT_MSG_TYPE
{
	TEXT_MSG, // 文本消息
	PIC_MSG, // 图片信息
	EMOJI, //表情消息
	FILE_MSG // 文件信息
};

// 聊天消息信息
struct ChatMessage {
	int message_id; // 消息id
	int thread_id; // 线程id
	int sender_id; // 发送者id
	int recv_id; // 接收者id
	std::string unique_id; // uuid
	std::string content; // 消息内容
	std::string chat_time; // 消息时间
	int status; // 消息状态（0 1 2）
	CHAT_MSG_TYPE type; // 消息类型（文本，图片等）
	/*
	status : 
	UN_READ = 0,  // 正在发送
    SEND_FAILED = 1,  //发送失败
    READED = 2,  // 发送成功
	*/
};

enum MsgStatus {
	UN_READ = 0,  // 正在发送
	SEND_FAILED = 1,  //发送失败
	READED = 2,  // 上传成功
	//UN_UPLOAD = 3 // 未上传完成
};

#endif