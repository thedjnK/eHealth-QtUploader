#-------------------------------------------------
#
# Project created by QtCreator 2015-12-08T12:05:46
#
#-------------------------------------------------

QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = eHealth
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    devicesettings.cpp

HEADERS  += mainwindow.h \
    devicesettings.h

FORMS    += mainwindow.ui \
    devicesettings.ui

RESOURCES += \
    data.qrc
