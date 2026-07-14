#ifndef MYSQLDAO_H
#define MYSQLDAO_H
#include "global.h"
class UserInfo;
class ApplyInfo;
class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lastTime) :con_(con), lastOperationTime_(lastTime) {}
	std::unique_ptr<sql::Connection> con_;
	int64_t lastOperationTime_;
};
class MysqlConnPool : public SingleTon<MysqlConnPool>
{
	friend class SingleTon<MysqlConnPool>;
public:
	MysqlConnPool()
		: failedCount_(0)
	{
		auto cfg = ConfigManager::getInstance();
		host_ = cfg["Mysql"]["Host"];
		port_ = cfg["Mysql"]["Port"];
		user_ = cfg["Mysql"]["User"];
		password_ = cfg["Mysql"]["Password"];
		schema_ = cfg["Mysql"]["Schema"];
		this->init(host_, user_, port_, password_, schema_);
		checkThread_ = std::thread([this]() {
			int count = 0;
			while (!b_stop_) {
				if (count >= 60) {
					count = 0;
					checkConnectionPro();
				}
				std::this_thread::sleep_for(std::chrono::seconds(1));
				count++;
			}
			});
		checkThread_.detach();
	}
	// Parameterized constructor for Slave pool
	MysqlConnPool(const std::string& host, const std::string& user,
	              const std::string& port, const std::string& pwd, const std::string& schema)
		: failedCount_(0), host_(host), user_(user), port_(port), password_(pwd), schema_(schema)
	{
		this->init(host_, user_, port_, password_, schema_);
	}
	~MysqlConnPool()
	{
		b_stop_ = true;
		cond_.notify_all();
		std::unique_lock<std::mutex> locker(mtx_);
		while (!connections_.empty())
		{
			connections_.pop();
		}
		std::cout << "MysqlConnPool destructed ." << std::endl;
	}
	std::unique_ptr<SqlConnection> getConnection()
	{
		std::unique_lock<std::mutex> locker_(mtx_);
		while (!b_stop_ && connections_.empty())
		{
			if (std::cv_status::timeout == cond_.wait_for(locker_, std::chrono::milliseconds(100))){
				std::cout << "Get mysqlConn failed��mysqlPool is empty." << std::endl;
				return nullptr;
			}
			else{
				break;
			}
		}
		if (b_stop_)
		{
			std::cout << "Get mysqlConn failed��mysqlPool has stopped. " << std::endl;
			return nullptr;
		}
		std::unique_ptr<SqlConnection> conn = std::move(connections_.front());
		connections_.pop();
		std::cout << "Get MysqlConnection success.\n";
		return conn;
	}
	void returnConnection(std::unique_ptr<SqlConnection> conn)
	{
		if (b_stop_)
		{
			std::cout << "MysqlConn Service stop. " << std::endl;
			return;
		}
		std::unique_lock<std::mutex> locker_(mtx_);
		connections_.push(std::move(conn));
		cond_.notify_all();
		//std::cout << "return mysqlConn success, mysqlPool.size() = " << connections_.size() << std::endl;
	}
	void checkConnectionPro()
	{
		std::size_t targetCount = 0;
		{
			std::lock_guard<std::mutex> locker(mtx_);
			targetCount = connections_.size();
		}
		std::size_t processdCount = 0;
		auto now = std::chrono::system_clock::now().time_since_epoch();
		long long timeStamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();
		while (!b_stop_ && processdCount < targetCount)
		{
			std::unique_ptr<SqlConnection> conn;
			{
				std::lock_guard<std::mutex> locker(mtx_);
				if (connections_.empty()) {
					break;
				}
				conn = std::move(connections_.front());
				connections_.pop();
			}
			bool healthy = true;
			if (timeStamp - conn->lastOperationTime_ >= MYSQL_CONN_OVERTIME)
			{
				try{
					std::unique_ptr<sql::Statement> stmt(conn->con_->createStatement());
					stmt->executeQuery("select 1");
					conn->lastOperationTime_ = timeStamp;
				}
				catch (sql::SQLException& e) {
					std::cout << "MysqlConnectin is not alive.\n";
					healthy = false;
					failedCount_++;
				}
			}
			if (healthy) {
				returnConnection(std::move(conn));
			}
			else {
				std::cout << "processdCount = " << processdCount << " " << "failedCount = " << failedCount_ << std::endl;
			}
			++processdCount;
		}
		//std::cout << " === totol failedCount: " << failedCount_ << std::endl;
		while (failedCount_ > 0)
		{
			if (reconnection(timeStamp)) {
				std::cout << "MysqlConnection reconnects success.\n";
				failedCount_--;
			}
			else {
				std::cout << "MysqlConnecion reconnects failed." << std::endl;
				break;
			}
		}
	}
	bool reconnection(long long timeStamp)
	{
		try {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			std::string hostName = "tcp://" + host_ + ":" + port_;
			sql::Connection* conn = driver->connect(hostName, user_, password_);
			conn->setSchema(schema_);
			std::unique_ptr<SqlConnection> newconn = std::make_unique<SqlConnection>(conn, timeStamp);
			{
				std::lock_guard<std::mutex> locker(mtx_);
				connections_.push(std::move(newconn));
			}
			return true;
		}
		catch (sql::SQLException& e) {
			std::cout << "Reconnect MysqlConnection failed ,error message:" << e.what() << std::endl;
			return false;
		}
	}
private:
	void init(std::string host, std::string user, std::string port, std::string passwd, std::string schema, std::size_t conn_size = DEFAULT_MYSQL_CONN_SIZE)
	{
		std::string hostName = "tcp://" + host + ":" + port;
		std::cout << "Mysql hostName = " << hostName << std::endl;
		try {
			for (std::size_t i = 0; i < conn_size; ++i)
			{
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto conn = driver->connect(hostName, user, passwd);
				conn->setSchema(schema);
				std::cout << "Mysql Connect success.";
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				long long timeStamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
				connections_.push(std::make_unique<SqlConnection>(conn, timeStamp));
			}
		}
		catch (sql::SQLException& e)
		{
			std::cout << "mysql error: " << e.getErrorCode() << std::endl;
			std::cout << "mysql error message: " << e.what() << std::endl;
		}
	}
private:
	std::atomic_bool b_stop_;
	size_t pool_size_;
	std::string host_;
	std::string port_;
	std::string user_;
	std::string password_;
	std::string schema_;
	std::queue<std::unique_ptr<SqlConnection>> connections_;
	std::mutex mtx_;
	std::condition_variable cond_;
	std::atomic_int failedCount_;
	std::thread checkThread_;
};
class MysqlDao
{
	friend class MysqlManager;
public: 
	MysqlDao();
	~MysqlDao();
	int registerUser(const std::string& name, const std::string& email, const std::string& password);
	int getUserFriendApply(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool forceMaster = false);
	int getUserFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& friendList, bool forceMaster = false);
	int addFriendApply(int fromuid, int touid, int& current_id, std::string& apply_time);
	int addFriendRelation(int fromuid, int touid, int& thread_id1, int& thread_id2, int& friend_id1, int& friend_id2);
	std::shared_ptr<UserInfo> getUserByUid(int uid, bool forceMaster = false);
	std::shared_ptr<UserInfo> getUserByName(std::string name, bool forceMaster = false);
	int setFriendApplyStatus(int fromuid, int touid, int status);
	int GetUserThreadInfos(int uid, int last_thread_id, int page_size, std::vector<std::shared_ptr<ChatThreadInfo>>& infos, bool& load_more, int& max_thread_id, bool forceMaster = false);
	int createPrivateThread(int user1_id, int user2_id, int& thread_id);
	int AddChatMsg(int shardIndex, std::vector<std::shared_ptr<ChatMessage>>& chat_datas);
	int getUserFriendListByLastId(int uid, int last_friend_id, std::map<int, std::shared_ptr<UserInfo>>& friend_list, bool forceMaster = false);
	int getUserFriendApplyByLastId(int uid, int last_friend_id, int page_size, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool& load_more, int& max_friend_apply_id, bool forceMaster = false);
	int updateChatMsgStatus(int shardIndex, int thread_id, int message_id, MsgStatus status);
	int loadChatMessage(int shardIndex, int thread_id, int& min_message_id, int& max_message_id, int page_size, bool& is_more, std::vector<ChatMessage>& msgs, bool forceMaster = false);
private:
	// Read/write split helpers
	std::unique_ptr<SqlConnection> getConn(bool forceMaster = false);
	void returnConn(std::unique_ptr<SqlConnection> conn);
	std::unique_ptr<MysqlConnPool> masterPool_;
	std::unique_ptr<MysqlConnPool> slavePool_;
};
#endif
