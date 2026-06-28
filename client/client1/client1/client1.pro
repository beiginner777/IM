QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

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
    adduseritem.cpp \
    applyfriend.cpp \
    applyfrienditem.cpp \
    applyfriendpage.cpp \
    authfriendapply.cpp \
    chatdatalist.cpp \
    chatdialog.cpp \
    chatinterface.cpp \
    chatitembase.cpp \
    chatthreaddata.cpp \
    chatuserlist.cpp \
    chatuserwidget.cpp \
    clickbutton.cpp \
    clicklabel.cpp \
    clickoncelabel.cpp \
    contactuserlist.cpp \
    contactuserwidget.cpp \
    customizeedit.cpp \
    fileuploadmsg.cpp \
    findfaildialog.cpp \
    findsuccessdialog.cpp \
    friendinfointerface.cpp \
    friendlabel.cpp \
    global.cpp \
    grouptipitem.cpp \
    httpmanager.cpp \
    imageviewerdialog.cpp \
    listitembase.cpp \
    loadingdialog.cpp \
    loadlocaldata.cpp \
    logindialog.cpp \
    logineduserlist.cpp \
    logineduserwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    messagetextedit.cpp \
    offlinedialog.cpp \
    registerdialog.cpp \
    searchlist.cpp \
    settingpage.cpp \
    sqlitemanager.cpp \
    statewidget.cpp \
    tcpmsg.cpp \
    tcpthread.cpp \
    timerbtn.cpp \
    userdata.cpp \
    usermanager.cpp

HEADERS += \
    adduseritem.h \
    applyfriend.h \
    applyfrienditem.h \
    applyfriendpage.h \
    authfriendapply.h \
    chatdatalist.h \
    chatdialog.h \
    chatinterface.h \
    chatitembase.h \
    chatthreaddata.h \
    chatuserlist.h \
    chatuserwidget.h \
    clickbutton.h \
    clicklabel.h \
    clickoncelabel.h \
    contactuserlist.h \
    contactuserwidget.h \
    customizeedit.h \
    fileuploadmsg.h \
    findfaildialog.h \
    findsuccessdialog.h \
    friendinfointerface.h \
    friendlabel.h \
    global.h \
    grouptipitem.h \
    httpmanager.h \
    imageviewerdialog.h \
    listitembase.h \
    loadingdialog.h \
    loadlocaldata.h \
    logindialog.h \
    logineduserlist.h \
    logineduserwidget.h \
    mainwindow.h \
    messagetextedit.h \
    offlinedialog.h \
    registerdialog.h \
    searchlist.h \
    settingpage.h \
    singleton.h \
    sqlitemanager.h \
    statewidget.h \
    tcpmsg.h \
    tcpthread.h \
    timerbtn.h \
    ui_adduseritem.h \
    ui_applyfriend.h \
    ui_applyfrienditem.h \
    ui_applyfriendpage.h \
    ui_authfriendapply.h \
    ui_chatdialog.h \
    ui_chatinterface.h \
    ui_chatuserwidget.h \
    ui_contactuserwidget.h \
    ui_findfaildialog.h \
    ui_findsuccessdialog.h \
    ui_friendinfointerface.h \
    ui_friendlabel.h \
    ui_grouptipitem.h \
    ui_loadingdialog.h \
    ui_logindialog.h \
    ui_logineduserwidget.h \
    ui_mainwindow.h \
    ui_offlinedialog.h \
    ui_registerdialog.h \
    ui_settingpage.h \
    userdata.h \
    usermanager.h

FORMS += \
    adduseritem.ui \
    applyfriend.ui \
    applyfrienditem.ui \
    applyfriendpage.ui \
    authfriendapply.ui \
    chatdialog.ui \
    chatinterface.ui \
    chatuserwidget.ui \
    contactuserwidget.ui \
    findfaildialog.ui \
    findsuccessdialog.ui \
    friendinfointerface.ui \
    friendlabel.ui \
    grouptipitem.ui \
    loadingdialog.ui \
    logindialog.ui \
    logineduserwidget.ui \
    mainwindow.ui \
    offlinedialog.ui \
    registerdialog.ui \
    settingpage.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rc.qrc

DISTFILES += \
    config.ini
