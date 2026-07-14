#ifndef MSGNODE_H
#define MSGNODE_H

#include "global.h"
class LogicSystem;
class CSession;
class MsgNode
{
	friend class LogicWorker;
public:
	// 传进来的参数是：消息实体的长度
	MsgNode(short msg_len) : totol_len_(msg_len), cur_len_(0)
	{
		data_ = new char[totol_len_ + 1];
		data_[totol_len_] = '\0';
	}
	~MsgNode()
	{
		std::cout << "destructe MsgNode" << std::endl;
		delete[] data_; // new出来的空间，需要手动释放
	}
	// 清空缓冲区
	void clear() 
	{
		::memset(data_, 0, totol_len_);
		cur_len_ = 0;
	}
public:
	// 当前接收 或者 处理的消息长度
	int cur_len_;
	// 需要接收 或者 需要处理的消息的总长度
	int totol_len_;
	// 消息缓冲区
	char* data_;
};
// 接收消息的结点
class RecvNode : public MsgNode
{
	friend class LogicWorker;
public:
	RecvNode(int max_len, short msg_id);
	~RecvNode() = default;
private:
	short msg_id_;
};
// 发送消息的结点
class SendNode : public MsgNode
{
public:
	SendNode(const char* msg, int max_len, short msg_id);
	~SendNode() = default;
private:
	short msg_id_;
};
#endif