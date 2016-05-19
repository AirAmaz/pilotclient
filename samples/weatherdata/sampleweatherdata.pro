load(common_pre)

QT       += core dbus

TARGET = sampleweatherdata
TEMPLATE = app

CONFIG   += console
CONFIG   += blackmisc blackcore
CONFIG  -= app_bundle

DEPENDPATH += . $$SourceRoot/src
INCLUDEPATH += . $$SourceRoot/src

HEADERS += *.h
SOURCES += *.cpp

DESTDIR = $$DestRoot/bin

target.path = $$PREFIX/bin
INSTALLS += target

load(common_post)
