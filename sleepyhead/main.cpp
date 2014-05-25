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
#include "translation.h"

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

MainWindow *mainwin = nullptr;

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
void MyOutputHandler(QtMsgType type, const char *msgtxt)
{
#else
void MyOutputHandler(QtMsgType type, const QMessageLogContext &context, const QString &msgtxt)
{
    Q_UNUSED(context)
#endif

    if (!mainwin) {
        //  qInstallMessageHandler(0);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        fprintf(stderr, "Pre/Post: %s\n", msgtxt.toLocal8Bit().constData());
#else
        fprintf(stderr, "Pre/Post: %s\n", msgtxt);
#endif
        return;
    }

    QString msg, typestr;

    switch (type) {
    case QtWarningMsg:
        typestr = QString("Warning: ");
        break;

    case QtFatalMsg:
        typestr = QString("Fatal: ");
        break;

    case QtCriticalMsg:
        typestr = QString("Critical: ");
        break;

    default:
        typestr = QString("Debug: ");
        break;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    msg = typestr +
          msgtxt; //+QString(" (%1:%2, %3)").arg(context.file).arg(context.line).arg(context.function);
#else
    msg = typestr + msgtxt;
#endif
    mainwin->Log(msg);

    if (type == QtFatalMsg) {
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
    relnotes.setWindowTitle(STR_TR_SleepyHead + " " + QObject::tr("Release Notes"));
    QVBoxLayout layout(&relnotes);
    QWebView web(&relnotes);

    // Language???

    web.load(QUrl("qrc:/docs/release_notes.html"));

    //web.page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOn);
    relnotes.setLayout(&layout);
    layout.insertWidget(0, &web, 1);
    QPushButton okbtn(QObject::tr("&Ok, get on with it.."), &relnotes);
    relnotes.connect(&okbtn, SIGNAL(clicked()), SLOT(accept()));
    layout.insertWidget(1, &okbtn, 1);
    QApplication::processEvents(); // MW: Needed on Mac, as the html has to finish loading

    relnotes.exec();
}

void sDelay(int s)
{
    // QThread::msleep() is exposed in Qt5
#ifdef Q_OS_WIN32
    Sleep(s * 1000);
#else
    sleep(s);
#endif
}

int compareVersion(QString version);

int main(int argc, char *argv[])
{
#ifdef Q_WS_X11
    XInitThreads();
#endif

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
#endif

    bool force_login_screen = false;
    bool force_data_dir = false;

    QApplication a(argc, argv);
    QStringList args = QCoreApplication::arguments();

    QSettings settings(getDeveloperName(), getAppName());

    for (int i = 1; i < args.size(); i++) {
        if (args[i] == "-l") { force_login_screen = true; }
        else if (args[i] == "-d") { force_data_dir = true; }
        else if (args[i] == "-language") {
            settings.setValue("Settings/Language","");
        } else if (args[i] == "-p") {
            sDelay(1);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Language Selection
    ////////////////////////////////////////////////////////////////////////////////////////////
    initTranslations(settings);
    initializeStrings(); // Important, call this AFTER translator is installed.
    a.setApplicationName(STR_TR_SleepyHead);

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Datafolder location Selection
    ////////////////////////////////////////////////////////////////////////////////////////////
    bool change_data_dir = force_data_dir;

    bool havefolder = false;

    if (!settings.contains("Settings/AppRoot")) {
        change_data_dir = true;
    } else {
        QDir dir(GetAppRoot());

        if (!dir.exists()) {
            change_data_dir = true;
        } else { havefolder = true; }
    }

    if (!havefolder && !force_data_dir) {
        if (QMessageBox::question(nullptr, STR_MessageBox_Question,
                                  QObject::tr("No SleepyHead data folder was found.")+"\n\n"+QObject::tr("Would you like SleepyHead to use the default location for storing its data?")+"\n\n"+
                                  QDir::toNativeSeparators(GetAppRoot()), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
            settings.setValue("Settings/AppRoot", GetAppRoot());
            change_data_dir = false;
        }
    }

retry_directory:

    if (change_data_dir) {
        QString datadir = QFileDialog::getExistingDirectory(nullptr,
                          QObject::tr("Choose or create new folder for SleepyHead data"), GetAppRoot(),
                          QFileDialog::ShowDirsOnly);

        if (datadir.isEmpty()) {
            if (!havefolder) {
                QMessageBox::information(nullptr, QObject::tr("Exiting"),
                                         QObject::tr("As you did not select a data folder, SleepyHead will exit.")+"\n\n"+QObject::tr("Next time you run, you will be asked again."));
                return 0;
            } else {
                QMessageBox::information(nullptr, STR_MessageBox_Warning,
                                         QObject::tr("You did not select a directory.")+"\n\n"+QObject::tr("SleepyHead will now start with your old one.")+"\n\n"+
                                         QDir::toNativeSeparators(GetAppRoot()), QMessageBox::Ok);
            }
        } else {
            QDir dir(datadir);
            QFile file(datadir + "/Preferences.xml");

            if (!file.exists()) {
                if (dir.count() > 2) {
                    // Not a new directory.. nag the user.
                    if (QMessageBox::question(nullptr, STR_MessageBox_Warning,
                                              QObject::tr("The folder you chose is not empty, nor does it already contain valid SleepyHead data.")
                                              + "\n\n"+QObject::tr("Are you sure you want to use this folder?")+"\n\n"
                                              + datadir, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                        goto retry_directory;
                    }
                }
            }

            settings.setValue("Settings/AppRoot", datadir);
            qDebug() << "Changing data folder to" << datadir;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Register Importer Modules for autoscanner
    ////////////////////////////////////////////////////////////////////////////////////////////
    initialize();
    PRS1Loader::Register();
    ResmedLoader::Register();
    IntellipapLoader::Register();
    FPIconLoader::Register();
    CMS50Loader::Register();
    //ZEOLoader::Register(); // Use outside of directory importer..

    p_pref = new Preferences("Preferences");
    p_layout = new Preferences("Layout");

    PREF.Open();
    LAYOUT.Open();

    // Scan for user profiles
    Profiles::Scan();


    //qRegisterMetaType<Preference>("Preference");
    PREF["AppName"] = STR_TR_SleepyHead;

    // Skip login screen, unless asked not to on the command line
    bool skip_login = PREF.ExistsAndTrue(STR_GEN_SkipLogin);

    if (force_login_screen) { skip_login = false; }

    // Todo: Make a wrapper for Preference settings, like Profile settings have..
    QDateTime lastchecked, today = QDateTime::currentDateTime();

    PREF.init(STR_GEN_UpdatesAutoCheck, true);
    PREF.init(STR_GEN_UpdateCheckFrequency, 7);    // days
    PREF.init(STR_PREF_AllowEarlyUpdates, false);

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Check when last checked for updates..
    ////////////////////////////////////////////////////////////////////////////////////////////
    bool check_updates = false;

    if (PREF[STR_GEN_UpdatesAutoCheck].toBool()) {
        int update_frequency = PREF[STR_GEN_UpdateCheckFrequency].toInt();
        int days = 1000;
        lastchecked = PREF[STR_GEN_UpdatesLastChecked].toDateTime();

        if (PREF.contains(STR_GEN_UpdatesLastChecked)) {
            days = lastchecked.secsTo(today);
            days /= 86400;
        };

        if (days > update_frequency) {
            check_updates = true;
        }
    }

    if (!Profiles::profiles.size()) {
        // Show New User wizard..
        NewProfile newprof(0);

        if (newprof.exec() == NewProfile::Rejected) {
            return 0;
        }

        release_notes();
    } else {
        if (PREF.contains(STR_PREF_VersionString)) {

            if (compareVersion(PREF[STR_PREF_VersionString].toString()) < 0) {
                release_notes();

                check_updates = false;
            }
        }

        ProfileSelect profsel(0);

        if (skip_login) {
            profsel.QuickLogin();

            if (profsel.result() == ProfileSelect::Rejected) {
                exit(1);
            }

            p_profile = Profiles::Get(PREF[STR_GEN_Profile].toString());
        } else { p_profile = nullptr; }

        if (!p_profile) {
            if (profsel.exec() == ProfileSelect::Rejected) {
                exit(1);
            }
        }
    }

    PREF[STR_PREF_VersionString] = FullVersionString;

    p_profile = Profiles::Get(PREF[STR_GEN_Profile].toString());

    qDebug() << "Opened Profile" << p_profile->user->userName();

    //    int id=QFontDatabase::addApplicationFont(":/fonts/FreeSans.ttf");
    //    QFontDatabase fdb;
    //    QStringList ffam=fdb.families();
    //    for (QStringList::iterator i=ffam.begin();i!=ffam.end();i++) {
    //        qDebug() << "Loaded Font: " << (*i);
    //    }

    if (!PREF.contains("Fonts_Application_Name")) {
        PREF["Fonts_Application_Name"] = "Sans Serif";
        PREF["Fonts_Application_Size"] = 10;
        PREF["Fonts_Application_Bold"] = false;
        PREF["Fonts_Application_Italic"] = false;
    }


    QApplication::setFont(QFont(PREF["Fonts_Application_Name"].toString(),
                                PREF["Fonts_Application_Size"].toInt(),
                                PREF["Fonts_Application_Bold"].toBool() ? QFont::Bold : QFont::Normal,
                                PREF["Fonts_Application_Italic"].toBool()));

    qDebug() << "Selected Font" << QApplication::font().family();

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    qInstallMessageHandler(MyOutputHandler);
#else
    qInstallMsgHandler(MyOutputHandler);
#endif

    // Must be initialized AFTER profile creation
    MainWindow w;
    mainwin = &w;

    if (check_updates) { mainwin->CheckForUpdates(); }

    w.show();

    return a.exec();
}
