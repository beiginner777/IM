#include "MsgNode.h"

RecvNode::RecvNode(short max_len, short msg_id, std::string uuid) 
	: MsgNode(max_len)
	, msg_id_(msg_id) 
	, uuid_(uuid) 
{
}

SendNode::SendNode(const char* msg, short max_len, short msg_id) : MsgNode(max_len + HEAD_TOTOL_LEN)
																	, msg_id_(msg_id)
{
	// 将 消息id 转化为 网络字节序 放在缓冲区中
	short msg_id_net = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
	::memcpy(data_, &msg_id_net, HEAD_ID_LEN);
	// 将 消息长度 转化为 网络字节序 放在缓冲区中
	short msg_len_net = boost::asio::detail::socket_ops::host_to_network_short(max_len);
	::memcpy(data_ + HEAD_ID_LEN, &msg_len_net, HEAD_DATA_LEN);
	// 将 消息实体 拷贝 到 缓冲区中
	::memcpy(data_ + HEAD_TOTOL_LEN, msg, max_len);
}

