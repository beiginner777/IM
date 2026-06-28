#ifndef LOGINEDUSERWIDGET_H
#define LOGINEDUSERWIDGET_H

#include <QWidget>
#include "listitembase.h"
#include "userdata.h"

namespace Ui {
class LoginedUserWidget;
}

class LoginedUserWidget : public ListItemBase
{
    Q_OBJECT

public:
    explicit LoginedUserWidget(QWidget *parent = nullptr);
    ~LoginedUserWidget();

    QSize sizeHint() const override{
        return QSize(380,30);
    }

    void SetUserInfo(std::shared_ptr<UserInfo> userInfo);
    std::shared_ptr<UserInfo> GetUserInfo() { return userInfo_; }

private:
    Ui::LoginedUserWidget *ui;

    std::shared_ptr<UserInfo> userInfo_;
};

#endif // LOGINEDUSERWIDGET_H
