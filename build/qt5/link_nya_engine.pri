CONFIG(debug, debug|release) {
    CONF = debug
} else:CONFIG(release, debug|release) {
    CONF = release
}

PLATFORM = $$first(QMAKE_PLATFORM)
android {
    PLATFORM = $$PLATFORM"_"$$ANDROID_TARGET_ARCH
} else:mingw {
    PLATFORM = mingw
}
BIN_DIR_NAME = "qtc_"$$PLATFORM"_"$$CONF

NYA_ENGINE_PATH = $$shell_path($$PWD/../..)
INCLUDEPATH += $$NYA_ENGINE_PATH
LIBS += -L$$NYA_ENGINE_PATH/bin/$$BIN_DIR_NAME -lnya_engine
