load(config_test_pre)

CONFIG -= qt
CONFIG += console

SOURCES = glu.cpp

win32 {
    DEFINES += Q_OS_WIN
    LIBS *= -lglu32
}
linux: LIBS *= -lGLU
macx {
    DEFINES += Q_OS_MAC
    LIBS += -framework OpenGL
}
