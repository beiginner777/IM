#ifndef CLICKLABEL_H
#define CLICKLABEL_H

#include "core/global.h"

class ClickLabel : public QLabel
{
    Q_OBJECT
public:
    ClickLabel(QWidget* parent = nullptr);
    void setState(QString normal = "",QString hover = "",QString press = "",
                  QString select = "",QString select_hover = "",QString select_press = "");
    ClickLbState getCurState();
    void resetNormalState();
    void setCurState(ClickLbState state);
protected:
    virtual void mousePressEvent(QMouseEvent *ev) override;
    virtual void mouseReleaseEvent(QMouseEvent *ev) override;
    virtual void enterEvent(QEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
private:
    QString normal_;
    QString normal_hover_;
    QString normal_press_;

    QString selected_;
    QString selected_hover_;
    QString selected_press_;

    // 只是控制 选择 / 不选择 两种状态
    ClickLbState curState_;

signals:
    void Clicked(QString text,ClickLbState state);
    void signalIsHide();
};

#endif // CLICKLABEL_H
