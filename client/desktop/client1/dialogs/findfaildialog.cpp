#include "findfaildialog.h"
#include "ui_findfaildialog.h"

FindFailDialog::FindFailDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindFailDialog)
{
    ui->setupUi(this);

    // 去掉边框和标题栏
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    // 加载qss样式
    // 设置qss样式
    QFile styleFile(":/style/findfaildialog.qss");
    if(!styleFile.open(QFile::ReadOnly)){
        qDebug("open chat.qss failed!");
    }
    QString style = QLatin1String(styleFile.readAll());
    this->setStyleSheet(style);

    ui->confirmButton->setState("normal","hover","press");
}

FindFailDialog::~FindFailDialog()
{
    delete ui;
}

void FindFailDialog::on_confirmButton_clicked()
{
    this->hide();
    // this->deleteLater();
}
