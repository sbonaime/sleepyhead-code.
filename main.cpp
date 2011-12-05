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
#include <QPushButton>
#include <QWebFrame>
#include <QWebView>

#include "SleepLib/schema.h"
#include "mainwindow.h"
#include "SleepLib/profiles.h"
#include "profileselect.h"
#include "newprofile.h"

// Gah! I must add the real darn plugin system one day.
#include "SleepLib/loader_plugins/prs1_loader.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/loader_plugins/zeo_loader.h"
#include "SleepLib/loader_plugins/resmed_loader.h"
#include "SleepLib/loader_plugins/intellipap_loader.h"

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
void release_notes()
{
    QDialog relnotes;
    QVBoxLayout layout(&relnotes);
    QWebView web(&relnotes);
    web.load(QUrl("qrc:/docs/release_notes.html"));
    //web.page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOn);
    relnotes.setLayout(&layout);
    layout.insertWidget(0,&web,1);
    QPushButton okbtn("&Ok, get on with it..",&relnotes);
    relnotes.connect(&okbtn,SIGNAL(clicked()),SLOT(accept()));
    layout.insertWidget(1,&okbtn,1);
    relnotes.exec();
}

int main(int argc, char *argv[])
{
#ifdef Q_WS_X11
    XInitThreads();
#endif
    QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);

    bool force_login_screen=false;
    QApplication a(argc, argv);
    QStringList args=QCoreApplication::arguments();

    for (int i=1;i<args.size();i++) {
        if (args[i]=="-l") force_login_screen=true;
    }

    a.setApplicationName("SleepyHead");
    initialize();

    PRS1Loader::Register();
    CMS50Loader::Register();
    ZEOLoader::Register();
    ResmedLoader::Register();
    IntellipapLoader::Register();
    Profiles::Scan();
    qRegisterMetaType<Preference>("Preference");
    PREF["AppName"]="SleepyHead";
    bool skip_login=(PREF.ExistsAndTrue("SkipLoginScreen"));
    if (force_login_screen) skip_login=false;

    QString Version=QString("%1.%2.%3").arg(major_version).arg(minor_version).arg(revision_number);

    QDateTime lastchecked, today=QDateTime::currentDateTime();
    if (!PREF.Exists("Updates_AutoCheck")) {
        PREF["Updates_AutoCheck"]=true;
        PREF["Updates_CheckFrequency"]=3;
    }
    bool check_updates=false;
    if (PREF["Updates_AutoCheck"].toBool()) {
        int update_frequency=PREF["Updates_CheckFrequency"].toInt();
        int days=1000;
        // p_pref ->Get
        lastchecked=PREF["Updates_LastChecked"].toDateTime();
        if (PREF.Exists("Updates_LastChecked")) {
            days=lastchecked.secsTo(today);
            days/=86400;
        };
        if (days>update_frequency) {
            //QMessageBox::information(NULL,"Check for updates","Placeholder. Would automatically check for updates here.",QMessageBox::Ok);
            check_updates=true;
            //PREF["Updates_LastChecked"]=today;
        }
    }

    if (!Profiles::profiles.size()) {
        NewProfile newprof(0);
        if (newprof.exec()==NewProfile::Rejected)
            return 0;
        release_notes();

        // Show New User wizard..
    } else {
        if (PREF.Exists("VersionString")) {
            QString V=PREF["VersionString"].toString();
            if (V!=Version) {

                release_notes();
                //QMessageBox::warning(0,"New Version Warning","This is a new version of SleepyHead. If you experience a crash right after clicking Ok, you will need to manually delete the SleepApp folder (it's located in your Documents folder) and reimport your data. After this things should work normally.",QMessageBox::Ok);
                check_updates=false;
            }
        }
        ProfileSelect profsel(0);
        if (skip_login) {
            profsel.QuickLogin();
            if (profsel.result()==ProfileSelect::Rejected) {
                exit(1);
            }
            p_profile=Profiles::Get(PREF["Profile"].toString());
        } else p_profile=NULL;
        if (!p_profile) {
            if (profsel.exec()==ProfileSelect::Rejected) {
                exit(1);
            }
        }
    }
    PREF["VersionString"]=Version;

    p_profile=Profiles::Get(PREF["Profile"].toString());

    //if (!PREF.Exists("Profile")) PREF["Profile"]=getUserName();

    //int id=QFontDatabase::addApplicationFont(":/fonts/FreeSans.ttf");
   /* QFontDatabase fdb;
    QStringList ffam=fdb.families();
    for (QStringList::iterator i=ffam.begin();i!=ffam.end();i++) {
        qDebug() << "Loaded Font: " << (*i);
    } */

    if (!PREF.Exists("Fonts_Application_Name")) {
        PREF["Fonts_Application_Name"]="Sans Serif";
        PREF["Fonts_Application_Size"]=10;
        PREF["Fonts_Application_Bold"]=false;
        PREF["Fonts_Application_Italic"]=false;
    }


    QApplication::setFont(QFont(PREF["Fonts_Application_Name"].toString(),
                                PREF["Fonts_Application_Size"].toInt(),
                                PREF["Fonts_Application_Bold"].toBool() ? QFont::Bold : QFont::Normal,
                                PREF["Fonts_Application_Italic"].toBool()));

    qInstallMsgHandler(MyOutputHandler);

    MainWindow w;
    mainwin=&w;

    if (check_updates) mainwin->CheckForUpdates();
    w.show();

    return a.exec();
}
