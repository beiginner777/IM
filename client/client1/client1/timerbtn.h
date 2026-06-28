

#ifndef TIMERBTN_H
#define TIMERBTN_H

#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <qevent.h>

class TimerBtn : public QPushButton
{
    Q_OBJECT
public:
    TimerBtn(QWidget* parent);
    ~TimerBtn();
    // 重写鼠标释放事件
    virtual void mouseReleaseEvent(QMouseEvent *e);
    // 是否注册成功的标志
    bool isRegisterSuccess = false;
signals:
    void signalSwitchToLogin();
private:
    QTimer* timer_;
    int counter_;
};

#endif // TIMERBTN_H
