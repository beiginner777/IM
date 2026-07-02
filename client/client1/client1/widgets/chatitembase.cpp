#include "chatitembase.h"

ChatItemBase::ChatItemBase(ChatRole role, QWidget* parent) : QWidget(parent), role_(role)
{
    // 名字的属性
    nameLabel_ = new QLabel();
    nameLabel_->setObjectName("chatUserName");
    QFont font("Microsoft YaHei");
    font.setPointSize(7);
    nameLabel_->setFont(font);
    nameLabel_->setFixedHeight(20);

    // 头像的标签
    iconLabel_ = new QLabel();
    iconLabel_->setFixedSize(60,60);

    // 消息框的标签
    bubble_ = new QWidget();

    // 将所有的都设置在QGridLayout中进行显示，所以对QGridLayout进行初始化
    QGridLayout* gridLayout_ = new QGridLayout();
    gridLayout_->setVerticalSpacing(3);
    gridLayout_->setHorizontalSpacing(3);
    gridLayout_->setMargin(3);

    // 弹簧
    QSpacerItem* spacer_ = new QSpacerItem(40,20,QSizePolicy::Expanding,QSizePolicy::Minimum);

    //添加状态图标控件
    statusLabel_ = new QLabel();
    statusLabel_->setFixedSize(25,25);
    statusLabel_->setScaledContents(true);

    if(role_ == ChatRole::self)
    {
        nameLabel_->setContentsMargins(0,0,8,0);
        nameLabel_->setAlignment(Qt::AlignRight);
        gridLayout_->addWidget(nameLabel_, 0,2, 1,1);
        gridLayout_->addWidget(iconLabel_, 0, 3, 2,1, Qt::AlignTop);
        gridLayout_->addItem(spacer_, 1, 0, 1, 1);
        gridLayout_->addWidget(statusLabel_,1,1,1,1,Qt::AlignCenter);
        gridLayout_->addWidget(bubble_, 1,2, 1,1);
        gridLayout_->setColumnStretch(0, 2);
        gridLayout_->setColumnStretch(1,0); //status图标(固定大小)
        gridLayout_->setColumnStretch(2,3); //名字+气泡(主要拉伸区域)
        gridLayout_->setColumnStretch(3, 0);
    }else{
        nameLabel_->setContentsMargins(8,0,0,0);
        nameLabel_->setAlignment(Qt::AlignLeft);
        gridLayout_->addWidget(iconLabel_, 0, 0, 2,1, Qt::AlignTop);
        gridLayout_->addWidget(nameLabel_, 0,1, 1,1);
        gridLayout_->addWidget(bubble_, 1,1, 1,1);
        gridLayout_->addItem(spacer_, 2, 2, 1, 1);
        gridLayout_->setColumnStretch(1, 3);
        gridLayout_->setColumnStretch(2, 2);
    }
    this->setLayout(gridLayout_);
}

void ChatItemBase::setUserName(const QString &name)
{
    nameLabel_->setText(name);
}

void ChatItemBase::setUserIcon(const QPixmap &icon)
{
    // 图片自适应QLabel的大小
    iconLabel_->setScaledContents(true);
    // 将图片设置到QLabel
    iconLabel_->setPixmap(icon);
}

void ChatItemBase::setWidget(QWidget *widget)
{
    // 相当于是替换掉当前的消息框

    // 获取当前整个聊天框
    QGridLayout* gridLayout = (qobject_cast<QGridLayout *>)(this->layout());
    // 将聊天框的Widget进行替换
    gridLayout->replaceWidget(bubble_,widget);
}

void ChatItemBase::setStatus(int status)
{
    if(status == MsgStatus::UN_READ){
        statusLabel_->setPixmap(QPixmap(":/res/unread.png"));
        return ;
    }

    if(status == MsgStatus::SEND_FAILED){
        statusLabel_->setPixmap(QPixmap(":/res/send_fail.png"));
        return ;
    }

    if(status == MsgStatus::READED){
        statusLabel_->setPixmap(QPixmap(":/res/readed.png"));
        return ;
    }
}
