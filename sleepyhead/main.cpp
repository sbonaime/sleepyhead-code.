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
#include <QSysInfo>

#include "version.h"
#include "logger.h"
#include "SleepLib/schema.h"
#include "mainwindow.h"
#include "SleepLib/profiles.h"
#include "profileselect.h"
#include "newprofile.h"
#include "translation.h"
#include "common_gui.h"


// Gah! I must add the real darn plugin system one day.
#include "SleepLib/loader_plugins/prs1_loader.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/loader_plugins/md300w1_loader.h"
#include "SleepLib/loader_plugins/zeo_loader.h"
#include "SleepLib/loader_plugins/somnopose_loader.h"
#include "SleepLib/loader_plugins/resmed_loader.h"
#include "SleepLib/loader_plugins/intellipap_loader.h"
#include "SleepLib/loader_plugins/icon_loader.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

MainWindow *mainwin = nullptr;

void initialize()
{
    schema::init();
}

void release_notes()
{
    QDialog relnotes;
    relnotes.setWindowTitle(STR_TR_SleepyHead + " " + QObject::tr("Release Notes") +" "+FullVersionString);
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

    initializeLogger();


    ////////////////////////////////////////////////////////////////////////////////////////////
    // Language Selection
    ////////////////////////////////////////////////////////////////////////////////////////////
    initTranslations(settings);
    initializeStrings(); // Important, call this AFTER translator is installed.
    a.setApplicationName(STR_TR_SleepyHead);

    float glversion = getOpenGLVersion();

    bool opengl2supported = glversion >= 2.0;
    bool bad_graphics = !opengl2supported;
    bool intel_graphics = getOpenGLVersionString().contains("INTEL", Qt::CaseInsensitive);

#if defined(Q_OS_WIN)
    bool angle_supported = getGraphicsEngine().contains(CSTR_GFX_ANGLE, Qt::CaseInsensitive) && (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA);
    if (bad_graphics) {
        bad_graphics = !angle_supported;
    }
#endif

    QString lookfor = QObject::tr("Look for this build in <a href='%1'>SleepyHead's files hosted on Sourceforge</a>.").arg("http://sf.net/projects/sleepyhead/files");
#ifdef BROKEN_OPENGL_BUILD
    Q_UNUSED(bad_graphics)
    Q_UNUSED(intel_graphics)

    QString betterbuild = "Settings/BetterBuild";
    QString fasterbuildavailable = QObject::tr("A faster build of SleepyHead may be available");
    QString notbotheragain = QObject::tr("You will not be bothered with this message again.");
    QString betterresults = QObject::tr("This version will run fine, but a \"<b>%1</b>\" tagged build of SleepyHead will likely run much smoother on your computer.");

    if (opengl2supported) {
        if (!settings.value(betterbuild, false).toBool()) {
            QMessageBox::information(nullptr, fasterbuildavailable,
                             QObject::tr("This build of SleepyHead was designed to work with older computers lacking OpenGL 2.0 support, but it looks like your computer has full support for it.") + "<br/><br/>"+
                             betterresults.arg("-OpenGL")+"<br/><br/>"+
                             lookfor + "<br/><br/>"+
                             notbotheragain, QMessageBox::Ok, QMessageBox::Ok);
            settings.setValue(betterbuild, true);
        }
    } else {
#if defined(Q_OS_WIN)
        if (angle_supported) {
            if (!settings.value(betterbuild, false).toBool()) {
                QMessageBox::information(nullptr, fasterbuildavailable,
                             QObject::tr("This build of SleepyHead was designed to work with older computers lacking OpenGL 2.0 support, which yours doesn't have, but there may still be a better version available for your computer.") + "<br/><br/>"+
                             betterresults.arg("-ANGLE")+"<br/><br/>"+
                             QObject::tr("If you are running this in a virtual machine like VirtualBox or VMware, please disregard this message, as no better build is available.")+"<br/><br/>"+
                             lookfor + "<br/><br/>"+
                             notbotheragain, QMessageBox::Ok, QMessageBox::Ok);
                settings.setValue(betterbuild, true);
            }
        }
#endif
    }
#else
    if (bad_graphics) {
        QMessageBox::warning(nullptr, QObject::tr("Incompatible Graphics Hardware"),
                             QObject::tr("This build of SleepyHead requires OpenGL 2.0 support to function correctly, and unfortunately your computer lacks this capability.") + "<br/><br/>"+
                             QObject::tr("You may need to update your computers graphics drivers from the GPU makers website. %1").
                                arg(intel_graphics ? QObject::tr("(<a href='http://intel.com/support'>Intel's support site</a>)") : "")+"<br/><br/>"+
                             QObject::tr("Because graphs will not render correctly, and it may cause crashes, this build will now exit.")+"<br/><br/>"+
                             QObject::tr("Don't be disheartened, there is another build available tagged \"<b>-BrokenGL</b>\" that should work on your computer.")+ "<br/><br/>"+
                             lookfor+ "<br/><br/>"


                             ,QMessageBox::Ok, QMessageBox::Ok);
        exit(1);
    }
#endif

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
    MD300W1Loader::Register();
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

    // Must be initialized AFTER profile creation
    MainWindow w;

    mainwin = &w;

  //  if (check_updates) { mainwin->CheckForUpdates(); }

    w.show();

    return a.exec();
}
