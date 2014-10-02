/* SleepyHead MainWindow Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QGLContext>

#include <QFileDialog>
#include <QMessageBox>
#include <QResource>
#include <QProgressBar>
#include <QWebHistory>
#include <QWebFrame>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QNetworkReply>
#include <QTimer>
#include <QSettings>
#include <QPixmap>
#include <QDesktopWidget>
#include <QListView>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QProcess>
#include <QFontMetrics>
#include <QTextDocument>
#include <QTranslator>
#include <QPushButton>
#include <QCalendarWidget>

#include "common_gui.h"
#include "version.h"


#include <cmath>

// Custom loaders that don't autoscan..
#include <SleepLib/loader_plugins/zeo_loader.h>
#include <SleepLib/loader_plugins/somnopose_loader.h>

#ifndef REMSTAR_M_SUPPORT
#include <SleepLib/loader_plugins/mseries_loader.h>
#endif

#include "logger.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "newprofile.h"
#include "exportcsv.h"
#include "SleepLib/schema.h"
#include "Graphs/glcommon.h"
#include "UpdaterWindow.h"
#include "SleepLib/calcs.h"
#include "SleepLib/progressdialog.h"
#include "version.h"

#include "reports.h"
#include "statistics.h"

QProgressBar *qprogress;
QLabel *qstatus;
QStatusBar *qstatusbar;

extern Profile *profile;

QString getOpenGLVersionString()
{
    static QString glversion;

    if (glversion.isEmpty()) {
        QGLWidget w;
        w.makeCurrent();

        glversion = QString(QLatin1String(reinterpret_cast<const char*>(glGetString(GL_VERSION))));
        qDebug() << "OpenGL Version:" << glversion;
    }
    return glversion;
}

float getOpenGLVersion()
{
    QString glversion = getOpenGLVersionString();
    glversion = glversion.section(" ",0,0);
    bool ok;
    float v = glversion.toFloat(&ok);

    if (!ok) {
        QString tmp = glversion.section(".",0,1);
        v = tmp.toFloat(&ok);
        if (!ok) {
            // just look at major, we are only interested in whether we have OpenGL 2.0 anyway
            tmp = glversion.section(".",0,0);
            v = tmp.toFloat(&ok);
        }
    }
    return v;
}

QString getGraphicsEngine()
{
    QString gfxEngine = QString();
#ifdef BROKEN_OPENGL_BUILD
    gfxEngine = CSTR_GFX_BrokenGL;
#else
    QString glversion = getOpenGLVersionString();
    if (glversion.contains(CSTR_GFX_ANGLE)) {
        gfxEngine = CSTR_GFX_ANGLE;
    } else {
        gfxEngine = CSTR_GFX_OpenGL;
    }
#endif
    return gfxEngine;
}

void MainWindow::logMessage(QString msg)
{
    ui->logText->appendPlainText(msg);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    Q_ASSERT(p_profile != nullptr);

    ui->setupUi(this);

    if (logger) {
        connect(logger, SIGNAL(outputLog(QString)), this, SLOT(logMessage(QString)));
    }

    QString version = VersionString;

#ifdef TEST_BUILD
    version += QString(STR_TestBuild);
#else
    ui->warningLabel->hide();
#endif

    version += " "+getGraphicsEngine();

    if (QString(GIT_BRANCH) != "master") { version += " [" + QString(GIT_BRANCH)+" branch]"; }


    this->setWindowTitle(STR_TR_SleepyHead + QString(" v%1 (" + tr("Profile") + ": %2)").arg(version).arg(PREF[STR_GEN_Profile].toString()));

    qDebug() << STR_TR_SleepyHead << VersionString << "built with Qt" << QT_VERSION_STR << "on" << __DATE__ << __TIME__;

#ifdef BROKEN_OPENGL_BUILD
    qDebug() << "This build has been created especially for computers with older graphics hardware.\n";
#endif

    //ui->tabWidget->setCurrentIndex(1);

#ifdef Q_OS_MAC

    ui->action_About->setMenuRole(QAction::ApplicationSpecificRole);
    ui->action_Preferences->setMenuRole(QAction::ApplicationSpecificRole);
    ui->action_Exit->setMenuRole(QAction::ApplicationSpecificRole);
#if(QT_VERSION<QT_VERSION_CHECK(5,0,0))
    // Disable Screenshot on Mac Platform,as it doesn't work in Qt4, and the system provides this functionality anyway.
    ui->action_Screenshot->setEnabled(false);
#endif
#endif

//#ifdef LOCK_RESMED_SESSIONS
//    QList<Machine *> machines = p_profile->GetMachines(MT_CPAP);
//    for (QList<Machine *>::iterator it = machines.begin(); it != machines.end(); ++it) {
//        QString mclass=(*it)->loaderName();
//        if (mclass == STR_MACH_ResMed) {
//            qDebug() << "ResMed machine found.. locking Session splitting capabilities";

//            // Have to sacrifice these features to get access to summary data.
//            p_profile->session->setCombineCloseSessions(0);
//            p_profile->session->setDaySplitTime(QTime(12,0,0));
//            p_profile->session->setIgnoreShortSessions(false);
//            break;
//        }
//    }
//#endif

    ui->actionToggle_Line_Cursor->setChecked(p_profile->appearance->lineCursorMode());

    overview = nullptr;
    daily = nullptr;
    prefdialog = nullptr;

    m_inRecalculation = false;
    m_restartRequired = false;
    // Initialize Status Bar objects
    qstatusbar = ui->statusbar;
    qprogress = new QProgressBar(this);
    qprogress->setMaximum(100);
    qstatus = new QLabel("", this);
    qprogress->hide();
    ui->statusbar->setMinimumWidth(200);
    ui->statusbar->addPermanentWidget(qstatus, 0);
    ui->statusbar->addPermanentWidget(qprogress, 1);

    ui->actionDebug->setChecked(p_profile->general->showDebug());

    QTextCharFormat format = ui->statStartDate->calendarWidget()->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    Qt::DayOfWeek dow=firstDayOfWeekFromLocale();
    ui->statStartDate->calendarWidget()->setWeekdayTextFormat(Qt::Saturday, format);
    ui->statStartDate->calendarWidget()->setWeekdayTextFormat(Qt::Sunday, format);
    ui->statStartDate->calendarWidget()->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    ui->statStartDate->calendarWidget()->setFirstDayOfWeek(dow);
    ui->statEndDate->calendarWidget()->setWeekdayTextFormat(Qt::Saturday, format);
    ui->statEndDate->calendarWidget()->setWeekdayTextFormat(Qt::Sunday, format);
    ui->statEndDate->calendarWidget()->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    ui->statEndDate->calendarWidget()->setFirstDayOfWeek(dow);

    ui->statEndDate->setVisible(false);
    ui->statStartDate->setVisible(false);

    ui->reportModeRange->setVisible(false);
    switch(p_profile->general->statReportMode()) {
        case 0:
            ui->reportModeStandard->setChecked(true);
            break;
        case 1:
            ui->reportModeMonthly->setChecked(true);
            break;
        case 2:
            ui->reportModeRange->setChecked(true);
            ui->statEndDate->setVisible(true);
            ui->statStartDate->setVisible(true);
            break;
        default:
            p_profile->general->setStatReportMode(0);
    }
    if (!p_profile->general->showDebug()) {
        ui->logText->hide();
    }

    ui->actionShow_Performance_Counters->setChecked(p_profile->general->showPerformance());

#ifdef Q_OS_MAC
    p_profile->appearance->setAntiAliasing(false);
#endif
    //ui->action_Link_Graph_Groups->setChecked(p_profile->general->linkGroups());

    first_load = true;

    // Using the dirty registry here. :(
    QSettings settings(getDeveloperName(), getAppName());

    // Load previous Window geometry (this is currently broken on Mac as of Qt5.2.1)
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());

    daily = new Daily(ui->tabWidget, nullptr);
    ui->tabWidget->insertTab(2, daily, STR_TR_Daily);


    // Start with the Summary Tab
    ui->tabWidget->setCurrentWidget(ui->statisticsTab); // setting this to daily shows the cube during loading..

    // Nifty Notification popups in System Tray (uses Growl on Mac)
    if (QSystemTrayIcon::isSystemTrayAvailable() && QSystemTrayIcon::supportsMessages()) {
        systray = new QSystemTrayIcon(QIcon(":/icons/bob-v3.0.png"), this);
        systray->show();
        systraymenu = new QMenu(this);
        systray->setContextMenu(systraymenu);
        QAction *a = systraymenu->addAction(STR_TR_SleepyHead + " v" + VersionString);
        a->setEnabled(false);
        systraymenu->addSeparator();
        systraymenu->addAction(tr("&About"), this, SLOT(on_action_About_triggered()));
        systraymenu->addAction(tr("Check for &Updates"), this,
                               SLOT(on_actionCheck_for_Updates_triggered()));
        systraymenu->addSeparator();
        systraymenu->addAction(tr("E&xit"), this, SLOT(close()));
    } else { // if not available, the messages will popup in the taskbar
        systray = nullptr;
        systraymenu = nullptr;
    }

    ui->toolBox->setCurrentIndex(0);
    bool b = p_profile->appearance->rightSidebarVisible();
    ui->action_Sidebar_Toggle->setChecked(b);
    ui->toolBox->setVisible(b);

    daily->graphView()->redraw();

    if (p_profile->cpap->AHIWindow() < 30.0) {
        p_profile->cpap->setAHIWindow(60.0);
    }

    ui->recordsBox->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    ui->statisticsView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    ui->bookmarkView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    QString loadingtxt =
        "<HTML><body style='text-align: center; vertical-align: center'><table width='100%' height='100%'><tr><td align=center><h1>"
        + tr("Loading...") + "</h1></td></tr></table></body></HTML>";
    ui->statisticsView->setHtml(loadingtxt);
    on_tabWidget_currentChanged(0);

#ifndef REMSTAR_M_SUPPORT
    ui->actionImport_RemStar_MSeries_Data->setVisible(false);
#endif

    on_homeButton_clicked();

    qsrand(QDateTime::currentDateTime().toTime_t());

    // Translators, these are only temporary messages, don't bother unless you really want to..

    warnmsg.push_back(tr("<b>Warning:</b> This is a pre-release build, and may at times show unstable behaviour. It is intended for testing purposes."));
    warnmsg.push_back(tr("If you experience CPAP chart/data errors after upgrading to a new version, try rebuilding your CPAP database from the Data menu."));
    warnmsg.push_back(tr("Make sure you keep your SleepyHead data folder backed up when trying testing versions."));
    warnmsg.push_back(tr("Please ensure you are running the latest version before reporting any bugs."));
    warnmsg.push_back(tr("When reporting bugs, please make sure to supply the SleepyHead version number, operating system details and CPAP machine model."));
    warnmsg.push_back(tr("Make sure you're willing and able to supply a .zip of your CPAP data or a crash report before you think about filing a bug report."));
    warnmsg.push_back(tr("Think twice before filing a bug report that already exists, PLEASE search first, as your likely not the first one to notice it!"));
    warnmsg.push_back(tr("This red message line is intentional, and will not be a feature in the final version..."));
    warnmsg.push_back(tr(""));

    wtimer.setParent(this);
    warnidx = 0;
    wtimer.singleShot(0, this, SLOT(on_changeWarningMessage()));

    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(on_aboutToQuit()));

    QList<int> a;
    int panel_width = p_profile->appearance->rightPanelWidth();
    a.push_back(this->width() - panel_width);
    a.push_back(panel_width);
    ui->splitter_2->setSizes(a);
    ui->splitter_2->setStretchFactor(0,1);
    ui->splitter_2->setStretchFactor(1,0);
}

void MainWindow::on_changeWarningMessage()
{
    int i=warnidx++ % warnmsg.size();
    QString warning = "<html><head/><body><p>"+warnmsg[i]+"</p></body></html>";
    ui->warningLabel->setText(warning);
    wtimer.singleShot(10000, this, SLOT(on_changeWarningMessage()));
}


quint16 chandata_version = 1;

void saveChannels()
{
    QString filename = p_profile->Get("{DataFolder}/") + "channels.dat";
    QFile f(filename);
    qDebug() << "Saving Channel States";
    f.open(QFile::WriteOnly);
    QDataStream out(&f);
    out.setVersion(QDataStream::Qt_4_6);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)magic;
    out << (quint16)chandata_version;

    quint16 size = schema::channel.channels.size();
    out << size;

    QHash<ChannelID, schema::Channel *>::iterator it;
    QHash<ChannelID, schema::Channel *>::iterator it_end = schema::channel.channels.end();

    for (it = schema::channel.channels.begin(); it != it_end; ++it) {
        schema::Channel * chan = it.value();
        out << it.key();
        out << chan->code();
        out << chan->enabled();
        out << chan->defaultColor();
        out << chan->fullname();
        out << chan->label();
        out << chan->description();
        out << chan->lowerThreshold();
        out << chan->lowerThresholdColor();
        out << chan->upperThreshold();
        out << chan->upperThresholdColor();
        out << chan->showInOverview();
    }

    f.close();

}


void loadChannels()
{
    QString filename = p_profile->Get("{DataFolder}/") + "channels.dat";
    QFile f(filename);
    if (!f.open(QFile::ReadOnly)) {
        return;
    }
    qDebug() << "Loading Channel States";

    QDataStream in(&f);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 mag;
    in >> mag;

    if (magic != mag)  {
        qDebug() << "LoadChannels: Faulty data";
        return;
    }
    quint16 version;
    in >> version;

    if (version < chandata_version) {
        return;
        //upgrade here..
    }

    quint16 size;
    in >> size;

    QString name;
    ChannelID code;
    bool enabled;
    QColor color;
    EventDataType lowerThreshold;
    QColor lowerThresholdColor;
    EventDataType upperThreshold;
    QColor upperThresholdColor;

    QString fullname;
    QString label;
    QString description;
    bool showOverview = false;

    for (int i=0; i < size; i++) {
        in >> code;
        schema::Channel * chan = &schema::channel[code];
        in >> name;
        if (chan->code() != name) {
            qDebug() << "Looking up channel" << name << "by name, as it's ChannedID must have changed";
            chan = &schema::channel[name];
        }
        in >> enabled;
        in >> color;
        in >> fullname;
        in >> label;
        in >> description;
        in >> lowerThreshold;
        in >> lowerThresholdColor;
        in >> upperThreshold;
        in >> upperThresholdColor;
        if (version >= 1) {
            in >> showOverview;
        }

        if (chan->isNull()) {
            qDebug() << "loadChannels has no idea about channel" << name;
            if (in.atEnd()) return;
            continue;
        }
        chan->setEnabled(enabled);
        chan->setDefaultColor(color);
        chan->setFullname(fullname);
        chan->setLabel(label);
        chan->setDescription(description);
        chan->setLowerThreshold(lowerThreshold);
        chan->setLowerThresholdColor(lowerThresholdColor);
        chan->setUpperThreshold(upperThreshold);
        chan->setUpperThresholdColor(upperThresholdColor);

        chan->setShowInOverview(showOverview);
        if (in.atEnd()) return;
    }

    f.close();
}

void MainWindow::on_aboutToQuit()
{
    Notify(QObject::tr("Don't forget to place your datacard back in your CPAP machine"), QObject::tr("SleepyHead Reminder"));
    QThread::msleep(1000);
    QApplication::processEvents();
}

void MainWindow::closeEvent(QCloseEvent * event)
{

    saveChannels();
    schema::channel.Save();

    if (daily) {
        daily->close();
        daily->deleteLater();
    }

    if (overview) {
        overview->close();
        overview->deleteLater();
    }

    // Shutdown and Save the current User profile
    Profiles::Done();

    // Save current window position
    QSettings settings(getDeveloperName(), getAppName());
    settings.setValue("MainWindow/geometry", saveGeometry());

    QMainWindow::closeEvent(event);
}

extern MainWindow *mainwin;
MainWindow::~MainWindow()
{
//    if (systraymenu) { delete systraymenu; }

//    if (systray) { delete systray; }

    // Trash anything allocated by the Graph objects
    DestroyGraphGlobals();

    disconnect(logger, SIGNAL(outputLog(QString)), this, SLOT(logMessage(QString)));
    shutdownLogger();

    mainwin = nullptr;
    delete ui;
}

void MainWindow::log(QString text)
{
    logger->appendClean(text);
}


void MainWindow::Notify(QString s, QString title, int ms)
{
    if (title.isEmpty()) {
        title = "SleepyHead v" + VersionString;
    }
    if (systray) {
        // GNOME3's systray hides the last line of the displayed Qt message.
        // As a workaround, add an extra line to bump the message back
        // into the visible area.
        QString msg = s;

#ifdef Q_OS_UNIX
        char *desktop = getenv("DESKTOP_SESSION");

        if (desktop && !strncmp(desktop, "gnome", 5)) {
            msg += "\n";
        }
#endif

        systray->showMessage(title, msg, QSystemTrayIcon::Information, ms);
    } else {
        ui->statusbar->showMessage(s, ms);
    }
}

class MyStatsPage: public QWebPage
{
  public:
    MyStatsPage(QObject *parent);
    virtual ~MyStatsPage();
  protected:
    //virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID);
    virtual void javaScriptAlert(QWebFrame *frame, const QString &msg);
};
MyStatsPage::MyStatsPage(QObject *parent)
    : QWebPage(parent)
{
}
MyStatsPage::~MyStatsPage()
{
}

void MyStatsPage::javaScriptAlert(QWebFrame *frame, const QString &msg)
{
    Q_UNUSED(frame);
    mainwin->sendStatsUrl(msg);
}

QString getCPAPPixmap(QString mach_class)
{
    QString cpapimage;
    if (mach_class == STR_MACH_ResMed) cpapimage = ":/icons/rms9.png";
    else if (mach_class == STR_MACH_PRS1) cpapimage = ":/icons/prs1.png";
    else if (mach_class == STR_MACH_Intellipap) cpapimage = ":/icons/intellipap.png";
    return cpapimage;
}

//QIcon getCPAPIcon(QString mach_class)
//{
//    QString cpapimage = getCPAPPixmap(mach_class);

//    return QIcon(cpapimage);
//}

void MainWindow::PopulatePurgeMenu()
{
    ui->menu_Rebuild_CPAP_Data->disconnect(ui->menu_Rebuild_CPAP_Data, SIGNAL(triggered(QAction*)), this, SLOT(on_actionRebuildCPAP(QAction *)));
    ui->menu_Rebuild_CPAP_Data->clear();

    ui->menuPurge_CPAP_Data->disconnect(ui->menuPurge_CPAP_Data, SIGNAL(triggered(QAction*)), this, SLOT(on_actionPurgeMachine(QAction *)));
    ui->menuPurge_CPAP_Data->clear();

    QList<Machine *> machines = p_profile->GetMachines(MT_CPAP);
    for (int i=0; i < machines.size(); ++i) {
        Machine *mach = machines.at(i);
        QString name =  mach->brand() + " "+
                        mach->model() + " "+
                        mach->serial();

        QAction * action = new QAction(name.replace("&","&&"), ui->menu_Rebuild_CPAP_Data);
        action->setIconVisibleInMenu(true);
        action->setIcon(mach->getPixmap());
        action->setData(mach->loaderName()+":"+mach->serial());
        ui->menu_Rebuild_CPAP_Data->addAction(action);

        action = new QAction(name.replace("&","&&"), ui->menuPurge_CPAP_Data);
        action->setIconVisibleInMenu(true);
        action->setIcon(mach->getPixmap()); //getCPAPIcon(mach->loaderName()));
        action->setData(mach->loaderName()+":"+mach->serial());

        ui->menuPurge_CPAP_Data->addAction(action);
    }
    ui->menu_Rebuild_CPAP_Data->connect(ui->menu_Rebuild_CPAP_Data, SIGNAL(triggered(QAction*)), this, SLOT(on_actionRebuildCPAP(QAction*)));
    ui->menuPurge_CPAP_Data->connect(ui->menuPurge_CPAP_Data, SIGNAL(triggered(QAction*)), this, SLOT(on_actionPurgeMachine(QAction*)));
}

QString GenerateWelcomeHTML();

void MainWindow::Startup()
{
    qstatus->setText(tr("Loading Data"));
    qprogress->show();
    //qstatusbar->showMessage(tr("Loading Data"),0);

    // profile is a global variable set in main after login
    p_profile->LoadMachineData();

    QList<MachineLoader *> loaders = GetLoaders();
    for (int i=0; i<loaders.size(); ++i) {
        connect(loaders.at(i), SIGNAL(machineUnsupported(Machine*)), this, SLOT(MachineUnsupported(Machine*)));
    }

    PopulatePurgeMenu();

//    SnapshotGraph = new gGraphView(this, daily->graphView());

//    // Snapshot graphs mess up with pixmap cache
//    SnapshotGraph->setUsePixmapCache(false);

//   // SnapshotGraph->setFormat(daily->graphView()->format());
//    //SnapshotGraph->setMaximumSize(1024,512);
//    //SnapshotGraph->setMinimumSize(1024,512);
//    SnapshotGraph->hide();

    overview = new Overview(ui->tabWidget, daily->graphView());
    ui->tabWidget->insertTab(3, overview, STR_TR_Overview);


//    GenerateStatistics();

    ui->statStartDate->setDate(p_profile->FirstDay());
    ui->statEndDate->setDate(p_profile->LastDay());

    if (overview) { overview->ReloadGraphs(); }

    qprogress->hide();
    qstatus->setText("");

    if (p_profile->p_preferences[STR_PREF_ReimportBackup].toBool()) {
        importCPAPBackups();
        p_profile->p_preferences[STR_PREF_ReimportBackup]=false;
    }

    ui->tabWidget->setCurrentWidget(ui->welcomeTab);

    if (daily) {
        daily->ReloadGraphs();
//        daily->populateSessionWidget();
    }

}

int MainWindow::importCPAP(ImportPath import, const QString &message)
{
    if (!import.loader) {
        return 0;
    }

    QDialog * popup = new QDialog(this);
    QLabel * waitmsg = new QLabel(message);
    QHBoxLayout *hlayout = new QHBoxLayout;

    QLabel * imglabel = new QLabel(popup);

    QPixmap image = import.loader->getPixmap(import.loader->PeekInfo(import.path).series);
//    QPixmap image(getCPAPPixmap(import.loader->loaderName()));
    image = image.scaled(64,64);
    imglabel->setPixmap(image);


    QVBoxLayout * vlayout = new QVBoxLayout;
    popup->setLayout(vlayout);
    vlayout->addLayout(hlayout);
    hlayout->addWidget(imglabel);
    hlayout->addWidget(waitmsg,1,Qt::AlignCenter);
    vlayout->addWidget(qprogress,1);

    qprogress->setVisible(true);
    popup->show();
    int c = import.loader->Open(import.path);

    if (c > 0) {
        Notify(tr("Imported %1 CPAP session(s) from\n\n%2").arg(c).arg(import.path), tr("Import Success"));
    } else if (c == 0) {
        Notify(tr("Already up to date with CPAP data at\n\n%1").arg(import.path), tr("Up to date"));
    } else {
        Notify(tr("Couldn't find any valid Machine Data at\n\n%1").arg(import.path),tr("Import Problem"));
    }

    popup->hide();
    vlayout->removeWidget(qprogress);
    ui->statusbar->insertWidget(1,qprogress,1);
    qprogress->setVisible(false);

    delete popup;

    disconnect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(on_aboutToQuit()));
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(on_aboutToQuit()));
    return c;
}

void MainWindow::finishCPAPImport()
{
    p_profile->StoreMachines();
    GenerateStatistics();

    if (overview) { overview->ReloadGraphs(); }
    if (daily) {
//        daily->populateSessionWidget();
        daily->ReloadGraphs();
    }
}

void MainWindow::importCPAPBackups()
{
    // Get BackupPaths for all CPAP machines
    QList<Machine *> machlist = p_profile->GetMachines(MT_CPAP);
    QList<ImportPath> paths;
    Q_FOREACH(Machine *m, machlist) {
        paths.append(ImportPath(m->getBackupPath(), lookupLoader(m)));
    }

    if (paths.size() > 0) {
        int c=0;
        Q_FOREACH(ImportPath path, paths) {
            c+=importCPAP(path, tr("Please wait, importing from backup folder(s)..."));
        }
        if (c>0) {
            finishCPAPImport();
        }
    }
}

#ifdef Q_OS_UNIX
# include <stdio.h>
# include <unistd.h>

# if defined(Q_OS_MAC) || defined(Q_OS_BSD4)
#  include <sys/mount.h>
# else
#  include <sys/statfs.h>
#  include <mntent.h>
# endif // Q_OS_MAC/BSD

#endif // Q_OS_UNIX

//! \brief Returns a list of drive mountpoints
QStringList getDriveList()
{
    QStringList drivelist;

#if defined(Q_OS_MAC) || defined(Q_OS_BSD4)
    struct statfs *mounts;
    int num_mounts = getmntinfo(&mounts, MNT_WAIT);
    if (num_mounts >= 0) {
        for (int i = 0; i < num_mounts; i++) {
            QString name = mounts[i].f_mntonname;

            // Only interested in drives mounted under /Volumes
            if (name.toLower().startsWith("/volumes/")) {
                drivelist.push_back(name);
//                qDebug() << QString("Disk type '%1' mounted at: %2").arg(mounts[i].f_fstypename).arg(mounts[i].f_mntonname);
            }
        }
    }

#elif defined(Q_OS_UNIX)
    // Unix / Linux (except BSD)
    FILE *mtab = setmntent("/etc/mtab", "r");
    struct mntent *m;
    struct mntent mnt;
    char strings[4096];

    // NOTE: getmntent_r is a GNU extension, requiring glibc.
    while ((m = getmntent_r(mtab, &mnt, strings, sizeof(strings)))) {

        struct statfs fs;
        if ((mnt.mnt_dir != NULL) && (statfs(mnt.mnt_dir, &fs) == 0)) {
            QString name = mnt.mnt_dir;
            quint64 size = fs.f_blocks * fs.f_bsize;

            if (size > 0) { // this should theoretically ignore /dev, /proc, /sys etc..
                drivelist.push_back(name);
            }
//            quint64 free = fs.f_bfree * fs.f_bsize;
//            quint64 avail = fs.f_bavail * fs.f_bsize;
        }
    }
    endmntent(mtab);

#elif defined(Q_OS_WIN)
    QFileInfoList list = QDir::drives();

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        QString name = fileInfo.filePath();
        if (name.at(0).toUpper() != QChar('C')) { // Ignore the C drive
            drivelist.push_back(name);
        }
    }

#endif

    return drivelist;
}


QList<ImportPath> MainWindow::detectCPAPCards()
{
    const int timeout = 20000;

    QList<ImportPath> detectedCards;

    QList<MachineLoader *>loaders = GetLoaders(MT_CPAP);
    QTime time;
    time.start();

    // Create dialog
    QDialog popup(this, Qt::SplashScreen);
    QLabel waitmsg(tr("Please insert your CPAP data card..."));
    QProgressBar progress;
    QVBoxLayout waitlayout;
    popup.setLayout(&waitlayout);

    QHBoxLayout layout2;
    QIcon icon("://icons/sdcard.png");
    QPushButton skipbtn(icon, tr("Choose a folder"));
    skipbtn.setMinimumHeight(40);
    waitlayout.addWidget(&waitmsg,1,Qt::AlignCenter);
    waitlayout.addWidget(&progress,1);
    waitlayout.addLayout(&layout2,1);
    layout2.addWidget(&skipbtn);
    popup.connect(&skipbtn, SIGNAL(clicked()), &popup, SLOT(hide()));
    progress.setValue(0);
    progress.setMaximum(timeout);
    progress.setVisible(true);
    popup.show();
    QApplication::processEvents();
    QString lastpath = (*p_profile)[STR_PREF_LastCPAPPath].toString();

    do {
        // Rescan in case card inserted
        QStringList AutoScannerPaths = getDriveList();
        //AutoScannerPaths.push_back(lastpath);

        if (!AutoScannerPaths.contains(lastpath)) {
            AutoScannerPaths.append(lastpath);
        }

        Q_FOREACH(const QString &path, AutoScannerPaths) {
            // Scan through available machine loaders and test if this folder contains valid folder structure
            Q_FOREACH(MachineLoader * loader, loaders) {
                if (loader->Detect(path)) {
                    detectedCards.append(ImportPath(path, loader));

                    qDebug() << "Found" << loader->loaderName() << "datacard at" << path;
                }
            }
        }
        int el=time.elapsed();
        progress.setValue(el);
        QApplication::processEvents();
        if (el > timeout) break;
        if (!popup.isVisible()) break;
        // needs a slight delay here
        QThread::msleep(200);
    } while (detectedCards.size() == 0);

    popup.hide();
    popup.disconnect(&skipbtn, SIGNAL(clicked()), &popup, SLOT(hide()));

    return detectedCards;
}


void MainWindow::on_action_Import_Data_triggered()
{
    if (m_inRecalculation) {
        Notify(tr("Access to Import has been blocked while recalculations are in progress."),STR_MessageBox_Busy);
        return;
    }

    QList<ImportPath> datacards = detectCPAPCards();

    QList<MachineLoader *>loaders = GetLoaders(MT_CPAP);

    QTime time;
    time.start();
    QDialog popup(this, Qt::FramelessWindowHint);
    popup.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QLabel * waitmsg = new QLabel(tr("Please wait, scanning for CPAP data cards..."));
    QVBoxLayout *waitlayout = new QVBoxLayout();
    waitlayout->addWidget(waitmsg,1,Qt::AlignCenter);
    waitlayout->addWidget(qprogress,1);
    popup.setLayout(waitlayout);

    bool asknew = false;
    qprogress->setVisible(false);

    if (datacards.size() > 0) {
        MachineInfo info = datacards[0].loader->PeekInfo(datacards[0].path);
        QString infostr;
        if (!info.model.isEmpty()) {
            QString infostr2 = info.model+" ("+info.serial+")";
            infostr = tr("A %1 file structure for a %2 was located at:").arg(info.brand).arg(infostr2);
        } else {
            infostr = tr("A %1 file structure was located at:").arg(datacards[0].loader->loaderName());
        }

        if (!p_profile->cpap->autoImport()) {
            QMessageBox mbox(QMessageBox::NoIcon,
                             tr("CPAP Data Located"),
                             infostr+"\n\n"+QDir::toNativeSeparators(datacards[0].path)+"\n\n"+
                             tr("Would you like to import from this location?"),
                             QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                             this);
            mbox.setDefaultButton(QMessageBox::Yes);
            mbox.setButtonText(QMessageBox::No, tr("Specify"));

            QPixmap pixmap = datacards[0].loader->getPixmap(datacards[0].loader->PeekInfo(datacards[0].path).series).scaled(64,64);

            //QPixmap pixmap = QPixmap(getCPAPPixmap(datacards[0].loader->loaderName())).scaled(64,64);
            mbox.setIconPixmap(pixmap);
            int res = mbox.exec();

            if (res == QMessageBox::Cancel) {
                // Give the communal progress bar back
                ui->statusbar->insertWidget(1,qprogress,1);
                return;
            } else if (res == QMessageBox::No) {
                waitmsg->setText(tr("Please wait, launching file dialog..."));
                datacards.clear();
                asknew = true;
            }
        }
    } else {
        waitmsg->setText(tr("No CPAP data card detected, launching file dialog..."));
        asknew = true;
    }

    if (asknew) {
        popup.show();
        mainwin->Notify(tr("Please remember to point the importer at the root folder or drive letter of your data-card, and not a subfolder."),
                        tr("Import Reminder"),8000);

        QFileDialog w(this);

        QString folder;
        if (p_profile->contains(STR_PREF_LastCPAPPath)) {
            folder = (*p_profile)[STR_PREF_LastCPAPPath].toString();
        } else {
#if QT_VERSION  < QT_VERSION_CHECK(5,0,0)
            folder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#else
            folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif
        }

        w.setDirectory(folder);
        w.setFileMode(QFileDialog::Directory);
        w.setOption(QFileDialog::ShowDirsOnly, true);

        // This doesn't work on WinXP

#if defined(Q_OS_MAC)
#if (QT_VERSION < QT_VERSION_CHECK(4,8,0))
        // Fix for tetragon, 10.6 barfs up Qt's custom dialog
        w.setOption(QFileDialog::DontUseNativeDialog, true);
#else
        w.setOption(QFileDialog::DontUseNativeDialog,false);
#endif // version check

#elif defined(Q_OS_UNIX)
        w.setOption(QFileDialog::DontUseNativeDialog,false);
#elif defined(Q_OS_WIN)
        // check the Os version.. winxp chokes
        w.setOption(QFileDialog::DontUseNativeDialog, true);
#endif
//#else
//        w.setOption(QFileDialog::DontUseNativeDialog, false);

//        QListView *l = w.findChild<QListView *>("listView");
//        if (l) {
//            l->setSelectionMode(QAbstractItemView::MultiSelection);
//        }

//        QTreeView *t = w.findChild<QTreeView *>();
//        if (t) {
//            t->setSelectionMode(QAbstractItemView::MultiSelection);
//        }

//#endif

        if (w.exec() != QDialog::Accepted) {
            popup.hide();
            ui->statusbar->insertWidget(1,qprogress,1);

            return;
        }

        popup.hide();

        for (int i = 0; i < w.selectedFiles().size(); i++) {
            Q_FOREACH(MachineLoader * loader, loaders) {
                if (loader->Detect(w.selectedFiles().at(i))) {
                    datacards.append(ImportPath(w.selectedFiles().at(i), loader));
                    break;
                }
            }
        }
    }

    bool newdata = false;

//    QStringList goodlocations;

//    waitmsg.setText(tr("Please wait, SleepyHead is importing data..."));
//    qprogress->setVisible(true);

//    popup.show();

    int c = -1;
    for (int i = 0; i < datacards.size(); i++) {
        QString dir = datacards[i].path;
        MachineLoader * loader = datacards[i].loader;
        if (!loader) continue;

        if (!dir.isEmpty()) {
//            qprogress->setValue(0);
//            qprogress->show();
//            qstatus->setText(tr("Importing Data"));
            c = importCPAP(datacards[i], tr("Importing Data"));
            qDebug() << "Finished Importing data" << c;

            if (c >= 0) {
            //    goodlocations.push_back(dir);
                QDir d(dir.section("/",0,-1));
                (*p_profile)[STR_PREF_LastCPAPPath] = d.absolutePath();
            }

            if (c > 0) {
                newdata = true;
            }

//            qstatus->setText("");
//            qprogress->hide();
        }
    }
//    popup.hide();

//    ui->statusbar->insertWidget(1, qprogress,1);

    if (newdata)  {
        finishCPAPImport();
        PopulatePurgeMenu();
    }
}

QMenu *MainWindow::CreateMenu(QString title)
{
    QMenu *menu = new QMenu(title, ui->menubar);
    ui->menubar->insertMenu(ui->menu_Help->menuAction(), menu);
    return menu;
}

void MainWindow::on_action_Fullscreen_triggered()
{
    if (ui->action_Fullscreen->isChecked()) {
        this->showFullScreen();
    } else {
        this->showNormal();
    }
}

void MainWindow::setRecBoxHTML(QString html)
{
    ui->recordsBox->setHtml(html);
}

QString MainWindow::getWelcomeHTML()
{
    // This is messy, but allows it to be translated easier
    return "<html>\n<head>"
           " <style type='text/css'>"
           "  <!--h1,p,a,td,body { font-family: 'FreeSans', 'Sans Serif' } --/>"
           "  p,a,td,body { font-size: 14px }"
           "  a:link,a:visited { color: \"#000020\"; text-decoration: none; font-weight: bold;}"
           "  a:hover { background-color: inherit; color: red; text-decoration:none; font-weight: bold; }"
           " </style>\n"
           "</head>"
           "<body leftmargin=0 topmargin=0 rightmargin=0>"
           "<table width=\"100%\" cellspacing=0 cellpadding=4 border=0 >"
           "<tr><td bgcolor=\"#d0d0d0\" colspan=2 cellpadding=0 valign=center align=center><font color=\"black\" size=+1><b>"
           + tr("Welcome to SleepyHead") + "</b></font></td></tr>"
           "<tr>"
           "<td valign=\"top\" leftmargin=0 cellpadding=6>"
           "<h3>" + tr("About SleepyHead") + "</h3>"
           "<p>" + tr("This software has been created to assist you in reviewing the data produced by CPAP Machines, used in the treatment of various Sleep Disorders.")
           + "</p>"
           "<p>" + tr("SleepyHead has been designed by a software developer with personal experience with a sleep disorder, and shaped by the feedback of many other willing testers dealing with similar conditions.")
           + "</p>"
           "<p><i><b>" + tr("This is a beta release, some features may not yet behave as expected.") +
           "</b></i><br/>" + tr("Please report any bugs you find to SleepyHead's SourceForge page.") + "</p>"

           "<h3>" + tr("Currenly supported machines:") + "</h3>"
           "<b>" + tr("CPAP") + "</b>"
           "<li>" + tr("Philips Respironics System One (CPAP Pro, Auto, BiPAP & ASV models)") + "</li>"
           "<li>" + tr("ResMed S9 models (CPAP, Auto, VPAP)") + "</li>"
           "<li>" + tr("DeVilbiss Intellipap (Auto)") + "</li>"
           "<li>" + tr("Fisher & Paykel ICON (CPAP, Auto)") + "</li>"
           "<b>" + tr("Oximetry") + "</b>"
           "<li>" + tr("Contec CMS50D+, CMS50E and CMS50F (not 50FW) Oximeters") + "</li>"
           "<li>" + tr("ResMed S9 Oximeter Attachment") + "</li>"
           "<p><h3>" + tr("Online Help Resources") + "</h3></p>"
           "<p><b>" + tr("Note:") + "</b>" +
           tr("I don't recommend using this built in web browser to do any major surfing in, it will work, but it's mainly meant as a help browser.")
           +
           tr("(It doesn't support SSL encryption, so it's not a good idea to type your passwords or personal details anywhere.)")
           + "</p>" +

           tr("SleepyHead's Online <a href=\"http://sleepyhead.sourceforge.net/wiki/index.php?title=SleepyHead_Users_Guide\">Users Guide</a><br/>")
           +
           tr("<a href=\"http://sleepyhead.sourceforge.net/wiki/index.php?title=Frequently_Asked_Questions\">Frequently Asked Questions</a><br/>")
           +
           tr("<a href=\"http://sleepyhead.sourceforge.net/wiki/index.php?title=Glossary\">Glossary of Sleep Disorder Terms</a><br/>")
           +
           tr("<a href=\"http://sleepyhead.sourceforge.net/wiki/index.php?title=Main_Page\">SleepyHead Wiki</a><br/>")
           +
           tr("SleepyHead's <a href='http://www.sourceforge.net/projects/sleepyhead'>Project Website</a> on SourceForge<br/>")
           +
           "<p><h3>" + tr("Further Information") + "</h3></p>"
           "<p>" +
           tr("Here are the <a href='qrc:/docs/release_notes.html'>release notes</a> for this version.") +
           "<br/>" +
           tr("Plus a few <a href='qrc:/docs/usage.html'>usage notes</a>, and some important information for Mac users.")
           + "<br/>" +
           "<p>" + tr("About <a href='http://en.wikipedia.org/wiki/Sleep_apnea'>Sleep Apnea</a> on Wikipedia")
           + "</p>"

           "<p>" + tr("Friendly forums to talk and learn about Sleep Apnea:") + "<br/>" +
           tr("<a href='http://www.cpaptalk.com'>CPAPTalk Forum</a>,") +
           tr("<a href='http://www.apneaboard.com/forums/'>Apnea Board</a>") + "</p>"
           "</td>"
           "<td><image src='qrc:/icons/bob-v3.0.png' width=220 height=220><br/>"
           "</td>"
           "</tr>"
           "<tr>"
           "<td colspan=2>"
           "<hr/>"
           "<p><b>" + tr("Copyright:") + "</b> " + tr("&copy;2011-2014") +
           " <a href=\"http://jedimark64.blogspot.com\">Mark Watkins</a> (jedimark)</p>"
           "<p><b>" + tr("License:") + "</b> " +
           tr("This software is released freely under the <a href=\"qrc:/COPYING\">GNU Public License</a>.") +
           "</p>"
           "<hr/>"
           "<p><b>" + tr("DISCLAIMER:") + "</b></p>"
           "<b><p>" +
           tr("This is <font color='red'><u>NOT</u></font> medical software. This application is merely a data viewer, and no guarantee is made regarding accuracy or correctness of any calculations or data displayed.")
           + "</p>"
           "<p>" + tr("The author will NOT be held liable by anyone who harms themselves or others by use or misuse of this software.")
           + "</p>"
           "<p>" + tr("Your doctor should always be your first and best source of guidance regarding the important matter of managing your health.")
           + "</p>"
           "<p>" + tr("*** <u>Use at your own risk</u> ***") + "</p></b>"
           "<hr/>"
           "</td></tr>"
           "</table>"
           "</body>"
           "</html>"

           ;
}

void MainWindow::on_homeButton_clicked()
{

    ui->webView->setHtml(getWelcomeHTML());

    //QString infourl="qrc:/docs/index.html"; // use this as a fallback
    //ui->webView->setUrl(QUrl(infourl));
}

void MainWindow::updateFavourites()
{
    QDate date = p_profile->LastDay(MT_JOURNAL);

    if (!date.isValid()) {
        return;
    }

    QString html = "<html><head><style type='text/css'>"
                   "p,a,td,body { font-family: '" + QApplication::font().family() + "'; }"
                   "p,a,td,body { font-size: " + QString::number(QApplication::font().pointSize() + 2) + "px; }"
                   "a:link,a:visited { color: inherit; text-decoration: none; }" //font-weight: normal;
                   "a:hover { background-color: inherit; color: white; text-decoration:none; font-weight: bold; }"
                   "</style></head><body>"
                   "<table width=100% cellpadding=2 cellspacing=0>";

    do {
        Day *journal = p_profile->GetDay(date, MT_JOURNAL);

        if (journal) {
            if (journal->size() > 0) {
                Session *sess = journal->firstSession(MT_JOURNAL);
                QString tmp;
                bool filtered = !bookmarkFilter.isEmpty();
                bool found = !filtered;

                if (sess->settings.contains(Bookmark_Start)) {
                    //QVariantList start=sess->settings[Bookmark_Start].toList();
                    //QVariantList end=sess->settings[Bookmark_End].toList();
                    QStringList notes = sess->settings[Bookmark_Notes].toStringList();

                    if (notes.size() > 0) {
                        tmp += QString("<tr><td><b><a href='daily=%1'>%2</a></b><br/>")
                               .arg(date.toString(Qt::ISODate))
                               .arg(date.toString());

                        tmp += "<list>";

                        for (int i = 0; i < notes.size(); i++) {
                            //QDate d=start[i].toDate();
                            QString note = notes[i];

                            if (filtered && note.contains(bookmarkFilter, Qt::CaseInsensitive)) {
                                found = true;
                            }

                            tmp += "<li>" + note + "</li>";
                        }

                        tmp += "</list></td>";
                    }
                }

                if (found) { html += tmp; }
            }
        }

        date = date.addDays(-1);
    } while (date >= p_profile->FirstDay(MT_JOURNAL));

    html += "</table></body></html>";
    ui->bookmarkView->setHtml(html);
}

void MainWindow::on_backButton_clicked()
{
    ui->webView->back();
}

void MainWindow::on_forwardButton_clicked()
{
    ui->webView->forward();
}

void MainWindow::on_webView_urlChanged(const QUrl &arg1)
{
    ui->urlBar->setEditText(arg1.toString());
}

void MainWindow::on_urlBar_activated(const QString &arg1)
{
    QUrl url(arg1);
    ui->webView->setUrl(url);
}

void MainWindow::on_dailyButton_clicked()
{
    ui->tabWidget->setCurrentWidget(daily);
    daily->RedrawGraphs();
}
void MainWindow::JumpDaily()
{
    on_dailyButton_clicked();
}

void MainWindow::on_overviewButton_clicked()
{
    ui->tabWidget->setCurrentWidget(overview);
}

void MainWindow::on_webView_loadFinished(bool arg1)
{
    Q_UNUSED(arg1);
    qprogress->hide();

    if (first_load) {
        QTimer::singleShot(0, this, SLOT(Startup()));
        first_load = false;
    } else {
        qstatus->setText("");
    }

    ui->backButton->setEnabled(ui->webView->history()->canGoBack());
    ui->forwardButton->setEnabled(ui->webView->history()->canGoForward());

    connect(ui->webView->page(), SIGNAL(linkHovered(QString, QString, QString)), this,
            SLOT(LinkHovered(QString, QString, QString)));
}

void MainWindow::on_webView_loadStarted()
{
    disconnect(ui->webView->page(), SIGNAL(linkHovered(QString, QString, QString)), this,
               SLOT(LinkHovered(QString, QString, QString)));

    if (!first_load) {
        qstatus->setText(tr("Loading"));
        qprogress->reset();
        qprogress->show();
    }
}

void MainWindow::on_webView_loadProgress(int progress)
{
    qprogress->setValue(progress);
}

void MainWindow::aboutBoxLinkClicked(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

void MainWindow::on_action_About_triggered()
{
    QString gitrev = QString(GIT_REVISION);

    if (!gitrev.isEmpty()) { gitrev = tr("Revision:")+" " + gitrev + " (" + QString(GIT_BRANCH) + " " + tr("branch") + ")"; }

    //    "<style type=\"text/css\">body { margin:0; padding:0; } html, body, #bg { height:100%; width:100% } #bg { position: absolute; left:0; right:0; bottom:0; top:0; overflow:hidden; z-index:1; } #bg img { width:100%; min-width:100%; min-height:100%; } #content { z-index:0; }</style><body><div id=\"bg\"> <img style=\"display:block;\" src=\"qrc:/icons/Bob Strikes Back.png\"></div><div id=\"content\">"
    QString msg = QString(
                "<html>"
                "<head><style type=\"text/css\">a:link, a:visited { color: #000044; text-decoration: underline; font-weight: normal;}"
                "a:hover { background-color: inherit; color: #4444ff; text-decoration:none; font-weight: normal; }"
                "</style></head>"

                "<body>"
                "<span style=\"color:#000000; font-weight:600; vertical-align:middle;\">"
                "<table width=100%><tr><td>"
                "<p><h1>" + STR_TR_SleepyHead +
                QString(" v%1 (%2)</h1></p><font color=black><p>").arg(VersionString).arg(ReleaseStatus) +
                tr("Build Date: %1 %2").arg(__DATE__).arg(__TIME__) +
                QString("<br/>%1<br/>").arg(gitrev) +
                tr("Graphics Engine: %1").arg(getGraphicsEngine())+
                "<br/>" +
                (tr("Data Folder Location: <a href=\"file://%1\">%2</a>").arg(GetAppRoot()).arg(QDir::toNativeSeparators(GetAppRoot())) +
                "<hr/>"+tr("Copyright") + " &copy;2011-2014 Mark Watkins (jedimark) <br/> \n" +
                tr("This software is released under the GNU Public License v3.0<br/>") +
                "<hr>"

                // Project links
                "<p>" +tr("SleepyHead Project Page") +
                ": <a href=\"http://sourceforge.net/projects/sleepyhead\">http://sourceforge.net/projects/sleepyhead</a><br/>"
                +
                tr("SleepyHead Wiki") +
                ": <a href=\"http://sleepyhead.sourceforge.net/wiki\">http://sleepyhead.sourceforge.net/wiki</a><p/>" +

                // Social media links.. (Dear Translators, if one of these isn't available in your country, it's ok to leave it out.)
                tr("Don't forget to Like/+1 SleepyHead on <a href=\"http://www.facebook.com/SleepyHeadCPAP\">Facebook</a> or <a href=\"http://plus.google.com/u/0/b/101426655252362287937\">Google+")
                + "</p>" +

                // Image
                "</td><td align='center'><img src=\"qrc:/icons/Jedimark.png\" width=260px><br/> <br/><i>"
                +tr("SleepyHead, brought to you by Jedimark") + "</i></td></tr><tr colspan><td colspan=2>" +


                // Credits section
                "<hr/><p><b><font size='+1'>" +tr("Kudos & Credits") + "</font></b></p><b>" +
                tr("Bugfixes, Patches and Platform Help:") + "</b> " +
                tr("James Marshall, Rich Freeman, John Masters, Keary Griffin, Patricia Shanahan, Alec Clews, manders99, Sean Stangl, Roy Stone, François Revol, Michael Masterson.")
                + "</p>"

                "<p><b>" + tr("Translators:") + "</b> " + tr("Arie Klerk (Dutch), Steffen Reitz and Marc Stephan (German), Chen Hao (Chinese), Lars-Erik Söderström (Swedish), Damien Vigneron (French), António Jorge Costa (Portuguese), Judith Guzmán (Spanish) and others I've still to add here.") +
                "</p>"

                "<p><b>" + tr("3rd Party Libaries:") + "</b> " +
                tr("SleepyHead is built using the <a href=\"http://qt-project.org\">Qt Application Framework</a>.")
                + " " +
                tr("In the updater code, SleepyHead uses <a href=\"http://sourceforge.net/projects/quazip\">QuaZip</a> by Sergey A. Tachenov, which is a C++ wrapper over Gilles Vollant's ZIP/UNZIP package.")
                + "<br/>"
                "<p>" + tr("Special thanks to Pugsy and Robysue from <a href='http://cpaptalk.com'>CPAPTalk</a> for their help with documentation and tutorials, as well as everyone who helped out by testing and sharing their CPAP data.")
                + "</p>"

                // Donations
                "<hr><p><font color=\"blue\">" +
                tr("Thanks for using SleepyHead. If you find it within your means, please consider encouraging future development by making a donation via Paypal.")
                + "</font>"


                "<hr><p><b>Disclaimer</b><br/><i>" +
                tr("This software comes with absolutely no warranty, either express of implied.") + " " +
                tr("It comes with no guarantee of fitness for any particular purpose.") + " " +
                tr("No guarantees are made regarding the accuracy of any data this program displays.") + "</i></p>"
                "<p><i>" +
                tr("This is NOT medical software, it is merely a research tool that provides a visual interpretation of data recorded by supported devices.")
                +
                "<b> " + tr("This software is NOT suitable for medical diagnostics purposes, neither is it fit for CPAP complaince reporting purposes, or ANY other medical use for that matter.")
                + "</b></i></p>"
                "<p><i>" +
                tr("The author and anyone associated with him accepts NO responsibilty for damages, issues or non-issues resulting from the use or mis-use of this software.")
                + "</p><p><b>" +
                tr("Use this software entirely at your own risk.") + "</b></i></p>"
                "</font></td></tr></table></span></body>"
               ));
    //"</div></body></html>"

    QDialog aboutbox;
    aboutbox.setWindowTitle(QObject::tr("About SleepyHead"));


    QVBoxLayout layout;
    aboutbox.setLayout(&layout);

    QWebView webview(&aboutbox);

    webview.setHtml(msg);
    webview.page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(&webview, SIGNAL(linkClicked(const QUrl &)), SLOT(aboutBoxLinkClicked(const QUrl &)));

    layout.insertWidget(0, &webview, 1);

    QHBoxLayout layout2;
    layout.insertLayout(1, &layout2, 1);
    QPushButton okbtn(QObject::tr("&Close"), &aboutbox);
    aboutbox.connect(&okbtn, SIGNAL(clicked()), SLOT(reject()));
    layout2.insertWidget(1, &okbtn, 1);

    QPushButton donatebtn(QObject::tr("&Donate"), &aboutbox);
    aboutbox.connect(&donatebtn, SIGNAL(clicked()),
                     SLOT(accept())); //hack this button to use the accepted slot, so clicking x closes like it shouldß
    layout2.insertWidget(1, &donatebtn, 1);

    QApplication::processEvents(); // MW: Needed on Mac, as the html has to finish loading


    if (aboutbox.exec() == QDialog::Accepted) {
        QDesktopServices::openUrl(QUrl("http://sourceforge.net/p/sleepyhead/donate"));
        //spawn browser with paypal site.
    }

    disconnect(&webview, SIGNAL(linkClicked(const QUrl &)), this,
               SLOT(aboutBoxLinkClicked(const QUrl &)));
}

void MainWindow::on_actionDebug_toggled(bool checked)
{
    p_profile->general->setShowDebug(checked);

    logger->strlock.lock();
    if (checked) {
        ui->logText->show();
    } else {
        ui->logText->hide();
    }
  //  QApplication::processEvents();
    logger->strlock.unlock();
}

void MainWindow::on_action_Reset_Graph_Layout_triggered()
{
    if (daily && (ui->tabWidget->currentWidget() == daily)) { daily->ResetGraphLayout(); }

    if (overview && (ui->tabWidget->currentWidget() == overview)) { overview->ResetGraphLayout(); }
}

void MainWindow::on_action_Preferences_triggered()
{
    //MW: TODO: This will crash if attempted to enter while still loading..

    if (m_inRecalculation) {
        mainwin->Notify(tr("Access to Preferences has been blocked until recalculation completes."));
        return;
    }

    PreferencesDialog pd(this, p_profile);
    prefdialog = &pd;

    if (pd.exec() == PreferencesDialog::Accepted) {
        qDebug() << "Preferences Accepted";

        //pd.Save();
        if (daily) {
            //daily->ReloadGraphs();
            daily->RedrawGraphs();
        }

        if (overview) {
            overview->RebuildGraphs(true);
            //overview->RedrawGraphs();
        }
    }

    prefdialog = nullptr;
}

#include "oximeterimport.h"
QDateTime datetimeDialog(QDateTime datetime, QString message);

void MainWindow::on_oximetryButton_clicked()
{
    OximeterImport oxiimp(this);
    oxiimp.exec();
}

void MainWindow::CheckForUpdates()
{
    on_actionCheck_for_Updates_triggered();
}

void MainWindow::on_actionCheck_for_Updates_triggered()
{
    UpdaterWindow *w = new UpdaterWindow(this);
    w->checkForUpdates();
}

bool toolbox_visible = false;
void MainWindow::on_action_Screenshot_triggered()
{
    daily->hideSpaceHogs();
    toolbox_visible = ui->toolBox->isVisible();
    ui->toolBox->hide();
    QTimer::singleShot(250, this, SLOT(DelayedScreenshot()));
}
void MainWindow::DelayedScreenshot()
{
    int w = width();
    int h = height();

    // Scale for high resolution displays (like Retina)
#if(QT_VERSION>=QT_VERSION_CHECK(5,0,0))
    qreal pr = devicePixelRatio();
    w /= pr;
    h /= pr;
#endif

#if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
     //QRect rec = QApplication::desktop()->screenGeometry();

     // grab the whole screen
     QPixmap desktop = QPixmap::grabWindow(QApplication::desktop()->winId());

     QPixmap pixmap = desktop.copy(x() * devicePixelRatio(), y() * devicePixelRatio(), (width()+6) * devicePixelRatio(), (height()+22) * devicePixelRatio());

#elif defined(Q_OS_MAC)
    QPixmap pixmap = QPixmap::grabWindow(this->winId(), x(), y(), w, h+10);
#endif

    QString a = PREF.Get("{home}/Screenshots");
    QDir dir(a);

    if (!dir.exists()) {
        dir.mkdir(a);
    }

    a += "/screenshot-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + ".png";

    qDebug() << "Saving screenshot to" << a;

    if (!pixmap.save(a)) {
        Notify(tr("There was an error saving screenshot to file \"%1\"").arg(QDir::toNativeSeparators(a)));
    } else {
        Notify(tr("Screenshot saved to file \"%1\"").arg(QDir::toNativeSeparators(a)));
    }
    daily->showSpaceHogs();
    ui->toolBox->setVisible(toolbox_visible);

}

void MainWindow::on_actionView_Oximetry_triggered()
{
    on_oximetryButton_clicked();
}
void MainWindow::updatestatusBarMessage(const QString &text)
{
    ui->statusbar->showMessage(text, 1000);
}

void MainWindow::on_actionPrint_Report_triggered()
{
#ifdef Q_WS_MAC
#if ((QT_VERSION <= QT_VERSION_CHECK(4, 8, 4)))
    QMessageBox::information(this, tr("Printing Disabled"),
                             tr("Please rebuild SleepyHead with Qt 4.8.5 or greater, as printing causes a crash with this version of Qt"),
                             QMessageBox::Ok);
    return;
#endif
#endif
    Report report;

    if (ui->tabWidget->currentWidget() == overview) {
        Report::PrintReport(overview->graphView(), STR_TR_Overview);
    } else if (ui->tabWidget->currentWidget() == daily) {
        Report::PrintReport(daily->graphView(), STR_TR_Daily, daily->getDate());
    } else {
        QPrinter printer(QPrinter::HighResolution);
#ifdef Q_WS_X11
        printer.setPrinterName("Print to File (PDF)");
        printer.setOutputFormat(QPrinter::PdfFormat);
        QString name;
        QString datestr;

        if (ui->tabWidget->currentWidget() == ui->statisticsTab) {
            name = "Statistics";
            datestr = QDate::currentDate().toString(Qt::ISODate);
        } else if (ui->tabWidget->currentWidget() == ui->helpTab) {
            name = "Help";
            datestr = QDateTime::currentDateTime().toString(Qt::ISODate);
        } else { name = "Unknown"; }

        QString filename = PREF.Get("{home}/" + name + "_" + p_profile->user->userName() + "_" + datestr + ".pdf");

        printer.setOutputFileName(filename);
#endif
        printer.setPrintRange(QPrinter::AllPages);
        printer.setOrientation(QPrinter::Portrait);
        printer.setFullPage(false); // This has nothing to do with scaling
        printer.setNumCopies(1);
        printer.setPageMargins(5, 5, 5, 5, QPrinter::Millimeter);
        QPrintDialog pdlg(&printer, this);

        if (pdlg.exec() == QPrintDialog::Accepted) {

            if (ui->tabWidget->currentWidget() == ui->statisticsTab) {
                ui->statisticsView->print(&printer);
            } else if (ui->tabWidget->currentWidget() == ui->helpTab) {
                ui->webView->print(&printer);
            }

        }

        //QMessageBox::information(this,tr("Not supported Yet"),tr("Sorry, printing from this page is not supported yet"),QMessageBox::Ok);
    }
}

void MainWindow::on_action_Edit_Profile_triggered()
{
    NewProfile *newprof = new NewProfile(this);
    QString name =PREF[STR_GEN_Profile].toString();
    newprof->edit(name);
    newprof->setWindowModality(Qt::ApplicationModal);
    newprof->setModal(true);
    newprof->exec();
    qDebug()  << newprof;
    delete newprof;
}

void MainWindow::on_action_CycleTabs_triggered()
{
    int i;
    qDebug() << "Switching Tabs";
    i = ui->tabWidget->currentIndex() + 1;

    if (i >= ui->tabWidget->count()) {
        i = 0;
    }

    ui->tabWidget->setCurrentIndex(i);
}

void MainWindow::on_actionOnline_Users_Guide_triggered()
{
    ui->webView->load(
        QUrl("http://sleepyhead.sourceforge.net/wiki/index.php?title=SleepyHead_Users_Guide"));
    ui->tabWidget->setCurrentWidget(ui->helpTab);
}

void MainWindow::on_action_Frequently_Asked_Questions_triggered()
{
    ui->webView->load(
        QUrl("http://sleepyhead.sourceforge.net/wiki/index.php?title=Frequently_Asked_Questions"));
    ui->tabWidget->setCurrentWidget(ui->helpTab);
}

void packEventList(EventList *el, EventDataType minval = 0)
{
    if (el->count() < 2) { return; }

    EventList nel(EVL_Waveform);
    EventDataType t = 0, lastt = 0; //el->data(0);
    qint64 ti = 0; //=el->time(0);
    //nel.AddEvent(ti,lastt);
    bool f = false;
    qint64 lasttime = 0;
    EventDataType min = 999, max = 0;


    for (quint32 i = 0; i < el->count(); i++) {
        t = el->data(i);
        ti = el->time(i);
        f = false;

        if (t > minval) {
            if (t != lastt) {
                if (!lasttime) {
                    nel.setFirst(ti);
                }

                nel.AddEvent(ti, t);

                if (t < min) { min = t; }

                if (t > max) { max = t; }

                lasttime = ti;
                f = true;
            }
        } else {
            if (lastt > minval) {
                nel.AddEvent(ti, lastt);
                lasttime = ti;
                f = true;
            }
        }

        lastt = t;
    }

    if (!f) {
        if (t > minval) {
            nel.AddEvent(ti, t);
            lasttime = ti;
        }
    }

    el->setFirst(nel.first());
    el->setLast(nel.last());
    el->setMin(min);
    el->setMax(max);

    el->getData().clear();
    el->getTime().clear();
    el->setCount(nel.count());

    el->getData() = nel.getData();
    el->getTime() = nel.getTime();
}

void MainWindow::on_action_Rebuild_Oximetry_Index_triggered()
{
    QVector<ChannelID> valid;
    valid.push_back(OXI_Pulse);
    valid.push_back(OXI_SPO2);
    valid.push_back(OXI_Plethy);
    //valid.push_back(OXI_PulseChange); // Delete these and recalculate..
    //valid.push_back(OXI_SPO2Drop);

    QVector<ChannelID> invalid;

    QList<Machine *> machines = p_profile->GetMachines(MT_OXIMETER);

    qint64 f = 0, l = 0;

    int discard_threshold = p_profile->oxi->oxiDiscardThreshold();
    Machine *m;

    for (int z = 0; z < machines.size(); z++) {
        m = machines.at(z);
        //m->sessionlist.erase(m->sessionlist.find(0));

        // For each Session
        for (QHash<SessionID, Session *>::iterator s = m->sessionlist.begin(); s != m->sessionlist.end();
                s++) {
            Session *sess = s.value();

            if (!sess) { continue; }

            sess->OpenEvents();

            // For each EventList contained in session
            invalid.clear();
            f = 0;
            l = 0;

            for (QHash<ChannelID, QVector<EventList *> >::iterator e = sess->eventlist.begin();
                    e != sess->eventlist.end(); e++) {

                // Discard any non data events.
                if (!valid.contains(e.key())) {
                    // delete and push aside for later to clean up
                    for (int i = 0; i < e.value().size(); i++)  {
                        delete e.value()[i];
                    }

                    e.value().clear();
                    invalid.push_back(e.key());
                } else {
                    // Valid event


                    //                    // Clean up outliers at start of eventlist chunks
                    //                    EventDataType baseline=sess->wavg(OXI_SPO2);
                    //                    if (e.key()==OXI_SPO2) {
                    //                        const int o2start_threshold=10000; // seconds since start of event

                    //                        EventDataType zz;
                    //                        int ii;

                    //                        // Trash suspect outliers in the first o2start_threshold milliseconds
                    //                        for (int j=0;j<e.value().size();j++) {
                    //                            EventList *ev=e.value()[j];
                    //                            if ((ev->count() <= (unsigned)discard_threshold))
                    //                                continue;

                    //                            qint64 ti=ev->time(0);

                    //                            zz=-1;
                    //                            // Peek o2start_threshold ms ahead and grab the value
                    //                            for (ii=0;ii<ev->count();ii++) {
                    //                                if (((ev->time(ii)-ti) > o2start_threshold)) {
                    //                                    zz=ev->data(ii);
                    //                                    break;
                    //                                }
                    //                            }
                    //                            if (zz<0)
                    //                                continue;
                    //                            // Trash any suspect outliers
                    //                            for (int i=0;i<ii;i++) {
                    //                                if (ev->data(i) < baseline) { //(zz-10)) {

                    //                                    ev->getData()[i]=0;
                    //                                }
                    //                            }
                    //                        }
                    //                    }
                    QVector<EventList *> newlist;

                    for (int i = 0; i < e.value().size(); i++)  {
                        if (e.value()[i]->count() > (unsigned)discard_threshold) {
                            newlist.push_back(e.value()[i]);
                        } else {
                            delete e.value()[i];
                        }
                    }

                    for (int i = 0; i < newlist.size(); i++) {
                        packEventList(newlist[i], 8);

                        EventList *el = newlist[i];

                        if (!f || f > el->first()) { f = el->first(); }

                        if (!l || l < el->last()) { l = el->last(); }
                    }

                    e.value() = newlist;
                }
            }

            for (int i = 0; i < invalid.size(); i++) {
                sess->eventlist.erase(sess->eventlist.find(invalid[i]));
            }

            if (f) { sess->really_set_first(f); }

            if (l) { sess->really_set_last(l); }

            sess->m_cnt.clear();
            sess->m_sum.clear();
            sess->m_min.clear();
            sess->m_max.clear();
            sess->m_cph.clear();
            sess->m_sph.clear();
            sess->m_avg.clear();
            sess->m_wavg.clear();
            sess->m_valuesummary.clear();
            sess->m_timesummary.clear();
            sess->m_firstchan.clear();
            sess->m_lastchan.clear();
            sess->SetChanged(true);
        }

    }

    for (int i = 0; i < machines.size(); i++) {
        Machine *m = machines[i];
        m->Save();
        m->SaveSummary();
    }

    daily->LoadDate(getDaily()->getDate());
    overview->ReloadGraphs();
}

void MainWindow::RestartApplication(bool force_login, QString cmdline)
{
    p_profile->removeLock();

    QString apppath;
#ifdef Q_OS_MAC
    // In Mac OS the full path of aplication binary is:
    //    <base-path>/myApp.app/Contents/MacOS/myApp

    // prune the extra bits to just get the app bundle path
    apppath = QApplication::instance()->applicationDirPath().section("/", 0, -3);

    QStringList args;
    args << "-n"; // -n option is important, as it opens a new process
    args << apppath;

    args << "--args"; // SleepyHead binary options after this
    args << "-p"; // -p starts with 1 second delay, to give this process time to save..


    if (force_login) { args << "-l"; }

    args << cmdline;

    if (QProcess::startDetached("/usr/bin/open", args)) {
        QApplication::instance()->exit();
    } else { QMessageBox::warning(nullptr, tr("Gah!"), tr("If you can read this, the restart command didn't work. Your going to have to do it yourself manually."), QMessageBox::Ok); }

#else
    apppath = QApplication::instance()->applicationFilePath();

    // If this doesn't work on windoze, try uncommenting this method
    // Technically should be the same thing..

    //if (QDesktopServices::openUrl(apppath)) {
    //    QApplication::instance()->exit();
    //} else
    QStringList args;
    args << "-p";

    if (force_login) { args << "-l"; }

    args << cmdline;

    //if (change_datafolder) { args << "-d"; }

    if (QProcess::startDetached(apppath, args)) {
        ::exit(0);
        //QApplication::instance()->exit();
    } else { QMessageBox::warning(nullptr, tr("Gah!"), tr("If you can read this, the restart command didn't work. Your going to have to do it yourself manually."), QMessageBox::Ok); }

#endif
}

void MainWindow::on_actionChange_User_triggered()
{
    p_profile->Save();
    PREF.Save();

    RestartApplication(true);
}

void MainWindow::on_actionPurge_Current_Day_triggered()
{
    QDate date = getDaily()->getDate();
    getDaily()->Unload(date);
    Day *day = p_profile->GetDay(date, MT_CPAP);
    Machine *cpap = nullptr;
    if (day) cpap = day->machine(MT_CPAP);

    if (cpap) {
        QList<Session *>::iterator s;

        QList<Session *> list;
        QList<SessionID> sidlist;
        for (s = day->begin(); s != day->end(); ++s) {
            list.push_back(*s);
            sidlist.push_back((*s)->session());
        }

        QHash<QString, SessionID> skipfiles;
        // Read the already imported file list

        QFile impfile(cpap->getDataPath()+"/imported_files.csv");
        if (impfile.exists()) {
            if (impfile.open(QFile::ReadOnly)) {
                QTextStream impstream(&impfile);
                QString serial;
                impstream >> serial;
                if (cpap->serial() == serial) {
                    QString line, file, str;
                    SessionID sid;
                    bool ok;
                    do {
                        line = impstream.readLine();
                        file = line.section(',',0,0);
                        str = line.section(',',1);
                        sid = str.toInt(&ok);
                        if (!sidlist.contains(sid)) {
                            skipfiles[file] = sid;
                        }
                    } while (!impstream.atEnd());
                }
            }
            impfile.close();
            // Delete the file
            impfile.remove();

            // Rewrite the file without the sessions being removed.
            if (impfile.open(QFile::WriteOnly)) {
                QTextStream out(&impfile);
                out << cpap->serial();
                QHash<QString, SessionID>::iterator skit;
                QHash<QString, SessionID>::iterator skit_end = skipfiles.end();
                for (skit = skipfiles.begin(); skit != skit_end; ++skit) {
                    QString a = QString("%1,%2\n").arg(skit.key()).arg(skit.value());;
                    out << a;
                }
                out.flush();
            }
            impfile.close();
        }


//        m->day.erase(m->day.find(date));

        for (int i = 0; i < list.size(); i++) {
            Session *sess = list.at(i);
            sess->Destroy();
            delete sess;
        }
    }
    day = p_profile->GetDay(date, MT_CPAP);

    getDaily()->clearLastDay();
    getDaily()->LoadDate(date);
}

void MainWindow::on_actionRebuildCPAP(QAction *action)
{
    QString data = action->data().toString();
    QString cls = data.section(":",0,0);
    QString serial = data.section(":", 1);
    QList<Machine *> machines = p_profile->GetMachines(MT_CPAP);
    Machine * mach = nullptr;
    for (int i=0; i < machines.size(); ++i) {
        Machine * m = machines.at(i);
        if ((m->loaderName() == cls) && (m->serial() == serial)) {
            mach = m;
            break;
        }
    }
    if (!mach) return;
    QString bpath = mach->getBackupPath();
    bool backups = (dirCount(bpath) > 0) ? true : false;

    if (backups) {
        if (QMessageBox::question(this,
                              STR_MessageBox_Question,
                              tr("Are you sure you want to rebuild all CPAP data for the following machine:")+ "\n\n" +
                              mach->brand() + " " + mach->model() + " " +
                              mach->modelnumber() + " (" + mach->serial() + ")" + "\n\n"+
                              tr("Please note, that this could result in loss of graph data if SleepyHead's internal backups have been disabled or interfered with in any way."),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::No) {
            return;
        }
    } else {
        if (QMessageBox::question(this,
                              STR_MessageBox_Warning,
                              "<p><b>"+STR_MessageBox_Warning+": </b>"+tr("For some reason, SleepyHead does not have internal backups for the following machine:")+ "</p>" +
                              "<p>"+mach->brand() + " " + mach->model() + " " +
                              mach->modelnumber() + " (" + mach->serial() + ")" + "</p>"+
                              "<p>"+tr("Provided you have made <i>your <b>own</b> backups for ALL of your CPAP data</i>, you can still complete this operation, but you will have to restore from your backups manually.")+"</p>"
                              "<p><b>"+tr("Are you really sure you want to do this?")+"</b></p>",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::No) {
            return;
        }
    }

    purgeMachine(mach);

    if (backups) {
        importCPAP(ImportPath(mach->getBackupPath(), lookupLoader(mach)), tr("Please wait, importing from backup folder(s)..."));
    } else {
        if (QMessageBox::information(this, STR_MessageBox_Warning,
                                 tr("Because there are no internal backups to rebuild from, you will have to restore from your own.")+"\n\n"+
                                 tr("Would you like to import from your own backups now? (you will have no data visible for this machine until you do)"),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
            on_action_Import_Data_triggered();
        } else {
        }
    }
    if (overview) overview->ReloadGraphs();
    if (daily) {
        daily->Unload();
        daily->clearLastDay(); // otherwise Daily will crash
        daily->ReloadGraphs();
    }
    GenerateStatistics();
    p_profile->StoreMachines();
}

void MainWindow::on_actionPurgeMachine(QAction *action)
{
    QString data = action->data().toString();
    QString cls = data.section(":",0,0);
    QString serial = data.section(":", 1);
    QList<Machine *> machines = p_profile->GetMachines(MT_CPAP);
    Machine * mach = nullptr;
    for (int i=0; i < machines.size(); ++i) {
        Machine * m = machines.at(i);
        if ((m->loaderName() == cls) && (m->serial() == serial)) {
            mach = m;
            break;
        }
    }
    if (!mach) return;

    if (QMessageBox::question(this, STR_MessageBox_Warning, "<p><b>"+STR_MessageBox_Warning+":</b> "+tr("You are about to <font size=+2>obliterate</font> SleepyHead's machine database for the following machine:")+"</p>"+
                              "<p>"+mach->brand() + " " + mach->model() + " " +
                              mach->modelnumber() + " (" + mach->serial() + ")" + "</p>"+
                              "<p>"+tr("Note as a precaution, the backup folder will be left in place.")+"</p>"+
                              "<p>"+tr("Are you <b>absolutely sure</b> you want to proceed?")+"</p>", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {

        purgeMachine(mach);

        p_profile->DelMachine(mach);
        delete mach;
        PopulatePurgeMenu();

    }

}

void MainWindow::purgeMachine(Machine * mach)
{
    // detect backups
    daily->Unload(daily->getDate());

    // Technicially the above won't sessions under short session limit.. Using Purge to clean up the rest.
    if (mach->Purge(3478216)) {
        mach->sessionlist.clear();
        mach->day.clear();
    } else {
        QMessageBox::warning(this, STR_MessageBox_Error,
                             tr("A file permission error or simillar screwed up the purge process, you will have to delete the following folder manually:")
                             +"\n\n"+
                             QDir::toNativeSeparators(mach->getDataPath()), QMessageBox::Ok, QMessageBox::Ok);
        if (overview) overview->ReloadGraphs();

        if (daily) {
            daily->clearLastDay(); // otherwise Daily will crash
            daily->ReloadGraphs();
        }
        //GenerateStatistics();
        return;
    }

    if (overview) overview->ReloadGraphs();
    QFile rxcache(p_profile->Get("{" + STR_GEN_DataFolder + "}/RXChanges.cache" ));
    rxcache.remove();

    if (daily) {
        daily->clearLastDay(); // otherwise Daily will crash
        daily->ReloadGraphs();
    }
    QApplication::processEvents();


//    GenerateStatistics();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event)
    //qDebug() << "Keypress:" << event->key();
}

void MainWindow::on_action_Sidebar_Toggle_toggled(bool visible)
{
    ui->toolBox->setVisible(visible);
    p_profile->appearance->setRightSidebarVisible(visible);
}

void MainWindow::on_recordsBox_linkClicked(const QUrl &linkurl)
{
    QString link = linkurl.toString().section("=", 0, 0).toLower();
    QString data = linkurl.toString().section("=", 1).toLower();
    qDebug() << linkurl.toString() << link << data;

    if (link == "daily") {
        QDate date = QDate::fromString(data, Qt::ISODate);
        ui->tabWidget->setCurrentWidget(daily);
        QApplication::processEvents();
        daily->LoadDate(date);
    } else if (link == "overview") {
        QString date1 = data.section(",", 0, 0);
        QString date2 = data.section(",", 1);

        QDate d1 = QDate::fromString(date1, Qt::ISODate);
        QDate d2 = QDate::fromString(date2, Qt::ISODate);
        overview->setRange(d1, d2);
        ui->tabWidget->setCurrentWidget(overview);
    } else if (link == "import") {
        if (data == "cpap") on_importButton_clicked();
        if (data == "oximeter") on_oximetryButton_clicked();
    } else if (link == "statistics") {
        ui->tabWidget->setCurrentWidget(ui->statisticsTab);
    }
}

void MainWindow::on_helpButton_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->helpTab);
}

void MainWindow::on_actionView_Statistics_triggered()
{
    ui->tabWidget->setCurrentWidget(ui->statisticsTab);
}

void MainWindow::on_webView_linkClicked(const QUrl &url)
{
    QString s = url.toString();
    qDebug() << "Link Clicked" << url;

    if (s.toLower().startsWith("https:")) {
        QDesktopServices().openUrl(url);
    } else {
        ui->webView->setUrl(url);
    }
}

void MainWindow::on_webView_statusBarMessage(const QString &text)
{
    ui->statusbar->showMessage(text);
}

void MainWindow::LinkHovered(const QString &link, const QString &title, const QString &textContent)
{
    Q_UNUSED(title);
    Q_UNUSED(textContent);
    ui->statusbar->showMessage(link);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index);
   // QWidget *widget = ui->tabWidget->currentWidget();
}


void MainWindow::on_bookmarkView_linkClicked(const QUrl &arg1)
{
    on_recordsBox_linkClicked(arg1);
}

void MainWindow::on_filterBookmarks_editingFinished()
{
    bookmarkFilter = ui->filterBookmarks->text();
    updateFavourites();
}

void MainWindow::on_filterBookmarksButton_clicked()
{
    if (!bookmarkFilter.isEmpty()) {
        ui->filterBookmarks->setText("");
        bookmarkFilter = "";
        updateFavourites();
    }
}

void MainWindow::reprocessEvents(bool restart)
{
    m_restartRequired = restart;
    QTimer::singleShot(0, this, SLOT(doReprocessEvents()));
}


void MainWindow::FreeSessions()
{
    QDate first = p_profile->FirstDay();
    QDate date = p_profile->LastDay();
    Day *day;
    QDate current = daily->getDate();

    do {
        day = p_profile->GetDay(date, MT_CPAP);

        if (day) {
            if (date != current) {
                day->CloseEvents();
            }
        }

        date = date.addDays(-1);
    }  while (date >= first);
}

void MainWindow::MachineUnsupported(Machine * m)
{
    Q_ASSERT(m != nullptr);
    QMessageBox::information(this, STR_MessageBox_Error, QObject::tr("Sorry, your %1 %2 machine is not currently supported.").arg(m->brand()).arg(m->model()), QMessageBox::Ok);
}

void MainWindow::doReprocessEvents()
{
    if (p_profile->countDays(MT_CPAP, p_profile->FirstDay(), p_profile->LastDay()) == 0) {
        return;
    }

    m_inRecalculation = true;
    QDate first = p_profile->FirstDay();
    QDate date = p_profile->LastDay();
    Session *sess;
    Day *day;
    //FlowParser flowparser;

    mainwin->Notify(tr("Performance will be degraded during these recalculations."),
                    tr("Recalculating Indices"));

    // For each day in history
    int daycount = first.daysTo(date);
    int idx = 0;

    QList<Machine *> machines = p_profile->GetMachines(MT_CPAP);

    // Disabling multithreaded save as it appears it's causing problems
    bool cache_sessions = false; //p_profile->session->cacheSessions();

    if (cache_sessions) { // Use multithreaded save to handle reindexing.. (hogs memory like hell)
        qstatus->setText(tr("Loading Event Data"));
    } else {
        qstatus->setText(tr("Recalculating Summaries"));
    }

    if (qprogress) {
        qprogress->setValue(0);
        qprogress->setVisible(true);
    }

    bool isopen;

    do {
        day = p_profile->GetDay(date, MT_CPAP);

        if (day) {
            for (int i = 0; i < day->size(); i++) {
                sess = (*day)[i];
                isopen = sess->eventsLoaded();

                // Load the events if they aren't loaded already
                sess->OpenEvents();

                //if (!sess->channelDataExists(CPAP_FlowRate)) continue;

                //QVector<EventList *> & flowlist=sess->eventlist[CPAP_FlowRate];

                // Destroy any current user flags..
                sess->destroyEvent(CPAP_UserFlag1);
                sess->destroyEvent(CPAP_UserFlag2);
                sess->destroyEvent(CPAP_UserFlag3);

                // AHI flags
                sess->destroyEvent(CPAP_AHI);
                sess->destroyEvent(CPAP_RDI);

                if (sess->machine()->loaderName() != STR_MACH_PRS1) {
                    sess->destroyEvent(CPAP_LargeLeak);
                }

                sess->SetChanged(true);

                if (!cache_sessions) {
                    sess->UpdateSummaries();
                    sess->machine()->SaveSession(sess);

                    if (!isopen) {
                        sess->TrashEvents();
                    }
                }
            }
        }

        date = date.addDays(-1);
        // if (qprogress && (++idx % 10) ==0) {
        qprogress->setValue(0 + (float(++idx) / float(daycount) * 100.0));
        QApplication::processEvents();
        // }

    } while (date >= first);

    if (cache_sessions) {
        qstatus->setText(tr("Recalculating Summaries"));

        for (int i = 0; i < machines.size(); i++) {
            machines.at(i)->Save();
        }
    }

    qstatus->setText(tr(""));
    qprogress->setVisible(false);
    m_inRecalculation = false;

    if (m_restartRequired) {
        QMessageBox::information(this, tr("Restart Required"),
                                 tr("Recalculations are complete, the application now needs to restart to display the changes."),
                                 QMessageBox::Ok);
        RestartApplication();
        return;
    } else {
        Notify(tr("Recalculations are now complete."), tr("Task Completed"));

        FreeSessions();
        QDate current = daily->getDate();
        daily->LoadDate(current);

        if (overview) { overview->ReloadGraphs(); }
    }
}

void MainWindow::on_actionImport_ZEO_Data_triggered()
{
    QFileDialog w;
    w.setFileMode(QFileDialog::ExistingFiles);
    w.setOption(QFileDialog::ShowDirsOnly, false);
    w.setOption(QFileDialog::DontUseNativeDialog, true);
    w.setNameFilters(QStringList("Zeo CSV File (*.csv)"));

    ZEOLoader zeo;

    if (w.exec() == QFileDialog::Accepted) {
        QString filename = w.selectedFiles()[0];

        if (!zeo.OpenFile(filename)) {
            Notify(tr("There was a problem opening ZEO File: ") + filename);
            return;
        }

        Notify(tr("Zeo CSV Import complete"));
        daily->LoadDate(daily->getDate());
    }


}

void MainWindow::on_actionImport_RemStar_MSeries_Data_triggered()
{
#ifdef REMSTAR_M_SUPPORT
    QFileDialog w;
    w.setFileMode(QFileDialog::ExistingFiles);
    w.setOption(QFileDialog::ShowDirsOnly, false);
    w.setOption(QFileDialog::DontUseNativeDialog, true);
    w.setNameFilters(QStringList("M-Series data file (*.bin)"));

    MSeriesLoader mseries;

    if (w.exec() == QFileDialog::Accepted) {
        QString filename = w.selectedFiles()[0];

        if (!mseries.Open(filename, p_profile)) {
            Notify(tr("There was a problem opening MSeries block File: ") + filename);
            return;
        }

        Notify(tr("MSeries Import complete"));
        daily->LoadDate(daily->getDate());
    }

#endif
}

void MainWindow::on_actionSleep_Disorder_Terms_Glossary_triggered()
{
    ui->webView->load(
        QUrl("http://sleepyhead.sourceforge.net/wiki/index.php?title=Glossary"));
    ui->tabWidget->setCurrentWidget(ui->helpTab);
}

void MainWindow::on_actionHelp_Support_SleepyHead_Development_triggered()
{
    QUrl url =
        QUrl("http://sleepyhead.sourceforge.net/wiki/index.php?title=Support_SleepyHead_Development");
    QDesktopServices().openUrl(url);
    //    ui->webView->load(url);
    //    ui->tabWidget->setCurrentWidget(ui->helpTab);
}

void MainWindow::on_actionChange_Language_triggered()
{
    //QSettings *settings = new QSettings(getDeveloperName(), getAppName());
    //settings->remove("Settings/Language");
    //delete settings;
    p_profile->Save();
    PREF.Save();

    RestartApplication(true, "-language");
}

void MainWindow::on_actionChange_Data_Folder_triggered()
{
    p_profile->Save();
    PREF.Save();
    p_profile->removeLock();

    RestartApplication(false, "-d");
}

void MainWindow::on_actionImport_Somnopose_Data_triggered()
{
    QFileDialog w;
    w.setFileMode(QFileDialog::ExistingFiles);
    w.setOption(QFileDialog::ShowDirsOnly, false);
    w.setOption(QFileDialog::DontUseNativeDialog, true);
    w.setNameFilters(QStringList("Somnopause CSV File (*.csv)"));

    SomnoposeLoader somno;

    if (w.exec() == QFileDialog::Accepted) {
        QString filename = w.selectedFiles()[0];

        if (!somno.OpenFile(filename)) {
            Notify(tr("There was a problem opening Somnopose Data File: ") + filename);
            return;
        }

        Notify(tr("Somnopause Data Import complete"));
        daily->LoadDate(daily->getDate());
    }

}

void MainWindow::GenerateStatistics()
{
    QDate first = p_profile->FirstDay();
    QDate last = p_profile->LastDay();
    ui->statStartDate->setMinimumDate(first);
    ui->statStartDate->setMaximumDate(last);

    ui->statEndDate->setMinimumDate(first);
    ui->statEndDate->setMaximumDate(last);

    Statistics stats;
    QString html = stats.GenerateHTML();

    updateFavourites();

    //QWebFrame *frame=ui->statisticsView->page()->currentFrame();
    //frame->addToJavaScriptWindowObject("mainwin",this);
    //ui->statisticsView->setHtml(html);
    MyStatsPage *page = new MyStatsPage(this);
    page->currentFrame()->setHtml(html);
    ui->statisticsView->setPage(page);



    MyStatsPage *page2 = new MyStatsPage(this);
    page2->currentFrame()->setHtml(GenerateWelcomeHTML());
    ui->welcomeView->setPage(page2);

    //    connect(ui->statisticsView->page()->currentFrame(),SIGNAL(javaScriptWindowObjectCleared())
    //    QString file="qrc:/docs/index.html";
    //    QUrl url(file);
    //    ui->webView->setUrl(url);
}


void MainWindow::on_statisticsButton_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->statisticsTab);
}

void MainWindow::on_statisticsView_linkClicked(const QUrl &arg1)
{
    //qDebug() << arg1;
    on_recordsBox_linkClicked(arg1);
}

void MainWindow::on_reportModeMonthly_clicked()
{
    ui->statStartDate->setVisible(false);
    ui->statEndDate->setVisible(false);
    if (p_profile->general->statReportMode() != 1) {
        p_profile->general->setStatReportMode(1);
        GenerateStatistics();
    }
}

void MainWindow::on_reportModeStandard_clicked()
{
    ui->statStartDate->setVisible(false);
    ui->statEndDate->setVisible(false);
    if (p_profile->general->statReportMode() != 0) {
        p_profile->general->setStatReportMode(0);
        GenerateStatistics();
    }
}


void MainWindow::on_reportModeRange_clicked()
{
    ui->statStartDate->setVisible(true);
    ui->statEndDate->setVisible(true);
    if (p_profile->general->statReportMode() != 2) {
        p_profile->general->setStatReportMode(2);
        GenerateStatistics();
    }
}

void MainWindow::on_actionPurgeCurrentDaysOximetry_triggered()
{
    if (!getDaily())
        return;
    QDate date = getDaily()->getDate();
    Day * day = p_profile->GetDay(date, MT_OXIMETER);
    if (day) {
        if (QMessageBox::question(this, STR_MessageBox_Warning,
            tr("Are you sure you want to delete oximetry data for %1").
                arg(getDaily()->getDate().toString(Qt::DefaultLocaleLongDate))+"<br/><br/>"+
            tr("<b>Please be aware you can not undo this operation!</b>"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
            return;
        }

        QList<Session *> sessionlist;
        sessionlist.append(day->sessions);

        for (int i=0; i < sessionlist.size(); ++i) {
            Session * sess = sessionlist.at(i);
            sess->Destroy();
            delete sess;
        }


        daily->clearLastDay(); // otherwise Daily will crash

        getDaily()->ReloadGraphs();
    } else {
        QMessageBox::information(this, STR_MessageBox_Information,
            tr("Select the day with valid oximetry data in daily view first."),QMessageBox::Ok);
    }
}

void MainWindow::on_importButton_clicked()
{
    on_action_Import_Data_triggered();
}


void MainWindow::on_actionToggle_Line_Cursor_toggled(bool b)
{
    p_profile->appearance->setLineCursorMode(b);
    if (ui->tabWidget->currentWidget() == getDaily()) {
        getDaily()->graphView()->timedRedraw(0);
    } else if (ui->tabWidget->currentWidget() == getOverview()) {
        getOverview()->graphView()->timedRedraw(0);
    }
}

void MainWindow::on_actionLeft_Daily_Sidebar_toggled(bool visible)
{
    getDaily()->setSidebarVisible(visible);
}

void MainWindow::on_actionDaily_Calendar_toggled(bool visible)
{
    getDaily()->setCalendarVisible(visible);
}

#include "SleepLib/journal.h"

void MainWindow::on_actionExport_Journal_triggered()
{
    QString folder;
#if QT_VERSION  < QT_VERSION_CHECK(5,0,0)
    folder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#else
    folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif
    folder += QDir::separator() + tr("%1's Journal").arg(p_profile->user->userName()) + ".xml";

    QString filename = QFileDialog::getSaveFileName(this, tr("Choose where to save journal"), folder, tr("XML Files (*.xml)"));

    BackupJournal(filename);
}

void MainWindow::on_actionShow_Performance_Counters_toggled(bool arg1)
{
    p_profile->general->setShowPerformance(arg1);
}

void MainWindow::on_actionExport_CSV_triggered()
{
    ExportCSV ex(this);

    if (ex.exec() == ExportCSV::Accepted) {
    }
}

void MainWindow::on_actionExport_Review_triggered()
{
    QMessageBox::information(nullptr, STR_MessageBox_Information, QObject::tr("Sorry, this feature is not implemented yet"), QMessageBox::Ok);
}

void MainWindow::on_splitter_2_splitterMoved(int, int)
{
    p_profile->appearance->setRightPanelWidth(ui->splitter_2->sizes()[1]);
}
