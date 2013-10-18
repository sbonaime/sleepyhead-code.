#-------------------------------------------------
#
# Project created by QtCreator 2011-06-20T22:05:30
#
#-------------------------------------------------

QT += core gui opengl network xml

greaterThan(QT_MAJOR_VERSION,4) {
    QT += widgets webkitwidgets
} else { # qt4
    QT += webkit
}

CONFIG += rtti

win32:CONFIG += use_bundled_libs
else:!use_bundled_libs:CONFIG += extserialport

use_bundled_libs:DEFINES += USE_BUNDLED_LIBS

#static {
#    CONFIG += static
#    QTPLUGIN += qsvg qgif qpng

#    DEFINES += STATIC // Equivalent to "#define STATIC" in source code
#    message("Static build.")
#}

TARGET = SleepyHead
unix:!macx {
    TARGET.path=/usr/bin
}

TEMPLATE = app

# GIT_VERSION = $$system(git describe --tags --long --abbrev=6 --dirty="*")

exists(../.git):{

    DEFINES += GIT_BRANCH=\\\"$$system(git rev-parse --abbrev-ref HEAD)\\\"
    DEFINES += GIT_REVISION=\\\"$$system(git rev-parse HEAD)\\\"

    equals(GIT_BRANCH,"unstable"):DEFINES += UNSTABLE_BUILD

}else{
    DEFINES += GIT_BRANCH=\\\"UNKNOWN\\\"
    DEFINES += GIT_REVISION=\\\"UNKNOWN\\\"
}



unix:!macx {
    LIBS        += -lX11 -lz -lGLU
    DEFINES         += _TTY_POSIX_
}

macx {
  LIBS             += -lz
  ICON              = icons/iconfile.icns
}

win32 {
  DEFINES          += WINVER=0x0501 # needed for mingw to pull in appropriate dbt business...probably a better way to do this
  LIBS             += -lsetupapi -lz
}
if (win32-msvc2008|win32-msvc2010|win32-msvc2012):!equals(TEMPLATE_PREFIX, "vc") {
   LIBS += -ladvapi32
   DEFINES += BUILD_WITH_MSVC=1
}


#include(..3rdparty/qextserialport/src/qextserialport.pri)
#include(3rdparty/quazip-0.5.1/quazip/quazip.pri)

#include(SleepLib2/sleeplib.pri)

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
    UpdaterWindow.cpp \
    SleepLib/common.cpp \
    SleepLib/loader_plugins/icon_loader.cpp \
    SleepLib/loader_plugins/mseries_loader.cpp \
    reports.cpp \
    summary.cpp \
    sessionbar.cpp

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
    UpdaterWindow.h \
    SleepLib/common.h \
    SleepLib/loader_plugins/icon_loader.h \
    SleepLib/loader_plugins/mseries_loader.h \
    reports.h \
    summary.h \
    sessionbar.h


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
    ../LICENSE.txt \
    docs/tooltips.css \
    docs/script.js \
    ../update.xml \
    docs/changelog.txt \
    docs/update_notes.html \
    docs/intro.html

win32 {
    CONFIG(debug, debug|release) {
        DDIR = $$OUT_PWD/debug/Translations
    }
    CONFIG(release, debug|release) {
        DDIR = $$OUT_PWD/release/Translations
    }
    DDIR ~= s,/,\\,g

    TRANS_FILES += $$PWD/../Translations/*.qm
    TRANS_FILES_WIN = $${TRANS_FILES}
    TRANS_FILES_WIN ~= s,/,\\,g

    system(mkdir $$quote($$DDIR))

    for(FILE,TRANS_FILES_WIN){
        system(xcopy /y $$quote($$FILE) $$quote($$DDIR))
    }
}

mac {
    TransFiles.files = $$files(../Translations/*.qm)
    TransFiles.path = Contents/Resources/Translations
    QMAKE_BUNDLE_DATA += TransFiles
}

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../3rdparty/quazip/quazip/release/ -lquazip
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../3rdparty/quazip/quazip/debug/ -lquazip
else:unix {
    use_bundled_libs:QMAKE_LFLAGS += -L$$OUT_PWD/../3rdparty/quazip/
    else:QMAKE_LFLAGS += -L/usr/lib -L/usr/local/lib

    LIBS += -lquazip
}

use_bundled_libs {
    INCLUDEPATH += $$PWD/../3rdparty/quazip
    DEPENDPATH += $$PWD/../3rdparty/quazip
} else {
    INCLUDEPATH += /usr/local/include
    INCLUDEPATH += /usr/include
    DEPENDPATH += /usr/local/include/quazip
    DEPENDPATH += /usr/include/quazip
}

use_bundled_libs: {
    greaterThan(QT_MAJOR_VERSION,4) {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../3rdparty/qextserialport/release/ -lQt5ExtSerialPort1
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../3rdparty/qextserialport/debug/ -lQt5ExtSerialPortd1
        else:unix:CONFIG(release, debug|release): LIBS += -lQt5ExtSerialPort1
        else:unix:CONFIG(debug, debug|release): LIBS += -lQt5ExtSerialPortd1
    } else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../3rdparty/qextserialport/release/ -lqextserialport
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../3rdparty/qextserialport/debug/ -lqextserialport
        else:unix: LIBS += -lqextserialport
    }
    INCLUDEPATH += $$PWD/../3rdparty/qextserialport
    DEPENDPATH += $$PWD/../3rdparty/qextserialport
} else {
    CONFIG += extserialport
}
