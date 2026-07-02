#include "searchlist.h"
#include "network/tcpmsg.h"
#include "adduseritem.h"
#include "data/usermanager.h"
#include "findfaildialog.h"
#include "loadingdialog.h"

SearchList::SearchList(QWidget *parent)
    : QListWidget(parent), send_pending_(false),
      findDialog_(nullptr),searchEdit_(nullptr),
      loadingDialog_(nullptr)
{
    // 关闭滚动条
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 安装事件过滤器
    this->installEventFilter(this);
    // 添加条目（因为一开始没有输入，那么就显示默认的界面）
    addTipItem();
    // 返回搜索结果
    connect(TcpMsg::GetInstance().get(),&TcpMsg::signalSearchUser,this,&SearchList::slotSearchUser);
    // 当搜索栏的条目被点击之后
    // void itemClicked(QListWidgetItem *item);
    connect(this,&QListWidget::itemClicked,this,&SearchList::slotItemClick);
}

void SearchList::closeDialog()
{

}

void SearchList::setStateEdit(QWidget *w)
{

}

void SearchList::setSearchLineEdit(CustomizeEdit *edit)
{
    searchEdit_ = edit;
}

bool SearchList::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == this->viewport())
    {
        if(event->type() == QEvent::Enter)
        {
            // 鼠标进入，显示滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        else if(event->type() == QEvent::Leave)
        {
            // 鼠标离开，隐藏滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        else if(event->type() == QEvent::Wheel)
        {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            int numDegree = wheelEvent->angleDelta().y() / 8;
            int numSteps = numDegree / 15;
            this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);
            return true;
        }
        return QListWidget::eventFilter(obj,event);
    }
}

void SearchList::waitPending(bool pending)
{
    if(pending)
    {
        // 正在发送，展示LoadingDialog
        if(loadingDialog_ == nullptr){
            loadingDialog_ = new LoadingDialog(this);
        }
        loadingDialog_->show();
        loadingDialog_->setModal(true);
    }
    else
    {
        loadingDialog_->hide();
    }
}

void SearchList::addTipItem()
{
    QListWidgetItem* item = new QListWidgetItem();
    item->setSizeHint(QSize(350,10));
    auto invalidItem = new ListItemBase();
    invalidItem->setItemType(INVALID_ITEM);

    this->addItem(item);
    this->setItemWidget(item,invalidItem);
    // item->setFlags(item->flags() & ~Qt::ItemIsSelectable);

    auto addUserItem = new AddUserItem();
    QListWidgetItem* item1 = new QListWidgetItem();
    item1->setSizeHint(QSize(350,80));
    this->addItem(item1);
    this->setItemWidget(item1,addUserItem);
}

void SearchList::slotSearchUser(std::shared_ptr<SearchInfo> si)
{
    // 搜索完成。
    send_pending_ = false;
    waitPending(false);

    if(si == nullptr){
        qDebug() << "find user failed.";
        findDialog_ = std::make_shared<FindFailDialog>(this);
        findDialog_->setObjectName("findFailDialog");
        findDialog_->show();
        return;
    }
    else if(si->name_ == UserManager::GetInstance()->getName())
    {
        qDebug() << "user is youself.";
        return;
    }
    else
    {
        findDialog_ = std::make_shared<FindSuccessDialog>();
        findDialog_->setObjectName("findSuccessDialog");
        std::dynamic_pointer_cast<FindSuccessDialog>(findDialog_)->setSearchInfo(si);
        findDialog_->show();
    }
}

void SearchList::slotItemClick(QListWidgetItem *item)
{
    // 获取这个item中的QWidget
    QWidget* widget = this->itemWidget(item);
    if(!widget)
    {
        qDebug() << "clicked the QListWidgetItem of nullptr";
        return;
    }
    // 判断这个条目是否是 添加好友的条目
    ListItemBase* baseItem = qobject_cast<ListItemBase*>(widget);
    ListItemType itemType = baseItem->getItemType();
    if(itemType == ListItemType::INVALID_ITEM){
        qDebug() << "it's a InvalidItem.";
        return;
    }
    if(itemType == ListItemType::ADD_USER_TIP_ITEM)
    {
        if(send_pending_){
            // 说明正在向服务器请求搜索的结果，直接返回
            return;
        }
        send_pending_ = true;
        waitPending(true);
        // 获取searchLineEdit的text并且将这个text发送给ChatServer，由ChatServer返回结果
        QString text = searchEdit_->text();
        QJsonObject obj;
        obj["uid"] = text;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        emit TcpMsg::GetInstance()->signalSendData(REQUEST_ID::ID_SEARCH_USER_REQ,data);
        return;
    }
    // 出现了未知的错误
    qDebug() << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ":    unexpected ListItemType.";
}




















