#-------------------------------------------------
#
# Project created by QtCreator 2011-06-20T22:05:30
#
#-------------------------------------------------

QT += core gui webkit opengl

#CONFIG += static

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
    SleepLib/binary_file.cpp \
    SleepLib/machine.cpp \
    SleepLib/machine_loader.cpp \
    SleepLib/preferences.cpp \
    SleepLib/profiles.cpp \
    SleepLib/loader_plugins/cms50_loader.cpp \
    SleepLib/loader_plugins/prs1_loader.cpp \
    SleepLib/loader_plugins/zeo_loader.cpp \
    SleepLib/tinyxml/tinystr.cpp \
    SleepLib/tinyxml/tinyxml.cpp \
    SleepLib/tinyxml/tinyxmlerror.cpp \
    SleepLib/tinyxml/tinyxmlparser.cpp \
    daily.cpp \
    overview.cpp \
    mainwindow.cpp \
    SleepLib/event.cpp \
    SleepLib/session.cpp \
    SleepLib/day.cpp \
    SleepLib/waveform.cpp \
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
    Graphs/gpiechart.cpp

HEADERS  += \
    SleepLib/binary_file.h \
    SleepLib/machine.h \
    SleepLib/machine_loader.h \
    SleepLib/preferences.h \
    SleepLib/profiles.h \
    SleepLib/loader_plugins/cms50_loader.h \
    SleepLib/loader_plugins/prs1_loader.h \
    SleepLib/loader_plugins/zeo_loader.h \
    SleepLib/tinyxml/tinystr.h \
    SleepLib/tinyxml/tinyxml.h \
    daily.h \
    overview.h \
    mainwindow.h \
    SleepLib/event.h \
    SleepLib/machine_common.h \
    SleepLib/session.h \
    SleepLib/day.h \
    SleepLib/waveform.h \
    Graphs/graphwindow.h \
    Graphs/graphlayer.h \
    Graphs/graphdata.h \
    Graphs/graphdata_custom.h \
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
    Graphs/gpiechart.h

FORMS    += \
    daily.ui \
    overview.ui \
    mainwindow.ui

RESOURCES += \
    Resources.qrc

OTHER_FILES += \
    docs/index.html
