#include "clicklabel.h"

ClickLabel::ClickLabel(QWidget *parent) : QLabel(parent),curState_(ClickLbState::normal)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickLabel::setState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
    normal_ = normal;
    normal_hover_ = hover;
    normal_press_ = press;
    selected_ = select;
    selected_hover_ = select_hover;
    selected_press_ = select_press;
    setProperty("state",normal);
    repolish(this);
    update();
}

ClickLbState ClickLabel::getCurState()
{
    return curState_;
}

void ClickLabel::resetNormalState()
{
    setProperty("state",normal_);
    repolish(this);
    update();
}

void ClickLabel::setCurState(ClickLbState state)
{
    curState_ = state;
    if(curState_ == ClickLbState::normal){
        setProperty("state",normal_);
    }else if(curState_ == ClickLbState::select){
        setProperty("state",selected_);
    }
    repolish(this);
    update();
}

void ClickLabel::mousePressEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
    {
        if(curState_ == ClickLbState::select){
            // qDebug() << "ClickLabel change to normal.";
            curState_ = ClickLbState::normal;
            setProperty("state",normal_press_);
            repolish(this);
            update();
        }
        else{
            // qDebug() << "ClickLabel change to select.";
            curState_ = ClickLbState::select;
            setProperty("state",selected_press_);
            repolish(this);
            update();
        }
        // 鼠标点击之后，触发切换 密码 显示形式的切换 的信号
        emit signalIsHide();
        // 申请好友Dialog
        emit Clicked(this->text(),curState_);

        return;
    }
    return QLabel::mousePressEvent(ev);
}

void ClickLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
    {
        if(curState_ == ClickLbState::normal){
            setProperty("state",normal_hover_);
        }
        else{
            setProperty("state",selected_hover_);
        }
        repolish(this);
        update();
    }
    return QLabel::mouseReleaseEvent(ev);
}

void ClickLabel::enterEvent(QEvent *event)
{
    if(curState_ == ClickLbState::normal){
        setProperty("state",normal_hover_);
    }
    else{
        setProperty("state",selected_hover_);
    }
    repolish(this);
    update();
    return QLabel::enterEvent(event);
}

void ClickLabel::leaveEvent(QEvent *event)
{
    if(curState_ == ClickLbState::normal){
        setProperty("state",normal_);
    }
    else{
        setProperty("state",selected_);
    }
    repolish(this);
    update();
    return QLabel::leaveEvent(event);
}

void ClickLabel::paintEvent(QPaintEvent *event)
{
    // 当前控件的样式
    QStyleOption opt;
    opt.init(this);
    // 表示绘画对象是当前的控件
    QPainter p(this);
    this->style()->drawPrimitive(QStyle::PE_Widget,&opt,&p,this);
    return QLabel::paintEvent(event);
}
