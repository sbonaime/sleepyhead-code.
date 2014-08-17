#-------------------------------------------------
#
# Project created by QtCreator 2011-06-20T22:05:30
#
#-------------------------------------------------

QT += core gui network xml

greaterThan(QT_MAJOR_VERSION,4) {
    QT += widgets webkitwidgets
} else { # qt4
    QT += webkit
}

#SleepyHead requires OpenGL 2.0 support to run smoothly
#On platforms where it's not available, it can still be built to work
#provided the BrokenGL file resides in the SleepyHead source root directory
exists($$PWD/../BrokenGL) {
    DEFINES += BROKEN_OPENGL_BUILD
    message("Building with QWidget gGraphView")
} else {
    message("Building with QGLWidget gGraphView")
    QT += opengl
}

#The following forces ResMed session locking.. it *may* not be necessary.. I'm still trying to assess this properly.
DEFINES += LOCK_RESMED_SESSIONS

#CONFIG += c++11
CONFIG += rtti


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

    GIT_BRANCH=$$system(git rev-parse --abbrev-ref HEAD)
    DEFINES += GIT_BRANCH=\\\"$$GIT_BRANCH\\\"
    DEFINES += GIT_REVISION=\\\"$$system(git rev-parse --short HEAD)\\\"

#    contains(GIT_BRANCH,"testing"):

} else {
    DEFINES += GIT_BRANCH=\\\"UNKNOWN\\\"
    DEFINES += GIT_REVISION=\\\"UNKNOWN\\\"
}

#Comment out for official builds
DEFINES += TEST_BUILD


unix:!macx {
    LIBS        += -lX11 -lz -lGLU
    DEFINES         += _TTY_POSIX_
}

macx {
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6.8

  LIBS             += -lz
  ICON              = icons/iconfile.icns
}

win32 {
    DEFINES          += WINVER=0x0501 # needed for mingw to pull in appropriate dbt business...probably a better way to do this
    RC_FILE          += win_icon.rc
    LIBS             += -lsetupapi

    INCLUDEPATH += $$PWD
    INCLUDEPATH += $$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib

    if (*-msvc*):!equals(TEMPLATE_PREFIX, "vc") {
        LIBS += -ladvapi32
        DEFINES += BUILD_WITH_MSVC=1
    } else {
        # MingW needs this
        LIBS += -lz
    }
     CONFIG(release, debug|release) {
        # Update build number
        build_nr.commands = $$PWD/scripts/inc_build.bat
        build_nr.depends = FORCE
        QMAKE_EXTRA_TARGETS += build_nr
        PRE_TARGETDEPS += build_nr

        HEADERS  += build_number.h
    }

    QT += serialport

}

unix {
    # Update build number
    CONFIG(debug, debug|release) {
        build_nr.commands = $$PWD/scripts/inc_build.sh debug
    } else {
        build_nr.commands = $$PWD/scripts/inc_build.sh release
    }
    build_nr.depends = FORCE
    QMAKE_EXTRA_TARGETS += build_nr
    PRE_TARGETDEPS += build_nr

    HEADERS  += build_number.h
}

#include(3rdparty/quazip-0.5.1/quazip/quazip.pri)

#include(SleepLib2/sleeplib.pri)

SOURCES += \
    common_gui.cpp \
    daily.cpp \
    exportcsv.cpp \
    main.cpp \
    mainwindow.cpp \
    newprofile.cpp \
    overview.cpp \
    preferencesdialog.cpp \
    profileselect.cpp \
    reports.cpp \
    sessionbar.cpp \
    updateparser.cpp \
    UpdaterWindow.cpp \
    Graphs/gFlagsLine.cpp \
    Graphs/gFooBar.cpp \
    Graphs/gGraph.cpp \
    Graphs/gGraphView.cpp \
    Graphs/glcommon.cpp \
    Graphs/gLineChart.cpp \
    Graphs/gLineOverlay.cpp \
    Graphs/gSegmentChart.cpp \
    Graphs/gspacer.cpp \
    Graphs/gStatsLine.cpp \
    Graphs/gSummaryChart.cpp \
    Graphs/gXAxis.cpp \
    Graphs/gYAxis.cpp \
    Graphs/layer.cpp \
    SleepLib/calcs.cpp \
    SleepLib/common.cpp \
    SleepLib/day.cpp \
    SleepLib/event.cpp \
    SleepLib/machine.cpp \
    SleepLib/machine_loader.cpp \
    SleepLib/preferences.cpp \
    SleepLib/profiles.cpp \
    SleepLib/schema.cpp \
    SleepLib/session.cpp \
    SleepLib/loader_plugins/cms50_loader.cpp \
    SleepLib/loader_plugins/icon_loader.cpp \
    SleepLib/loader_plugins/intellipap_loader.cpp \
    SleepLib/loader_plugins/mseries_loader.cpp \
    SleepLib/loader_plugins/prs1_loader.cpp \
    SleepLib/loader_plugins/resmed_loader.cpp \
    SleepLib/loader_plugins/somnopose_loader.cpp \
    SleepLib/loader_plugins/zeo_loader.cpp \
    translation.cpp \
    statistics.cpp \
    oximeterimport.cpp \
    SleepLib/serialoximeter.cpp \
    SleepLib/loader_plugins/md300w1_loader.cpp \
    Graphs/gSessionTimesChart.cpp \
    logger.cpp \
    welcome.cpp \
    SleepLib/machine_common.cpp \
    SleepLib/loader_plugins/weinmann_loader.cpp \
    Graphs/gdailysummary.cpp \
    Graphs/MinutesAtPressure.cpp \
    SleepLib/journal.cpp \
    SleepLib/progressdialog.cpp \
    SleepLib/loader_plugins/cms50f37_loader.cpp

HEADERS  += \
    common_gui.h \
    daily.h \
    exportcsv.h \
    mainwindow.h \
    newprofile.h \
    overview.h \
    preferencesdialog.h \
    profileselect.h \
    reports.h \
    sessionbar.h \
    updateparser.h \
    UpdaterWindow.h \
    version.h \
    Graphs/gFlagsLine.h \
    Graphs/gFooBar.h \
    Graphs/gGraph.h \
    Graphs/gGraphView.h \
    Graphs/glcommon.h \
    Graphs/gLineChart.h \
    Graphs/gLineOverlay.h \
    Graphs/gSegmentChart.h\
    Graphs/gspacer.h \
    Graphs/gStatsLine.h \
    Graphs/gSummaryChart.h \
    Graphs/gXAxis.h \
    Graphs/gYAxis.h \
    Graphs/layer.h \
    SleepLib/calcs.h \
    SleepLib/common.h \
    SleepLib/day.h \
    SleepLib/event.h \
    SleepLib/machine.h \
    SleepLib/machine_common.h \
    SleepLib/machine_loader.h \
    SleepLib/preferences.h \
    SleepLib/profiles.h \
    SleepLib/schema.h \
    SleepLib/session.h \
    SleepLib/loader_plugins/cms50_loader.h \
    SleepLib/loader_plugins/icon_loader.h \
    SleepLib/loader_plugins/intellipap_loader.h \
    SleepLib/loader_plugins/mseries_loader.h \
    SleepLib/loader_plugins/prs1_loader.h \
    SleepLib/loader_plugins/resmed_loader.h \
    SleepLib/loader_plugins/somnopose_loader.h \
    SleepLib/loader_plugins/zeo_loader.h \
    translation.h \
    statistics.h \
    oximeterimport.h \
    SleepLib/serialoximeter.h \
    SleepLib/loader_plugins/md300w1_loader.h \
    Graphs/gSessionTimesChart.h \
    logger.h \
    SleepLib/loader_plugins/weinmann_loader.h \
    Graphs/gdailysummary.h \
    Graphs/MinutesAtPressure.h \
    SleepLib/journal.h \
    SleepLib/progressdialog.h \
    SleepLib/loader_plugins/cms50f37_loader.h

FORMS += \
    daily.ui \
    overview.ui \
    mainwindow.ui \
    oximetry.ui \
    preferencesdialog.ui \
    report.ui \
    profileselect.ui \
    newprofile.ui \
    exportcsv.ui \
    UpdaterWindow.ui \
    oximeterimport.ui

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
    docs/intro.html \
    docs/statistics.xml

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

bundlelibs = $$cat($$PWD/../Bundle3rdParty)

contains(bundlelibs, true) {
    include(../3rdparty/quazip/quazip/quazip.pri)
    INCLUDEPATH += $$PWD/../3rdparty/quazip
    DEPENDPATH += $$PWD/../3rdparty/quazip

 !win32 {
    include(../3rdparty/qtserialport/src/serialport/serialport-lib.pri)
    INCLUDEPATH += $$PWD/../3rdparty/qtserialport/include/QtSerialPort/5.3.1/QtSerialPort
    DEPENDPATH +=  $$PWD/../3rdparty/qtserialport/include/QtSerialPort/5.3.1/QtSerialPort
 #   DEPENDPATH += $$PWD/../3rdparty/qtserialport/src/serialport/include/QtSerialPort/5.3.1
    }
} else {
    unix {
        message("Attempting to build with system quazip.");
        QMAKE_LFLAGS += -L/usr/lib -L/usr/local/lib
        INCLUDEPATH += /usr/local/include
        INCLUDEPATH += /usr/include
        DEPENDPATH += /usr/local/include/quazip
        DEPENDPATH += /usr/include/quazip
    } else {
        #Configure it if you need it...
        warning("Building with externally linked quazip is unsupported on this platform");
    }
    LIBS += -lquazip
}
