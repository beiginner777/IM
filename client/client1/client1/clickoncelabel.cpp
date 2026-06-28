#include "clickoncelabel.h"

ClickOnceLabel::ClickOnceLabel(QWidget *parent) : QLabel(parent)
{
    // 设置鼠标悬停在QLabel上的样式
    setCursor(Qt::PointingHandCursor);
}

void ClickOnceLabel::mousePressEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
    {
        qDebug() << "tip label was clicked.";
        emit Clicked(this->text());
        return;
    }
    return QLabel::mousePressEvent(ev);
}
