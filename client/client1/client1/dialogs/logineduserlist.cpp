#include "logineduserlist.h"
#include "logineduserwidget.h"

LoginedUserList::LoginedUserList(QWidget* parent)
    : QListWidget(parent)
{
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->viewport()->installEventFilter(this);

    connect(this,&QListWidget::itemClicked,this,&LoginedUserList::slotItemClicked);
}

void LoginedUserList::slotItemClicked(QListWidgetItem *item)
{
    QWidget* wid = this->itemWidget(item);

    ListItemBase* base = static_cast<ListItemBase*>(wid);
    ListItemType type = base->getItemType();

    if(type != ListItemType::LOGINED_USER_ITEM){
        qDebug() << "error ListItemType: " << type;
        return;
    }

    auto t = dynamic_cast<LoginedUserWidget*>(base);

    if(t != nullptr){
        // 发送信号，给LoginDialog
        emit signalFullLineEdit(t->GetUserInfo());
    }

}

bool LoginedUserList::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == this->viewport())
    {
        if(event->type() == QEvent::Enter)
        {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        else if(event->type() == QEvent::Leave)
        {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        else if(event->type() == QEvent::Wheel)
        {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            // 获取鼠标的滚动角度 （因为angleDelta().y()的单位是 1/8度，所以要 除以8）
            int numDegrees = wheelEvent->angleDelta().y() / 8;
            int numSteps = numDegrees / 15;
            // 设置滚动幅度
            this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);
            // 检查是否滚顶到了底部
            QScrollBar* scrollBar = this->verticalScrollBar();
            int maxScrollValue = scrollBar->maximum();
            int currentValue = scrollBar->minimum();

            if(maxScrollValue - currentValue <= 0)
            {
                // 表明加载到了底部
                qDebug() << "no more history user.";
            }
            return true;
        }
    }
    return QListWidget::eventFilter(watched,event);
}
