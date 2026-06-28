#ifndef FRIENDINFOINTERFACE_H
#define FRIENDINFOINTERFACE_H

#include "global.h"
#include "userdata.h"

namespace Ui {
class friendInfoInterface;
}

class friendInfoInterface : public QWidget
{
    Q_OBJECT

public:
    explicit friendInfoInterface(QWidget *parent = nullptr);
    ~friendInfoInterface();
    void setUserInfo(std::shared_ptr<UserInfo> userInfo);

private slots:
    void on_chatLabel_clicked();

signals:
    void signalSwitchFromCotactToChat(int);

private:
    Ui::friendInfoInterface *ui;

    std::shared_ptr<UserInfo> userInfo_;

    int selected_uid_;
};

#endif // FRIENDINFOINTERFACE_H
