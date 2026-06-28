#include "statewidget.h"

StateWidget::StateWidget(QWidget *parent) : QWidget(parent)
{
    // 设置鼠标样式
    setCursor(Qt::PointingHandCursor);
    // 添加红点
    addRedPoint();
}

void StateWidget::setState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
    normal_ = normal;
    normal_hover_ = hover;
    normal_press_ = press;

    selected_ = select;
    selected_hover_ = select_hover;
    selected_press_ = select_press;

    // 设置状态
    setProperty("state",normal_);
    // 刷新样式
    repolish(this);
    update();
}

ClickLbState StateWidget::getCurState()
{
    return curState_;
}

void StateWidget::clearState()
{
    curState_ =  ClickLbState::normal;
    setProperty("state",normal_);
    redPoint_->setVisible(false);
    // 更改样式
    repolish(this);
    // 重新绘制
    update();
}

void StateWidget::setSelected(bool selected)
{
    if(selected){
        curState_ = ClickLbState::select;
        setProperty("state",selected_);
    }
    else{
        curState_ = ClickLbState::normal;
        setProperty("state",normal_);
    }
    repolish(this);
    update();
}

void StateWidget::addRedPoint()
{
    redPoint_ = new QLabel(this);  // 直接指定父窗口
    redPoint_->setObjectName("redPoint");
    redPoint_->setGeometry(0, 0, 10, 10);   // 设置在(0,0)位置，大小10x10
    redPoint_->setVisible(false);
}

void StateWidget::showRedPoint(bool show)
{
    redPoint_->setVisible(true);
    repolish(this);
    update();
}

void StateWidget::paintEvent(QPaintEvent *event)
{
    // 当前控件的样式
    QStyleOption opt;
    opt.init(this);
    // 表示绘画对象是当前的控件
    QPainter p(this);
    this->style()->drawPrimitive(QStyle::PE_Widget,&opt,&p,this);

    return QWidget::paintEvent(event);
}

void StateWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        if(curState_ == ClickLbState::select)
        {
            qDebug() << "PressEvent:: Widget already selected";
        }
        else if(curState_ == ClickLbState::normal)
        {
            qDebug() << "PressEvent:: change to selected state";
            curState_ = ClickLbState::select;
            setProperty("state",selected_press_);
            repolish(this);
            update();
        }

        // 触发点击事件，这样侧边栏的状态才可以进行切换
        emit clicked();
    }

    return QWidget::mousePressEvent(event);
}

void StateWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        if(curState_ == ClickLbState::normal)
        {
            setProperty("state",normal_hover_);
            repolish(this);
            update();
        }
        else
        {
            setProperty("state",selected_hover_);
            repolish(this);
            update();
        }
    }

    return QWidget::mouseReleaseEvent(event);
}

void StateWidget::enterEvent(QEvent *event)
{
    if(curState_ == ClickLbState::normal)
    {
        setProperty("state",normal_hover_);
        repolish(this);
        update();
    }
    else
    {
        setProperty("state",selected_hover_);
        repolish(this);
        update();
    }
    return QWidget::enterEvent(event);
}

void StateWidget::leaveEvent(QEvent *event)
{
    if(curState_ == ClickLbState::normal)
    {
        setProperty("state",normal_);
        repolish(this);
        update();
    }
    else
    {
        setProperty("state",selected_);
        repolish(this);
        update();
    }
    return QWidget::leaveEvent(event);
}
