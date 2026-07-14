#include "BatchMessageWriter.h"
#include "data.h"
#include "MysqlManager.h"
#include "RedisManager.h"
#include "AlertManager.h"
#include <json/json.h>
BatchMessageWriter::BatchMessageWriter()
{
}
BatchMessageWriter::~BatchMessageWriter()
{
	stop();
}

void BatchMessageWriter::start()
{
	bStop_ = false;
	flushThread_     = std::thread(&BatchMessageWriter::flushWorker, this);
	recoveryThread_  = std::thread(&BatchMessageWriter::recoveryWorker, this);
	std::cout << "[BatchWriter] Started (shared queues, maxRetries=" << MAX_RETRIES << ")" << std::endl;
}

void BatchMessageWriter::stop()
{
	bStop_ = true;
	if (flushThread_.joinable())    flushThread_.join();
	if (recoveryThread_.joinable()) recoveryThread_.join();
	std::cout << "[BatchWriter] Stopped. Written=" << totalWritten_.load()
	          << " Failed=" << totalFailed_.load() << std::endl;
}

void BatchMessageWriter::enqueue(std::shared_ptr<ChatMessage> msg)
{
	std::string json = serialize(*msg);
	RedisManager::getInstance()->LPush(MAIN_QUEUE_KEY, json);
}
// ==== flushWorker: RPOP from main queue -> batch insert to MySQL ====
void BatchMessageWriter::flushWorker()
{
	std::vector<std::shared_ptr<ChatMessage>> batch;
	batch.reserve(BATCH_SIZE);
	while (!bStop_) {
		auto redis = RedisManager::getInstance();
		std::string item = redis->RPop(MAIN_QUEUE_KEY);
		if (!item.empty()) {
			auto msg = deserialize(item);
			if (msg) batch.push_back(msg);
		}
		bool shouldFlush = (batch.size() >= BATCH_SIZE) || (item.empty() && !batch.empty());
		if (shouldFlush) {
			if (batchInsert(batch)) {
				totalWritten_ += batch.size();
			} else {
				totalFailed_ += batch.size();
				std::cerr << "[BatchWriter] INSERT failed -> backup queue" << std::endl;
				pushToBackupQueue(batch);
			}
			batch.clear();
		}
		if (item.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	// exit: drain remaining
	while (true) {
		auto redis = RedisManager::getInstance();
		std::string item = redis->RPop(MAIN_QUEUE_KEY);
		if (item.empty()) break;
		auto msg = deserialize(item);
		if (msg) batch.push_back(msg);
	}
	if (!batch.empty()) {
		if (batchInsert(batch)) {
			totalWritten_ += batch.size();
		} else {
			totalFailed_ += batch.size();
			pushToBackupQueue(batch);
		}
	}
}
// ==== recoveryWorker: retry failed messages every RECOVERY_INTERVAL_S seconds ====
void BatchMessageWriter::recoveryWorker()
{
	while (!bStop_) {
		for (int i = 0; i < RECOVERY_INTERVAL_S && !bStop_; i++) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		if (bStop_) break;
		auto pending = popBackupQueue();
		if (pending.empty()) continue;
		std::cout << "[BatchWriter] Recovery: retrying " << pending.size() << " msgs" << std::endl;
		if (batchInsert(pending)) {
			totalWritten_ += pending.size();
			std::cout << "[BatchWriter] Recovery OK: " << pending.size() << " msgs" << std::endl;
		} else {
			pushToBackupQueue(pending);
			std::cerr << "[BatchWriter] Recovery failed, retry in "
			          << RECOVERY_INTERVAL_S << "s" << std::endl;
		}
	}
}
// ==== batch INSERT ====
bool BatchMessageWriter::batchInsert(const std::vector<std::shared_ptr<ChatMessage>>& batch)
{
	auto mutableBatch = const_cast<std::vector<std::shared_ptr<ChatMessage>>&>(batch);
	int ret = MysqlManager::getInstance()->AddChatMsg(mutableBatch);
	return (ret == SUCCESS);
}
// ==== backup queue (with retry limit) ====
void BatchMessageWriter::pushToBackupQueue(const std::vector<std::shared_ptr<ChatMessage>>& batch)
{
	auto redis = RedisManager::getInstance();
	if (!redis) return;
	for (int i = (int)batch.size() - 1; i >= 0; i--) {
		Json::Value obj;
		Json::Reader reader;
		std::string existing = serialize(*batch[i]);
		if (reader.parse(existing, obj)) {
			int retryCount = obj.get("retry_count", 0).asInt();
			if (retryCount >= MAX_RETRIES) {
				// exceeded max retries -> dead letter queue, stop retrying
				redis->LPush(DEAD_QUEUE_KEY, existing);
				totalFailed_++;
				std::cerr << "[BatchWriter] msg " << batch[i]->unique_id
				          << " exceeded max retries -> dead letter queue" << std::endl;
				AlertManager::getInstance()->crit("[MySQL] Batch write failed " + std::to_string(MAX_RETRIES) + " times, msg " + batch[i]->unique_id + " -> dead letter queue");
				continue;
			}
			obj["retry_count"] = retryCount + 1;
			Json::FastWriter writer;
			redis->LPush(BACKUP_QUEUE_KEY, writer.write(obj));
		}
	}
}

std::vector<std::shared_ptr<ChatMessage>> BatchMessageWriter::popBackupQueue()
{
	std::vector<std::shared_ptr<ChatMessage>> result;
	auto redis = RedisManager::getInstance();
	if (!redis) return result;
	std::string item;
	while (!(item = redis->RPop(BACKUP_QUEUE_KEY)).empty()) {
		auto msg = deserialize(item);
		if (msg) result.push_back(msg);
	}
	return result;
}
// ==== JSON serialize / deserialize ====
std::string BatchMessageWriter::serialize(const ChatMessage& msg) const
{
	Json::Value obj;
	obj["message_id"] = msg.message_id;
	obj["thread_id"]  = msg.thread_id;
	obj["sender_id"]  = msg.sender_id;
	obj["recv_id"]    = msg.recv_id;
	obj["content"]    = msg.content;
	obj["chat_time"]  = msg.chat_time;
	obj["status"]     = msg.status;
	obj["type"]       = static_cast<int>(msg.type);
	obj["unique_id"]  = msg.unique_id;
	obj["retry_count"] = 0;  // first enqueue, retry count starts at 0
	Json::FastWriter writer;
	return writer.write(obj);
}

std::shared_ptr<ChatMessage> BatchMessageWriter::deserialize(const std::string& json) const
{
	Json::Reader reader;
	Json::Value obj;
	if (!reader.parse(json, obj)) return nullptr;
	auto msg = std::make_shared<ChatMessage>();
	msg->message_id = obj["message_id"].asInt();
	msg->thread_id  = obj["thread_id"].asInt();
	msg->sender_id  = obj["sender_id"].asInt();
	msg->recv_id    = obj["recv_id"].asInt();
	msg->content    = obj["content"].asString();
	msg->chat_time  = obj["chat_time"].asString();
	msg->status     = obj["status"].asInt();
	msg->type       = static_cast<CHAT_MSG_TYPE>(obj["type"].asInt());
	msg->unique_id  = obj["unique_id"].asString();
	return msg;
}
