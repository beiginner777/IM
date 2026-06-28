#ifndef CLICKBUTTON_H
#define CLICKBUTTON_H

#include "global.h"

class ClickButton : public QPushButton
{
public:
    ClickButton(QWidget* parent = nullptr);
    void setState(QString normal,QString hover,QString press);
protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void enterEvent(QEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
private:
    QString normal_;
    QString hover_;
    QString press_;
};

#endif // CLICKBUTTON_H
