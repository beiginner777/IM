#include "loadingdialog.h"
#include "ui_loadingdialog.h"
#include <QMovie>
#include <QDebug>

LoadingDialog::LoadingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoadingDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog |               // 作为对话框
                   Qt::FramelessWindowHint |  // 无边框
                   Qt::WindowSystemMenuHint | // 显示系统菜单
                   Qt::WindowStaysOnTopHint); // 始终置顶
    setAttribute(Qt::WA_TranslucentBackground); // 设置背景透明
    // 获取父窗口的屏幕尺寸 ==》ChatDialog
    setFixedSize(parent->size());
    // 创建动画对象
    QMovie* movie = new QMovie(":/res/loading.gif");
    // 将动画对象设置到LoadingLabel中
    ui->loadingLabel->setMovie(movie);
    // 展示动画
    qDebug() << "show movie;";
    movie->start();
}

LoadingDialog::~LoadingDialog()
{
    delete ui;
}
