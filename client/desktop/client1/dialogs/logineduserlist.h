#ifndef LOGINEDUSERLIST_H
#define LOGINEDUSERLIST_H

#include "core/global.h"
#include "core/userdata.h"

class LoginedUserList: public QListWidget
{
    Q_OBJECT
public:
    LoginedUserList(QWidget* parent = nullptr);
public slots:
    void slotItemClicked(QListWidgetItem* item);
protected:
    bool eventFilter(QObject* watched,QEvent* event) override;
signals:
    void signalFullLineEdit(std::shared_ptr<UserInfo>);
};

#endif // LOGINEDUSERLIST_H
