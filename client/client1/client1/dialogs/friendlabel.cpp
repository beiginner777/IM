#include "friendlabel.h"
#include "ui_friendlabel.h"

FriendLabel::FriendLabel(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FriendLabel)
{
    ui->setupUi(this);
    ui->closeLabel->setState("normal","hover","press",
                           "selected_normal","selected_hover","selected_press");
    setCursor(Qt::PointingHandCursor);
    connect(ui->closeLabel,&ClickLabel::Clicked,this,&FriendLabel::slotClose);

    /*qDebug() << "FriendLabel constructor";
    qDebug() << "closeLabel geometry:" << ui->closeLabel->geometry();
    qDebug() << "closeLabel size:" << ui->closeLabel->size();
    qDebug() << "closeLabel visible:" << ui->closeLabel->isVisible();

    // 确保closeLabel可见
    ui->closeLabel->setVisible(true);
    ui->widget->setVisible(true); // 确保父容器也可见

    qDebug() << "After setting visible - closeLabel visible:" << ui->closeLabel->isVisible();

    // 设置图片
    QPixmap closePixmap(":/res/tipclose.png");
    if (!closePixmap.isNull()) {
        qDebug() << "Close pixmap loaded, size:" << closePixmap.size();
        ui->closeLabel->setPixmap(closePixmap);
        ui->closeLabel->setScaledContents(true);
    } else {
        qDebug() << "Failed to load close pixmap!";
        ui->closeLabel->setText("X");
    }*/
}

FriendLabel::~FriendLabel()
{
    delete ui;
}

void FriendLabel::setText(QString text)
{
    text_ = text;
    ui->tipLabel->setText(text_);
    // why ....
    // 重新调整tipLabel的大小
    ui->tipLabel->adjustSize();

    QFontMetrics fontMetrics(ui->tipLabel->font());

    // 用文字本身来算宽度
    int textWidth  = fontMetrics.width(ui->tipLabel->text());
    int textHeight = fontMetrics.height();

    // 先打印，确保数值正确
    //qDebug() << "Text =" << ui->tipLabel->text();
    //qDebug() << "TextWidth =" << textWidth;
    //qDebug() << "TextHeight =" << textHeight;

    // 用我们自己算的值来固定大小
    //qDebug() << "closeLabel.width = "<< ui->closeLabel->width();
    int fixedW = textWidth + ui->closeLabel->width() + 5;
    int fixedH = textHeight + 2;
    this->setFixedSize(fixedW, fixedH);

    // 打印我们自己算的值（不要打印 this->width()/height()）
    //qDebug() << "FixedWidth =" << fixedW;
    //qDebug() << "FixedHeight =" << fixedH;

    //width_ = this->width();
    //height_ = this->height();

    width_ = fixedW;
    height_ = fixedH;
}

int FriendLabel::width()
{
    return width_;
}

int FriendLabel::height()
{
    return height_;
}

QString FriendLabel::getText()
{
    return text_;
}

void FriendLabel::slotClose()
{
    emit signalClose(text_);
}



