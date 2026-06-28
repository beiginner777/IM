#include "clickbutton.h"

ClickButton::ClickButton(QWidget *parent) : QPushButton(parent)
{
    // 设置鼠标的样式为 小手
    setCursor(Qt::PointingHandCursor);
}

void ClickButton::setState(QString normal, QString hover, QString press)
{
    normal_ = normal;
    hover_ = hover;
    press_ = press;

    setProperty("state",normal);
    repolish(this);
    update();
}

void ClickButton::mousePressEvent(QMouseEvent *event)
{
    setProperty("state",press_);
    repolish(this);
    update();
    QPushButton::mousePressEvent(event);
}

void ClickButton::mouseReleaseEvent(QMouseEvent *event)
{
    setProperty("state",hover_);
    repolish(this);
    update();

    QPushButton::mouseReleaseEvent(event);
}

void ClickButton::enterEvent(QEvent *event)
{
    setProperty("state",hover_);
    repolish(this);
    update();
    QPushButton::enterEvent(event);
}

void ClickButton::leaveEvent(QEvent *event)
{
    setProperty("state",normal_);
    repolish(this);
    update();

    QPushButton::leaveEvent(event);
}
