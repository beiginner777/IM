#include "MysqlDao.h"
#include "data.h"
#include "crypto/BCryptHasher.h"

MysqlDao::MysqlDao()
{
    auto cfg = ConfigManager::getInstance();
    std::string host  = cfg["Mysql"]["Host"];
    std::string port  = cfg["Mysql"]["Port"];
    std::string user  = cfg["Mysql"]["User"];
    std::string pwd   = cfg["Mysql"]["Password"];
    std::string schema = cfg["Mysql"]["Schema"];

    // Master pool (config.ini Host/Port)
    masterPool_ = std::make_unique<MysqlConnPool>();
    std::cout << "[MysqlDao] Master pool: " << host << ":" << port << std::endl;

    // Slave pool (optional)
    std::string slaveHost = cfg["Mysql"]["SlaveHost"];
    std::string slavePort = cfg["Mysql"]["SlavePort"];
    if (!slaveHost.empty() && !slavePort.empty()) {
        slavePool_ = std::make_unique<MysqlConnPool>(slaveHost, user, slavePort, pwd, schema);
        std::cout << "[MysqlDao] Slave pool: " << slaveHost << ":" << slavePort << std::endl;
    }
}

std::unique_ptr<SqlConnection> MysqlDao::getMasterConn()
{
	return masterPool_->getConnection();
}

std::unique_ptr<SqlConnection> MysqlDao::getSlaveConn()
{
	if (!slavePool_) {
		return masterPool_->getConnection();
	}
	auto conn = slavePool_->getConnection();
	if (!conn) {
		return masterPool_->getConnection();
	}
	return conn;
}

MysqlDao::~MysqlDao()
{
}

int MysqlDao::registerUser(const std::string& name, const std::string& email, const std::string& password)
{
	std::unique_ptr<SqlConnection> conn = getMasterConn();

    if (conn == nullptr) {
        std::cout << "mysqlConn is nullptr, register failed.\n";
        return ERROR_REGISTER;
    }

    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });

	try {
		std::unique_ptr < sql::PreparedStatement > stmt(conn->con_->prepareStatement("CALL reg_user(?,?,?,@result)"));

		stmt->setString(1, name);
		stmt->setString(2, email);
		// bcrypt 哈希后再存储
		std::string hashedPwd = BCryptHasher::generateHash(password, 10);
		stmt->setString(3, hashedPwd);

		stmt->execute();

		std::unique_ptr<sql::Statement> stmtResult(conn->con_->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));

        int result = res->next();

		if (result > 0) {
			int result = res->getInt("result");
			std::cout << "注册用户 uid: " << result << std::endl;
			return SUCCESS;
        }
        else if (result == ERROR_NAME_EXIST) {
            int result = res->getInt("result");
            std::cout << "Result: " << result << "," << "Message: Name exists\n";
            return result;
        }
        else if (result == ERROR_EMAIL_EXIST) {
            int result = res->getInt("result");
            std::cout << "Result: " << result << "," << "Message: Email exists\n";
            return result;
        }
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return ERROR_REGISTER;
	}
}

int MysqlDao::addFriendApply(int fromuid, int touid, int& current_id, std::string& apply_time)
{
    auto con = getMasterConn();
    if (con == nullptr) {
        return ERROR_FRIEND_APPLY;
    }

    Defer defer([this, &con]() {
        masterPool_->returnConnection(std::move(con));
        });

    try {
		con->con_->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> checkStmt(
            con->con_->prepareStatement(
                "SELECT id FROM friendapply WHERE fromuid = ? AND touid = ? AND status = 0 LIMIT 1"
            )
        );
        checkStmt->setInt(1, fromuid);
        checkStmt->setInt(2, touid);

        std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());
        if (checkRes->next()) {
            std::cout << "Friend apply already exists fromuid = " << fromuid
                << " to touid = " << touid << std::endl;
            return ERROR_MULTIPLE_FRIEND_APPLY;
        }

        std::unique_ptr<sql::PreparedStatement> insertStmt(
            con->con_->prepareStatement(
                "INSERT INTO friendapply (fromuid, touid, status, apply_time) "
                "VALUES (?, ?, 0, NOW())"
            )
        );
        insertStmt->setInt(1, fromuid);
        insertStmt->setInt(2, touid);

        int rowAffected = insertStmt->executeUpdate();
        if (rowAffected <= 0) {
            std::cerr << "Failed to insert friend apply, rowAffected = " << rowAffected << std::endl;
            return ERROR_FRIEND_APPLY;
        }

        std::unique_ptr<sql::PreparedStatement> getKeyStmt(
            con->con_->prepareStatement(
                "SELECT LAST_INSERT_ID() AS id, "
                "       (SELECT apply_time FROM friendapply WHERE id = LAST_INSERT_ID()) AS apply_time"
            )
        );
        std::unique_ptr<sql::ResultSet> res(getKeyStmt->executeQuery());

        if (res->next()) {
            current_id = res->getInt("id");
            std::string apply_time = res->getString("apply_time");

            std::cout << "Inserted friend apply with ID: " << current_id
                << ", apply_time: " << apply_time << std::endl;
        }
        else {
            std::cerr << "Failed to get last insert ID" << std::endl;
            return ERROR_FRIEND_APPLY;
        }

		con->con_->commit();

        return SUCCESS;
    }
    catch (sql::SQLException& e) {
		con->con_->rollback();

        std::cerr << "SQLException in addFriendApply: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;

        if (e.getErrorCode() == 1062) { // ER_DUP_ENTRY
            return ERROR_MULTIPLE_FRIEND_APPLY;
        }
        return ERROR_FRIEND_APPLY;
    }
    catch (const std::exception& e) {
        con->con_->rollback();
        std::cerr << "Exception in addFriendApply: " << e.what() << std::endl;
        return ERROR_FRIEND_APPLY;
    }
}

int MysqlDao::getUserFriendApply(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList)
{
    auto con = getSlaveConn();
    if (con == nullptr) {
        return ERROR_GET_FRIEND_APPLY_LIST;
    }

    Defer defer([this, &con]() {
        masterPool_->returnConnection(std::move(con));
        });


    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT u.uid,u.name,u.email,u.desc,u.icon, u.sex,u.nick,f.fromuid FROM friendapply f JOIN user u ON f.fromuid = u.uid WHERE f.touid = ?;"));

        pstmt->setInt(1, uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next()) {
            auto name = res->getString("name");
            auto uid = res->getInt("fromuid");
            auto nick = res->getString("nick");
            auto sex = res->getInt("sex");
            auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex);
            applyList.push_back(apply_ptr);
        }
        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return ERROR_GET_FRIEND_APPLY_LIST;
    }
}

int MysqlDao::getUserFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& friendList)
{
    auto con = getSlaveConn();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        masterPool_->returnConnection(std::move(con));
        });


    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("select * from friend where self_id = ? "));

        pstmt->setInt(1, uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while (res->next()) {
            auto friend_id = res->getInt("friend_id");
            auto back = res->getString("back");
            auto user_info = getUserByUid(friend_id);
            if (user_info == nullptr) {
                continue;
            }
            user_info->back_ = user_info->name_;
            friendList.push_back(user_info);
        }
        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return ERROR_GET_FRIEND_LIST;
    }

    return true;
}

int MysqlDao::addFriendRelation(int fromuid, int touid, int& thread_id1, int& thread_id2, int& friend_id1, int& friend_id2)
{
    auto conn = getMasterConn();
    if (conn == nullptr) {
        return ERROR_ADD_FRIEND_RELATION;
    }

    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });

    try {
        conn->con_->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->con_->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id) "
            "VALUES (?, ?) "
        ));
        pstmt->setInt(1, fromuid); // from id
        pstmt->setInt(2, touid); // to uid

        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
			std::cout << "addfriend insert friend failed, uid1 = " << fromuid << ", uid2 = " << touid << std::endl;
            conn->con_->rollback();
            return ERROR_ADD_FRIEND_RELATION;
        }else {
            std::unique_ptr<sql::PreparedStatement> getKeyStmt(
                conn->con_->prepareStatement(
                    "SELECT LAST_INSERT_ID() AS friend1_id"
                )
            );
            std::unique_ptr<sql::ResultSet> res(getKeyStmt->executeQuery());
            if (res->next()) {
                friend_id1 = res->getInt("friend1_id");
                std::cout << "Inserted friend with ID: " << friend_id1 << std::endl;
            }
            else {
                std::cerr << "Failed to get last insert ID" << std::endl;
                return ERROR_FRIEND_APPLY;
            }
        }

        std::unique_ptr<sql::PreparedStatement> pstmt2(conn->con_->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id) "
            "VALUES (?, ?) "
        ));
        pstmt2->setInt(1, touid);
        pstmt2->setInt(2, fromuid);

        int rowAffected2 = pstmt2->executeUpdate();
        if (rowAffected2 <= 0) {
			std::cout << "addfriend insert friend failed, uid1 = " << touid << ", uid2 = " << fromuid << std::endl;
            conn->con_->rollback();
            return ERROR_ADD_FRIEND_RELATION;
        }
        else {
            std::unique_ptr<sql::PreparedStatement> getKeyStmt(
                conn->con_->prepareStatement(
                    "SELECT LAST_INSERT_ID() AS friend_id2"
                )
            );
            std::unique_ptr<sql::ResultSet> res(getKeyStmt->executeQuery());
            if (res->next()) {
                friend_id2 = res->getInt("friend_id2");
				std::cout << "Inserted friend with ID: " << friend_id2 << std::endl;
            }else {
                std::cerr << "Failed to get last insert ID" << std::endl;
                return ERROR_FRIEND_APPLY;
            }
        }

		int ret = createPrivateThread(fromuid, touid, thread_id1);
        if(ret != SUCCESS) {
            conn->con_->rollback();
            return ERROR_ADD_FRIEND_RELATION;
		}
		thread_id2 = thread_id1;
        conn->con_->commit();
        std::cout << "addfriend insert friends success" << std::endl;

        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        if (conn) {
            conn->con_->rollback();
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return ERROR_ADD_FRIEND_RELATION;
    }
}

std::shared_ptr<UserInfo> MysqlDao::getUserByUid(int uid)
{
    auto con = getSlaveConn();
    if (con == nullptr) {
        return nullptr;
    }

    Defer defer([this, &con]() {
        masterPool_->returnConnection(std::move(con));
        });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT * FROM user WHERE uid = ?"));

        pstmt->setInt(1, uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        while (res->next()) {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd_ = res->getString("password");
            user_ptr->email_ = res->getString("email");
            user_ptr->name_ = res->getString("name");
            user_ptr->nick_ = res->getString("nick");
            user_ptr->desc_ = res->getString("desc");
            user_ptr->sex_ = res->getInt("sex");
            user_ptr->icon_ = res->getString("icon");
            user_ptr->uid_ = uid;
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MysqlDao::getUserByName(std::string name)
{
    auto con = getSlaveConn();
    if (con == nullptr) {
        return nullptr;
    }

    Defer defer([this, &con]() {
        masterPool_->returnConnection(std::move(con));
        });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        while (res->next()) {
            user_ptr.reset(new UserInfo);
            user_ptr->pwd_ = res->getString("pwd");
            user_ptr->email_ = res->getString("email");
            user_ptr->name_ = res->getString("name");
            user_ptr->nick_ = res->getString("nick");
            user_ptr->desc_ = res->getString("desc");
            user_ptr->sex_ = res->getInt("sex");
            user_ptr->icon_ = res->getString("icon");
            user_ptr->uid_ = res->getInt("uid");
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

int MysqlDao::setFriendApplyStatus(int fromuid, int touid, int status)
{
    auto conn = getMasterConn();
    if (conn == nullptr) {
        std::cout << "setFriendApplyStatus failed: fromuid=" << fromuid << " touid=" << touid << std::endl;
        return ERROR_MODIFLY_APPLY_STATUS_FAILED;
    }

    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });

    try {
        conn->con_->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->con_->prepareStatement("UPDATE friendapply SET status = ? WHERE fromuid = ? AND touid = ? ;"));
        pstmt->setInt(1, status);
        pstmt->setInt(2, fromuid);
        pstmt->setInt(3, touid);

        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            conn->con_->rollback();
            return ERROR_MODIFLY_APPLY_STATUS_FAILED;
        }

        conn->con_->commit();

        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        conn->con_->rollback();
        return ERROR_MODIFLY_APPLY_STATUS_FAILED;
    }
}

int MysqlDao::GetUserThreadInfos(int uid, int last_thread_id, int page_size, std::vector<std::shared_ptr<ChatThreadInfo>>& infos, bool& load_more, int& max_thread_id)
{
    auto conn = getSlaveConn();

	max_thread_id = last_thread_id;

    if (!conn) {
        std::cout << "uid = " << uid << "GetUserThreadInfos failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });

    try
    {
        if (conn->con_ == nullptr) {
            std::cout << "conn->con_ is nullptr.\n";
        }
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->con_->prepareStatement(
            "WITH all_threads AS ( "
            "  SELECT thread_id, 'private' AS type, user1_id, user2_id "
            "    FROM privatechat "
            "   WHERE (user1_id = ? OR user2_id = ?) "
            "     AND thread_id > ? "
            "  UNION ALL "
            "  SELECT thread_id, 'group'   AS type, 0 AS user1_id, 0 AS user2_id "
            "    FROM groupchatmember "
            "   WHERE user_id   = ? "
            "     AND thread_id > ? "
            ") "
            "SELECT thread_id, type, user1_id, user2_id "
            "  FROM all_threads "
            " ORDER BY thread_id "
            " LIMIT ?;"
        ));

        int idx = 1;
        pstmt->setInt64(idx++, uid);              // private.user1_id
		pstmt->setInt64(idx++, uid);              // private.user2_id
        pstmt->setInt64(idx++, last_thread_id);              // private.thread_id > lastId
        pstmt->setInt64(idx++, uid);              // group.user_id
        pstmt->setInt64(idx++, last_thread_id);              // group.thread_id > lastId
        pstmt->setInt(idx++, page_size + 1);          // LIMIT pageSize+1

		std::unique_ptr < sql::ResultSet > res(pstmt->executeQuery());

        std::vector<std::shared_ptr<ChatThreadInfo>> tmp;
        while (res->next()) {
            auto cti = std::make_shared<ChatThreadInfo>();
            cti->threadId_ = res->getInt("thread_id");
            cti->threadType_ = res->getString("type") == "private" ? CHAT_THREAD_TYPE_PRIVATE : CHAT_THREAD_TYPE_GROUP;
            cti->user1_id_ = res->getInt("user1_id");
            cti->user2_id_ = res->getInt("user2_id");
            tmp.push_back(cti);
        }

        if ((int)tmp.size() > page_size) {
            load_more = true;
            tmp.pop_back();
        }
        else {
			load_more = false;
        }
        infos = std::move(tmp);

        if (!infos.empty()) {
            max_thread_id = infos.back()->threadId_;
        }

    }catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return ERROR_LOAD_CHAT_THREAD;
	}
    return SUCCESS;
}

int MysqlDao::createPrivateThread(int user1_id, int user2_id, int& thread_id)
{
    auto conn = getMasterConn();
    if (!conn) {
        std::cout << "uid = " << user1_id << "GetUserThreadInfos failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });

    try {
		conn->con_->setAutoCommit(false);

		// 2. 获取新的 thread_id
		std::unique_ptr<sql::PreparedStatement> pstmt1(conn->con_->prepareStatement
        ("INSERT INTO chatthread(type, created_at) VALUES ('private',NOW());"));
		pstmt1->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> pstmt2(conn->con_->prepareStatement
		("SELECT LAST_INSERT_ID() AS thread_id;"));
		std::unique_ptr<sql::ResultSet> res(pstmt2->executeQuery());
        res->next();
		thread_id = res->getInt("thread_id");

        int minn = std::min(user1_id, user2_id);
		int maxx = std::max(user1_id, user2_id);

		std::string insert_sql = "INSERT INTO privatechat(thread_id, user1_id, user2_id) VALUES (?,?,?);";
		std::unique_ptr<sql::PreparedStatement> pstmt3(conn->con_->prepareStatement(insert_sql));
        pstmt3->setInt(1, thread_id);
		pstmt3->setInt(2, minn);
		pstmt3->setInt(3, maxx);
		pstmt3->executeUpdate();

		conn->con_->commit();

		return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;

		std::cout << "Create private thread failed for users " << user1_id << " and " << user2_id << std::endl;
		conn->con_->rollback();

        return ERROR_MODIFLY_APPLY_STATUS_FAILED;
    }
}

int MysqlDao::AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas)
{
    auto conn = getMasterConn();
    if (!conn) {
        std::cout << "Add ChatMsg failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });


    size_t n = chat_datas.size();
    if (n == 0) {
        return SUCCESS;
    }

    try {
        conn->con_->setAutoCommit(false);

        // 单条 SQL + 多行 VALUES：INSERT INTO ... VALUES (r1), (r2), ... (rN)
        // N 条消息只发一次 MySQL 请求，减少 N-1 次网络往返
        std::string insertSql = "INSERT INTO chatmessage "
            "(message_id, thread_id, sender_id, recv_id, content, "
            " created_at, updated_at, status, message_type, client_msg_id) VALUES ";
        for (size_t i = 0; i < n; i++) {
            if (i > 0) insertSql += ", ";
            insertSql += "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        }

        auto pstmt = conn->con_->prepareStatement(insertSql);

        int idx = 1;
        for (auto& msg : chat_datas) {
            pstmt->setInt(idx++, msg->message_id);
            pstmt->setInt(idx++, msg->thread_id);
            pstmt->setInt(idx++, msg->sender_id);
            pstmt->setInt(idx++, msg->recv_id);
            pstmt->setString(idx++, msg->content);
            pstmt->setString(idx++, msg->chat_time);
            pstmt->setString(idx++, msg->chat_time);
            pstmt->setInt(idx++, msg->status);
            pstmt->setInt(idx++, static_cast<int>(msg->type));
            pstmt->setString(idx++, msg->unique_id);
        }
        pstmt->executeUpdate();

        std::cout << "[MysqlDao] Batch INSERT " << n << " rows OK" << std::endl;
        conn->con_->commit();
        return SUCCESS;

    }
    catch (sql::SQLException& e)
    {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        for (auto& msg : chat_datas) {
            msg->status = 1;
        }
        conn->con_->rollback();
        // 备份重试由 BatchMessageWriter 统一管理，MysqlDao 只负责返回错误
        return ERROR_SEND_MSG_FAILED;
    }
}

int MysqlDao::getUserFriendListByLastId(int uid, int last_friend_id, std::map<int, std::shared_ptr<UserInfo>>& friend_list)
{
    auto conn = getSlaveConn();
    if (!conn) {
        std::cout << "get friend list failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });

    try {
        conn->con_->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->con_->prepareStatement("SELECT id, friend_id FROM friend WHERE id > ? AND self_id = ?")
        );

        std::cout << "SELECT id, friend_id FROM friend WHERE id > " << last_friend_id << " AND self_id = " << uid << std::endl;

        pstmt->setInt(1, last_friend_id);
        pstmt->setInt(2, uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next()) {
            int friendId = res->getInt("friend_id");
            std::unique_ptr<sql::PreparedStatement> userStmt(
                conn->con_->prepareStatement("SELECT uid, name, email, password, nick, icon, sex, `desc` FROM user WHERE uid = ?")
            );
            userStmt->setInt(1, friendId);

            std::unique_ptr<sql::ResultSet> userRes(userStmt->executeQuery());

            if (userRes->next()) {
                int uid = userRes->getInt("uid");
                std::string name = userRes->getString("name");
                std::string email = userRes->getString("email");
                std::string password = userRes->getString("password");
                std::string nick = userRes->getString("nick");
                std::string icon = userRes->getString("icon");
                int sex = userRes->getInt("sex");
                std::string desc = userRes->getString("desc");

                std::shared_ptr<UserInfo> userInfo = std::make_shared<UserInfo>(uid, name, email, password, nick, icon, sex, desc);

                int friendRecordId = res->getInt("id");
                friend_list[friendRecordId] = userInfo;
            }
        }
        conn->con_->commit();
        return SUCCESS;

    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;

        conn->con_->rollback();

        return ERROR_LOAD_MORE_FRIEND;
    }
}

int MysqlDao::getUserFriendApplyByLastId(int uid, int last_friend_id, int page_size, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool& load_more, int& max_friend_apply_id)
{
    auto conn = getSlaveConn();
    if (!conn) {
        std::cout << "get friend apply list failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });

    try {
        conn->con_->setAutoCommit(false);

        std::string query = R"(
            SELECT id, fromuid, apply_time, status
            FROM friendapply
            WHERE touid = ? AND id > ?
            ORDER BY id ASC
            LIMIT ?)";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->con_->prepareStatement(query));
        pstmt->setInt(1, uid);
        pstmt->setInt(2, last_friend_id);
        pstmt->setInt(3, page_size + 1);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        int record_count = 0;
        max_friend_apply_id = last_friend_id;

        while (res->next()) {
            record_count++;
            if (record_count > page_size) {
                break;
            }

            int apply_id = res->getInt("id");
            int fromuid = res->getInt("fromuid");
            std::string apply_time = res->getString("apply_time");
            int status = res->getInt("status");

            std::string user_query = R"(
                SELECT name, email, `desc`, icon, sex, uid
                FROM user
                WHERE uid = ?)";

            std::unique_ptr<sql::PreparedStatement> user_pstmt(conn->con_->prepareStatement(user_query));
            user_pstmt->setInt(1, fromuid);

            std::unique_ptr<sql::ResultSet> user_res(user_pstmt->executeQuery());

            if (user_res->next()) {
                std::shared_ptr<ApplyInfo> apply_info = std::make_shared<ApplyInfo>(
                    apply_id,
                    fromuid,
                    user_res->getString("name"),
                    user_res->getString("email"),
                    user_res->getString("desc"),
                    user_res->getString("icon"),
                    user_res->getInt("sex"),
                    apply_time,
                    status
                );

                applyList.push_back(apply_info);
                max_friend_apply_id = apply_id;
            }
        }

        if (record_count > page_size) {
            load_more = true;
        }
        else {
            load_more = false;
        }

        conn->con_->commit();
        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;

        conn->con_->rollback();
        return ERROR_LOAD_FRIEND_APPLY;
    }
}

int MysqlDao::updateChatMsgStatus(int message_id, MsgStatus status)
{
    auto conn = getMasterConn();
    if (!conn) {
        std::cout << "get friend apply list failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });

    try {
        conn->con_->setAutoCommit(false);

        std::string query = R"(
            UPDATE chatmessage
            SET status = ?
            WHERE message_id = ? )";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->con_->prepareStatement(query));
        pstmt->setInt(1, (int)status);
        pstmt->setInt(2, message_id);

        int rowAffected = pstmt->executeUpdate();
        if (rowAffected <= 0) {
            conn->con_->rollback();
            return ERROR_MODIFY_MSG_STATUS;
        }
        std::cout << "Update messgae_id = " << message_id << " status is " << status << " success.\n";
        conn->con_->commit();
        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        conn->con_->rollback();
        return ERROR_MODIFY_MSG_STATUS;
    }
}

int MysqlDao::loadChatMessage(int thread_id, int& min_message_id, int& max_message_id, int page_size, bool& is_more, std::vector<ChatMessage>& msgs)
{
    auto conn = getSlaveConn();
    if (!conn) {
        std::cout << "get friend apply list failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        masterPool_->returnConnection(std::move(conn));
        });

    try {
        conn->con_->setAutoCommit(false);

        std::string query = "SELECT message_id, sender_id, recv_id, content, created_at, updated_at, status, message_type "
            "FROM chatmessage "
            "WHERE thread_id = ? AND message_id > ? AND message_id <= ? "
            "ORDER BY message_id ASC LIMIT ?";

        std::unique_ptr<sql::PreparedStatement> stmt(conn->con_->prepareStatement(query));
        stmt->setInt(1, thread_id);
        stmt->setInt(2, min_message_id);
        stmt->setInt(3, max_message_id);
        stmt->setInt(4, page_size + 1);

        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        msgs.clear();
        bool has_more = false;
        int count = 0;
        while (res->next()) {
            count++;
            if (count > page_size) {
                has_more = true;
                break;
            }
            ChatMessage msg;
            msg.message_id = res->getInt("message_id");
            msg.thread_id = thread_id;
            msg.sender_id = res->getInt("sender_id");
            msg.recv_id = res->getInt("recv_id");
            msg.content = res->getString("content");
            msg.chat_time = res->getString("created_at");
            msg.status = res->getInt("status");
            msg.type = static_cast<CHAT_MSG_TYPE>(res->getInt("message_type"));

            msgs.push_back(msg);
            min_message_id = msg.message_id ;
        }
        is_more = has_more;
        conn->con_->commit();

        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what() << std::endl;
        std::cerr << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        // 回滚事务
        conn->con_->rollback();
        return ERROR_LOAD_CHAT_MESSAGE;
    }
}
