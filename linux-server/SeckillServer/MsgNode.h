#ifndef MSGNODE_H
#define MSGNODE_H

#include "global.h"
// TCP 消息节点：[2字节 msg_id][2字节 msg_len]（网络字节序）+ body
// 参考 ChatServer 的 MsgNode，SeckillServer 只作为客户端与 StatusServer 通信，无需 uuid 头
class MsgNode
{
public:
	MsgNode(short msg_len) : totol_len_(msg_len), cur_len_(0)
	{
		data_ = new char[totol_len_ + 1];
		data_[totol_len_] = '\0';
	}
	~MsgNode()
	{
		delete[] data_;
	}
	void clear()
	{
		::memset(data_, 0, totol_len_);
		cur_len_ = 0;
	}
public:
	short cur_len_;
	short totol_len_;
	char* data_;
};
class SendNode : public MsgNode
{
public:
	SendNode(const char* msg, short max_len, short msg_id);
	~SendNode() = default;
	short GetMsgId() { return msg_id_; }
private:
	short msg_id_;
};
#endif
