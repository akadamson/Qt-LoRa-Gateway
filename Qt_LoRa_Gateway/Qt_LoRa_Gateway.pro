#-------------------------------------------------
#
# Project created by QtCreator 2016-10-05T10:33:06
#
#-------------------------------------------------

QT += core gui
QT += widgets
QT += serialport
QT += network

win32:RC_FILE = myapp.rc

TARGET = Qt-LoRa-Gateway
TEMPLATE = app


SOURCES += main.cpp\
mainwindow.cpp \
    serialbase.cpp \
    serialdialog.cpp \
    dlghistory.cpp \
    dlgssdv.cpp \
    rs8.c \
    ssdv.c

HEADERS  += mainwindow.h \
    serialbase.h \
    serialdialog.h \
    threadwrapper.h \
    dlghistory.h \
    dlgssdv.h \
    rs8.h \
    ssdv.h

FORMS    += mainwindow.ui \
    serialdialog.ui \
    dlghistory.ui \
    dlgssdv.ui
