#ifndef MSGNODE_H
#define MSGNODE_H
#include "global.h"
class LogicSystem;
class MsgNode
{
	friend class LogicSystem;
public:
	MsgNode(short msg_len) : totol_len_(msg_len), cur_len_(0)
	{
		data_ = new char[totol_len_ + 1];
		data_[totol_len_] = '\0';
	}
	~MsgNode()
	{
		std::cout << "destructe MsgNode" << std::endl;
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
class RecvNode : public MsgNode
{
	friend class LogicSystem;
public:
	RecvNode(short max_len, short msg_id, std::string uuid);
	~RecvNode() = default;
	short msg_id_;
	std::string uuid_;
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
