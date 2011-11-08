#-------------------------------------------------
#
# Project created by QtCreator 2011-06-20T22:05:30
#
#-------------------------------------------------

QT += core gui webkit opengl network xml

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
TEMPLATE = app

# GIT_VERSION = $$system(git describe --tags --long --abbrev=6 --dirty="*")


#exists(.git):DEFINES += GIT_BRANCH=\\\"$$system(git rev-parse --symbolic-full-name --abbrev-ref HEAD)\\\"
exists(.git):DEFINES += GIT_BRANCH=\\\"$$system(git rev-parse --abbrev-ref HEAD)\\\"
else:DEFINES += GIT_BRANCH=\\\"NOT BUILT FROM GIT SOURCE\\\"

exists(.git):DEFINES += GIT_REVISION=\\\"$$system(git rev-parse HEAD)\\\"
else:DEFINES += GIT_BRANCH=\\\"UNKNOWN\\\"

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
    Graphs/gSessionTime.cpp \
    qextserialport/qextserialport.cpp \
    preferencesdialog.cpp \
    Graphs/gGraphView.cpp \
    Graphs/gStatsLine.cpp \
    report.cpp \
    Graphs/gSummaryChart.cpp \
    SleepLib/schema.cpp \
    profileselect.cpp \
    newprofile.cpp

unix:SOURCES           += qextserialport/posix_qextserialport.cpp
unix:!macx:SOURCES     += qextserialport/qextserialenumerator_unix.cpp
unix:!macx:LIBS        += -lX11

macx {
  SOURCES          += qextserialport/qextserialenumerator_osx.cpp
  LIBS             += -framework IOKit -framework CoreFoundation
}

win32 {
  SOURCES          += qextserialport/win_qextserialport.cpp qextserialport/qextserialenumerator_win.cpp
  DEFINES          += WINVER=0x0501 # needed for mingw to pull in appropriate dbt business...probably a better way to do this
  LIBS             += -lsetupapi
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
    Graphs/gSessionTime.h \
    SleepLib/loader_plugins/resmed_loader.h \
    qextserialport/qextserialport_global.h \
    qextserialport/qextserialport.h \
    qextserialport/qextserialenumerator.h \
    preferencesdialog.h \
    Graphs/gGraphView.h \
    Graphs/gStatsLine.h \
    report.h \
    Graphs/gSummaryChart.h \
    SleepLib/schema.h \
    profileselect.h \
    newprofile.h


FORMS    += \
    daily.ui \
    overview.ui \
    mainwindow.ui \
    oximetry.ui \
    preferencesdialog.ui \
    report.ui \
    profileselect.ui \
    newprofile.ui

RESOURCES += \
    Resources.qrc

OTHER_FILES += \
    docs/index.html \
    docs/usage.html \
    docs/template_overview.sht \
    docs/schema.xml \
    docs/graphs.xml \
    docs/channels.xml \
    docs/release_notes.html



