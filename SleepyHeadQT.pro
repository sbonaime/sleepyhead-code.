#-------------------------------------------------
#
# Project created by QtCreator 2011-06-20T22:05:30
#
#-------------------------------------------------

QT += core gui webkit opengl xml

CONFIG += rtti

#static {
#    CONFIG += static
#    QTPLUGIN += qsvg qgif

#    DEFINES += STATIC // Equivalent to "#define STATIC" in source code
#    message("Static build.")
#}

#CONFIG += link_pkgconfig
#PKGCONFIG += freetype2

TARGET = SleepyHeadQT
TEMPLATE = app


SOURCES += main.cpp\
    SleepLib/machine.cpp \
    SleepLib/machine_loader.cpp \
    SleepLib/preferences.cpp \
    SleepLib/profiles.cpp \
    SleepLib/loader_plugins/cms50_loader.cpp \
    SleepLib/loader_plugins/prs1_loader.cpp \
    SleepLib/loader_plugins/zeo_loader.cpp \
    daily.cpp \
    overview.cpp \
    mainwindow.cpp \
    SleepLib/event.cpp \
    SleepLib/session.cpp \
    SleepLib/day.cpp \
    Graphs/graphwindow.cpp \
    Graphs/graphlayer.cpp \
    Graphs/graphdata.cpp \
    Graphs/graphdata_custom.cpp \
    Graphs/gLineChart.cpp \
    Graphs/gLineOverlay.cpp \
    Graphs/gFooBar.cpp \
    Graphs/gXAxis.cpp \
    Graphs/gYAxis.cpp \
    Graphs/gFlagsLine.cpp \
    Graphs/glcommon.cpp \
    Graphs/gTitle.cpp \
    Graphs/gCandleStick.cpp \
    Graphs/gBarChart.cpp \
    SleepLib/loader_plugins/resmed_loader.cpp \
    Graphs/gpiechart.cpp \
    SleepLib/loader_plugins/sleep_database.cpp \
    Graphs/gSessionTime.cpp \
    qextserialport/qextserialport.cpp \
    oximetry.cpp

unix:SOURCES           += qextserialport/posix_qextserialport.cpp
unix:!macx:SOURCES     += qextserialport/qextserialenumerator_unix.cpp

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
    daily.h \
    overview.h \
    mainwindow.h \
    SleepLib/event.h \
    SleepLib/machine_common.h \
    SleepLib/session.h \
    SleepLib/day.h \
    Graphs/graphwindow.h \
    Graphs/graphlayer.h \
    Graphs/gLineChart.h \
    Graphs/gLineOverlay.h \
    Graphs/gFooBar.h \
    Graphs/gXAxis.h \
    Graphs/gYAxis.h \
    Graphs/gFlagsLine.h \
    Graphs/glcommon.h \
    Graphs/gTitle.h \
    Graphs/gCandleStick.h \
    Graphs/gBarChart.h \
    SleepLib/loader_plugins/resmed_loader.h \
    Graphs/gpiechart.h \
    SleepLib/loader_plugins/sleep_database.h \
    Graphs/gSessionTime.h \
    qextserialport/qextserialport_global.h \
    qextserialport/qextserialport.h \
    qextserialport/qextserialenumerator.h \
    oximetry.h


FORMS    += \
    daily.ui \
    overview.ui \
    mainwindow.ui \
    oximetry.ui

RESOURCES += \
    Resources.qrc

OTHER_FILES += \
    docs/index.html
