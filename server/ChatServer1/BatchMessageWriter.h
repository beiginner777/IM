#ifndef BATCH_MESSAGE_WRITER_H
#define BATCH_MESSAGE_WRITER_H

#include "global.h"
#include "SingleTon.h"
#include <vector>
#include <thread>
#include <atomic>
#include <string>
struct ChatMessage;
// Redis-backed async batch writer.
// All instances share the same Redis queues: any server can consume and write to MySQL.
// Failed messages retry up to MAX_RETRIES, then move to dead-letter queue.
class BatchMessageWriter : public SingleTon<BatchMessageWriter>
{
	friend class SingleTon<BatchMessageWriter>;
public:
	BatchMessageWriter();
	~BatchMessageWriter();
	void start();
	void stop();
	// Serialize and LPUSH to Redis main queue
	void enqueue(std::shared_ptr<ChatMessage> msg);
	size_t totalWritten() const { return totalWritten_.load(); }
	size_t totalFailed()  const { return totalFailed_.load(); }
private:
	void flushWorker();
	void recoveryWorker();
	bool batchInsert(const std::vector<std::shared_ptr<ChatMessage>>& batch);
	// Push to backup queue (with retry count), pop from backup queue
	void pushToBackupQueue(const std::vector<std::shared_ptr<ChatMessage>>& batch);
	std::vector<std::shared_ptr<ChatMessage>> popBackupQueue();
	std::string serialize(const ChatMessage& msg) const;
	std::shared_ptr<ChatMessage> deserialize(const std::string& json) const;
	static constexpr int BATCH_SIZE          = 100;
	static constexpr int RECOVERY_INTERVAL_S = 10;
	static constexpr int MAX_RETRIES         = 3;   // cap retry attempts
	// Shared across all ChatServer instances
	static constexpr const char* MAIN_QUEUE_KEY   = "msg:queue:pending";
	static constexpr const char* BACKUP_QUEUE_KEY = "batch_msg_queue_failed";
	static constexpr const char* DEAD_QUEUE_KEY   = "batch_msg_queue_dead";
	std::thread flushThread_;
	std::thread recoveryThread_;
	std::atomic_bool bStop_{false};
	std::atomic<size_t> totalWritten_{0};
	std::atomic<size_t> totalFailed_{0};
};
#endif
