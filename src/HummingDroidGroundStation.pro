#-------------------------------------------------
#
# Project created by QtCreator 2013-07-26T22:44:59
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = HummingDroidGroundStation
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    Communication.pb.cc \
    qjoystick.cpp \
    plotarea.cpp \
    controlleraxiswidget.cpp

HEADERS  += mainwindow.h \
    Communication.pb.h \
    qjoystick.h \
    plotarea.h \
    controlleraxiswidget.h

FORMS    += mainwindow.ui \
    controlleraxiswidget.ui \
    plotarea.ui

OTHER_FILES += \
    Communication.proto

CONFIG += link_pkgconfig
PKGCONFIG += protobuf-lite

RESOURCES += \
    resources.qrc
