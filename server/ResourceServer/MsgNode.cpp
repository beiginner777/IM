#include "MsgNode.h"

RecvNode::RecvNode(int max_len, short msg_id) : MsgNode(max_len), msg_id_(msg_id)
{
}

SendNode::SendNode(const char* msg, int max_len, short msg_id) : MsgNode(max_len + HEAD_TOTOL_LEN)
																	, msg_id_(msg_id)
{
	// 将 消息id 转化为 网络字节序 放在缓冲区中
	short msg_id_net = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
	::memcpy(data_, &msg_id_net, HEAD_ID_LEN);
	// 将 消息长度 转化为 网络字节序 放在缓冲区中
	int msg_len_net = boost::asio::detail::socket_ops::host_to_network_long(max_len);
	::memcpy(data_ + HEAD_ID_LEN, &msg_len_net, HEAD_DATA_LEN);
	// 将 消息实体 拷贝 到 缓冲区中
	::memcpy(data_ + HEAD_TOTOL_LEN, msg, max_len);

	//std::cout << "id = " << msg_id << " " << "len = " << max_len;
}

