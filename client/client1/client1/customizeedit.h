#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H

#include "global.h"

class CustomizeEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomizeEdit(QWidget* parent = nullptr);
    void SetMaxLength(int max_len);
protected:
    // 重写失去焦点的函数
    void focusOutEvent(QFocusEvent * event) override;
    void keyPressEvent(QKeyEvent *) override;
private:
    // 限制文本的长度
    void limitTextlength(QString text);
    // 文本的最大长度
    int max_len_;
signals:
    // 失去焦点时，发出信号
    void signalFoucusOut();
};

#endif // CUSTOMIZEEDIT_H
