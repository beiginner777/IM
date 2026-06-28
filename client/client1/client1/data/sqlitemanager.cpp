#include "sqlitemanager.h"
#include "core/userdata.h"
#include "usermanager.h"
#include "dialogs/chatuserwidget.h"
#include "core/userdata.h"
#include "fileuploadmsg.h"

SqliteManager::SqliteManager()
    : last_friend_id_(0)
    , last_friend_apply_id_(0)
    , last_thread_id_(0)
    , is_load_more_chatUser_(true)
    , is_load_more_ConnUser_(true)
    , is_load_more_FriendApply_(true)
{
    // 连接 SQLite 数据库
    db_ = QSqlDatabase::addDatabase("QSQLITE");  // 使用 SQLite 驱动
    db_.setDatabaseName(DB_FILE_LOCATION);  // 设置 SQLite 数据库文件路径

    // 检查数据库是否成功打开
    if (!db_.open()) {
        qDebug() << "Error: Unable to open the database!" << db_.lastError().text();
        exit(-1);
    }
    //qDebug() << "Database opened successfully!";
}

SqliteManager::~SqliteManager()
{
    db_.close();
}
void SqliteManager::addUser(std::shared_ptr<UserInfo> userInfo)
{
    int uid = userInfo->uid_;
    QString name = userInfo->name_;
    QString email = userInfo->email_;
    QString password = userInfo->pwd_;
    QString desc = userInfo->desc_;
    QString icon = userInfo->icon_;
    int sex = userInfo->sex_;
    QString nick = userInfo->nick_;

    // 创建查询对象
    QSqlQuery query(db_);
    try {
        // 开始事务
        db_.transaction();
        // 准备 SQL 插入语句
        query.prepare("INSERT OR IGNORE INTO user (uid, name, email, password, desc, icon, sex, nick) "
                      "VALUES (:uid, :name, :email, :password, :desc, :icon, :sex, :nick)");
        // 绑定参数
        query.bindValue(":uid", uid);
        query.bindValue(":name", name);
        query.bindValue(":email", email);
        query.bindValue(":password", password);  // 在实际应用中，密码需要加密存储
        query.bindValue(":desc", desc);
        query.bindValue(":icon", icon);
        query.bindValue(":sex", sex);
        query.bindValue(":nick", nick);
        // 执行查询
        if (!query.exec()) {
            // 如果插入失败，输出错误信息
            db_.rollback();
            qDebug() << "Error: Unable to insert data into user table!" << query.lastError().text();
        }
        else{
            //qDebug() << "User data inserted successfully!";
        }

        query.prepare("SELECT id, uid, thread_id, friend_id, friend_apply_id FROM load_info WHERE uid = :uid LIMIT 1");
        query.bindValue(":uid",uid);
        if (!query.exec()) {
            qDebug() << "Query failed:" << query.lastError().text();
            db_.rollback();
            return;
        }
        if(!query.next()){
            query.prepare("INSERT INTO load_info(uid, thread_id, friend_id, friend_apply_id) values (:self_uid,0,0,0)");
            query.bindValue(":self_uid",uid);
            if(!query.exec()){
                qDebug() << "[ERROR]: " << __FILE__ << ":" << __LINE__ << "(" << __FUNCTION__  << ").";
                db_.rollback();
                return;
            }else{
                qDebug() << "Add Message into load_info success.";
            }
        }
        db_.commit();

    } catch (const std::exception &e) {
        // 如果发生错误，回滚事务
        qDebug() << "Error: " << e.what();
        db_.rollback();
        return;
    }
}

void SqliteManager::loadChatUserList()
{
    // to do .... 加载群聊（还需查询表 GroupChat）

    int self_uid = UserManager::GetInstance()->getUid();
    // 加载的数量CHATUSERLIST_LOAD_PAGESIZE_FROM_LOCAL
    QSqlQuery query;
    query.prepare(R"(
        SELECT thread_id, user1_id, user2_id, created_at
        FROM privatechat
        WHERE thread_id <= :last_thread_id AND (user1_id = :user1_id OR user2_id = :user2_id)
        ORDER BY thread_id DESC
    )");
    query.bindValue(":last_thread_id", last_thread_id_);
    query.bindValue(":user1_id",self_uid);
    query.bindValue(":user2_id",self_uid);
    //query.bindValue(":page_size", CHATUSERLIST_LOAD_PAGESIZE_FROM_LOCAL);

    if (!query.exec()) {
        qDebug() << "Query failed:" << query.lastError().text();
        return;
    }

    std::vector<ChatThreadInfo> thread_infos;
    while (query.next()) {
        ChatThreadInfo info;
        info.threadId_ = query.value(0).toInt();
        info.user1_id_ = query.value(1).toInt();
        info.user2_id_ = query.value(2).toInt();
        info.threadType_ = CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE;
        thread_infos.push_back(info);

        int peerUid = 0;
        if(info.user1_id_ == UserManager::GetInstance()->getUid()){
            peerUid = info.user2_id_;
        }else{
            peerUid = info.user1_id_;
        }
        assert(UserManager::GetInstance()->isCreateThreadByThreadID(info.threadId_) == true);
    }
    // 更新最大记录
    last_thread_id_ -= thread_infos.size();
    bool hasMore = false;
    is_load_more_chatUser_ = hasMore;
    emit signalDrawChatUserToList(thread_infos,hasMore);
}

void SqliteManager::loadFriendList()
{
    std::shared_ptr<UserInfo> userInfo = UserManager::GetInstance()->getUserInfo();
    int uid = userInfo->uid_;

    //int max_friend_id = 0;

    // 查询所有 friend_id
    QSqlQuery query;
    query.prepare("SELECT friend_id FROM friend WHERE self_id = :uid");
    query.bindValue(":uid", uid);

    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError().text();
        return;
    }

    std::vector<std::shared_ptr<FriendInfo>> friends;
    // 查找对应的 friend_info 数据
    while (query.next()) {
        int friend_id = query.value(0).toInt();
        // 查询 friend_info 表获取朋友的详细信息
        QSqlQuery infoQuery;
        infoQuery.prepare("SELECT id, uid, name, nick, icon, sex, desc FROM friend_info WHERE uid = :friend_id");
        infoQuery.bindValue(":friend_id", friend_id);

        if (!infoQuery.exec()) {
            qDebug() << "Error executing infoQuery:" << infoQuery.lastError().text();
            continue;  // 如果查询失败，跳过此记录
        }

        // 处理查询结果
        if (infoQuery.next()) {
            int id = infoQuery.value(0).toInt();
            int uid = infoQuery.value(1).toInt();
            QString name = infoQuery.value(2).toString();
            QString nick = infoQuery.value(3).toString();
            QString icon = infoQuery.value(4).toString();
            int sex = infoQuery.value(5).toInt();
            QString desc = infoQuery.value(6).toString();

            // 创建 FriendInfo 对象并添加到向量中
            friends.push_back(std::make_shared<FriendInfo>(uid, name, nick, icon, sex, desc));
            //max_friend_id = std::max(max_friend_id,id);
        }
    }
    // 将好友信息设置到UserManager（to do ... 加锁）
    UserManager::GetInstance()->appendFriendList(friends);

    // 因为将这些信息保存在 load_last_msg 中，所以

    // 因为有可能是服务器的数据先加载完成，那么last_friend_id就是对的，不需要修改（所以就需要判断一下）
    //int lasT_friend_id = UserManager::GetInstance()->get_last_friend_id();
    //UserManager::GetInstance()->set_last_friend_id(std::max(max_friend_id +1,lasT_friend_id));
}

void SqliteManager::loadFriendApplyList()
{
    int self_uid = UserManager::GetInstance()->getUid();
    int max_id = last_friend_apply_id_;   // 成员变量

    QSqlQuery query(db_);

    // 多查一条，用来判断是否还有数据
    query.prepare(
        "SELECT id, uid, name, email, `desc`, icon, sex, apply_time, status "
        "FROM apply_info "
        "WHERE self_uid = :self_uid "
        "AND id <= :max_id "
        "ORDER BY id DESC "
        "LIMIT :limit"
    );

    query.bindValue(":self_uid", self_uid);
    query.bindValue(":max_id", max_id);
    query.bindValue(":limit", FRIENDAPPLYLIST_LOAD_PAGESIZE_FROM_LOCAL + 1);

    if (!query.exec()) {
        qWarning() << "loadFriendApplyListIncrement failed:"
                   << query.lastError().text();
        return;
    }

    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    int count = 0;
    bool is_more = true;

    while (query.next()) {
        if (count == FRIENDAPPLYLIST_LOAD_PAGESIZE_FROM_LOCAL) {
            // 第 page_size + 1 条，只用于判断是否还有数据
            is_more = true;
            break;
        }

        int id = query.value("id").toInt();
        int uid = query.value("uid").toInt();
        QString name = query.value("name").toString();
        QString email = query.value("email").toString();
        QString desc = query.value("desc").toString();
        QString icon = query.value("icon").toString();
        int sex = query.value("sex").toInt();
        QString apply_time = query.value("apply_time").toString();
        int status = query.value("status").toInt();

        apply_list.emplace_back(std::make_shared<ApplyInfo>(
            id, uid, name, email, desc, icon, sex, apply_time, status
        ));

        count++;
    }
    // 不足 page_size，说明一定没有更多了
    if (count < FRIENDAPPLYLIST_LOAD_PAGESIZE_FROM_LOCAL) {
        is_more = false;
    }
    last_friend_apply_id_ -= count;
    // 写回 UserManager / 内存缓存
    UserManager::GetInstance()->appendApplyList(apply_list);
    // 通知ChatDialog去渲染数据
    is_load_more_FriendApply_ = is_more;
    emit signalDrawFriendApplyToList(apply_list,is_more);
}


void SqliteManager::loadMessgaeThroughThreadId(int thread_id)
{

}

void SqliteManager::GetLoginedUser()
{
    std::vector<std::shared_ptr<UserInfo>> users;

    // 执行 SQL 查询，查找 is_show = 1 的所有用户
    QSqlQuery query(db_);
    query.prepare("SELECT uid, name, email, password, nick, icon, sex, desc FROM user WHERE is_show = 1");

    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError().text();
        emit signaloadHistortUserFinish(users);
        return;  // 如果查询失败，返回空 vector
    }

    // 遍历查询结果并填充 vector
    while (query.next()) {
        int uid = query.value("uid").toInt();
        QString name = query.value("name").toString();
        QString email = query.value("email").toString();
        QString password = query.value("password").toString();
        QString nick = query.value("nick").toString();
        QString icon = query.value("icon").toString();
        int sex = query.value("sex").toInt();
        QString desc = query.value("desc").toString();
        // 创建 UserInfo 对象并加入到 vector 中
        std::shared_ptr<UserInfo> user = std::make_shared<UserInfo>(uid, name, email, password, nick, icon, sex, desc);
        users.push_back(user);
    }
    emit signaloadHistortUserFinish(users);
}

void SqliteManager::loadLocalInfo()
{
    int uid = UserManager::GetInstance()->getUid();

    QSqlQuery query1(db_);
    query1.prepare("SELECT id, uid, thread_id, friend_id, friend_apply_id FROM load_info WHERE uid = :uid LIMIT 1");
    query1.bindValue(":uid", uid);

    if(!query1.exec()) {
        qDebug() << "Query failed:" << query1.lastError().text();
        return;
     }

    if(query1.next()) {
        // 获取查询结果
        last_thread_id_ = query1.value(2).toInt();
        last_friend_id_ = query1.value(3).toInt();
        last_friend_apply_id_ = query1.value(4).toInt();
        // 将结果保存到缓存中
        UserManager::GetInstance()->setLastChatThreadID(last_thread_id_);
        UserManager::GetInstance()->set_last_friend_id(last_friend_id_);
        UserManager::GetInstance()->set_last_friend_apply_id(last_friend_apply_id_);
     }else {
        qDebug() << "No record found for UID:" << uid;
        UserManager::GetInstance()->setLastChatThreadID(0);
        UserManager::GetInstance()->set_last_friend_id(0);
        UserManager::GetInstance()->set_last_friend_apply_id(0);
        return;
   }

    QSqlQuery query2(db_);
    query2.prepare("SELECT thread_id, min_message_id, max_message_id FROM thread_message_range where self_uid = :self_uid");

    int self_uid = UserManager::GetInstance()->getUid();
    query2.bindValue(":self_uid",self_uid);

  // qDebug() << "-------------------- self_uid = " << self_uid << " -------------------------";

    if(!query2.exec()) {
        qDebug() << "Query failed:" << query2.lastError().text();
        return;
     }

    std::vector<int> threadIds_;
    while(query2.next()) {
        // 获取查询结果
        int thread_id = query2.value("thread_id").toInt();
        threadIds_.push_back(thread_id);
        int min_message_id = query2.value("min_message_id").toInt();
        //int max_message_id = query2.value("max_message_id").toInt();
        last_chat_message_id_[thread_id] = min_message_id + 1;
        is_load_more_chatmessage_[thread_id] = true;
        // 将结果保存到缓存中
        UserManager::GetInstance()->set_threadId_min_message_id(thread_id, min_message_id);
        UserManager::GetInstance()->set_threadId_max_message_id(thread_id,INT_MAX);
    }
    // 查找所有thread_id对应的peerid
    for(int thread_id : threadIds_){
        QSqlQuery query3(db_);
        query3.prepare(R"(
            SELECT thread_id, user1_id, user2_id, created_at
            FROM privatechat
            WHERE thread_id = :thread_id
        )");
        query3.bindValue(":thread_id", thread_id);

        if (!query3.exec()) {
            qDebug() << "Query failed:" << query3.lastError().text();
            return;
        }

        while (query3.next()) {
            ChatThreadInfo info;
            info.threadId_ = query3.value(0).toInt();
            info.user1_id_ = query3.value(1).toInt();
            info.user2_id_ = query3.value(2).toInt();
            info.threadType_ = CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE;

            int peerUid = 0;
            if(info.user1_id_ == UserManager::GetInstance()->getUid()){
                peerUid = info.user2_id_;
            }else{
                peerUid = info.user1_id_;
            }
            // 创建新的ChatThreadData 并且 添加到UserManager
            std::shared_ptr<ChatThreadData> chatThreadData = std::make_shared<ChatThreadData>(peerUid,info.threadId_);
            UserManager::GetInstance()->addChatThreadData(chatThreadData);
        }
    }
    emit signalLoadLocalInfoFinish();
}

void SqliteManager::storeThreadInfo(std::shared_ptr<ChatThreadInfo> thread_info)
{
    // 将这一条记录插入到数据库当中
    int thread_id = thread_info->threadId_;
    int type = thread_info->threadType_;
    int user_1 = thread_info->user1_id_;
    int user_2 = thread_info->user2_id_;

    int uid = UserManager::GetInstance()->getUid();

    // 私聊
    if(type == 1){
       QSqlQuery query(db_);
       // 开始事务
       db_.transaction();
       try {
           // 插入 chatthread 表，根据 type 插入不同的值
           QString threadType = (type == 1) ? "private" : "group";  // 如果 type = 1, 插入 'private'; 如果 type = 2, 插入 'group'
           query.prepare("INSERT OR IGNORE INTO chatthread (id, type) VALUES (:thread_id, :type)");
           query.bindValue(":thread_id", thread_id);
           query.bindValue(":type", threadType);
           if (!query.exec()) {
               throw std::runtime_error("Failed to insert into chatthread: " + query.lastError().text().toStdString());
           }

           // 插入 privatechat 表，确保 user_1 < user_2
           query.prepare("INSERT OR IGNORE INTO privatechat (thread_id, user1_id, user2_id) "
                         "VALUES (:thread_id, MIN(:user_1, :user_2), MAX(:user_1, :user_2))");
           query.bindValue(":thread_id", thread_id);
           query.bindValue(":user_1", user_1);
           query.bindValue(":user_2", user_2);
           if (!query.exec()) {
               throw std::runtime_error("Failed to insert into privatechat: " + query.lastError().text().toStdString());
           }

           // 插入 load_info 表，加入 uid 和 thread_id
           query.prepare("UPDATE load_info set thread_id = :thread_id where uid = :uid");
           query.bindValue(":uid", uid);
           query.bindValue(":thread_id", thread_id);
           if (!query.exec()) {
               throw std::runtime_error("Failed to insert into load_info: " + query.lastError().text().toStdString());
           } 

           // 插入 thread_message_range
           modifyMsgId(thread_id,0,INT_MAX);

           // 提交事务
           db_.commit();
           return;

       } catch (const std::exception &e) {
           // 如果发生错误，回滚事务
           qDebug() << "Error: " << e.what();
           db_.rollback();
           return;
       }
    }
    // 群聊
    else {

    }

}

void SqliteManager::storeFriends(std::vector<std::shared_ptr<FriendInfo>> friends)
{
    // 将好友关系插入到friends表中，并且将好友信息插入到friend_info表中
    int self_id = UserManager::GetInstance()->getUid();

    // 1. 开始事务
    QSqlQuery query(db_);
    if (!query.exec("BEGIN TRANSACTION;")) {
        qDebug() << "Failed to begin transaction:" << query.lastError().text();
        return;
    }

    // 2. 插入 friend_info 表和 friend 表
    for (const auto& friendInfo : friends) {
        // 插入 friend_info 表 (手动设置 id)
        query.prepare("INSERT INTO friend_info (id, uid, name, nick, sex, desc, icon, email) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
        query.addBindValue(friendInfo->id_);  // 手动设置 id
        query.addBindValue(friendInfo->uid_);
        query.addBindValue(friendInfo->name_);
        query.addBindValue(friendInfo->nick_);
        query.addBindValue(friendInfo->sex_);
        query.addBindValue(friendInfo->desc_);
        query.addBindValue(friendInfo->icon_);
        query.addBindValue(friendInfo->email_);

        if (!query.exec()) {
            qDebug() << "Error inserting into friend_info table:" << query.lastError().text();
            query.exec("ROLLBACK;");
            return;
        }

        // 插入 friend 表 (手动设置 id)
        query.prepare("INSERT INTO friend (id, self_id, friend_id) VALUES (?, ?, ?);");
        query.addBindValue(friendInfo->id_);  // 手动设置 id
        query.addBindValue(self_id);
        query.addBindValue(friendInfo->uid_);

        if (!query.exec()) {
            qDebug() << "Error inserting into friend table:" << query.lastError().text();
            query.exec("ROLLBACK;");
            return;
        }
    }

    // 3. 更新 load_info 表
    query.prepare("UPDATE load_info SET friend_id = ? WHERE uid = ?;");
    query.addBindValue(UserManager::GetInstance()->get_last_friend_id());
    query.addBindValue(self_id);

    if (!query.exec()) {
      qDebug() << "Error updating load_info table:" << query.lastError().text();
      query.exec("ROLLBACK;");
      return;
    }

    // 4. 提交事务
    if (!query.exec("COMMIT;")) {
        qDebug() << "Failed to commit transaction:" << query.lastError().text();
    } else {
        //qDebug() << "Friends successfully inserted into both tables!";
    }
}

void SqliteManager::updateSelfIcon(QString icon)
{
    int uid = UserManager::GetInstance()->getUid();

    QSqlQuery query(db_);

        query.prepare(
            "UPDATE user SET icon = :icon WHERE uid = :uid"
        );
        query.bindValue(":icon", icon);
        query.bindValue(":uid", uid);

        if (!query.exec()) {
            qWarning() << "Failed to update user icon:"
                       << query.lastError().text() << ",更新头像失败.";
            emit FileUploadMsg::GetInstance()->signalUploadHeadIconIsSuccess(false);
            return;
        }

        // SQLite 下判断是否真的更新到数据
        if (query.numRowsAffected() > 0) {
            qDebug() << "更新本地数据成功：当前用户uid(" << uid << ")的icon为" << icon;
            UserManager::GetInstance()->setSelfIcon(icon);
            emit FileUploadMsg::GetInstance()->signalUploadHeadIconIsSuccess(true);
        } else {
            qDebug() << "用户表中没有当前用户的uid(" << uid << "),更新头像失败.";
            emit FileUploadMsg::GetInstance()->signalUploadHeadIconIsSuccess(false);
        }
}

void SqliteManager::updateFriendIcon(int friend_id, QString icon)
{
    QSqlQuery query(db_);

    query.prepare(
        "UPDATE friend_info "
        "SET icon = :icon "
        "WHERE uid = :uid"
    );

    query.bindValue(":icon", icon);
    query.bindValue(":uid", friend_id);

    if (!query.exec()) {
        qWarning() << "updateFriendIcon failed:"
                   << query.lastError().text();
        return;
    }

    if (query.numRowsAffected() == 0) {
        qWarning() << "updateFriendIcon: no row updated, uid ="
                   << friend_id;
    } else {
        qDebug() << "updateFriendIcon succeed, uid =" << friend_id;
    }
}

void SqliteManager::storeFriendApply(std::vector<std::shared_ptr<ApplyInfo>> apply_list)
{
    QSqlQuery query(db_);

    if (!db_.transaction()) {
        qWarning() << "start transaction failed";
        return;
    }

    query.prepare(R"(
        INSERT OR REPLACE INTO apply_info
        (id, uid, name, email, desc, icon, sex, apply_time, status,self_uid)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    int uid = UserManager::GetInstance()->getUid();
    for (const auto& apply : apply_list) {
        query.bindValue(0, apply->id_);
        query.bindValue(1, apply->uid_);
        query.bindValue(2, apply->name_);
        query.bindValue(3, apply->email_);
        query.bindValue(4, apply->desc_);
        query.bindValue(5, apply->icon_);
        query.bindValue(6, apply->sex_);
        query.bindValue(7, apply->apply_time_);
        query.bindValue(8, apply->status_);
        query.bindValue(9, uid);
        if (!query.exec()) {
            qWarning() << "insert apply_info failed:"
                       << query.lastError().text();
            db_.rollback();
            return;
        }
    }
    //qDebug() << "Store friend list success.";
    int last_friend_apply_id = UserManager::GetInstance()->get_last_friend_apply_id();
    QSqlQuery updateQuery(db_);
    updateQuery.prepare(R"(
        UPDATE load_info
        SET friend_apply_id = ?
        WHERE uid = ?
    )");
    updateQuery.addBindValue(last_friend_apply_id);
    updateQuery.addBindValue(uid);

    if (!updateQuery.exec()) {
        qWarning() << "update last_friend_apply_id failed:"
                   << updateQuery.lastError().text();
        db_.rollback();
        return;
    }

    if (!db_.commit()) {
        qWarning() << "commit transaction failed";
        db_.rollback();
    }
    //qDebug() << "update last_friend_apply_id in load_info success.";
}

void SqliteManager::CheckLocalChatUser(int uid)
{
    QSqlQuery query(db_);

    // Step1：在privateChat中查找是否有这个线程
    query.prepare(R"(
        SELECT thread_id, user1_id, user2_id, created_at
        FROM privatechat where user1_id = :user1_id or user2_id = :user2_id
    )");
    query.bindValue(":user1_id", uid);
    query.bindValue(":user2_id", uid);

    // Step2 : 获取结果
    if (!query.exec()) {
        qDebug() << "Query failed:" << query.lastError().text();
        return;
    }

    std::vector<ChatThreadInfo> thread_infos;
    std::vector<int> friend_ids;
    if (query.next()) {
        ChatThreadInfo info;
        info.threadId_ = query.value(0).toInt();
        info.user1_id_ = query.value(1).toInt();
        info.user2_id_ = query.value(2).toInt();
        info.threadType_ = CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE;
        thread_infos.push_back(info);

        int peerUid = 0;
        if(info.user1_id_ == UserManager::GetInstance()->getUid()){
            peerUid = info.user2_id_;
        }else{
            peerUid = info.user1_id_;
        }
        friend_ids.push_back(peerUid);

        assert(UserManager::GetInstance()->isCreateThreadByThreadID(peerUid));

        // Step3: 通知ChatUserList去渲染这条数据
        emit signalDrawChatUserToList(thread_infos,is_load_more_chatUser_);
        // Step4: 通知ChatDialog去跳转界面
        emit signalChatDialogSwitchFromConnToChat(uid);
    }
}

void SqliteManager::loadspecifiedChatUser(int uid)
{
    int self_uid = UserManager::GetInstance()->getUid();
    qDebug() << "uid  = " << uid << " " << "self_uid = " << self_uid;
    assert(uid != self_uid);

    if (!db_.open()) {
        qDebug() << "Error: " << db_.lastError().text();
        return;
    }

    std::shared_ptr<ChatThreadInfo> thread_info = std::make_shared<ChatThreadInfo>();

    // 找到其中 user1_id == uid && user2_id == self_uid || user1_id == self_uid && user2_id == uid 的记录。
    QString queryStr = QString("SELECT thread_id, user1_id, user2_id "
                               "FROM privatechat "
                               "WHERE (user1_id = :uid AND user2_id = :self_uid) "
                               "OR (user1_id = :self_uid AND user2_id = :uid)");
    QSqlQuery query(db_);
    query.prepare(queryStr);
    query.bindValue(":uid", uid);  // 绑定 uid 参数
    query.bindValue(":self_uid", self_uid);  // 绑定当前用户的 uid

    // 执行查询
    if (!query.exec()) {
        qDebug() << "Query failed: " << query.lastError().text();
        return;
    }

    // 存储查询结果
    if (query.next()) {
        int threadId = query.value("thread_id").toInt();
        int user1_id = query.value("user1_id").toInt();
        int user2_id = query.value("user2_id").toInt();
        int threadType = CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE;  // 这里可以根据需求来设置类型（1 表示私聊）
        thread_info->threadId_ = threadId;
        thread_info->user1_id_ = user1_id;
        thread_info->user2_id_ = user2_id;
        thread_info->threadType_ = threadType;
        int peeruid = user1_id;
        if(user1_id == self_uid){
            peeruid = user2_id;
        }
        assert(UserManager::GetInstance()->isCreateThreadByThreadID(peeruid));
    }
    // 告知ChatDialog这个记录加载完成
    emit signalSpecifiedChatUserLoadFinished(thread_info);
}

void SqliteManager::modifyMsgId(int thread_id, int min_message_id, int max_message_id)
{
    QSqlQuery query(db_);
    query.prepare(R"SQL(
      INSERT INTO thread_message_range (thread_id, min_message_id, max_message_id, updated_at, self_uid)
      VALUES (:thread_id, :min_id, :max_id, CURRENT_TIMESTAMP, :self_uid)
      ON CONFLICT(thread_id, self_uid)
      DO UPDATE SET
          min_message_id = excluded.min_message_id,
          max_message_id = excluded.max_message_id,
          updated_at = CURRENT_TIMESTAMP
    )SQL");

    int self_uid = UserManager::GetInstance()->getUid();
    query.bindValue(":thread_id", thread_id);
    query.bindValue(":min_id", min_message_id);
    query.bindValue(":max_id", max_message_id);
    query.bindValue(":self_uid", self_uid);

    if (!query.exec()) {
        qDebug() << "upsertThreadMessageRange failed:" << query.lastError().text();
    }
}

void SqliteManager::storeMessages(std::vector<std::shared_ptr<ChatDataBase>> messages)
{
    if (messages.empty()) {
        return; // 空向量，无需操作
    }

    // 开始事务
    if (!db_.transaction()) {
        qWarning() << "Failed to start transaction:" << db_.lastError().text();
        return;
    }

    QSqlQuery query(db_);
    QDateTime currentTime = QDateTime::currentDateTimeUtc();

    // 注意：这可能会删除并重新插入记录
    query.prepare(
        "INSERT OR REPLACE INTO messages "
        "(message_id, thread_id, sender_id, recv_id, content, created_at, updated_at, status, message_type) "
        "VALUES (:message_id, :thread_id, :sender_id, :recv_id, :content, :created_at, :updated_at, :status, :message_type)"
    );

    // 批量绑定并执行
    for (const auto& msg : messages) {
        if (!msg) {
            continue; // 跳过空指针
        }
        //qDebug() << "Add msgId = " <<  msg->GetMsgId() << " content = " << msg->GetContent() << "in local database.";
        // 绑定参数
        query.bindValue(":message_id", msg->GetMsgId());
        query.bindValue(":thread_id", msg->GetThreadId());
        query.bindValue(":sender_id", msg->_send_uid);
        query.bindValue(":recv_id", msg->_recv_uid_);
        query.bindValue(":content", msg->_msg_content);

        query.bindValue(":created_at", msg->GetChatTime());
        query.bindValue(":updated_at", currentTime);
        query.bindValue(":status", msg->_status);
        query.bindValue(":message_type", msg->_msg_type);

        if (!query.exec()) {
            qWarning() << "Failed to insert/update message ID" << msg->GetMsgId()
                      << ":" << query.lastError().text();
            db_.rollback();
            return;
        }

        query.finish();
    }

    // 提交事务
    if (!db_.commit()) {
        qWarning() << "Failed to commit transaction:" << db_.lastError().text();
        db_.rollback();
        return;
    }
    //qDebug() << "Successfully stored" << messages.size() << "messages";
    return;
}

void SqliteManager::loadChatMessage(int thread_id)
{
    // 检查数据库是否打开
    if (!db_.isOpen()) {
        qWarning() << "Database is not open!";
        return;
    }

    // 获取当前线程的last_message_id
    int last_message_id = last_chat_message_id_[thread_id];

    // 准备查询：获取message_id <= last_message_id的最近20条记录
    QSqlQuery query(db_);
    query.prepare(
        "SELECT message_id, thread_id, sender_id, recv_id, content, "
        "created_at, updated_at, status, message_type "
        "FROM messages "
        "WHERE thread_id = :thread_id AND message_id < :last_message_id "
        "ORDER BY message_id DESC "  // 按message_id降序，获取最新的
        "LIMIT :page_size"  // 取 CHATMESSAGE_LOAD_PAGESIZE_FROM_LOCAL + 1 条，用于判断是否还有更多
    );

    qDebug() << "[DB] " << "thread_id = " << thread_id << " ,last_message_id = " << last_message_id;

    query.bindValue(":thread_id", thread_id);
    query.bindValue(":last_message_id", last_message_id);
    query.bindValue(":page_size",CHATMESSAGE_LOAD_PAGESIZE_FROM_LOCAL + 1);

    if (!query.exec()) {
        qWarning() << "Failed to load messages for thread" << thread_id
                  << ":" << query.lastError().text();
        return;
    }

    // 保存在缓存
    auto thread_data = UserManager::GetInstance()->GetChatThreadDataBuThreadID(thread_id);
    assert(thread_data != nullptr);
    std::vector<std::shared_ptr<ChatDataBase>> messages;
    int count = 0;
    int min_message_id = INT_MAX;
    while (query.next()) {
        count++;
        // 只取前 CHATMESSAGE_LOAD_PAGESIZE_FROM_LOCAL 条
        if (count > CHATMESSAGE_LOAD_PAGESIZE_FROM_LOCAL) {
            break;
        }
        int message_id = query.value("message_id").toInt();
        min_message_id = std::min(message_id,min_message_id);
        int thread_id = query.value("thread_id").toInt();
        int sender_id = query.value("sender_id").toInt();
        int recv_id = query.value("recv_id").toInt();
        QString content = query.value("content").toString();
        int status = query.value("status").toInt();
        QDateTime createdTime = query.value("created_at").toDateTime();
        QDateTime updatedTime = query.value("created_at").toDateTime();
        int message_type = query.value("message_type").toInt();

        std::shared_ptr<ChatDataBase> data = nullptr;
        if(message_type == int(CHAT_MSG_TYPE::TEXT_MSG)){
            data = std::make_shared<TextChatData>(message_id,"",thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                          CHAT_MSG_TYPE::TEXT_MSG,content,sender_id,recv_id,status,createdTime);
        }else if(message_type == int(CHAT_MSG_TYPE::PIC_MSG)){
            std::shared_ptr<MsgInfo> msg_info;
            //o do ... 需要将这个信息，保存到 UserManager 的 trans_info/ download_info,如果是上传，那么就执行续传逻辑； 如果是下载，那么就执行 继续下载的逻辑
            int self_uid = UserManager::GetInstance()->getUid();
            if(sender_id == self_uid){
                // 上传
                msg_info = queryTransferMsgInfo(self_uid, TRANSFER_TYPE::Upload, content);
                UserManager::GetInstance()->add_trans_file(content, msg_info);
            }else{
                // 下载
                msg_info = queryTransferMsgInfo(self_uid, TRANSFER_TYPE::Download, content);
//                std::shared_ptr<DownloadFileInfo> download_file = std::make_shared<DownloadFileInfo>();
//                download_file->state_ = msg_info->_state;
//                download_file->trans_size_ = msg_info->current_size_;
//                download_file->total_size_ = msg_info->total_size_;
//                UserManager::GetInstance()->add_download_file(content, download_file);
                UserManager::GetInstance()->add_trans_file(content, msg_info);
            }
            QUuid uuid = QUuid::createUuid();
            QString unique_id = uuid.toString();
            data = std::make_shared<ImageDataBase>(msg_info,message_id,unique_id,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                   CHAT_MSG_TYPE::PIC_MSG,sender_id,recv_id,status,createdTime);

        }else if(message_type == int(CHAT_MSG_TYPE::FILE_MSG)){
            std::shared_ptr<MsgInfo> msg_info;
            //o do ... 需要将这个信息，保存到 UserManager 的 trans_info/ download_info,如果是上传，那么就执行续传逻辑； 如果是下载，那么就执行 继续下载的逻辑
            int self_uid = UserManager::GetInstance()->getUid();
            if(sender_id == self_uid){
                // 上传
                msg_info = queryTransferMsgInfo(self_uid, TRANSFER_TYPE::Upload, content);
                UserManager::GetInstance()->add_trans_file(content, msg_info);
            }else{
                // 下载
                msg_info = queryTransferMsgInfo(self_uid, TRANSFER_TYPE::Download, content);
//                std::shared_ptr<DownloadFileInfo> download_file = std::make_shared<DownloadFileInfo>();
//                download_file->state_ = msg_info->_state;
//                download_file->trans_size_ = msg_info->current_size_;
//                download_file->total_size_ = msg_info->total_size_;
//                UserManager::GetInstance()->add_download_file(content, download_file);
                UserManager::GetInstance()->add_trans_file(content, msg_info);
            }
            QUuid uuid = QUuid::createUuid();
            QString unique_id = uuid.toString();
            data = std::make_shared<ImageDataBase>(msg_info,message_id,unique_id,thread_id,CHAT_THREAD_TYPE::CHAT_THREAD_TYPE_PRIVATE,
                                                   CHAT_MSG_TYPE::FILE_MSG,sender_id,recv_id,status,createdTime);
        }else{

        }
        messages.push_back(data);
        thread_data->AddMsg(data);
    }
    last_chat_message_id_[thread_id] = min_message_id;
    // 记录实际取出的数据条数
   // qDebug() << "Thread" << thread_id << ": Loaded" << messages.size() << "messages, count =" << count;
    // 判断是否还有更多消息可加载
    if (count <= CHATMESSAGE_LOAD_PAGESIZE_FROM_LOCAL) {
        is_load_more_chatmessage_[thread_id] = false;
    } else {
        is_load_more_chatmessage_[thread_id] = true;
    }

    // 通知chatView去渲染数据
    emit signalNotifyIsLoadMoreMsg(thread_id,is_load_more_chatmessage_[thread_id],messages);
}

void SqliteManager::setFriendApplyStatus(int peeruid, FRIEND_APPLY status)
{
    // 设置好友的申请状态
    int self_uid = UserManager::GetInstance()->getUid();
    // 当self_uid = self_uid 以及 peeruid = uid 以及 status = 0 的时候，修改这个记录的 status

    QSqlQuery query(db_);
    query.prepare("UPDATE apply_info SET status = :status where self_uid = :self_uid AND uid = :peeruid AND status = 0");
    query.bindValue(":status",int(status));
    query.bindValue(":self_uid",self_uid);
    query.bindValue(":peeruid",peeruid);
    if(!query.exec()){
        qWarning() << "Failed to setFriendApplyStatus for peeruid " << peeruid
                  << ":" << query.lastError().text();
        return;
    }
    qDebug() << "Set set friend apply status to " << status << " success, peeruid = " << peeruid;
}

void SqliteManager::addTransferToDb(QString unique_name,int trans_size,int total_size,QString local_path,int state,int trans_type,QString error)
{
    qDebug() << "[addTransferToDb] unique_name = " << unique_name << "trans_type = " << trans_type;

    const int operator_uid = UserManager::GetInstance()->getUid();
    const qint64 now_ms = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(db_);
    query.prepare(R"SQL(
        INSERT INTO trans_situation
        (unique_name, direction, operator_uid, trans_bytes, total_bytes, local_path, status, error_msg, created_at_ms, updated_at_ms)
        VALUES
        (:unique_name, :direction, :operator_uid, :trans_bytes, :total_bytes, :local_path, :status, :error_msg, :created_at_ms, :updated_at_ms)
    )SQL");

    query.bindValue(":unique_name", unique_name);
    query.bindValue(":direction",   trans_type);     // 0 download / 1 upload
    query.bindValue(":operator_uid", operator_uid);
    query.bindValue(":trans_bytes", trans_size);
    query.bindValue(":total_bytes", total_size);
    query.bindValue(":local_path",  local_path);
    query.bindValue(":status",      state);          // 0..4
    query.bindValue(":error_msg",   error);
    query.bindValue(":created_at_ms", now_ms);
    query.bindValue(":updated_at_ms", now_ms);

    if (!query.exec()) {
        qDebug() << "[DB] addTransferToDb INSERT failed:"
                 << query.lastError().text()
                 << " unique_name=" << unique_name;
    }
}

void SqliteManager::saveTransferToDb(QString unique_name,
                                     int trans_size,
                                     int total_size,
                                     int trans_status,
                                     int trans_type,
                                     QString error)
{
    const int operator_uid = UserManager::GetInstance()->getUid();
    const qint64 now_ms = QDateTime::currentMSecsSinceEpoch();

    QSqlQuery query(db_);
    query.prepare(R"SQL(
        UPDATE trans_situation
        SET
            direction     = :direction,
            operator_uid  = :operator_uid,
            trans_bytes   = :trans_bytes,
            total_bytes   = :total_bytes,
            status        = :status,
            error_msg     = :error_msg,
            updated_at_ms = :updated_at_ms
        WHERE unique_name = :unique_name AND operator_uid = :operator_uid AND direction = :direction
    )SQL");

    query.bindValue(":direction",   trans_type);
    query.bindValue(":operator_uid", operator_uid);
    query.bindValue(":trans_bytes", trans_size);
    query.bindValue(":total_bytes", total_size);
    query.bindValue(":status",      trans_status);
    query.bindValue(":error_msg",   error);
    query.bindValue(":updated_at_ms", now_ms);
    query.bindValue(":unique_name", unique_name);
    query.bindValue(":operator_uid",UserManager::GetInstance()->getUid());
    query.bindValue(":direction",trans_type);

    if (!query.exec()) {
        qDebug() << "[DB] saveTransferToDb UPDATE failed:"
                 << query.lastError().text()
                 << " unique_name=" << unique_name;
        return;
    }

    // 可选：如果没更新到任何行，说明 unique_name 不存在
    if (query.numRowsAffected() == 0) {
        qDebug() << "[DB] saveTransferToDb: no row updated, unique_name not found:"
                 << unique_name;
    }
}

std::shared_ptr<MsgInfo> SqliteManager::queryTransferMsgInfo(int operator_uid, int type, const QString &unique_name)
{
    // 精确匹配：operator_uid + status + unique_name
    // 如果你希望 status = -1 时不筛选状态，可用下面“可选写法”
    QSqlQuery query;
    query.prepare(R"SQL(
        SELECT
            unique_name,
            local_path,
            total_bytes,
            trans_bytes,
            direction,
            status
        FROM trans_situation
        WHERE operator_uid = :operator_uid
          AND direction    = :direction
          AND unique_name  = :unique_name
        LIMIT 1
    )SQL");

    query.bindValue(":operator_uid", operator_uid);
    query.bindValue(":direction", type);
    query.bindValue(":unique_name", unique_name);

    if (!query.exec()) {
        qDebug() << "[DB] queryTransferMsgInfo exec failed:" << query.lastError().text()
                 << " operator_uid=" << operator_uid
                 << " direction=" << type
                 << " unique_name=" << unique_name;
        return nullptr;
    }

    if (!query.next()) {
        return nullptr;
    }

    auto info = std::make_shared<MsgInfo>();
    info->unique_name_   = query.value(0).toString();
    info->text_or_url_   = query.value(1).toString();        // 本地路径
    info->total_size_    = query.value(2).toLongLong();
    info->current_size_  = query.value(3).toLongLong();

    const int direction  = query.value(4).toInt();           // 1 download /  0 upload
    const int st         = query.value(5).toInt();

    info->_type  = static_cast<TRANSFER_TYPE>(direction);
    info->_state = static_cast<TRANSFER_STATE>(st);

    return info;
}



























