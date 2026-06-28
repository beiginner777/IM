#ifndef CONTACTUSERWIDGET_H
#define CONTACTUSERWIDGET_H

#include "global.h"
#include "listitembase.h"
#include "userdata.h"

namespace Ui {
class ContactUserWidget;
}

class ContactUserWidget : public ListItemBase
{
    Q_OBJECT
public:
    explicit ContactUserWidget(QWidget *parent = nullptr);
    ~ContactUserWidget();

    QSize sizeHint() const override{
        return QSize(350,80);
    }

    void SetInfo(int uid, QString name, QString icon);
    void SetInfo(std::shared_ptr<FriendInfo> friendInfo);
    void setIcon(QString icon);

    void showRedPoint(bool show = true);

    std::shared_ptr<UserInfo> getUserInfo() { return info_; }

private:
    Ui::ContactUserWidget *ui;

    QString head_;
    QString name_;

    std::shared_ptr<UserInfo> info_;
};

#endif // CONTACTUSERWIDGET_H
