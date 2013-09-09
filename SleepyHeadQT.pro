#-------------------------------------------------
#
# Project created by QtCreator 2011-06-20T22:05:30
#
#-------------------------------------------------

QT += core gui opengl network xml

greaterThan(QT_MAJOR_VERSION,4): QT += widgets webkitwidgets
lessThan(QT_MAJOR_VERSION,5): QT += webkit

CONFIG += rtti

#static {
#    CONFIG += static
#    QTPLUGIN += qsvg qgif

#    DEFINES += STATIC // Equivalent to "#define STATIC" in source code
#    message("Static build.")
#}

#CONFIG += link_pkgconfig
#PKGCONFIG += freetype2

TARGET = SleepyHead
unix:!macx {
TARGET.path=/usr/bin
}

TEMPLATE = app

# GIT_VERSION = $$system(git describe --tags --long --abbrev=6 --dirty="*")


#exists(.git):DEFINES += GIT_BRANCH=\\\"$$system(git rev-parse --symbolic-full-name --abbrev-ref HEAD)\\\"
exists(.git):DEFINES += GIT_BRANCH=\\\"$$system(git rev-parse --abbrev-ref HEAD)\\\"
else:DEFINES += GIT_BRANCH=\\\"UNKNOWN\\\"

exists(.git):DEFINES += GIT_REVISION=\\\"$$system(git rev-parse HEAD)\\\"
else:DEFINES += GIT_REVISION=\\\"UNKNOWN\\\"

SOURCES += main.cpp\
    SleepLib/machine.cpp \
    SleepLib/machine_loader.cpp \
    SleepLib/preferences.cpp \
    SleepLib/profiles.cpp \
    SleepLib/loader_plugins/cms50_loader.cpp \
    SleepLib/loader_plugins/prs1_loader.cpp \
    SleepLib/loader_plugins/zeo_loader.cpp \
    SleepLib/loader_plugins/resmed_loader.cpp \
    daily.cpp \
    oximetry.cpp \
    overview.cpp \
    mainwindow.cpp \
    SleepLib/event.cpp \
    SleepLib/session.cpp \
    SleepLib/day.cpp \
    Graphs/gLineChart.cpp \
    Graphs/gLineOverlay.cpp \
    Graphs/gFooBar.cpp \
    Graphs/gXAxis.cpp \
    Graphs/gYAxis.cpp \
    Graphs/gFlagsLine.cpp \
    Graphs/glcommon.cpp \
    Graphs/gSegmentChart.cpp \
    qextserialport/qextserialport.cpp \
    preferencesdialog.cpp \
    Graphs/gGraphView.cpp \
    Graphs/gStatsLine.cpp \
    Graphs/gSummaryChart.cpp \
    SleepLib/schema.cpp \
    profileselect.cpp \
    newprofile.cpp \
    exportcsv.cpp \
    common_gui.cpp \
    SleepLib/loader_plugins/intellipap_loader.cpp \
    SleepLib/calcs.cpp \
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
    UpdaterWindow.cpp \
    SleepLib/common.cpp \
    SleepLib/loader_plugins/icon_loader.cpp \
    SleepLib/loader_plugins/mseries_loader.cpp

unix:SOURCES           += qextserialport/posix_qextserialport.cpp
unix:!macx:SOURCES     += qextserialport/qextserialenumerator_unix.cpp
unix:!macx:LIBS        += -lX11 -lz -lGLU

macx {
  SOURCES          += qextserialport/qextserialenumerator_osx.cpp
  LIBS             += -framework IOKit -framework CoreFoundation -lz
  ICON              = icons/iconfile.icns
}

win32 {
  SOURCES          += qextserialport/win_qextserialport.cpp qextserialport/qextserialenumerator_win.cpp
  DEFINES          += WINVER=0x0501 # needed for mingw to pull in appropriate dbt business...probably a better way to do this
  LIBS             += -lsetupapi -lz


}
if (win32-msvc2008|win32-msvc2010):!equals(TEMPLATE_PREFIX, "vc") {
   LIBS += -ladvapi32
   DEFINES += BUILD_WITH_MSVC=1
}
HEADERS  += \
    SleepLib/machine.h \
    SleepLib/machine_loader.h \
    SleepLib/preferences.h \
    SleepLib/profiles.h \
    SleepLib/loader_plugins/cms50_loader.h \
    SleepLib/loader_plugins/prs1_loader.h \
    SleepLib/loader_plugins/zeo_loader.h \
    oximetry.h \
    daily.h \
    overview.h \
    mainwindow.h \
    SleepLib/event.h \
    SleepLib/machine_common.h \
    SleepLib/session.h \
    SleepLib/day.h \
    Graphs/gLineChart.h \
    Graphs/gLineOverlay.h \
    Graphs/gFooBar.h \
    Graphs/gXAxis.h \
    Graphs/gYAxis.h \
    Graphs/gFlagsLine.h \
    Graphs/glcommon.h \
    Graphs/gSegmentChart.h\
    SleepLib/loader_plugins/resmed_loader.h \
    qextserialport/qextserialport_global.h \
    qextserialport/qextserialport.h \
    qextserialport/qextserialenumerator.h \
    preferencesdialog.h \
    Graphs/gGraphView.h \
    Graphs/gStatsLine.h \
    Graphs/gSummaryChart.h \
    SleepLib/schema.h \
    profileselect.h \
    newprofile.h \
    exportcsv.h \
    common_gui.h \
    SleepLib/loader_plugins/intellipap_loader.h \
    SleepLib/calcs.h \
    version.h \
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
    UpdaterWindow.h \
    SleepLib/common.h \
    SleepLib/loader_plugins/icon_loader.h \
    SleepLib/loader_plugins/mseries_loader.h


FORMS    += \
    daily.ui \
    overview.ui \
    mainwindow.ui \
    oximetry.ui \
    preferencesdialog.ui \
    report.ui \
    profileselect.ui \
    newprofile.ui \
    exportcsv.ui \
    UpdaterWindow.ui

TRANSLATIONS += \
    Translations/sleepyhead_nl.ts \
    Translations/sleepyhead_fr.ts

RESOURCES += \
    Resources.qrc

OTHER_FILES += \
    docs/index.html \
    docs/usage.html \
    docs/schema.xml \
    docs/graphs.xml \
    docs/channels.xml \
    docs/release_notes.html \
    docs/startup_tips.txt \
    docs/countries.txt \
    docs/tz.txt \
    LICENSE.txt \
    docs/tooltips.css \
    docs/script.js \
    update.xml \
    docs/changelog.txt \
    docs/update_notes.html

mac {
    TransFiles.files = Translations
    TransFiles.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += TransFiles
}













