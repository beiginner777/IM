#ifndef REDISMANAGER
#define REDISMANAGER

// 可以为队列中的共享智能指针指定一个删除器
// std::shared_ptr<X> p1;
// std::shared_ptr<X> p2(p1.get(),删除器)

#include "global.h"

class RedisConnPool
{
public:
	RedisConnPool()
		: failedCount_(0)
	{
		// std::cout << "construct RedisConnPool ." << std::endl;

		auto cfg = ConfigManager::getInstance();
		host_ = cfg["Redis"]["Host"];
		port_ = cfg["Redis"]["Port"].c_str();
		password_ = cfg["Redis"]["Password"];

		/*
		std::cout << "Host: " << Host << std::endl;
		std::cout << "Port: " << Port << std::endl;
		std::cout << "Password: " << Password << std::endl;
		*/

		this->init(host_, port_, password_);
	}

	~RedisConnPool()
	{
		b_stop_ = true;
		cond_.notify_all();
		std::unique_lock<std::mutex> locker(mtx_);
		while (!connections_.empty())
		{
			// to do ... （释放上下文）
			connections_.pop();
		}
		std::cout << "RedisConnPool destructed ." << std::endl;
	}

	redisContext* getConnection()
	{
		std::unique_lock<std::mutex> locker_(mtx_);
		while (!b_stop_ && connections_.empty())
		{
			if (std::cv_status::timeout == cond_.wait_for(locker_, std::chrono::milliseconds(100)))
			{
				std::cout << "redisConn Server busy." << std::endl;
				return nullptr;
			}
			else
			{
				break;
			}
		}
		if (b_stop_)
		{
			std::cout << "redisConn Service stop. " << std::endl;
			return nullptr;
		}
		redisContext* conn = connections_.front();
		connections_.pop();
		return conn;
	}

	void returnConnection(redisContext* context)
	{
		if (b_stop_)
		{
			std::cout << "redisConn Service stop. " << std::endl;
			return;
		}
		std::unique_lock<std::mutex> locker_(mtx_);
		connections_.push(std::move(context));
		cond_.notify_all();
	}

	bool reconnect() 
	{
		auto context = redisConnect(host_.c_str(), atoi(port_.c_str()));
		if (context == nullptr || context->err != 0) {
			if (context != nullptr) {
				redisFree(context);
			}
			return false;
		}
		auto reply = (redisReply*)redisCommand(context, "AUTH %s", password_);
		if (reply->type == REDIS_REPLY_ERROR) {
			//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
			freeReplyObject(reply);
			redisFree(context);
			return false;
		}

		//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
		freeReplyObject(reply);
		returnConnection(context);
		return true;
	}

	void checkThreadPro() 
	{
		std::size_t pool_size = 0;
		{
			// 先拿到当前连接数
			std::lock_guard<std::mutex> lock(mtx_);
			pool_size = connections_.size();
		}
		for (int i = 0; i < pool_size && !b_stop_; ++i) {
			redisContext* ctx = nullptr;
			// 1) 取出一个连接(持有锁)
			bool healthy = true;
			redisContext* context = getConnection();
			if (context == nullptr) {
				break;
			}

			redisReply* reply = nullptr;
			try {
				reply = (redisReply*)redisCommand(context, "PING");
				// 2. 先看底层 I/O／协议层有没有错
				if (context->err) {
					std::cout << "Connection error: " << context->err << std::endl;
					if (reply) {
						freeReplyObject(reply);
					}
					redisFree(context);
					failedCount_++;
					continue;
				}

				// 3. 再看 Redis 自身返回的是不是 ERROR
				if (!reply || reply->type == REDIS_REPLY_ERROR) {
					std::cout << "reply is null, redis ping failed: " << std::endl;
					if (reply) {
						freeReplyObject(reply);
					}
					redisFree(context);
					failedCount_++;
					continue;
				}
				// 4. 如果都没问题，则还回去
				std::cout << "connection alive" << std::endl;
				freeReplyObject(reply);
				returnConnection(context);
			}
			catch (std::exception& exp) {
				if (reply) {
					freeReplyObject(reply);
				}

				redisFree(context);
				failedCount_++;
			}

		}

		//执行重连操作
		while (failedCount_ > 0) {
			auto res = reconnect();
			if (res) {
				failedCount_--;
			}
			else {
				//留给下次再重试
				break;
			}
		}
	}

private:
	void init(std::string host, std::string port, std::string pwd, std::size_t conn_size = DEFAULT_REDIS_CONN_SIZE)
	{
		pool_size_ = conn_size;
		host_ = host;
		port_ = port;
		b_stop_ = false;

		for (size_t i = 0; i < conn_size; ++i)
		{
			auto connect = redisConnect(host.c_str(),atoi(port.c_str()));
			if (connect == nullptr || connect->err != 0)
			{
				if (connect == nullptr)
				{
					continue;
				}
				std::cout << "connect redis failed: " << connect->errstr << std::endl;
				exit(0);
			}
			auto reply = (redisReply*)redisCommand(connect, "AUTH %s", pwd.c_str());
			if (reply->type == REDIS_REPLY_ERROR)
			{
				std::cout << "AUTH failed in RedisConnPoll." << std::endl;
				freeReplyObject(reply);
				continue;
			}

			freeReplyObject(reply);

			// std::cout << i + 1 << (i > 0 ? " connections":" conncetion") << " connect and auth success." << std::endl;

			connections_.push(connect);
		}
		// std::cout << "construct RedisConnPool finish !" << std::endl;
	}
private:
	std::atomic_bool b_stop_;
	size_t pool_size_;
	std::string host_;
	std::string port_;
	std::string password_;
	std::queue<redisContext*> connections_;
	std::mutex mtx_;
	std::condition_variable cond_;
	std::atomic_int failedCount_;

	std::thread checkThread_;
};


class RedisManager : public SingleTon<RedisManager>
{
	friend class SingleTon<RedisManager>;
public:
	RedisManager();
	~RedisManager();
	std::string Get(const std::string& key);
	bool Set(const std::string& key,const std::string& value);
	bool Auth(const std::string& password);
	bool LPush(const std::string& key, const std::string& value);
	std::string LPop(const std::string& key);
	bool RPush(const std::string& key, const std::string& value);
	std::string RPop(const std::string& key);
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	// bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string HGet(const std::string& key, const std::string& hkey);
	bool Del(const std::string& key);
	bool ExistsKey(const std::string& key);

	std::string acqueireLock(const std::string& lockName, int lockTimeOut, int expireTime);
	bool releaseLock(const std::string& lockName, const std::string& lockValue);

private:
	std::unique_ptr<RedisConnPool> pool_;
};

#endif