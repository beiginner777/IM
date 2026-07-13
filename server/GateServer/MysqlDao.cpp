#include "MysqlDao.h"
#include "MysqlManager.h"

#include "crypto/BCryptHasher.h"



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

        // reg_user ���� Mysql �Ĵ洢����



		// ׼�����ô洢���̣�ǰ������������������һ������������������Է�ֹsqlע��

		std::unique_ptr < sql::PreparedStatement > stmt(conn->con_->prepareStatement("CALL reg_user(?,?,?,@result)"));



		// �����������

		stmt->setString(1, name);

		stmt->setString(2, email);

		stmt->setString(3, password);



		// ִ�д洢����

		stmt->execute();



		// ����洢���������˻Ự��������������ʽ��ȡ���������ֵ�������������ִ��SELECT��ѯ����ȡ����

	    // ���磬����洢����������һ���Ự����@result���洢������������������ȡ��

		std::unique_ptr<sql::Statement> stmtResult(conn->con_->createStatement());

		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));

        // ע��ɹ������ط�0



        int result = res->next();



		if (result > 0) {

			int uid = res->getInt("result");
			std::cout << "注册用户 uid: " << uid << std::endl;
			auto* bloomBf = MysqlManager::getInstance()->getBloomFilter();
			if (bloomBf) {
				bloomBf->add((uint64_t)uid);
				bloomBf->saveToRedis("bloom:user_search");
			}
			return SUCCESS;;

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



	// Bloom pre-check
	auto* bf = MysqlManager::getInstance()->getBloomFilter();
	if (bf && !bf->contains(name)) {
		return ERROR_USER_NOT_EXIST;
	}
    try {

        std::unique_ptr < sql::PreparedStatement > stmt(conn->con_->prepareStatement("select * from user where name = ?"));

        stmt->setString(1, name);

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

            std::cout << "name=" << name_ << std::endl;

            if (name == name_ && BCryptHasher::verifyPassword(password, password_)) {

                std::cout << "User(" << name << ") login success.\n";

                userInfo->uid_ = uid;

                userInfo->name_ = name;

                userInfo->email_ = email;

                userInfo->pwd_ = password_;

                userInfo->desc_ = desc_;

                userInfo->icon_ = icon_;

                userInfo->sex_ = sex_;

                userInfo->nick_ = nick_;

                return SUCCESS;

            }

            std::cout << "User(" << name << ") password error." << std::endl;

            return ERROR_PASSWORD;

        }

        // �û�������

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

