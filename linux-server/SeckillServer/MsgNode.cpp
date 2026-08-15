#include "MsgNode.h"
SendNode::SendNode(const char* msg, short max_len, short msg_id)
	: MsgNode(max_len + HEAD_TOTOL_LEN)
	, msg_id_(msg_id)
{
	short msg_id_net = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
	short msg_len_net = boost::asio::detail::socket_ops::host_to_network_short(max_len);
	::memcpy(data_, &msg_id_net, HEAD_ID_LEN);
	::memcpy(data_ + HEAD_ID_LEN, &msg_len_net, HEAD_DATA_LEN);
	::memcpy(data_ + HEAD_TOTOL_LEN, msg, max_len);
}
