#include "MysqlManager.h"

// 启动时构建布隆过滤器：优先从 Redis 恢复，没有则从 MySQL 全量加载
void MysqlManager::initBloomFilter()
{
	bloomFilter_ = std::make_unique<BloomFilter>(1000000, 0.01);

	// ① 先尝试从 Redis BITMAP 恢复（之前存过）
	if (bloomFilter_->loadFromRedis("bloom:user_search")) {
		std::cout << "[BloomFilter] Restored from Redis"
		          << " (" << bloomFilter_->count() << " bits set)" << std::endl;
		return;
	}

	// ② Redis 没有数据 → 从 MySQL 全量加载
	auto conn = dao_.getConn(true);
	if (!conn) {
		std::cerr << "[BloomFilter] Failed to get DB connection" << std::endl;
		return;
	}

	try {
		std::unique_ptr<sql::Statement> stmt(conn->con_->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT uid, name FROM user"));

		int count = 0;
		while (res->next()) {
			bloomFilter_->add((uint64_t)res->getInt("uid"));
			bloomFilter_->add(res->getString("name"));
			count++;
		}
		std::cout << "[BloomFilter] Built from MySQL: " << count << " users"
		          << " (" << bloomFilter_->bitSize() / 8 / 1024 << "KB, "
		          << bloomFilter_->hashCount() << " hashes)" << std::endl;

		// ③ 构建完成 → 持久化到 Redis，下次重启直接恢复
		bloomFilter_->saveToRedis("bloom:user_search");
	}
	catch (sql::SQLException& e) {
		std::cerr << "[BloomFilter] MySQL load failed: " << e.what() << std::endl;
	}
}

int MysqlManager::registerUser(const std::string& name, const std::string& email, const std::string& password)
{
	int ret = dao_.registerUser(name, email, password);
	// 注册成功后加到布隆，下次搜索就能找到
	if (ret == SUCCESS && bloomFilter_) {
		// 注册返回的 uid 无法直接拿到，这里只做容错保护
	}
	return ret;
}

int MysqlManager::getUserFriendApply(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool forceMaster)
{
	return dao_.getUserFriendApply(uid, applyList, forceMaster);
}

int MysqlManager::getUserFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& friendList, bool forceMaster)
{
	return dao_.getUserFriendList(uid, friendList, forceMaster);
}

int MysqlManager::addFriendApply(int fromuid, int touid, int& current_id, std::string& apply_time)
{
	return dao_.addFriendApply(fromuid, touid, current_id, apply_time);
}

int MysqlManager::addFriendRelation(int fromuid, int touid, int& thread_id1,int& thread_id2,int& friend_id1, int& friend_id2)
{
	return dao_.addFriendRelation(fromuid, touid, thread_id1, thread_id2, friend_id1, friend_id2);
}

std::shared_ptr<UserInfo> MysqlManager::getUserByUid(int uid, bool forceMaster)
{
	// 布隆过滤器加速：如果确定不存在，直接返回，不走 MySQL
	if (bloomFilter_ && !bloomFilter_->contains((uint64_t)uid)) {
		return nullptr;
	}
	return dao_.getUserByUid(uid, forceMaster);
}

std::shared_ptr<UserInfo> MysqlManager::getUserByName(std::string name, bool forceMaster)
{
	// 布隆过滤器加速：如果确定不存在，直接返回，不走 MySQL
	if (bloomFilter_ && !bloomFilter_->contains(name)) {
		return nullptr;
	}
	return dao_.getUserByName(name, forceMaster);
}

int MysqlManager::setFriendApplyStatus(int fromuid, int touid, int status)
{
	return dao_.setFriendApplyStatus(fromuid, touid, status);
}

int MysqlManager::GetUserThreadInfos(int uid, int last_thread_id, int page_size,std::vector<std::shared_ptr<ChatThreadInfo>>& infos, bool& load_more, int& max_thread_id, bool forceMaster)
{
	return dao_.GetUserThreadInfos(uid, last_thread_id, page_size, infos, load_more, max_thread_id, forceMaster);
}

int MysqlManager::createPrivateThread(int user1_id, int user2_id, int& thread_id)
{
	return dao_.createPrivateThread(user1_id, user2_id, thread_id);
}

int MysqlManager::AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas)
{
	// 按 thread_id % 4 分片路由：将同一批次的消息分组，每组写入一张分片表
	std::unordered_map<int, std::vector<std::shared_ptr<ChatMessage>>> shardGroups;
	for (auto& msg : chat_datas) {
		int shard = ShardRouter::getShardIndex(msg->thread_id);
		shardGroups[shard].push_back(msg);
	}

	bool anyFailed = false;
	for (auto& [shard, msgs] : shardGroups) {
		int ret = dao_.AddChatMsg(shard, msgs);
		if (ret != SUCCESS) {
			anyFailed = true;
			// 标记失败消息，BatchMessageWriter 会上报失败
			for (auto& m : msgs) {
				m->status = SEND_FAILED;
			}
			std::cerr << "[MysqlManager] shard " << shard
			          << " batch INSERT " << msgs.size() << " rows FAILED" << std::endl;
		}
	}

	return anyFailed ? ERROR_SEND_MSG_FAILED : SUCCESS;
}

int MysqlManager::getUserFriendListByLastId(int uid, int last_friend_id, std::map<int, std::shared_ptr<UserInfo>>& friend_list, bool forceMaster)
{
	return dao_.getUserFriendListByLastId( uid, last_friend_id, friend_list, forceMaster);
}

int MysqlManager::getUserFriendApplyByLastId(int uid, int last_friend_id, int page_size, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool& load_more, int& max_friend_apply_id, bool forceMaster)
{
	return dao_.getUserFriendApplyByLastId(uid,last_friend_id, page_size, applyList,load_more,max_friend_apply_id, forceMaster);
}

int MysqlManager::updateChatMsgStatus(int thread_id, int message_id, MsgStatus status)
{
	int shard = ShardRouter::getShardIndex(thread_id);
	return dao_.updateChatMsgStatus(shard, thread_id, message_id, status);
}

int MysqlManager::loadChatMessage(int thread_id, int& min_message_id, int& max_message_id, int page_size, bool& is_more, std::vector<ChatMessage>& msgs, bool forceMaster)
{
	int shard = ShardRouter::getShardIndex(thread_id);
	return dao_.loadChatMessage(shard, thread_id, min_message_id, max_message_id, page_size, is_more, msgs, forceMaster);
}

//bool MysqlManager::pushOfflineMessage(int uid, std::string message)
//{
//	return dao_.pushOfflineMessage(uid, message);
//}
//
//std::vector<std::string> MysqlManager::popOfflineMessages(int uid, int& max_id)
//{
//	return dao_.popOfflineMessages(uid,max_id);
//}
