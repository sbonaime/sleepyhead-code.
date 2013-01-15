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
#include "SleepLib/loader_plugins/icon_loader.h"

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
    relnotes.setWindowTitle(QObject::tr("SleepyHead Release Notes"));
    QVBoxLayout layout(&relnotes);
    QWebView web(&relnotes);

    // Language???

    web.load(QUrl("qrc:/docs/release_notes.html"));

    //web.page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOn);
    relnotes.setLayout(&layout);
    layout.insertWidget(0,&web,1);
    QPushButton okbtn(QObject::tr("&Ok, get on with it.."),&relnotes);
    relnotes.connect(&okbtn,SIGNAL(clicked()),SLOT(accept()));
    layout.insertWidget(1,&okbtn,1);
    QApplication::processEvents(); // MW: Needed on Mac, as the html has to finish loading

    relnotes.exec();
}

void build_notes()
{
    QDialog relnotes;
    relnotes.setWindowTitle(QObject::tr("SleepyHead Update Notes"));
    QVBoxLayout layout(&relnotes);
    QWebView web(&relnotes);
    relnotes.setWindowTitle("SleepyHead v"+FullVersionString+" Update");
    // Language???

    web.load(QUrl("qrc:/docs/update_notes.html"));
    //web.page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOn);

    relnotes.setLayout(&layout);
    layout.insertWidget(0,&web,1);
    QPushButton okbtn(QObject::tr("&Ok, get on with it.."),&relnotes);
    relnotes.connect(&okbtn,SIGNAL(clicked()),SLOT(accept()));
    layout.insertWidget(1,&okbtn,1);
    layout.setMargin(0);
    QApplication::processEvents(); // MW: Needed on Mac, as the html has to finish loading

    relnotes.setFixedSize(500,400);
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
        if (args[i]=="-p") {
#ifdef Q_WS_WIN32
            Sleep(1000);
#else
            sleep(1);
#endif

        }
    }

    a.setApplicationName("SleepyHead");
    initialize();


    ////////////////////////////////////////////////////////////////////////////////////////////
    // Register Importer Modules
    ////////////////////////////////////////////////////////////////////////////////////////////
    PRS1Loader::Register();
    CMS50Loader::Register();
    //ZEOLoader::Register(); // Use outside of directory importer..
    ResmedLoader::Register();
    IntellipapLoader::Register();
    FPIconLoader::Register();

    // Scan for user profiles
    Profiles::Scan();
    //qRegisterMetaType<Preference>("Preference");
    PREF["AppName"]=QObject::tr("SleepyHead");


    // Skip login screen, unless asked not to on the command line
    bool skip_login=PREF.ExistsAndTrue(STR_GEN_SkipLogin);
    if (force_login_screen) skip_login=false;

    // Todo: Make a wrapper for Preference settings, like Profile settings have..
    QDateTime lastchecked, today=QDateTime::currentDateTime();
    if (!PREF.contains(STR_GEN_UpdatesAutoCheck)) {
        PREF[STR_GEN_UpdatesAutoCheck]=true;
        PREF[STR_GEN_UpdateCheckFrequency]=7;
    }
    if (!PREF.contains(STR_PREF_AllowEarlyUpdates)) {
        PREF[STR_PREF_AllowEarlyUpdates]=false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Check when last checked for updates..
    ////////////////////////////////////////////////////////////////////////////////////////////
    bool check_updates=false;
    if (PREF[STR_GEN_UpdatesAutoCheck].toBool()) {
        int update_frequency=PREF[STR_GEN_UpdateCheckFrequency].toInt();
        int days=1000;
        lastchecked=PREF[STR_GEN_UpdatesLastChecked].toDateTime();
        if (PREF.contains(STR_GEN_UpdatesLastChecked)) {
            days=lastchecked.secsTo(today);
            days/=86400;
        };
        if (days>update_frequency) {
            check_updates=true;
        }
    }

    if (!Profiles::profiles.size()) {
        NewProfile newprof(0);
        if (newprof.exec()==NewProfile::Rejected)
            return 0;
        release_notes();

        // Show New User wizard..
    } else {
        if (PREF.contains("VersionString")) {
            QString V=PREF["VersionString"].toString();

            if (FullVersionString>V) {
                QString V2=V.section("-",0,0);

                if (VersionString>V2) {
                    release_notes();
                    //QMessageBox::warning(0,"New Version Warning","This is a new version of SleepyHead. If you experience a crash right after clicking Ok, you will need to manually delete the "+AppRoot+" folder (it's located in your Documents folder) and reimport your data. After this things should work normally.",QMessageBox::Ok);
                } else {
                    build_notes();
                }
                check_updates=false;
            }
        }
        ProfileSelect profsel(0);
        if (skip_login) {
            profsel.QuickLogin();
            if (profsel.result()==ProfileSelect::Rejected) {
                exit(1);
            }
            p_profile=Profiles::Get(PREF[STR_GEN_Profile].toString());
        } else p_profile=NULL;
        if (!p_profile) {
            if (profsel.exec()==ProfileSelect::Rejected) {
                exit(1);
            }
        }
    }
    PREF["VersionString"]=FullVersionString;

    p_profile=Profiles::Get(PREF[STR_GEN_Profile].toString());

    qDebug() << "Selected Profile" << p_profile->user->userName();

//    int id=QFontDatabase::addApplicationFont(":/fonts/FreeSans.ttf");
//    QFontDatabase fdb;
//    QStringList ffam=fdb.families();
//    for (QStringList::iterator i=ffam.begin();i!=ffam.end();i++) {
//        qDebug() << "Loaded Font: " << (*i);
//    }

    if (!PREF.contains("Fonts_Application_Name")) {
        PREF["Fonts_Application_Name"]="Sans Serif";
        PREF["Fonts_Application_Size"]=10;
        PREF["Fonts_Application_Bold"]=false;
        PREF["Fonts_Application_Italic"]=false;
    }


    QApplication::setFont(QFont(PREF["Fonts_Application_Name"].toString(),
                                PREF["Fonts_Application_Size"].toInt(),
                                PREF["Fonts_Application_Bold"].toBool() ? QFont::Bold : QFont::Normal,
                                PREF["Fonts_Application_Italic"].toBool()));

    qDebug() << "Selected" << QApplication::font().family();
    qInstallMsgHandler(MyOutputHandler);

    MainWindow w;
    mainwin=&w;

    if (check_updates) mainwin->CheckForUpdates();
    w.show();

    return a.exec();
}
