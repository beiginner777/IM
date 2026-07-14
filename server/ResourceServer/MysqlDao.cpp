#include "MysqlDao.h"
#include "data.h"
MysqlDao::MysqlDao()
{
    pool_ = std::make_unique<MysqlConnPool>();
}
MysqlDao::~MysqlDao()
{
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
int MysqlDao::updateUserIcon(int uid, const std::string& icon)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        std::cout << "update head icon failed, mysqlConn is nullptr.\n";
        return ERROR_UPDATE_HEAD_ICON;
    }
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });
    try {
        
        std::unique_ptr<sql::PreparedStatement> pstmt(
            con->con_->prepareStatement(
                "UPDATE user SET icon = ? WHERE uid = ?"
            )
        );
        pstmt->setString(1, icon);
        pstmt->setInt(2, uid);
        int affectedRows = pstmt->executeUpdate();
        if (affectedRows > 0) {
            return SUCCESS;   // 成功更新
        }
        else {
            return ERROR_UPDATE_HEAD_ICON;   // uid 不存在
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return ERROR_UPDATE_HEAD_ICON;
    }
}
bool MysqlDao::GetFriendList(int uid, std::vector<int>& friend_list)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return ERROR_GET_FRIEND_APPLY_LIST;
    }
    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });
    try {
        // 准备SQL语句
        std::unique_ptr<sql::PreparedStatement> pstmt(con->con_->prepareStatement("SELECT friend_id from friend where self_id = ?"));
        pstmt->setInt(1, uid); // 将uid替换为你要查询的uid
        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 遍历结果集
        while (res->next()) {
			int friend_id = res->getInt("friend_id");
            friend_list.push_back(friend_id);
        }
        return SUCCESS;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return ERROR_GET_FRIEND_LIST;
    }
}
