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
    serialdialog.cpp

HEADERS  += mainwindow.h \
    serialbase.h \
    serialdialog.h \
    threadwrapper.h

FORMS    += mainwindow.ui \
    serialdialog.ui
