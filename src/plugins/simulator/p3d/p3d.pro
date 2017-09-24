load(common_pre)

REQUIRES += contains(BLACK_CONFIG,P3D)

QT       += core dbus gui network xml

TARGET = simulatorp3d
TEMPLATE = lib

CONFIG += plugin shared
CONFIG += blackmisc blackcore

DEPENDPATH += . $$SourceRoot/src
INCLUDEPATH += . $$SourceRoot/src

SOURCES += *.cpp
HEADERS += *.h

equals(WORD_SIZE,64) {
    INCLUDEPATH *= $$EXTERNALSROOT/common/include/simconnect/P3D-v4
}
equals(WORD_SIZE,32) {
    INCLUDEPATH *= $$EXTERNALSROOT/common/include/simconnect/FSX-XPack
}

LIBS += -lsimulatorfscommon -lsimulatorfsxcommon -lfsuipc
equals(WORD_SIZE,64) {
    LIBS *= -L$$EXTERNALS_LIB_DIR/P3D-v4
    CONFIG(debug, debug|release): LIBS *= -lSimConnectDebug
    else:                         LIBS *= -lSimConnect
}
equals(WORD_SIZE,32) {
    LIBS *= -L$$EXTERNALS_LIB_DIR/FSX-XPack
    LIBS *= -lSimConnect
}
LIBS += -ldxguid -lole32
addStaticLibraryDependency(simulatorfscommon)
addStaticLibraryDependency(simulatorfsxcommon)
addStaticLibraryDependency(fsuipc)

# Ignore linker warning about missing pdb files from Simconnect
msvc: QMAKE_LFLAGS *= /ignore:4099

DISTFILES += simulatorp3d.json

DESTDIR = $$DestRoot/bin/plugins/simulator

win32 {
    dlltarget.path = $$PREFIX/bin/plugins/simulator
    INSTALLS += dlltarget
} else {
    target.path = $$PREFIX/bin/plugins/simulator
    INSTALLS += target
}

load(common_post)
