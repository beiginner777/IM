#include "timerbtn.h"
#include <QDebug>

TimerBtn::TimerBtn(QWidget *parent)
   : QPushButton(parent)
   , counter_(3)
{
    timer_ = new QTimer(this);

    connect(timer_,&QTimer::timeout,this,[this](){
        this->setText(QString::number(counter_));
        counter_--;
        if(counter_ < 0)
        {
            this->setText("GET");
            this->counter_ = 3;
            this->setEnabled(true);
            timer_->stop();
        }
    });
}

TimerBtn::~TimerBtn()
{
    timer_->stop();
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton){
        qDebug() << "leftButton is clicked.";
        this->timer_->start(1000); // 左键释放之后，每一秒触发一次 QTimer::timerout 信号
        this->setEnabled(false);

        // 发送点击信号
        emit clicked();
    }

    // 调用基类的mouseReleaseEvent以确保正常的事件处理（如点击效果）
    QPushButton::mouseReleaseEvent(e);
}


