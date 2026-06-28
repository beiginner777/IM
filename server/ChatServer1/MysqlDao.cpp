#include "MysqlDao.h"
#include "data.h"

MysqlDao::MysqlDao()
{
    pool_ = std::make_unique<MysqlConnPool>();
}

MysqlDao::~MysqlDao()
{
}

int MysqlDao::registerUser(const std::string& name, const std::string& email, const std::string& password)
{
	std::unique_ptr<SqlConnection> conn = pool_->getConnection();

    if (conn == nullptr) {
        std::cout << "mysqlConn is nullptr, register failed.\n";
        return ERROR_REGISTER;
    }

    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
        });

	try {
        // reg_user 定义 Mysql 的存储过程

		// 准备调用存储过程（前三个是输入变量，最后一个是输出变量），可以防止sql注入
		std::unique_ptr < sql::PreparedStatement > stmt(conn->con_->prepareStatement("CALL reg_user(?,?,?,@result)"));

		// 设置输入参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, password);

		// 执行存储过程
		stmt->execute();

		// 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
	    // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
		std::unique_ptr<sql::Statement> stmtResult(conn->con_->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        // 注册成功，返回非0

        int result = res->next();

		if (result > 0) {
			int result = res->getInt("result");
			std::cout << "注册用户的uid: " << result << std::endl;
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
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return ERROR_FRIEND_APPLY;
    }

    // 修复：使用值捕获避免悬空引用
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    try {
        // 开启事务
		con->con_->setAutoCommit(false);

        // 1. 检查是否存在未处理的好友申请
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            con->con_->prepareStatement(
                "SELECT id FROM friendapply WHERE fromuid = ? AND touid = ? AND status = 0 LIMIT 1"
            )
        );
        checkStmt->setInt(1, fromuid);
        checkStmt->setInt(2, touid);

        std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());
        if (checkRes->next()) {
            // 存在未处理的申请，返回错误
            std::cout << "Friend apply already exists fromuid = " << fromuid
                << " to touid = " << touid << std::endl;
            return ERROR_MULTIPLE_FRIEND_APPLY;
        }

        // 2. 插入新的好友申请
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

        // 3. 获取自增ID和申请时间
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

		// 提交事务
		con->con_->commit();

        return SUCCESS;
    }
    catch (sql::SQLException& e) {
		// 回滚事务
		con->con_->rollback();

        std::cerr << "SQLException in addFriendApply: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;

        // 如果是唯一键冲突，返回重复申请错误
        if (e.getErrorCode() == 1062) { // ER_DUP_ENTRY
            return ERROR_MULTIPLE_FRIEND_APPLY;
        }
        return ERROR_FRIEND_APPLY;
    }
    catch (const std::exception& e) {
        // 回滚事务
        con->con_->rollback();
        std::cerr << "Exception in addFriendApply: " << e.what() << std::endl;
        return ERROR_FRIEND_APPLY;
    }
}

int MysqlDao::getUserFriendApply(int uid, std::vector<std::shared_ptr<ApplyInfo>>& applyList)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return ERROR_GET_FRIEND_APPLY_LIST;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });


    try {
        // 准备SQL语句, 根据起始id和限制条数返回列表
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT u.uid,u.name,u.email,u.desc,u.icon, u.sex,u.nick,f.fromuid FROM friendapply f JOIN user u ON f.fromuid = u.uid WHERE f.touid = ?;"));

        pstmt->setInt(1, uid); // 将uid替换为你要查询的uid

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 遍历结果集
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
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });


    try {
        // 准备SQL语句, 根据起始id和限制条数返回列表
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("select * from friend where self_id = ? "));

        pstmt->setInt(1, uid); // 将uid替换为你要查询的uid

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 遍历结果集
        while (res->next()) {
            auto friend_id = res->getInt("friend_id");
            auto back = res->getString("back");
            //再一次查询friend_id对应的信息
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
    auto conn = pool_->getConnection();
    if (conn == nullptr) {
        return ERROR_ADD_FRIEND_RELATION;
    }

    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
        });

    try {
        //开始事务
        conn->con_->setAutoCommit(false);

        // 准备第一个SQL语句, 插入认证方好友数据
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->con_->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id) "
            "VALUES (?, ?) "
        ));
        pstmt->setInt(1, fromuid); // from id
        pstmt->setInt(2, touid); // to uid

        // 执行更新
        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
			std::cout << "addfriend insert friend failed, uid1 = " << fromuid << ", uid2 = " << touid << std::endl;
            conn->con_->rollback();
            return ERROR_ADD_FRIEND_RELATION;
        }else {
            // 获取ID
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

        // 准备第二个SQL语句，插入申请方好友数据
        std::unique_ptr<sql::PreparedStatement> pstmt2(conn->con_->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id) "
            "VALUES (?, ?) "
        ));
        pstmt2->setInt(1, touid);
        pstmt2->setInt(2, fromuid);

        // 执行更新
        int rowAffected2 = pstmt2->executeUpdate();
        if (rowAffected2 <= 0) {
			std::cout << "addfriend insert friend failed, uid1 = " << touid << ", uid2 = " << fromuid << std::endl;
            conn->con_->rollback();
            return ERROR_ADD_FRIEND_RELATION;
        }
        else {
            // 获取ID
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

        // 创建私聊线程
		int ret = createPrivateThread(fromuid, touid, thread_id1);
        if(ret != SUCCESS) {
            conn->con_->rollback();
            return ERROR_ADD_FRIEND_RELATION;
		}
		thread_id2 = thread_id1;
        // 提交事务
        conn->con_->commit();
        std::cout << "addfriend insert friends success" << std::endl;

        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        // 如果发生错误，回滚事务
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
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return nullptr;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    try {
        // 准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT * FROM user WHERE uid = ?"));

        pstmt->setInt(1, uid); // 将uid替换为你要查询的uid

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        // 遍历结果集
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
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return nullptr;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    try {
        // 准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name); // 将uid替换为你要查询的uid

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        // 遍历结果集
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
    auto conn = pool_->getConnection();
    if (conn == nullptr) {
        std::cout << "修改好友记录d fromuid = " << fromuid << " " << "touid = " << touid << " 的状态失败。" << std::endl;
        return ERROR_MODIFLY_APPLY_STATUS_FAILED;
    }

    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
        });

    try {
        //开始事务
        conn->con_->setAutoCommit(false);

        // 准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->con_->prepareStatement("UPDATE friendapply SET status = ? WHERE fromuid = ? AND touid = ? ;"));
        pstmt->setInt(1, status); 
        pstmt->setInt(2, fromuid);
        pstmt->setInt(3, touid);
       
        // 执行更新
        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            conn->con_->rollback();
            return ERROR_MODIFLY_APPLY_STATUS_FAILED;
        }

        // 提交事务
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
    // 获取连接
    auto conn = pool_->getConnection();

	max_thread_id = last_thread_id;

    if (!conn) {
        std::cout << "uid = " << uid << "GetUserThreadInfos failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
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

        // 先把所有行读到临时容器
        std::vector<std::shared_ptr<ChatThreadInfo>> tmp;
        while (res->next()) {
            auto cti = std::make_shared<ChatThreadInfo>();
            cti->threadId_ = res->getInt("thread_id");
            cti->threadType_ = res->getString("type") == "private" ? CHAT_THREAD_TYPE_PRIVATE : CHAT_THREAD_TYPE_GROUP;
            cti->user1_id_ = res->getInt("user1_id");
            cti->user2_id_ = res->getInt("user2_id");
            tmp.push_back(cti);
        }

        // 判断是否多取到一条
        if ((int)tmp.size() > page_size) {
            load_more = true;
            tmp.pop_back();  // 丢掉第 pageSize+1 条
        }
		// 如果没有的话，说明没有更多数据了
        else {
			load_more = false;
        }
        // 移入输出向量
        infos = std::move(tmp);

        // 如果还有数据，更新 nextLastId 为最后一条的 thread_id
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
    // 获取连接
    auto conn = pool_->getConnection();
    if (!conn) {
        std::cout << "uid = " << user1_id << "GetUserThreadInfos failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
        });

    try {
        // 需要这个操作是原子化的
        // 1. 开启事务
		conn->con_->setAutoCommit(false);
        
		// 2. 获取新的 thread_id
		std::unique_ptr<sql::PreparedStatement> pstmt1(conn->con_->prepareStatement
        ("INSERT INTO chatthread(type, created_at) VALUES ('private',NOW());"));
		pstmt1->executeUpdate();
        
        // 返回 刚才插入的那条记录的自增主键 ID。
        std::unique_ptr<sql::PreparedStatement> pstmt2(conn->con_->prepareStatement
		("SELECT LAST_INSERT_ID() AS thread_id;"));
		std::unique_ptr<sql::ResultSet> res(pstmt2->executeQuery());
        res->next();
		thread_id = res->getInt("thread_id");

        int minn = std::min(user1_id, user2_id);
		int maxx = std::max(user1_id, user2_id);

		// 3. 插入 privatechat 表
		std::string insert_sql = "INSERT INTO privatechat(thread_id, user1_id, user2_id) VALUES (?,?,?);";
		std::unique_ptr<sql::PreparedStatement> pstmt3(conn->con_->prepareStatement(insert_sql));
        pstmt3->setInt(1, thread_id);
		pstmt3->setInt(2, minn);
		pstmt3->setInt(3, maxx);
		pstmt3->executeUpdate();

        // 提交事务
		conn->con_->commit();

		return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;

		std::cout << "Create private thread failed for users " << user1_id << " and " << user2_id << std::endl;
		// 如果发生错误，回滚事务
		conn->con_->rollback();

        return ERROR_MODIFLY_APPLY_STATUS_FAILED;
    }
}

int MysqlDao::AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas)
{
    // 获取连接
    auto conn = pool_->getConnection();
    if (!conn) {
		std::cout << "Add ChatMsg failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
        });

    try {
        // 关闭事务自动提交
        conn->con_->setAutoCommit(false);
        auto pstmt = conn->con_->prepareStatement(
            "INSERT INTO chatmessage (thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
		);
        for (auto& msg : chat_datas) {
			pstmt->setInt(1, msg->thread_id);
			pstmt->setInt(2, msg->sender_id);
			pstmt->setInt(3, msg->recv_id);
			pstmt->setString(4, msg->content);
			pstmt->setString(5, msg->chat_time);
			pstmt->setString(6, msg->chat_time);
            pstmt->setInt(7, msg->status);
			pstmt->setInt(8, static_cast<int>(msg->type));
			pstmt->executeUpdate();
            // 取出上一次插入的自增ID
            std::unique_ptr<sql::Statement> keyStmt(
                conn->con_->createStatement()
            );
            std::unique_ptr<sql::ResultSet> res(
                keyStmt->executeQuery("SELECT LAST_INSERT_ID()")
            );
            if (res->next()) {
				msg->message_id = res->getInt(1);
            }
            else {
                continue;
            }
        }
		std::cout << "insert chat messages success, count = " << chat_datas.size() << std::endl;
        // 提交事务
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
        // 如果发生错误，回滚事务
        conn->con_->rollback();
        return ERROR_SEND_MSG_FAILED;
    }
}

int MysqlDao::getUserFriendListByLastId(int uid, int last_friend_id, std::map<int, std::shared_ptr<UserInfo>>& friend_list)
{
    // 获取连接
    auto conn = pool_->getConnection();
    if (!conn) {
        std::cout << "get friend list failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
        });

    try {
        // 关闭事务自动提交
        conn->con_->setAutoCommit(false);

        // Step 1: 查找符合条件的好友记录
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->con_->prepareStatement("SELECT id, friend_id FROM friend WHERE id > ? AND self_id = ?")
        );

        std::cout << "SELECT id, friend_id FROM friend WHERE id > " << last_friend_id << " AND self_id = " << uid << std::endl;

        pstmt->setInt(1, last_friend_id);
        pstmt->setInt(2, uid);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        // Step 2: 查找好友的详细信息，并将结果存入 std::map<int, std::shared_ptr<UserInfo>>

        while (res->next()) {
            int friendId = res->getInt("friend_id");

            // Step 3: 根据 friend_id 获取用户详细信息
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

                // 创建 UserInfo 对象并保存
                std::shared_ptr<UserInfo> userInfo = std::make_shared<UserInfo>(uid, name, email, password, nick, icon, sex, desc);

                // 将用户信息保存到 map 中，key 为 friend 表的 id
                int friendRecordId = res->getInt("id");
                friend_list[friendRecordId] = userInfo;
            }
        }
        // 提交事务
        conn->con_->commit();
        return SUCCESS;

    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;

        // 如果发生错误，回滚事务
        conn->con_->rollback();

        return ERROR_LOAD_MORE_FRIEND;
    }
}

int MysqlDao::getUserFriendApplyByLastId(int uid, int last_friend_id, int page_size, std::vector<std::shared_ptr<ApplyInfo>>& applyList, bool& load_more, int& max_friend_apply_id)
{
    // 获取连接
    auto conn = pool_->getConnection();
    if (!conn) {
        std::cout << "get friend apply list failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
        });

    try {
        // 关闭事务自动提交
        conn->con_->setAutoCommit(false);

        // Step 1: 查找 friendapply 表中 touid = uid 且 id > last_friend_id 的 page_size + 1条记录的所有 id, fromuid, apply_time, status
        std::string query = R"(
            SELECT id, fromuid, apply_time, status 
            FROM friendapply 
            WHERE touid = ? AND id > ? 
            ORDER BY id ASC 
            LIMIT ?)";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->con_->prepareStatement(query));
        pstmt->setInt(1, uid);             // 设置 touid = uid
        pstmt->setInt(2, last_friend_id);  // 设置 last_friend_id
        pstmt->setInt(3, page_size + 1);   // 设置 page_size + 1

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        // Step 2: 处理查询结果
        int record_count = 0;
        max_friend_apply_id = last_friend_id; // 默认情况下，max_friend_apply_id 为 last_friend_id

        while (res->next()) {
            record_count++;

            // 如果读取到超过 page_size 条数据，丢弃多余的数据
            if (record_count > page_size) {
                // 退出循环，丢弃剩余的记录
                break; // 不再添加多余的记录
            }

            int apply_id = res->getInt("id");
            int fromuid = res->getInt("fromuid");
            std::string apply_time = res->getString("apply_time");  // 读取申请时间
            int status = res->getInt("status");  // 读取状态

            // 查询该 fromuid 对应的用户详细信息
            std::string user_query = R"(
                SELECT name, email, `desc`, icon, sex, uid 
                FROM user 
                WHERE uid = ?)";

            std::unique_ptr<sql::PreparedStatement> user_pstmt(conn->con_->prepareStatement(user_query));
            user_pstmt->setInt(1, fromuid);

            std::unique_ptr<sql::ResultSet> user_res(user_pstmt->executeQuery());

            if (user_res->next()) {
                // 构建 ApplyInfo 对象并加入 applyList
                std::shared_ptr<ApplyInfo> apply_info = std::make_shared<ApplyInfo>(
                    apply_id,
                    fromuid,
                    user_res->getString("name"),
                    user_res->getString("email"),
                    user_res->getString("desc"),
                    user_res->getString("icon"),
                    user_res->getInt("sex"),
                    apply_time,  // 使用查询到的申请时间
                    status        // 使用查询到的状态
                );

                applyList.push_back(apply_info);
                max_friend_apply_id = apply_id; // 更新为当前合法记录的最大 id
            }
        }

        // Step 3: 判断是否有更多数据
        if (record_count > page_size) {
            load_more = true; // 还有更多数据
        }
        else {
            load_more = false; // 没有更多数据
        }

        // 提交事务
        conn->con_->commit();
        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;

        // 如果发生错误，回滚事务
        conn->con_->rollback();
        return ERROR_LOAD_FRIEND_APPLY;
    }
}

int MysqlDao::updateChatMsgStatus(int message_id, MsgStatus status)
{
    // 获取连接
    auto conn = pool_->getConnection();
    if (!conn) {
        std::cout << "get friend apply list failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
        });

    try {
        // 关闭事务自动提交
        conn->con_->setAutoCommit(false);

        std::string query = R"(
            UPDATE chatmessage 
            SET status = ? 
            WHERE message_id = ? )";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->con_->prepareStatement(query));
        pstmt->setInt(1, (int)status);            
        pstmt->setInt(2, message_id);  

        // 执行更新
        int rowAffected = pstmt->executeUpdate();
        if (rowAffected <= 0) {
            conn->con_->rollback();
            return ERROR_MODIFY_MSG_STATUS;
        }
        std::cout << "Update messgae_id = " << message_id << " status is " << status << " success.\n";
        // 提交事务
        conn->con_->commit();
        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        // 如果发生错误，回滚事务
        conn->con_->rollback();
        return ERROR_MODIFY_MSG_STATUS;
    }
}

int MysqlDao::loadChatMessage(int thread_id, int& min_message_id, int& max_message_id, int page_size, bool& is_more, std::vector<ChatMessage>& msgs)
{
    // 获取连接
    auto conn = pool_->getConnection();
    if (!conn) {
        std::cout << "get friend apply list failed, mysqlConn is nullptr.\n";
        return ERROR_LOAD_CHAT_THREAD;
    }
    Defer defer([this, &conn]() {
        pool_->returnConnection(std::move(conn));
        });

    try {
        // 关闭事务自动提交
        conn->con_->setAutoCommit(false);

        // 准备SQL查询，获取消息数据
        std::string query = "SELECT message_id, sender_id, recv_id, content, created_at, updated_at, status, message_type "
            "FROM chatmessage "
            "WHERE thread_id = ? AND message_id > ? AND message_id <= ? "
            "ORDER BY message_id ASC LIMIT ?";

        std::unique_ptr<sql::PreparedStatement> stmt(conn->con_->prepareStatement(query));
        stmt->setInt(1, thread_id);        // 绑定 thread_id
        stmt->setInt(2, min_message_id);  // 绑定 min_message_id
        stmt->setInt(3, max_message_id);  // 绑定 max_message_id
        stmt->setInt(4, page_size + 1);   // 绑定 page_size + 1 用于判断是否有更多数据

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        // 清空之前的消息数据
        msgs.clear();
        bool has_more = false;
        int count = 0;
        while (res->next()) {
            count++;
            if (count > page_size) {
                has_more = true;
                break;
            }
            // 解析结果并存储到 ChatMessage 结构体
            ChatMessage msg;
            msg.message_id = res->getInt("message_id");
            msg.thread_id = thread_id;
            msg.sender_id = res->getInt("sender_id");
            msg.recv_id = res->getInt("recv_id");
            msg.content = res->getString("content");
            msg.chat_time = res->getString("created_at");
            msg.status = res->getInt("status");
            msg.type = static_cast<CHAT_MSG_TYPE>(res->getInt("message_type"));

            // 将消息添加到结果列表中
            msgs.push_back(msg);
            min_message_id = msg.message_id ;
        }
        // 设置 is_more 标志
        is_more = has_more;
        // 提交事务
        conn->con_->commit();

        return SUCCESS; // 成功
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what() << std::endl;
        std::cerr << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        // 回滚事务
        conn->con_->rollback();
        return ERROR_LOAD_CHAT_MESSAGE; // 发生错误，返回错误码
    }
}