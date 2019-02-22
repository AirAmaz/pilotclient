load(common_pre)

TARGET = blackconfig
TEMPLATE = lib
CONFIG += staticlib

buildconfig_gen.cpp.input = buildconfig_gen.cpp.in
buildconfig_gen.cpp.output = $$BuildRoot/generated/buildconfig_gen.cpp

buildconfig_gen.inc.input = buildconfig_gen.inc.in
buildconfig_gen.inc.output = $$BuildRoot/generated/buildconfig_gen.inc

GENERATED_SOURCES += $$BuildRoot/generated/buildconfig_gen.cpp
GENERATED_FILES += $$BuildRoot/generated/buildconfig_gen.inc
QMAKE_SUBSTITUTES += buildconfig_gen.cpp buildconfig_gen.inc

INCLUDEPATH += ..

DEFINES += LOG_IN_FILE
HEADERS +=  *.h
SOURCES +=  *.cpp
DESTDIR = $$DestRoot/lib
OTHER_FILES += buildconfig_gen.cpp.in buildconfig_gen.inc.in

win32: GIT_BIN = $$system($$(SYSTEMROOT)\system32\where git 2> nul)
else: GIT_BIN = $$system(which git 2> /dev/null)

isEmpty(GIT_BIN) {
    GIT_HEAD_SHA1="<unknown>"
    GIT_COMMIT_TS="0"
} else {
    GIT_HEAD_SHA1=$$system(git rev-parse --short HEAD)
    GIT_COMMIT_TS=$$system(git log -1 --date=format:'%Y%m%d%H%M' --pretty=format:%cd)
}

load(common_post)
