/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Main
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

//#include <QtPlugin>
#include <QApplication>
#include <QMessageBox>
#include <QFontDatabase>
#include <QStringList>
#include <QDebug>
#include <QPushButton>
#include <QWebFrame>
#include <QWebView>
#include <QTranslator>
#include <QDir>
#include <QComboBox>
#include <QPushButton>
#include <QSettings>
#include <QFileDialog>

#include "SleepLib/schema.h"
#include "mainwindow.h"
#include "SleepLib/profiles.h"
#include "profileselect.h"
#include "newprofile.h"

// Gah! I must add the real darn plugin system one day.
#include "SleepLib/loader_plugins/prs1_loader.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/loader_plugins/zeo_loader.h"
#include "SleepLib/loader_plugins/somnopose_loader.h"
#include "SleepLib/loader_plugins/resmed_loader.h"
#include "SleepLib/loader_plugins/intellipap_loader.h"
#include "SleepLib/loader_plugins/icon_loader.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

MainWindow *mainwin=NULL;

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
void MyOutputHandler(QtMsgType type, const char *msgtxt) {
#else
void MyOutputHandler(QtMsgType type, const QMessageLogContext & context, const QString &msgtxt) {
#endif
    if (!mainwin) {
      //  qInstallMessageHandler(0);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        fprintf(stderr,"Pre/Post: %s\n",msgtxt.toLocal8Bit().constData());
#else
        fprintf(stderr,"Pre/Post: %s\n",msgtxt);
#endif
        return;
    }

    QString msg,typestr;
    switch (type) {
        case QtWarningMsg:
            typestr=QString("Warning: ");
            break;
        case QtFatalMsg:
            typestr=QString("Fatal: ");
            break;
        case QtCriticalMsg:
            typestr=QString("Critical: ");
            break;
        default:
            typestr=QString("Debug: ");
            break;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    msg=typestr+msgtxt;//+QString(" (%1:%2, %3)").arg(context.file).arg(context.line).arg(context.function);
#else
    msg=typestr+msgtxt;
#endif
    mainwin->Log(msg);

    if (type==QtFatalMsg) {
        abort();
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
    relnotes.setWindowTitle(STR_TR_SleepyHead+" "+QObject::tr("Release Notes"));
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
    relnotes.setWindowTitle(STR_TR_SleepyHead+" "+QObject::tr("SleepyHead Update Notes"));
    QVBoxLayout layout(&relnotes);
    QWebView web(&relnotes);
    relnotes.setWindowTitle(STR_TR_SleepyHead+" v"+FullVersionString+QObject::tr(" Update"));
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

void sDelay(int s)
{
    // QThread::msleep() is exposed in Qt5
#ifdef Q_OS_WIN32
            Sleep(s*1000);
#else
            sleep(s);
#endif
}

int main(int argc, char *argv[])
{
#ifdef Q_WS_X11
    XInitThreads();
#endif

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
#endif

    bool force_login_screen=false;
    bool force_data_dir=false;

    QApplication a(argc, argv);
    QStringList args=QCoreApplication::arguments();

    for (int i=1;i<args.size();i++) {
        if (args[i]=="-l") force_login_screen=true;
        else if (args[i]=="-d") force_data_dir=true;
        else if (args[i]=="-p") {
            sDelay(1);
        }
    }

    QSettings settings(getDeveloperName(),getAppName());

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Language Selection
    ////////////////////////////////////////////////////////////////////////////////////////////
    QDialog langsel(NULL,Qt::CustomizeWindowHint|Qt::WindowTitleHint);
    langsel.setWindowTitle(QObject::tr("Language"));

    QHBoxLayout lang_layout(&langsel);

    QComboBox lang_combo(&langsel);
    QPushButton lang_okbtn("->",&langsel);

    lang_layout.addWidget(&lang_combo,1);
    lang_layout.addWidget(&lang_okbtn);

#ifdef Q_OS_MAC
    QString transdir=QDir::cleanPath(QCoreApplication::applicationDirPath()+"/../Resources/Translations/");

#else
    const QString transdir=QCoreApplication::applicationDirPath()+"/Translations/";
#endif

    QDir dir(transdir);
    qDebug() << "Scanning \"" << transdir << "\" for translations";
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList("*.qm"));

    qDebug() << "Available Translations";
    QFileInfoList list=dir.entryInfoList();
    QString language=settings.value("Settings/Language").toString();

    QString langfile,langname;

    // Fake english for now..
    lang_combo.addItem("English","English.en_US.qm");

    // Scan translation directory
    for (int i=0;i<list.size();++i) {
        QFileInfo fi=list.at(i);
        langname=fi.fileName().section('.',0,0);

        lang_combo.addItem(langname,fi.fileName());
        qDebug() << "Found Translation" << QDir::toNativeSeparators(fi.fileName());
    }

    for (int i=0;i<lang_combo.count();i++) {
        langname=lang_combo.itemText(i);
        if (langname==language) {
            langfile=lang_combo.itemData(i).toString();
            break;
        }
    }

    if (language.isEmpty()) {
        langsel.connect(&lang_okbtn,SIGNAL(clicked()),&langsel, SLOT(close()));

        langsel.exec();
        langsel.disconnect(&lang_okbtn,SIGNAL(clicked()),&langsel, SLOT(close()));
        langname=lang_combo.currentText();
        langfile=lang_combo.itemData(lang_combo.currentIndex()).toString();
        settings.setValue("Settings/Language",langname);
    }

    qDebug() << "Loading " << langname << " Translation" << langfile << "from" << transdir;
    QTranslator translator;

    if (!translator.load(langfile,transdir)) {
        qDebug() << "Could not load translation" << langfile << "reverting to english :(";

    }


    a.installTranslator(&translator);
    initializeStrings(); // Important, call this AFTER translator is installed.

    a.setApplicationName(STR_TR_SleepyHead);


    ////////////////////////////////////////////////////////////////////////////////////////////
    // Datafolder location Selection
    ////////////////////////////////////////////////////////////////////////////////////////////

    bool change_data_dir=force_data_dir;

    bool havefolder=false;
    if (!settings.contains("Settings/AppRoot")) {
        change_data_dir=true;
    } else {
        QDir dir(GetAppRoot());
        if (!dir.exists())
            change_data_dir=true;
        else havefolder=true;
    }

    if (!havefolder && !force_data_dir) {
        if (QMessageBox::question(NULL,QObject::tr("Question"),QObject::tr("No SleepyHead data folder was found.\n\nWould you like SleepyHead to use the default location for storing its data?\n\n")+GetAppRoot(),QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes) {
            settings.setValue("Settings/AppRoot",GetAppRoot());
            change_data_dir=false;
        }
    }

retry_directory:
    if (change_data_dir) {
        QString datadir=QFileDialog::getExistingDirectory(NULL,QObject::tr("Choose or create new folder for SleepyHead data"),GetAppRoot(),QFileDialog::ShowDirsOnly);
        if (datadir.isEmpty()) {
            if (!havefolder) {
               QMessageBox::information(NULL,QObject::tr("Exiting"),QObject::tr("As you did not select a data folder, SleepyHead will exit.\n\nNext time you run, you will be asked again."));
               return 0;
            } else {
                QMessageBox::information(NULL,QObject::tr("No Directory"),QObject::tr("You did not select a directory.\n\nSleepyHead will now start with your old one.\n\n")+GetAppRoot(),QMessageBox::Ok);
            }
        } else {
            QDir dir(datadir);
            QFile file(datadir+"/Preferences.xml");
            if (!file.exists()) {
                if (dir.count() > 2) {
                    // Not a new directory.. nag the user.
                    if (QMessageBox::question(NULL,QObject::tr("Warning"),QObject::tr("The folder you chose is not empty, nor does it already contain valid SleepyHead data.\n\nAre you sure you want to use this folder?\n\n")+datadir,QMessageBox::Yes,QMessageBox::No)==QMessageBox::No) {
                        goto retry_directory;
                    }
                }
            }

            settings.setValue("Settings/AppRoot",datadir);
            qDebug() << "Changing data folder to" << datadir;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Register Importer Modules for autoscanner
    ////////////////////////////////////////////////////////////////////////////////////////////
    initialize();
    PRS1Loader::Register();
    CMS50Loader::Register();
    //ZEOLoader::Register(); // Use outside of directory importer..
    ResmedLoader::Register();
    IntellipapLoader::Register();
    FPIconLoader::Register();

    // Scan for user profiles
    Profiles::Scan();
    //qRegisterMetaType<Preference>("Preference");
    PREF["AppName"]=STR_TR_SleepyHead;    

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
        if (PREF.contains(STR_PREF_VersionString)) {
            QString V=PREF[STR_PREF_VersionString].toString();

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
    PREF[STR_PREF_VersionString]=FullVersionString;

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

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    qInstallMessageHandler(MyOutputHandler);
#else
    qInstallMsgHandler(MyOutputHandler);
#endif
    //#endif
    MainWindow w;
    mainwin=&w;

    if (check_updates) mainwin->CheckForUpdates();
    w.show();

    return a.exec();
}
