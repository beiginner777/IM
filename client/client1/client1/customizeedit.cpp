#include "customizeedit.h"

CustomizeEdit::CustomizeEdit(QWidget *parent) : QLineEdit(parent),max_len_(20)
{
    // 一旦text的内容发生变化，那么就触发槽函数 limitTextLength 来限制最大长度
    connect(this,&QLineEdit::textChanged,this,&CustomizeEdit::limitTextlength);
}

void CustomizeEdit::SetMaxLength(int max_len)
{
    max_len_ = max_len;
}

void CustomizeEdit::focusOutEvent(QFocusEvent * event)
{
    //qDebug() << "CustomizeEdit Foucus out.";
    // 调用父类的函数，保证基类的基本行为
    QLineEdit::focusOutEvent(event);
    // 触发失去焦点的信号
    emit signalFoucusOut();
}

void CustomizeEdit::keyPressEvent(QKeyEvent* ev)
{
    if(ev->key() == Qt::Key_Enter)
    {
        emit QLineEdit::returnPressed();
        return;
    }
    return QLineEdit::keyPressEvent(ev);
}

void CustomizeEdit::limitTextlength(QString text)
{
    if(max_len_ <= 0){
        return;
    }
    // 因为中文和英文每个字符所占的字节数不同，因此转化为字节数组，统一管理最大长度
    QByteArray byteArrary = text.toUtf8();
    if(byteArrary.size() > max_len_){
        // 超过了最大长度，那么就只取最大长度的部分，其它部分舍去
        byteArrary = byteArrary.left(max_len_);
        // 将一个存储着 UTF-8 编码文本数据的 QByteArray 对象，转换（解码）成一个 QString 对象。
        this->setText(QString::fromUtf8(byteArrary));
    }

}
