#include "MsgNode.h"

RecvNode::RecvNode(short max_len, short msg_id, std::string uuid) 
	: MsgNode(max_len)
	, msg_id_(msg_id) 
	, uuid_(uuid) 
{
}

SendNode::SendNode(const char* msg, short max_len, short msg_id, std::string uuid)
	: MsgNode(max_len + (uuid.empty() ? HEAD_TOTOL_LEN : HEAD_TOTOL_LEN_WITH_UUID))
	, msg_id_(msg_id)
	, uuid_(uuid)
{
	short msg_id_net = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
	short msg_len_net = boost::asio::detail::socket_ops::host_to_network_short(max_len);
	if (!uuid.empty()) {
		::memcpy(data_, uuid.c_str(), HEAD_UUID_LEN);
		::memcpy(data_ + HEAD_UUID_LEN, &msg_id_net, HEAD_ID_LEN);
		::memcpy(data_ + HEAD_UUID_LEN + HEAD_ID_LEN, &msg_len_net, HEAD_DATA_LEN);
		::memcpy(data_ + HEAD_TOTOL_LEN_WITH_UUID, msg, max_len);
	} else {
		::memcpy(data_, &msg_id_net, HEAD_ID_LEN);
		::memcpy(data_ + HEAD_ID_LEN, &msg_len_net, HEAD_DATA_LEN);
		::memcpy(data_ + HEAD_TOTOL_LEN, msg, max_len);
	}
}

