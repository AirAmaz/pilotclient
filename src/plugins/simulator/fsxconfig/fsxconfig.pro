load(common_pre)

QT       += core widgets dbus network

TARGET = simulatorfsxconfig
TEMPLATE = lib
CONFIG += plugin shared
CONFIG += blackmisc blackcore blackgui

DEPENDPATH += . $$SourceRoot/src
INCLUDEPATH += . $$SourceRoot/src

SOURCES += *.cpp
HEADERS += *.h
DISTFILES += simulatorfsxconfig.json
LIBS +=  -lsimulatorfsxcommon
addStaticLibraryDependency(simulatorfsxcommon)

DESTDIR = $$DestRoot/bin/plugins/simulator

win32 {
    dlltarget.path = $$PREFIX/bin/plugins/simulator
    INSTALLS += dlltarget
} else {
    target.path = $$PREFIX/bin/plugins/simulator
    INSTALLS += target
}

load(common_post)
