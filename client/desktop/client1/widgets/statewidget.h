#ifndef STATEWIDGET_H
#define STATEWIDGET_H

#include "core/global.h"

class StateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StateWidget(QWidget* parent = nullptr);

    void setState(QString normal = "",QString hover = "",QString press = "",
                  QString select = "",QString select_hover = "", QString select_press = "");

    ClickLbState getCurState();
    void clearState();

    void setSelected(bool selected = true);
    void addRedPoint();
    void showRedPoint(bool show = true);

protected:
    void paintEvent(QPaintEvent *event) override;

    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void enterEvent(QEvent* event) override;
    virtual void leaveEvent(QEvent *event) override;

private:
    QString normal_;
    QString normal_hover_;
    QString normal_press_;

    QString selected_;
    QString selected_hover_;
    QString selected_press_;

    ClickLbState curState_;
    QLabel* redPoint_;

signals:
    void clicked(void);

};

#endif // STATEWIDGET_H
