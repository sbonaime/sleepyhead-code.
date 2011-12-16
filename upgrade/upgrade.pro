#-------------------------------------------------
#
# Project created by QtCreator 2011-12-14T23:44:40
#
#-------------------------------------------------

QT       += core gui network xml webkit

TARGET = upgrade
TEMPLATE = app

DEFINES += QUAZIP_BUILD
CONFIG(staticlib): DEFINES += QUAZIP_STATIC

unix:!symbian {
    headers.path=$$PREFIX/include/quazip
    headers.files=$$HEADERS
    target.path=$$PREFIX/lib
    INSTALLS += headers target

        OBJECTS_DIR=.obj
        MOC_DIR=.moc

        LIBS += -lz
}

win32 {
    headers.path=$$PREFIX/include/quazip
    headers.files=$$HEADERS
    target.path=$$PREFIX/lib
    INSTALLS += headers target

    *-g++*: LIBS += -lz.dll
    *-msvc*: LIBS += -lzlibwapi
    *-msvc*: QMAKE_LFLAGS += /IMPLIB:$$DESTDIR\\quazip.lib
}

SOURCES += main.cpp\
    updateparser.cpp \
    quazip/zip.c \
    quazip/unzip.c \
    quazip/quazipnewinfo.cpp \
    quazip/quazipfile.cpp \
    quazip/quazip.cpp \
    quazip/quacrc32.cpp \
    quazip/quaadler32.cpp \
    quazip/qioapi.cpp \
    quazip/JlCompress.cpp \
    UpdateWindow.cpp

HEADERS  += \
    updateparser.h \
    quazip/zip.h \
    quazip/unzip.h \
    quazip/quazipnewinfo.h \
    quazip/quazip_global.h \
    quazip/quazipfileinfo.h \
    quazip/quazipfile.h \
    quazip/quazip.h \
    quazip/quacrc32.h \
    quazip/quachecksum32.h \
    quazip/quaadler32.h \
    quazip/JlCompress.h \
    quazip/ioapi.h \
    quazip/crypt.h \
    UpdateWindow.h

FORMS    += \
    UpdateWindow.ui

RESOURCES += \
    resources.qrc
