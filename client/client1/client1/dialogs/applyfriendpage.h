#ifndef APPLYFRIENDPAGE_H
#define APPLYFRIENDPAGE_H

#include "core/global.h"
#include "core/userdata.h"
#include "applyfrienditem.h"

namespace Ui {
class ApplyFriendPage;
}

class ApplyFriendPage : public QWidget
{
    Q_OBJECT

public:
    explicit ApplyFriendPage(QWidget *parent = nullptr);
    ~ApplyFriendPage();
    void addFriendApply(std::vector<std::shared_ptr<ApplyInfo>> apply_info); // 渲染本地数据
    void addNewFriendApply(std::shared_ptr<AddFriendApply> apply); // 添加新的好友申请

public slots:
    // 收到 tcpMsg 传回来的好友申请通过的信息
    void slotAuthRsp(std::shared_ptr<AuthRsp> authRsp);
    void acceptApply(ApplyFriendItem* item);
    void refuseApply(ApplyFriendItem* item);
    void slotLoadFriendApplyList(QJsonObject obj);
    void slotDrawFriendApplyToList(std::vector<std::shared_ptr<ApplyInfo>> apply_list);
    void slotUserIconChange(QString icon);
    void slotOnlineFriendIconFinished(int icon_uid, QString icon);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *ev) override;

private:
    Ui::ApplyFriendPage *ui;
    // 没有通过申请的用户的  uid,ApplyFriendItem
    std::unordered_map<int, ApplyFriendItem*> unAuthItems_;
    QMap<int,QListWidgetItem*> ApplyAddedItem_;
    QMap<QString,int> icons_;

    // 是否可以继续加载
    bool is_load_more_from_server_;
    bool is_load_more_from_local_;

signals:
    void signalShowSearch(bool);
    void signalRequestMoreFromServer();

};

#endif // APPLYFRIENDPAGE_H
