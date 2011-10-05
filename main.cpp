/*
 Main
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

//#include <QtPlugin>
#include <QtGui/QApplication>
#include <QMessageBox>
#include <QFontDatabase>
#include <QStringList>
#include <QDebug>

#include "SleepLib/schema.h"
#include "mainwindow.h"
#include "SleepLib/profiles.h"
#include "profileselect.h"
#include "newprofile.h"

#include "SleepLib/loader_plugins/prs1_loader.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/loader_plugins/zeo_loader.h"
#include "SleepLib/loader_plugins/resmed_loader.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

MainWindow *mainwin=NULL;

void MyOutputHandler(QtMsgType type, const char *msg) {
    if (!mainwin) {
        return;
    }

    switch (type) {
        case QtDebugMsg:
            mainwin->Log(msg);
            break;
        case QtWarningMsg:
            mainwin->Log(QString("Warning: ")+msg);
            break;
        case QtFatalMsg:
            mainwin->Log(QString("Fatal: ")+msg);
            break;
        case QtCriticalMsg:
            mainwin->Log(QString("Critical: ")+msg);
            break;
            // Popup a messagebox
            //abort();
    }
    //loglock.unlock();
}

void initialize()
{
    schema::init();
}

int main(int argc, char *argv[])
{
#ifdef Q_WS_X11
    XInitThreads();
#endif
    QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
    QApplication a(argc, argv);
    a.setApplicationName("SleepyHead");
    initialize();

    PRS1Loader::Register();
    CMS50Loader::Register();
    ZEOLoader::Register();
    ResmedLoader::Register();
    Profiles::Scan();
    PREF["AppName"]="SleepyHead";


    QString Version=QString("%1.%2.%3").arg(major_version).arg(minor_version).arg(revision_number);

    if (!Profiles::profiles.size()) {
        NewProfile newprof(0);
        if (newprof.exec()==NewProfile::Rejected)
            return 0;

        // Show New User wizard..
    } else {
        if (PREF.Exists("VersionString")) {
            QString V=PREF["VersionString"].toString();
            if (V!=Version) {
                QMessageBox::warning(0,"New Version Warning","This is a new version of SleepyHead. If you experience a crash right after clicking Ok, you will need to manually delete the SleepApp folder (it's located in your Documents folder) and reimport your data. After this things should work normally.",QMessageBox::Ok);
            }
        }
        ProfileSelect profsel(0);
        if (profsel.exec()==ProfileSelect::Rejected) {
            exit(1);
        }
    }
    PREF["VersionString"]=Version;
    p_profile=Profiles::Get(PREF["Profile"].toString());

    //if (!pref.Exists("Profile")) pref["Profile"]=getUserName();

    /*int id=QFontDatabase::addApplicationFont(":/fonts/FreeSans.ttf");
    QStringList ffam=QFontDatabase::applicationFontFamilies(id);
    for (QStringList::iterator i=ffam.begin();i!=ffam.end();i++) {
        qDebug() << "Loaded Font: " << (*i);
    }*/

    a.setFont(QFont("Sans Serif",10));

    qInstallMsgHandler(MyOutputHandler);

    MainWindow w;
    mainwin=&w;
    w.show();

    return a.exec();
}
