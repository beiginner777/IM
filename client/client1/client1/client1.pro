QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += . core widgets dialogs network data

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dialogs/adduseritem.cpp \
    dialogs/applyfriend.cpp \
    dialogs/applyfrienditem.cpp \
    dialogs/applyfriendpage.cpp \
    dialogs/authfriendapply.cpp \
    data/chatdatalist.cpp \
    dialogs/chatdialog.cpp \
    dialogs/chatinterface.cpp \
    data/chatthreaddata.cpp \
    dialogs/chatuserlist.cpp \
    dialogs/chatuserwidget.cpp \
    widgets/clickbutton.cpp \
    widgets/clicklabel.cpp \
    widgets/clickoncelabel.cpp \
    dialogs/contactuserlist.cpp \
    dialogs/contactuserwidget.cpp \
    widgets/customizeedit.cpp \
    data/fileuploadmsg.cpp \
    dialogs/findfaildialog.cpp \
    dialogs/findsuccessdialog.cpp \
    dialogs/friendinfointerface.cpp \
    dialogs/friendlabel.cpp \
    core/global.cpp \
    widgets/grouptipitem.cpp \
    network/httpmanager.cpp \
    dialogs/imageviewerdialog.cpp \
    widgets/listitembase.cpp \
    dialogs/loadingdialog.cpp \
    data/loadlocaldata.cpp \
    dialogs/logindialog.cpp \
    dialogs/logineduserlist.cpp \
    dialogs/logineduserwidget.cpp \
    core/main.cpp \
    core/mainwindow.cpp \
    widgets/messagetextedit.cpp \
    dialogs/offlinedialog.cpp \
    dialogs/registerdialog.cpp \
    dialogs/searchlist.cpp \
    dialogs/settingpage.cpp \
    data/sqlitemanager.cpp \
    widgets/statewidget.cpp \
    network/tcpmsg.cpp \
    network/tcpthread.cpp \
    widgets/timerbtn.cpp \
    core/userdata.cpp \
    data/usermanager.cpp

HEADERS += \
    dialogs/adduseritem.h \
    dialogs/applyfriend.h \
    dialogs/applyfrienditem.h \
    dialogs/applyfriendpage.h \
    dialogs/authfriendapply.h \
    data/chatdatalist.h \
    dialogs/chatdialog.h \
    dialogs/chatinterface.h \
    data/chatthreaddata.h \
    dialogs/chatuserlist.h \
    dialogs/chatuserwidget.h \
    widgets/clickbutton.h \
    widgets/clicklabel.h \
    widgets/clickoncelabel.h \
    dialogs/contactuserlist.h \
    dialogs/contactuserwidget.h \
    widgets/customizeedit.h \
    data/fileuploadmsg.h \
    dialogs/findfaildialog.h \
    dialogs/findsuccessdialog.h \
    dialogs/friendinfointerface.h \
    dialogs/friendlabel.h \
    core/global.h \
    widgets/grouptipitem.h \
    network/httpmanager.h \
    dialogs/imageviewerdialog.h \
    widgets/listitembase.h \
    dialogs/loadingdialog.h \
    data/loadlocaldata.h \
    dialogs/logindialog.h \
    dialogs/logineduserlist.h \
    dialogs/logineduserwidget.h \
    core/mainwindow.h \
    widgets/messagetextedit.h \
    dialogs/offlinedialog.h \
    dialogs/registerdialog.h \
    dialogs/searchlist.h \
    dialogs/settingpage.h \
    core/singleton.h \
    data/sqlitemanager.h \
    widgets/statewidget.h \
    network/tcpmsg.h \
    network/tcpthread.h \
    widgets/timerbtn.h \
    core/userdata.h \
    data/usermanager.h

FORMS += \
    dialogs/adduseritem.ui \
    dialogs/applyfriend.ui \
    dialogs/applyfrienditem.ui \
    dialogs/applyfriendpage.ui \
    dialogs/authfriendapply.ui \
    dialogs/chatdialog.ui \
    dialogs/chatinterface.ui \
    dialogs/chatuserwidget.ui \
    dialogs/contactuserwidget.ui \
    dialogs/findfaildialog.ui \
    dialogs/findsuccessdialog.ui \
    dialogs/friendinfointerface.ui \
    dialogs/friendlabel.ui \
    widgets/grouptipitem.ui \
    dialogs/loadingdialog.ui \
    dialogs/logindialog.ui \
    dialogs/logineduserwidget.ui \
    core/mainwindow.ui \
    dialogs/offlinedialog.ui \
    dialogs/registerdialog.ui \
    dialogs/settingpage.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rc.qrc

DISTFILES += \
    core/config.ini
