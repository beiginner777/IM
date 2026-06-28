#include "MysqlDao.h"

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

int MysqlDao::userLogin(std::string name, std::string password, std::shared_ptr<UserInfo> userInfo)
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
        std::unique_ptr < sql::PreparedStatement > stmt(conn->con_->prepareStatement("select * from user where name = ?"));

        // 设置输入参数
        stmt->setString(1, name);

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery()
        );


        while (res->next()) {
            int uid = res->getInt("uid");
            std::string name_ = res->getString("name");
            std::string email = res->getString("email");
            std::string password_ = res->getString("password");
            std::string desc_ = res->getString("desc");
            std::string icon_ = res->getString("icon");
            int sex_ = res->getInt("sex");
            std::string nick_ = res->getString("nick");

            std::cout << "name=" << name_ << ", password=" << password_ << std::endl;

            if (name == name_ && password_ == password) {
                std::cout << "User(" << name << ") login success.\n";

                userInfo->uid_ = uid;
                userInfo->name_ = name;
                userInfo->email_ = email;
                userInfo->pwd_ = password_;
                userInfo->desc_ = desc_;
                userInfo->icon_ = icon_;
                userInfo->sex_ = sex_;
                userInfo->nick_ = nick_;

                std::cout << "================ " << "uid = " << userInfo->uid_ << std::endl;


                return SUCCESS;
            }
            // 密码错误
            std::cout << "User(" << name << ") password error." << std::endl;
            return ERROR_PASSWORD;
        }
        // 用户不存在
        std::cout << "User(" << name << ") not exists." << std::endl;
        return ERROR_USER_NOT_EXIST;
    }
    catch (sql::SQLException& e) {
        std::cout << "===============================\n";
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return ERROR_LOGIN;
    }
}
