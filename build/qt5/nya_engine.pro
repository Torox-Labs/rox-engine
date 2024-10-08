TARGET = nya_engine
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
QT =

unix: QMAKE_CXXFLAGS += -stdlib=libc++

mingw|unix: QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wno-unused-function -Wno-reorder


QMAKE_CXXFLAGS += -msse -msse2

CONFIG(debug, debug|release) {
    CONFIG += CDEBUG
    DEFINES += DEBUG _DEBUG
    CONF = debug
} else:CONFIG(release, debug|release) {
    CONFIG += CRELEASE
    DEFINES += NDEBUG RELEASE
    CONF = release
}
PLATFORM = $$first(QMAKE_PLATFORM)
android {
    PLATFORM = $$PLATFORM"_"$$ANDROID_TARGET_ARCH
} else:mingw {
    PLATFORM = mingw
}
BIN_DIR_NAME = "qtc_"$$PLATFORM"_"$$CONF

DESTDIR = $${PWD}/../../bin/$${BIN_DIR_NAME}
OBJECTS_DIR = $${PWD}/../../obj/$${BIN_DIR_NAME}
MOC_DIR = $${OBJECTS_DIR}

include( nya_sources.pri )
