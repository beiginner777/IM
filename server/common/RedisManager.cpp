#include "RedisManager.h"
#include "RedisLocker.h"
#include "Defer.h"
#include <sstream>

std::string RedisManager::Get(const std::string& key, bool forceMaster)
{
	auto connect_ = getConn();
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "GET %s", key.c_str());
	if (reply_ == nullptr)
	{
		std::cout << "[ GET " << key << " ] failed ." << std::endl;
		return std::string();
	}
	if (reply_->type != REDIS_REPLY_STRING)
	{
		std::cout << "[ GET " << key << " ] failed ." << std::endl;
		freeReplyObject(reply_);
		return std::string();
	}

	std::string value = reply_->str;
	freeReplyObject(reply_);

	std::cout << "Succeed to execute command [ GET " << key << " ]" << std::endl;
	return value;
}

bool RedisManager::MGet(const std::vector<std::string>& keys, std::unordered_map<std::string, std::string>& values, bool forceMaster)
{
	if (keys.empty()) {
		return false;
	}
	auto connect_ = getConn(forceMaster);
	if (!connect_) {
		std::cout << "MGet RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});

	// �Ȱ����� key �Ž� map��Ĭ�� ""
	for (const auto& key : keys) {
		values.emplace(key, "");
	}

	// ���� MGET ����
	std::string cmd = "MGET";
	for (const auto& key : keys) {
		cmd += (" " + key);
	}

	redisReply* reply = static_cast<redisReply*>(redisCommand(connect_, cmd.c_str()));

	if (!reply || reply->type != REDIS_REPLY_ARRAY) {
		if (reply) {
			freeReplyObject(reply);
		}
		return false;
	}

	// Redis ��֤��reply->elements == keys.size()
	for (size_t i = 0; i < reply->elements && i < keys.size(); ++i) {
		redisReply* elem = reply->element[i];

		if (elem && elem->type == REDIS_REPLY_STRING) {
			values[keys[i]] = std::string(elem->str, elem->len);
		}
	}
	freeReplyObject(reply);
	std::cout << "Succeed to execute command [ MGET ]" << std::endl;
	return true;
}

bool RedisManager::Set(const std::string& key, const std::string& value)
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "set %s %s", key.c_str(), value.c_str());
	if (reply_ == nullptr)
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		return false;  // ��Ҫ freeReplyObject
	}

	if (reply_->type != REDIS_REPLY_STATUS || strcmp(reply_->str, "OK") != 0)
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	freeReplyObject(reply_);
	std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
	return true;

}

bool RedisManager::SetExp(const std::string& key, const std::string& value, int expire_seconds) 
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});

	auto reply = (redisReply*)redisCommand(connect_, "SETEX %s %d %s", key.c_str(),
		expire_seconds,
		value.c_str());

	if (NULL == reply) {
		std::cout << "Execute command [ SETEX " << key << " " << expire_seconds
			<< " " << value << " ] failure ! " << std::endl;
		return false;
	}

	if (!(reply->type == REDIS_REPLY_STATUS &&
		(strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0))) {
		std::cout << "Execute command [ SETEX " << key << " " << expire_seconds
			<< " " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	std::cout << "Execute command [ SETEX " << key << " " << expire_seconds
		<< " " << value << " ] success ! " << std::endl;
	return true;
}

bool RedisManager::Auth(const std::string& password)
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "AUTH %s", password.c_str());
	if (reply_->type == REDIS_REPLY_ERROR)
	{
		std::cout << "AUTH failed: password: " << password << std::endl;
		freeReplyObject(reply_);
		return false;
	}
	freeReplyObject(reply_);
	std::cout << "AUTH Success: password: " << password << std::endl;
	return true;
}

bool RedisManager::LPush(const std::string& key, const std::string& value)
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "LPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply_)
	{
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	if (reply_->type != REDIS_REPLY_INTEGER || reply_->integer <= 0) {
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

std::string RedisManager::LPop(const std::string& key) 
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "LPOP %s ", key.c_str());
	if (reply_ == nullptr || reply_->type == REDIS_REPLY_NIL) {
		// queue empty, normal
		freeReplyObject(reply_);
		return std::string();
	}
	std::string value = reply_->str;
	std::cout << "Execut command [ LPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return value;
}

bool RedisManager::RPush(const std::string& key, const std::string& value) 
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "RPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply_)
	{
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	if (reply_->type != REDIS_REPLY_INTEGER || reply_->integer <= 0) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}

	std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

std::string RedisManager::RPop(const std::string& key) 
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "RPOP %s ", key.c_str());
	if (reply_ == nullptr || reply_->type == REDIS_REPLY_NIL) {
		// queue empty, normal
		freeReplyObject(reply_);
		return std::string();
	}
	std::string value = reply_->str;
	std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return value;
}

bool RedisManager::HSet(const std::string& key, const std::string& hkey, const std::string& value) 
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
	if (reply_ == nullptr || reply_->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

//bool RedisManager::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
//{
//	const char* argv[4];
//	size_t argvlen[4];
//	argv[0] = "HSET";
//	argvlen[0] = 4;
//	argv[1] = key;
//	argvlen[1] = strlen(key);
//	argv[2] = hkey;
//	argvlen[2] = strlen(hkey);
//	argv[3] = hvalue;
//	argvlen[3] = hvaluelen;
//	reply_ = (redisReply*)redisCommandArgv(connect_, 4, argv, argvlen);
//	if (reply_ == nullptr || reply_->type != REDIS_REPLY_INTEGER) {
//		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
//		freeReplyObject(reply_);
//		return false;
//	}
//	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
//	freeReplyObject(reply_);
//	return true;
//}

std::string RedisManager::HGet(const std::string& key, const std::string& hkey, bool forceMaster)
{
	auto connect_ = getConn(forceMaster);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	const char* argv[3];
	size_t argvlen[3];
	argv[0] = "HGET";
	argvlen[0] = 4;
	argv[1] = key.c_str();
	argvlen[1] = key.length();
	argv[2] = hkey.c_str();
	argvlen[2] = hkey.length();
	redisReply* reply_ = (redisReply*)redisCommandArgv(connect_, 3, argv, argvlen);
	if (reply_ == nullptr || reply_->type == REDIS_REPLY_NIL) {
		freeReplyObject(reply_);
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		return "";
	}

	std::string value = reply_->str;
	freeReplyObject(reply_);
	std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
	return value;
}

bool RedisManager::Del(const std::string& key)
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "DEL %s", key.c_str());
	if (reply_ == nullptr || reply_->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}
	std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

bool RedisManager::ExistsKey(const std::string& key, bool forceMaster)
{
	auto connect_ = getConn(forceMaster);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;  // bugfix: bool函数不应返回字符串字面量
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	redisReply* reply_ = (redisReply*)redisCommand(connect_, "exists %s", key.c_str());
	if (reply_ == nullptr || reply_->type != REDIS_REPLY_INTEGER || reply_->integer == 0) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		freeReplyObject(reply_);
		return false;
	}
	std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
	freeReplyObject(reply_);
	return true;
}

std::string RedisManager::acqueireLock(const std::string& lockName, int lockTimeOut, int expireTime)
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return "";
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	return RedisLocker::GetInstance()->acquireLock(connect_, lockName, lockTimeOut, expireTime);
}

bool RedisManager::releaseLock(const std::string& lockName, const std::string& lockValue)
{
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		returnConn(connect_);
		});
	return RedisLocker::GetInstance()->releaseLock(connect_, lockName, lockValue);
}

bool RedisManager::pushOfflineMessage(int uid, const std::string& message)
{
	// �� Redis���б� notify_message:uid ��������Ϣ message
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return false;
	}
	Defer defer([this, &connect_]() {
		masterPool_->returnConnection(connect_);
		});
	redisReply* reply = (redisReply*)redisCommand(connect_, "LPUSH notify_messages:%d %s", uid, message.c_str());
	if (reply == NULL) {
		std::cerr << "Error executing Redis command" << std::endl;
	}
	else {
		if (reply->type == REDIS_REPLY_ERROR) {
			std::cerr << "Redis error: " << reply->str << std::endl;
		}
		else {
			std::cout << "Offline message pushed to Redis: " << message << std::endl;
		}
	}
	return true;
}

std::vector<std::string> RedisManager::popOfflineMessages(int uid)
{
	// �� Redis���б� notify_message:uid �л�ȡ���е���Ϣ
	auto connect_ = getConn(true);
	if (connect_ == nullptr) {
		std::cout << "Get RedisConn failed.\n";
		return std::vector<std::string>();
	}
	Defer defer([this, &connect_]() {
		masterPool_->returnConnection(connect_);
		});

	// ����һ�� Lua �ű���һ���Ի�ȡ����ն���
	const char* lua_script =
		"local key = KEYS[1] "
		"local messages = redis.call('LRANGE', key, 0, -1) "
		"redis.call('DEL', key) "
		"return messages";

	redisReply* reply = (redisReply*)redisCommand(connect_,
		"EVAL %s 1 notify_messages:%d", lua_script, uid);

	std::vector<std::string> messages;
	if (reply && reply->type == REDIS_REPLY_ARRAY) {
		for (size_t i = 0; i < reply->elements; ++i) {
			if (reply->element[i]->str) {
				messages.push_back(reply->element[i]->str);
			}
		}
	}
	return messages;
}


// ============================================================================
// Distributed Message ID Generation
// Primary: Redis INCR (atomic, strictly increasing)
// Fallback: Snowflake (timestamp + server_id + sequence)
// ============================================================================

long long RedisManager::generateMsgId()
{
	// --- Path 1: Redis INCR (happy path) ---
	auto connect_ = getConn(true);
	if (connect_ != nullptr) {
		Defer defer([this, &connect_]() {
			returnConn(connect_);
		});

		redisReply* reply = (redisReply*)redisCommand(connect_, "INCR msg_id_counter");
		if (reply != nullptr && reply->type == REDIS_REPLY_INTEGER) {
			long long id = reply->integer;

			// WAIT for replication to at least 1 slave (durability guarantee)
			redisReply* waitReply = (redisReply*)redisCommand(connect_, "WAIT 1 100");
			if (waitReply != nullptr) {
				freeReplyObject(waitReply);
			}

			freeReplyObject(reply);
			return id;
		}
		if (reply != nullptr) {
			freeReplyObject(reply);
		}
	}

	// --- Path 2: Snowflake fallback (Redis unavailable) ---
	// 64-bit layout: [42 bits timestamp_ms][5 bits server_id][17 bits sequence]
	static constexpr long long SNOWFLAKE_EPOCH = 1700000000000LL; // ~Nov 2023
	static constexpr int SERVER_BITS = 5;
	static constexpr int SEQ_BITS    = 17;
	static constexpr long long MAX_SEQ = (1LL << SEQ_BITS) - 1;

	std::lock_guard<std::mutex> lock(snowflakeMutex_);

	long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	if (now == snowflakeLastMs_.load()) {
		// Same millisecond: increment sequence
		long long seq = snowflakeSequence_.fetch_add(1) + 1;
		if (seq > MAX_SEQ) {
			// Sequence exhausted this ms — spin-wait for next ms
			while (now <= snowflakeLastMs_.load()) {
				now = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch()).count();
			}
			snowflakeSequence_.store(0);
		}
	} else {
		snowflakeSequence_.store(0);
	}
	snowflakeLastMs_.store(now);

	long long seq = snowflakeSequence_.fetch_add(1);
	long long timestamp = now - SNOWFLAKE_EPOCH;
	long long id = (timestamp << (SERVER_BITS + SEQ_BITS))
	            | ((long long)(serverId_ & 0x1F) << SEQ_BITS)
	            | (seq & MAX_SEQ);

	return id;
}

// ============================================================================
// Sentinel 发现 Master 地址
// 逐个尝试 Sentinel，直到一个返回 Master 地址
// 失败返回空字符串，调用方回退到 config.ini 直连
// ============================================================================

static bool discoverMasterFromSentinel(std::string& outHost, std::string& outPort,
                                        const std::string& sentinelHosts, const std::string& sentinelPorts)
{
	// 解析 Sentinel 地址列表
	std::vector<std::string> hosts, ports;
	{
		std::istringstream hs(sentinelHosts), ps(sentinelPorts);
		std::string h, p;
		while (std::getline(hs, h, ',')) hosts.push_back(h);
		while (std::getline(ps, p, ',')) ports.push_back(p);
	}
	if (hosts.empty() || ports.empty()) return false;

	// 单个 Host 自动应用于全部 Port（常见于同一台机器部署多个 Sentinel）
	if (hosts.size() == 1 && ports.size() > 1) {
		std::string singleHost = hosts[0];
		hosts.resize(ports.size(), singleHost);
	}
	if (hosts.size() != ports.size()) return false;

	for (size_t i = 0; i < hosts.size(); i++) {
		redisContext* ctx = redisConnect(hosts[i].c_str(), std::stoi(ports[i]));
		if (ctx == nullptr || ctx->err) {
			if (ctx) redisFree(ctx);
			continue;
		}

		redisReply* reply = (redisReply*)redisCommand(ctx,
			"SENTINEL get-master-addr-by-name mymaster");
		if (reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
			outHost = reply->element[0]->str;
			outPort = reply->element[1]->str;
			freeReplyObject(reply);
			redisFree(ctx);
			std::cout << "[RedisManager] Discovered Master via Sentinel: "
			          << outHost << ":" << outPort << std::endl;
			return true;
		}
		if (reply) freeReplyObject(reply);
		redisFree(ctx);
	}
	return false;
}

RedisManager::RedisManager()
{
	auto cfg = ConfigManager::getInstance();
	std::string host = cfg["Redis"]["Host"];
	std::string port = cfg["Redis"]["Port"];
	std::string pwd  = cfg["Redis"]["Password"];

	// Master pool (default config.ini Host/Port)
	masterPool_ = std::make_unique<RedisConnPool>();
	std::cout << "[RedisManager] Master pool: " << host << ":" << port << std::endl;

	// Slave pools (for read scaling — optional, configurable via SlavePorts)
	std::string slavePorts = cfg["Redis"]["SlavePorts"];
	if (!slavePorts.empty()) {
		std::istringstream ps(slavePorts);
		std::string sp;
		while (std::getline(ps, sp, ',')) {
			slavePools_.push_back(std::make_unique<RedisConnPool>(host, sp, pwd));
			std::cout << "[RedisManager] Slave pool: " << host << ":" << sp << std::endl;
		}
	}
	if (slavePools_.empty()) {
		std::cout << "[RedisManager] No slave configured — reads fallback to master" << std::endl;
	}

	// Sentinel discovery (optional — verifies master address)
	std::string sentinelHost = cfg["Redis"]["SentinelHost"];
	std::string sentinelPort = cfg["Redis"]["SentinelPort"];
	std::string masterHost, masterPort;
	if (!sentinelHost.empty() && !sentinelPort.empty()
	    && discoverMasterFromSentinel(masterHost, masterPort, sentinelHost, sentinelPort)) {
		std::cout << "[RedisManager] Sentinel verified Master: " << masterHost
		          << ":" << masterPort << std::endl;
	}
}

// ============================================================================
// Read/Write routing helpers
// ============================================================================

static thread_local int tlsPoolIdx = -1;  // -1=master, >=0=slavePools_[N]

redisContext* RedisManager::getConn(bool forceMaster)
{
	if (forceMaster) {
		tlsPoolIdx = -1;
		std::lock_guard<std::mutex> lock(masterPoolMutex_);
		return masterPool_->getConnection();
	}
	// 读走 Slave：随机轮询，Slave 不可用时回退 Master
	if (slavePools_.empty()) {
		tlsPoolIdx = -1;
		std::lock_guard<std::mutex> lock(masterPoolMutex_);
		return masterPool_->getConnection();
	}
	static std::atomic<size_t> idx{0};
	size_t i = idx.fetch_add(1) % slavePools_.size();
	redisContext* conn = slavePools_[i]->getConnection();
	if (conn == nullptr) {
		tlsPoolIdx = -1;
		return masterPool_->getConnection();
	}
	tlsPoolIdx = (int)i;
	return conn;
}

void RedisManager::returnConn(redisContext* conn)
{
	if (conn == nullptr) return;
	if (tlsPoolIdx < 0 || (size_t)tlsPoolIdx >= slavePools_.size()) {
		masterPool_->returnConnection(conn);
	} else {
		slavePools_[tlsPoolIdx]->returnConnection(conn);
	}
}

// ============================================================================
// Sentinel Polling: detect master switch every 5s, auto-rebuild master pool
// ============================================================================

void RedisManager::startSentinelPoll()
{
	auto cfg = ConfigManager::getInstance();
	std::string sentinelHost = cfg["Redis"]["SentinelHost"];
	std::string sentinelPort = cfg["Redis"]["SentinelPort"];
	if (sentinelHost.empty() || sentinelPort.empty()) {
		std::cout << "[SentinelPoll] No Sentinel configured, skipping" << std::endl;
		return;
	}
	cachedMasterAddr_ = "127.0.0.1:" + cfg["Redis"]["Port"];
	sentinelPollStop_ = false;
	sentinelPollThread_ = std::thread(&RedisManager::sentinelPollWorker, this);
	std::cout << "[SentinelPoll] Started (interval=5s)" << std::endl;
}

void RedisManager::stopSentinelPoll()
{
	sentinelPollStop_ = true;
	if (sentinelPollThread_.joinable()) {
		sentinelPollThread_.join();
	}
}

void RedisManager::sentinelPollWorker()
{
	auto cfg = ConfigManager::getInstance();
	std::string sentinelHost = cfg["Redis"]["SentinelHost"];
	std::string sentinelPort = cfg["Redis"]["SentinelPort"];
	std::string masterHost, masterPort;

	while (!sentinelPollStop_) {
		for (int i = 0; i < 5 && !sentinelPollStop_; i++) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		if (sentinelPollStop_) break;

		// Query Sentinel for current master
		if (discoverMasterFromSentinel(masterHost, masterPort, sentinelHost, sentinelPort)) {
			std::string newAddr = masterHost + ":" + masterPort;
			if (newAddr != cachedMasterAddr_) {
				std::cout << "[SentinelPoll] Master changed: "
				          << cachedMasterAddr_ << " -> " << newAddr
				          << " — rebuilding master pool" << std::endl;
				// rebuild master pool (lock protected)
				{
					std::lock_guard<std::mutex> lock(masterPoolMutex_);
					masterPool_ = std::make_unique<RedisConnPool>();
					cachedMasterAddr_ = newAddr;
				}
				std::cout << "[SentinelPoll] Master pool rebuilt for " << newAddr << std::endl;
			}
		}
	}
}

RedisManager::~RedisManager()
{
	stopSentinelPoll();
}