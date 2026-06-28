#ifndef FRIENDLABEL_H
#define FRIENDLABEL_H

#include "core/global.h"

namespace Ui {
class FriendLabel;
}

class FriendLabel : public QFrame
{
    Q_OBJECT

public:
    explicit FriendLabel(QWidget *parent = nullptr);
    ~FriendLabel();

    void setText(QString text);
    int width();
    int height();
    QString getText();

private:
    Ui::FriendLabel *ui;

    QString text_;
    int width_;
    int height_;

public slots:
    void slotClose();

signals:
    void signalClose(QString);
};

#endif // FRIENDLABEL_H
