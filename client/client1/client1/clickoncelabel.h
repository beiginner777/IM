#ifndef CLICKONCELABEL_H
#define CLICKONCELABEL_H

#include "global.h"

class ClickOnceLabel : public QLabel
{
    Q_OBJECT
public:
    ClickOnceLabel(QWidget* parent = nullptr);

protected:
    virtual void mousePressEvent(QMouseEvent *ev) override;

signals:
    void Clicked(QString);
};

#endif // CLICKONCELABEL_H
