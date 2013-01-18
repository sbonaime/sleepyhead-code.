/*
 MainWindow Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QGLFormat>
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
#include <cmath>

// Custom loaders that don't autoscan..
#include <SleepLib/loader_plugins/zeo_loader.h>
#include <SleepLib/loader_plugins/mseries_loader.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "newprofile.h"
#include "exportcsv.h"
#include "SleepLib/schema.h"
#include "Graphs/glcommon.h"
#include "UpdaterWindow.h"
#include "SleepLib/calcs.h"
#include "version.h"


QProgressBar *qprogress;
QLabel *qstatus;
QLabel *qstatus2;
QStatusBar *qstatusbar;

extern Profile * profile;

void MainWindow::Log(QString s)
{

    if (!strlock.tryLock())
        return;

//  strlock.lock();
    QString tmp=QString("%1: %2").arg(logtime.elapsed(),5,10,QChar('0')).arg(s);

    logbuffer.append(tmp); //QStringList appears not to be threadsafe
    strlock.unlock();

    strlock.lock();
    // only do this in the main thread?
    for (int i=0;i<logbuffer.size();i++)
        ui->logText->appendPlainText(logbuffer[i]);
    logbuffer.clear();
    strlock.unlock();

    //loglock.unlock();
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    Q_ASSERT(p_profile!=NULL);

    logtime.start();
    ui->setupUi(this);

    QString version=FullVersionString;
    if (QString(GIT_BRANCH)!="master") version+=QString(" ")+QString(GIT_BRANCH);
    this->setWindowTitle(tr("SleepyHead")+QString(" v%1 (Profile: %2)").arg(version).arg(PREF[STR_GEN_Profile].toString()));
    //ui->tabWidget->setCurrentIndex(1);

    // Disable Screenshot on Mac Platform,as it doesn't work, and the system provides this functionality anyway.
#ifdef Q_OS_MAC
    ui->action_Screenshot->setVisible(false);
#endif

    overview=NULL;
    daily=NULL;
    oximetry=NULL;
    prefdialog=NULL;

    m_inRecalculation=false;
    m_restartRequired=false;
    // Initialize Status Bar objects
    qstatusbar=ui->statusbar;
    qprogress=new QProgressBar(this);
    qprogress->setMaximum(100);
    qstatus2=new QLabel("Welcome",this);
    qstatus2->setFrameStyle(QFrame::Raised);
    qstatus2->setFrameShadow(QFrame::Sunken);
    qstatus2->setFrameShape(QFrame::Box);
    //qstatus2->setMinimumWidth(100);
    qstatus2->setMaximumWidth(100);
    qstatus2->setAlignment(Qt::AlignRight |Qt::AlignVCenter);
    qstatus=new QLabel("",this);
    qprogress->hide();
    ui->statusbar->setMinimumWidth(200);
    ui->statusbar->addPermanentWidget(qstatus,0);
    ui->statusbar->addPermanentWidget(qprogress,1);
    ui->statusbar->addPermanentWidget(qstatus2,0);

    ui->actionDebug->setChecked(PROFILE.general->showDebug());

    if (!PROFILE.general->showDebug()) {
        ui->logText->hide();
    }

#ifdef Q_OS_MAC
    PROFILE.appearance->setAntiAliasing(false);
#endif
    ui->action_Link_Graph_Groups->setChecked(PROFILE.general->linkGroups());

    first_load=true;

    // Using the dirty registry here. :(
    QSettings settings("Jedimark", "SleepyHead");

    // Load previous Window geometry
    this->restoreGeometry(settings.value("MainWindow/geometry").toByteArray());

    daily=new Daily(ui->tabWidget,NULL);
    ui->tabWidget->insertTab(1,daily,STR_TR_Daily);


    // Start with the Summary Tab
    ui->tabWidget->setCurrentWidget(ui->summaryTab); // setting this to daily shows the cube during loading..

    // Nifty Notification popups in System Tray (uses Growl on Mac)
    if (QSystemTrayIcon::isSystemTrayAvailable() && QSystemTrayIcon::supportsMessages()) {
        systray=new QSystemTrayIcon(QIcon(":/icons/bob-v3.0.png"),this);
        systray->show();
        systraymenu=new QMenu(this);
        systray->setContextMenu(systraymenu);
        QAction *a=systraymenu->addAction("SleepyHead v"+VersionString);
        a->setEnabled(false);
        systraymenu->addSeparator();
        systraymenu->addAction("&About",this,SLOT(on_action_About_triggered()));
        systraymenu->addAction("Check for &Updates",this,SLOT(on_actionCheck_for_Updates_triggered()));
        systraymenu->addSeparator();
        systraymenu->addAction("E&xit",this,SLOT(close()));
    } else { // if not available, the messages will popup in the taskbar
        systray=NULL;
        systraymenu=NULL;
    }
    ui->toolBox->setCurrentIndex(0);
    daily->graphView()->redraw();

    if (PROFILE.cpap->AHIWindow() < 30.0) {
        PROFILE.cpap->setAHIWindow(60.0);
    }

    ui->recordsBox->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    ui->summaryView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    ui->bookmarkView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    ui->toolBox->setStyleSheet(
                               "QToolBox::tab {"
                               "background: #6789ab;"
                               "color: lightGray;}"
                               "QToolBox { icon-size: 32px; }"
                               "QToolBox::tab:selected {"
                               "font: bold;"
                               "background: #9090ee;"
                               "color: white; }"
                               );

    //"font-weight: bold; "
    //"border-top-left-radius: 8px;"
    //"border-top-right-radius: 8px;"

    QString loadingtxt="<HTML><body style='text-align: center; vertical-align: center'><table width='100%' height='100%'><tr><td align=center><h1>Loading...</h1></td></tr></table></body></HTML>";
    ui->summaryView->setHtml(loadingtxt);
    on_tabWidget_currentChanged(0);
    //ui->actionImport_RemStar_MSeries_Data->setVisible(false);
}
extern MainWindow *mainwin;
MainWindow::~MainWindow()
{
    if (systraymenu) delete systraymenu;
    if (systray) delete systray;

    // Save current window position
        QSettings settings("Jedimark", "SleepyHead");
        settings.setValue("MainWindow/geometry", saveGeometry());

    if (daily) {
        daily->close();
        delete daily;
    }
    if (overview) {
        overview->close();
        delete overview;
    }
    if (oximetry) {
        oximetry->close();
        delete oximetry;
    }

    // Trash anything allocated by the Graph objects
    DoneGraphs();

    // Shutdown and Save the current User profile
    Profiles::Done();
    mainwin=NULL;
    delete ui;
}


void MainWindow::Notify(QString s,QString title,int ms)
{
    if (systray) {
        systray->showMessage(title,s,QSystemTrayIcon::Information,ms);
    } else {
        ui->statusbar->showMessage(s,ms);
    }
}

void MainWindow::Startup()
{
    qDebug() << PREF["AppName"].toString().toLatin1()+" v"+VersionString.toLatin1() << "built with Qt"<< QT_VERSION_STR << "on" << __DATE__ << __TIME__;
    qstatus->setText(tr("Loading Data"));
    qprogress->show();
    //qstatusbar->showMessage(tr("Loading Data"),0);

    // profile is a global variable set in main after login
    PROFILE.LoadMachineData();

    SnapshotGraph=new gGraphView(this,daily->graphView());

    // the following are platform overides for the UsePixmapCache preference settings
#ifdef Q_OS_MAC
    //Mac needs this to be able to offscreen render
    SnapshotGraph->setUsePixmapCache(true);
#else
    //Windows & Linux barfs when offscreen rendering with pixmap cached text
    SnapshotGraph->setUsePixmapCache(false);
#endif

    SnapshotGraph->setFormat(daily->graphView()->format());
    //SnapshotGraph->setMaximumSize(1024,512);
    //SnapshotGraph->setMinimumSize(1024,512);
    SnapshotGraph->hide();

    overview=new Overview(ui->tabWidget,daily->graphView());
    ui->tabWidget->insertTab(2,overview,STR_TR_Overview);
    if (PROFILE.oxi->oximetryEnabled()) {
        oximetry=new Oximetry(ui->tabWidget,daily->graphView());
        ui->tabWidget->insertTab(3,oximetry,STR_TR_Oximetry);
    }


    ui->tabWidget->setCurrentWidget(ui->summaryTab);
    if (daily) daily->ReloadGraphs();
    if (overview) overview->ReloadGraphs();
    qprogress->hide();
    qstatus->setText("");
    on_summaryButton_clicked();

}

void MainWindow::on_action_Import_Data_triggered()
{
    if (m_inRecalculation) {
        Notify("Access to Import has been blocked while recalculations are in progress.");
        return;

    }

    QStringList importLocations;
    {
        QString filename=PROFILE.Get("{DataFolder}/ImportLocations.txt");
        QFile file(filename);
        file.open(QFile::ReadOnly);
        QTextStream textStream(&file);
        while (1) {
            QString line = textStream.readLine();
             if (line.isNull())
                 break;
             else if (line.isEmpty())
                 continue;
             else {
                 importLocations.append(line);
             }
        }
        file.close();
    }

    //bool addnew=false;
    QString newdir;

    bool asknew=false;
    if (importLocations.size()==0) {
        asknew=true;
    } else {
        int res=QMessageBox::question(this,"Import from where?","Do you just want to Import from the usual (remembered) locations?\n","The Usual","New Location","Cancel",0,2);
        if (res==1) {
            asknew=true;
        }
        if (res==2) return;
    }
    if (asknew) {
        //mainwin->Notify("Please remember to point the importer at the root folder or drive letter of your data-card, and not a subfolder.","Import Reminder",8000);
    }

    QStringList importFrom;

    if (asknew) {
        QFileDialog w;
        w.setFileMode(QFileDialog::Directory);
        w.setOption(QFileDialog::ShowDirsOnly, true);

#if defined(Q_OS_MAC) && (QT_VERSION_CHECK(4,8,0) > QT_VERSION)
        // Fix for tetragon, 10.6 barfs up Qt's custom dialog
        w.setOption(QFileDialog::DontUseNativeDialog,true);
#else
        w.setOption(QFileDialog::DontUseNativeDialog,false);

        QListView *l = w.findChild<QListView*>("listView");
        if (l) {
            l->setSelectionMode(QAbstractItemView::MultiSelection);
        }
        QTreeView *t = w.findChild<QTreeView*>();
        if (t) {
            t->setSelectionMode(QAbstractItemView::MultiSelection);
        }
#endif
        if (w.exec()!=QDialog::Accepted) {
            return;
        }
        for (int i=0;i<w.selectedFiles().size();i++) {
            QString newdir=w.selectedFiles().at(i);
            if (!importFrom.contains(newdir)) {
                importFrom.append(newdir);
                //addnew=true;
            }
        }
    } else importFrom=importLocations;

    int successful=false;

    QStringList goodlocations;
    for (int i=0;i<importFrom.size();i++) {
        QString dir=importFrom[i];
        if (!dir.isEmpty()) {
            qprogress->setValue(0);
            qprogress->show();
            qstatus->setText(tr("Importing Data"));
            int c=PROFILE.Import(dir);
            qDebug() << "Finished Importing data" << c;
            if (c) {
                if (!importLocations.contains(dir))
                    goodlocations.push_back(dir);
                successful=true;
            }
            qstatus->setText("");
            qprogress->hide();
        }
    }
    if (successful) {
        PROFILE.Save();
        if (overview) overview->ReloadGraphs();
        on_summaryButton_clicked();
        if (daily) daily->ReloadGraphs();
        if ((goodlocations.size()>0) && (QMessageBox::question(this,"Remember this Location?","Would you like to remember this import location for next time?\n"+newdir,QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes)) {
            for (int i=0;i<goodlocations.size();i++) {
                importLocations.push_back(goodlocations[i]);
            }
            QString filename=PROFILE.Get("{DataFolder}/ImportLocations.txt");
            QFile file(filename);
            file.open(QFile::WriteOnly);
            QTextStream ts(&file);
            for (int i=0;i<importLocations.size();i++) {
                ts << importLocations[i] << endl;
               //file.write(importLocations[i].toUtf8());
            }
            file.close();
        }
    } else {
        mainwin->Notify("Import Problem\n\nCouldn't find any new Machine Data at the locations given");
    }
}
QMenu * MainWindow::CreateMenu(QString title)
{
    QMenu *menu=new QMenu(title,ui->menubar);
    ui->menubar->insertMenu(ui->menu_Help->menuAction(),menu);
    return menu;
}

void MainWindow::on_action_Fullscreen_triggered()
{
    if (ui->action_Fullscreen->isChecked())
        this->showFullScreen();
    else
        this->showNormal();
}

QString htmlHeader()
{

    //   "a:link,a:visited { color: '#000020'; text-decoration: none; font-weight: bold;}"
//   "a:hover { background-color: inherit; color: red; text-decoration:none; font-weight: bold; }"
    return QString("<html><head>"
"</head>"
"<style type='text/css'>"
"<!--h1,p,a,td,body { font-family: 'FreeSans', 'Sans Serif' } --/>"
"p,a,td,body { font-size: 14px }"
"</style>"
"<link rel='stylesheet' type='text/css' href='qrc:/docs/tooltips.css' />"
"<script type='text/javascript'>"
"function ChangeColor(tableRow, highLight)"
"{ tableRow.style.backgroundColor = highLight; }"
"function Go(url) { throw(url); }"
"</script>"
"</head>"
"<body leftmargin=0 topmargin=0 rightmargin=0>"
"<div align=center><table cellpadding=3 cellspacing=0 border=0>"
"<tr><td><img src='qrc:/icons/bob-v3.0.png' width=100px height=100px><td valign=center align=center><h1>SleepyHead v"+VersionString+"</h1><i>This is a beta software and some functionality may not work as intended yet.<br/>Please report any bugs you find to SleepyHead's SourceForge page.</i></td></tr></table>"
"</div>"
"<hr/>");
}
QString htmlFooter()
{
    return QString("</body></html>");
}

EventDataType calcAHI(QDate start, QDate end)
{
    EventDataType val=(p_profile->calcCount(CPAP_Obstructive,MT_CPAP,start,end)
              +p_profile->calcCount(CPAP_Hypopnea,MT_CPAP,start,end)
              +p_profile->calcCount(CPAP_ClearAirway,MT_CPAP,start,end)
              +p_profile->calcCount(CPAP_Apnea,MT_CPAP,start,end));
    if (PROFILE.general->calculateRDI())
        val+=p_profile->calcCount(CPAP_RERA,MT_CPAP,start,end);
    EventDataType hours=p_profile->calcHours(MT_CPAP,start,end);

    if (hours>0)
        val/=hours;
    else
        val=0;

    return val;
}

EventDataType calcFL(QDate start, QDate end)
{
    EventDataType val=(p_profile->calcCount(CPAP_FlowLimit,MT_CPAP,start,end));
    EventDataType hours=p_profile->calcHours(MT_CPAP,start,end);

    if (hours>0)
        val/=hours;
    else
        val=0;

    return val;
}


struct RXChange
{
    RXChange() { highlight=0; machine=NULL; }
    RXChange(const RXChange & copy) {
        first=copy.first;
        last=copy.last;
        days=copy.days;
        ahi=copy.ahi;
	fl=copy.fl;
        mode=copy.mode;
        min=copy.min;
        max=copy.max;
        maxhi=copy.maxhi;
        machine=copy.machine;
        per1=copy.per1;
        per2=copy.per2;
        highlight=copy.highlight;
        weighted=copy.weighted;
        prelief=copy.prelief;
        prelset=copy.prelset;
    }
    QDate first;
    QDate last;
    int days;
    EventDataType ahi;
    EventDataType fl;
    CPAPMode mode;
    EventDataType min;
    EventDataType max;
    EventDataType maxhi;
    EventDataType per1;
    EventDataType per2;
    EventDataType weighted;
    PRTypes prelief;
    Machine * machine;
    short prelset;
    short highlight;
};

enum RXSortMode { RX_first, RX_last, RX_days, RX_ahi, RX_mode, RX_min, RX_max, RX_maxhi, RX_per1, RX_per2, RX_weighted };
RXSortMode RXsort=RX_first;
bool RXorder=false;

bool operator<(const RXChange & c1, const RXChange & c2) {
    const RXChange * comp1=&c1;
    const RXChange * comp2=&c2;
    if (RXorder) {
        switch (RXsort) {
        case RX_ahi: return comp1->ahi < comp2->ahi;
        case RX_days: return comp1->days < comp2->days;
        case RX_first: return comp1->first < comp2->first;
        case RX_last: return comp1->last < comp2->last;
        case RX_mode: return comp1->mode < comp2->mode;
        case RX_min:  return comp1->min < comp2->min;
        case RX_max:  return comp1->max < comp2->max;
        case RX_maxhi: return comp1->maxhi < comp2->maxhi;
        case RX_per1:  return comp1->per1 < comp2->per1;
        case RX_per2:  return comp1->per2 < comp2->per2;
        case RX_weighted:  return comp1->weighted < comp2->weighted;
        };
    } else {
        switch (RXsort) {
        case RX_ahi: return comp1->ahi > comp2->ahi;
        case RX_days: return comp1->days > comp2->days;
        case RX_first: return comp1->first > comp2->first;
        case RX_last: return comp1->last > comp2->last;
        case RX_mode: return comp1->mode > comp2->mode;
        case RX_min:  return comp1->min > comp2->min;
        case RX_max:  return comp1->max > comp2->max;
        case RX_maxhi: return comp1->maxhi > comp2->maxhi;
        case RX_per1:  return comp1->per1 > comp2->per1;
        case RX_per2:  return comp1->per2 > comp2->per2;
        case RX_weighted:  return comp1->weighted > comp2->weighted;
        };
    }
    return true;
}

bool RXSort(const RXChange * comp1, const RXChange * comp2) {
    if (RXorder) {
        switch (RXsort) {
        case RX_ahi: return comp1->ahi < comp2->ahi;
        case RX_days: return comp1->days < comp2->days;
        case RX_first: return comp1->first < comp2->first;
        case RX_last: return comp1->last < comp2->last;
        case RX_mode: return comp1->mode < comp2->mode;
        case RX_min:  return comp1->min < comp2->min;
        case RX_max:  return comp1->max < comp2->max;
        case RX_maxhi: return comp1->maxhi < comp2->maxhi;
        case RX_per1:  return comp1->per1 < comp2->per1;
        case RX_per2:  return comp1->per2 < comp2->per2;
        case RX_weighted:  return comp1->weighted < comp2->weighted;
        };
    } else {
        switch (RXsort) {
        case RX_ahi: return comp1->ahi > comp2->ahi;
        case RX_days: return comp1->days > comp2->days;
        case RX_first: return comp1->first > comp2->first;
        case RX_last: return comp1->last > comp2->last;
        case RX_mode: return comp1->mode > comp2->mode;
        case RX_min:  return comp1->min > comp2->min;
        case RX_max:  return comp1->max > comp2->max;
        case RX_maxhi: return comp1->maxhi > comp2->maxhi;
        case RX_per1:  return comp1->per1 > comp2->per1;
        case RX_per2:  return comp1->per2 > comp2->per2;
        case RX_weighted:  return comp1->weighted > comp2->weighted;
        };
    }
    return true;
}
struct UsageData {
    UsageData() { ahi=0; hours=0; }
    UsageData(QDate d, EventDataType v, EventDataType h) { date=d; ahi=v; hours=h; }
    UsageData(const UsageData & copy) { date=copy.date; ahi=copy.ahi; hours=copy.hours; }
    QDate date;
    EventDataType ahi;
    EventDataType hours;
};
bool operator <(const UsageData & c1, const UsageData & c2)
{
    if (c1.ahi < c2.ahi)
        return true;
    if ((c1.ahi == c2.ahi) && (c1.date > c2.date)) return true;
    return false;
    //return c1.value < c2.value;
}

class MyStatsPage:public QWebPage
{
public:
    MyStatsPage(QObject *parent);
    virtual ~MyStatsPage();
protected:
    //virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID);
    virtual void javaScriptAlert ( QWebFrame * frame, const QString & msg );
};
MyStatsPage::MyStatsPage(QObject *parent)
:QWebPage(parent)
{
}
MyStatsPage::~MyStatsPage()
{
}

void MyStatsPage::javaScriptAlert(QWebFrame * frame, const QString & msg)
{
    Q_UNUSED(frame);
    mainwin->sendStatsUrl(msg);
}


//bool operator<(const RXChange * & comp1, const RXChange * & comp2) {

//}

void MainWindow::on_homeButton_clicked()
{
    ui->webView->setUrl(QUrl("qrc:/docs/index.html"));
}

QString formatTime(float time)
{
    int hours=time;
    int seconds=time*3600.0;
    int minutes=(seconds / 60) % 60;
    seconds %= 60;
    return QString().sprintf("%02i:%02i",hours,minutes); //,seconds);
}


void MainWindow::on_summaryButton_clicked()
{
    QString html=htmlHeader();

    QDate lastcpap=p_profile->LastGoodDay(MT_CPAP);
    QDate firstcpap=p_profile->FirstGoodDay(MT_CPAP);
    QDate cpapweek=lastcpap.addDays(-7);
    QDate cpapmonth=lastcpap.addDays(-30);
    QDate cpap6month=lastcpap.addMonths(-6);
    QDate cpapyear=lastcpap.addYears(-12);
    if (cpapweek<firstcpap) cpapweek=firstcpap;
    if (cpapmonth<firstcpap) cpapmonth=firstcpap;
    if (cpap6month<firstcpap) cpap6month=firstcpap;
    if (cpapyear<firstcpap) cpapyear=firstcpap;


    QList<Machine *> cpap_machines=PROFILE.GetMachines(MT_CPAP);
    QList<Machine *> oximeters=PROFILE.GetMachines(MT_OXIMETER);
    QList<Machine *> mach;
    mach.append(cpap_machines);
    mach.append(oximeters);


    if (mach.size()==0) {
        html+="<table cellpadding=2 cellspacing=0 border=0 width=100% height=60%>";
        QString datacard;
        html+="<tr><td align=center><h1>Please Import Some Data</h1><i>SleepyHead is pretty much useless without it.</i><br/><p>It might be a good idea to check preferences first,</br>as there are some options that affect import.</p><p>First import can take a few minutes.</p></td></tr></table>";
        html+=htmlFooter();
        ui->summaryView->setHtml(html);
        return;
    }
    int cpapdays=PROFILE.countDays(MT_CPAP,firstcpap,lastcpap);
    int cpapweekdays=PROFILE.countDays(MT_CPAP,cpapweek,lastcpap);
    int cpapmonthdays=PROFILE.countDays(MT_CPAP,cpapmonth,lastcpap);
    int cpapyeardays=PROFILE.countDays(MT_CPAP,cpapyear,lastcpap);
    int cpap6monthdays=PROFILE.countDays(MT_CPAP,cpap6month,lastcpap);

    CPAPMode cpapmode=(CPAPMode)(int)p_profile->calcSettingsMax(CPAP_Mode,MT_CPAP,firstcpap,lastcpap);

    float percentile=PROFILE.general->prefCalcPercentile()/100.0;

//    int mididx=PROFILE.general->prefCalcMiddle();
//    SummaryType ST_mid;
//    if (mididx==0) ST_mid=ST_PERC;
//    if (mididx==1) ST_mid=ST_WAVG;
//    if (mididx==2) ST_mid=ST_AVG;

    QString ahitxt;
    if (PROFILE.general->calculateRDI()) {
        ahitxt=tr("RDI");
    } else {
        ahitxt=tr("AHI");
    }

    int decimals=2;
    html+="<div align=center>";
    html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");
    if (cpapdays==0)  {
        html+="<tr><td colspan=6 align=center>No CPAP Machine Data Imported</td></tr>";
    } else {
        html+=QString("<tr><td colspan=6 align=center><b>CPAP Statistics as of %1</b></td></tr>").arg(lastcpap.toString(Qt::SystemLocaleLongDate));

        if (cpap_machines.size()>0) {
           // html+=QString("<tr><td colspan=6 align=center><b>%1</b></td></tr>").arg(tr("CPAP Summary"));

            if (!cpapdays) {
                html+=QString("<tr><td colspan=6 align=center><b>%1</b></td></tr>").arg(tr("No CPAP data available."));
            } else if (cpapdays==1) {
                html+=QString("<tr><td colspan=6 align=center>%1</td></tr>").arg(QString(tr("%1 day of CPAP Data, on %2.")).arg(cpapdays).arg(firstcpap.toString(Qt::SystemLocaleShortDate)));
            } else {
                html+=QString("<tr><td colspan=6 align=center>%1</td></tr>").arg(QString(tr("%1 days of CPAP Data, between %2 and %3")).arg(cpapdays).arg(firstcpap.toString(Qt::SystemLocaleShortDate)).arg(lastcpap.toString(Qt::SystemLocaleShortDate)));
            }

            html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</td></tr>")
                .arg(tr("Details")).arg(tr("Most Recent")).arg(tr("Last 7 Days")).arg(tr("Last 30 Days")).arg(tr("Last 6 months")).arg(tr("Last Year"));
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(ahitxt)
            .arg(calcAHI(lastcpap,lastcpap),0,'f',decimals)
            .arg(calcAHI(cpapweek,lastcpap),0,'f',decimals)
            .arg(calcAHI(cpapmonth,lastcpap),0,'f',decimals)
            .arg(calcAHI(cpap6month,lastcpap),0,'f',decimals)
            .arg(calcAHI(cpapyear,lastcpap),0,'f',decimals);

            if (PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpapyear,lastcpap)) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("RERA Index"))
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,lastcpap,lastcpap)/PROFILE.calcHours(MT_CPAP,lastcpap,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpapweek,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpapmonth,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpap6month,lastcpap)/PROFILE.calcHours(MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpapyear,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            }

            if (PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpapyear,lastcpap)) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Flow Limit Index"))
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,lastcpap,lastcpap)/PROFILE.calcHours(MT_CPAP,lastcpap,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpapweek,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpapmonth,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpap6month,lastcpap)/PROFILE.calcHours(MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpapyear,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            }



            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(tr("Hours per Night"))
            .arg(formatTime(p_profile->calcHours(MT_CPAP)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpapweek,lastcpap)/float(cpapweekdays)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpapmonth,lastcpap)/float(cpapmonthdays)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpap6month,lastcpap)/float(cpap6monthdays)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpapyear,lastcpap)/float(cpapyeardays)));

            if (cpapmode>=MODE_BIPAP) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Min EPAP"))
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("%1% EPAP").arg(percentile*100.0,0,'f',0))
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Max IPAP"))
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("%1% IPAP").arg(percentile*100.0,0,'f',0))
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            } else if (cpapmode>=MODE_APAP) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Average Pressure"))
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("%1% Pressure").arg(percentile*100.0,0,'f',0))
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            } else {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Pressure"))
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            }
            //html+="<tr><td colspan=6>TODO: 90% pressure.. Any point showing if this is all CPAP?</td></tr>";


            ChannelID leak;
            if (p_profile->calcCount(CPAP_LeakTotal,MT_CPAP,cpapyear,lastcpap)>0) {
                leak=CPAP_LeakTotal;
            } else leak=CPAP_Leak;

            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(tr("Average %1").arg(schema::channel[leak].label()))
            .arg(p_profile->calcWavg(leak,MT_CPAP),0,'f',decimals)
            .arg(p_profile->calcWavg(leak,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
            .arg(p_profile->calcWavg(leak,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
            .arg(p_profile->calcWavg(leak,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
            .arg(p_profile->calcWavg(leak,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(tr("%1% %2").arg(percentile*100.0f,0,'f',0).arg(schema::channel[leak].label()))
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP),0,'f',decimals)
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
        }
    }
    int oxisize=oximeters.size();
    if (oxisize>0) {
        QDate lastoxi=p_profile->LastGoodDay(MT_OXIMETER);
        QDate firstoxi=p_profile->FirstGoodDay(MT_OXIMETER);
        int days=PROFILE.countDays(MT_OXIMETER,firstoxi,lastoxi);
        if (days>0) {
            html+=QString("<tr><td colspan=6 align=center><b>%1</b></td></tr>").arg(tr("Oximetry Summary"));
            if (days==1) {
                html+=QString("<tr><td colspan=6 align=center>%1</td></tr>").arg(QString(tr("%1 day of Oximetry Data, on %2.")).arg(days).arg(firstoxi.toString(Qt::SystemLocaleShortDate)));
            } else {
                html+=QString("<tr><td colspan=6 align=center>%1</td></tr>").arg(QString(tr("%1 days of Oximetry Data, between %2 and %3")).arg(days).arg(firstoxi.toString(Qt::SystemLocaleShortDate)).arg(lastoxi.toString(Qt::SystemLocaleShortDate)));
            }

            html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</td></tr>")
                    .arg(tr("Details")).arg(tr("Most Recent")).arg(tr("Last 7 Days")).arg(tr("Last 30 Days")).arg(tr("Last 6 months")).arg(tr("Last Year"));
            QDate oxiweek=lastoxi.addDays(-7);
            QDate oximonth=lastoxi.addDays(-30);
            QDate oxi6month=lastoxi.addMonths(-6);
            QDate oxiyear=lastoxi.addYears(-12);
            if (oxiweek<firstoxi) oxiweek=firstoxi;
            if (oximonth<firstoxi) oximonth=firstoxi;
            if (oxi6month<firstoxi) oxi6month=firstoxi;
            if (oxiyear<firstoxi) oxiyear=firstoxi;
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Average SpO2"))
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Minimum SpO2"))
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("SpO2 Events / Hour"))
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER)/p_profile->calcHours(MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oxiweek,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oximonth,lastoxi)/p_profile->calcHours(MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oxi6month,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oxiyear,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2\%</td><td>%3\%</td><td>%4\%</td><td>%5\%</td><td>%6\%</td></tr>")
                .arg(tr("% of time in SpO2 Events"))
                .arg(100.0/p_profile->calcHours(MT_OXIMETER)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER)/3600.0,0,'f',decimals)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oxiweek,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oxiweek,lastoxi)/3600.0,0,'f',decimals)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oximonth,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oximonth,lastoxi)/3600.0,0,'f',decimals)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oxi6month,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oxi6month,lastoxi)/3600.0,0,'f',decimals)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oxiyear,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oxiyear,lastoxi)/3600.0,0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Average Pulse Rate"))
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Minimum Pulse Rate"))
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Maximum Pulse Rate"))
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Pulse Change Events / Hour"))
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER)/p_profile->calcHours(MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER,oxiweek,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER,oximonth,lastoxi)/p_profile->calcHours(MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER,oxi6month,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER,oxiyear,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
        }
    }

    html+="</table>";
    html+="</div>";

    QList<UsageData> AHI;

    //QDate bestAHIdate, worstAHIdate;
    //EventDataType bestAHI=999.0, worstAHI=0;
    if (cpapdays>0) {
        QDate first,last=lastcpap;
        CPAPMode mode,cmode=MODE_UNKNOWN;
        EventDataType cmin=0,cmax=0,cmaxhi=0, min,max,maxhi;
        Machine *mach,*lastmach=NULL;
        PRTypes lastpr, prelief=PR_UNKNOWN;
        short prelset, lastprelset=-1;
        QDate date=lastcpap;
        Day * day;
        bool lastchanged=false;
        QVector<RXChange> rxchange;
        EventDataType hours;

        int compliant=0;
        do {
            day=PROFILE.GetGoodDay(date,MT_CPAP);

            if (day) {
                lastchanged=false;

                hours=day->hours();

                if (hours > PROFILE.cpap->complianceHours())
                    compliant++;

                EventDataType ahi=day->count(CPAP_Obstructive)+day->count(CPAP_Hypopnea)+day->count(CPAP_Apnea)+day->count(CPAP_ClearAirway);
                if (PROFILE.general->calculateRDI()) ahi+=day->count(CPAP_RERA);
                ahi/=hours;
                AHI.push_back(UsageData(date,ahi,hours));

                prelief=(PRTypes)(int)round(day->settings_wavg(CPAP_PresReliefType));
                prelset=round(day->settings_wavg(CPAP_PresReliefSet));
                mode=(CPAPMode)(int)round(day->settings_wavg(CPAP_Mode));
                mach=day->machine;
                if (mode>=MODE_ASV) {
                    min=day->settings_min(CPAP_EPAP);
                    max=day->settings_max(CPAP_IPAPLo);
                    maxhi=day->settings_max(CPAP_IPAPHi);
                } else if (mode>=MODE_BIPAP) {
                    min=day->settings_min(CPAP_EPAP);
                    max=day->settings_max(CPAP_IPAP);
                } else if (mode>=MODE_APAP) {
                    min=day->settings_min(CPAP_PressureMin);
                    max=day->settings_max(CPAP_PressureMax);
                } else {
                    min=day->settings_min(CPAP_Pressure);
                }
                if ((mode!=cmode) || (min!=cmin) || (max!=cmax) || (mach!=lastmach) || (prelset!=lastprelset))  {
                    if ((cmode!=MODE_UNKNOWN) && (lastmach!=NULL)) {
                        first=date.addDays(1);
                        int days=PROFILE.countDays(MT_CPAP,first,last);
                        RXChange rx;
                        rx.first=first;
                        rx.last=last;
                        rx.days=days;
                        rx.ahi=calcAHI(first,last);
			rx.fl=calcFL(first,last);
                        rx.mode=cmode;
                        rx.min=cmin;
                        rx.max=cmax;
                        rx.maxhi=cmaxhi;
                        rx.prelief=lastpr;
                        rx.prelset=lastprelset;
                        rx.machine=lastmach;

                        if (mode<MODE_BIPAP) {
                            rx.per1=p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,first,last);
                            rx.per2=0;
                        } else if (mode<MODE_ASV) {
                            rx.per1=p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,first,last);
                            rx.per2=p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,first,last);
                        } else {
                            rx.per1=p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,first,last);
                            rx.per2=p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,first,last);
                        }
                        rx.weighted=float(rx.days)/float(cpapdays)*rx.ahi;
                        rxchange.push_back(rx);
                    }
                    cmode=mode;
                    cmin=min;
                    cmax=max;
                    cmaxhi=maxhi;
                    lastpr=prelief;
                    lastprelset=prelset;
                    last=date;
                    lastmach=mach;
                    lastchanged=true;
                }

            }
            date=date.addDays(-1);
        } while (date>=firstcpap);

        // Sort list by AHI
        qSort(AHI);

        lastchanged=false;
        if (!lastchanged && (mach!=NULL)) {
           // last=date.addDays(1);
            first=firstcpap;
            int days=PROFILE.countDays(MT_CPAP,first,last);
            RXChange rx;
            rx.first=first;
            rx.last=last;
            rx.days=days;
            rx.ahi=calcAHI(first,last);
	    rx.fl=calcFL(first,last);
            rx.mode=mode;
            rx.min=min;
            rx.max=max;
            rx.maxhi=maxhi;
            rx.prelief=prelief;
            rx.prelset=prelset;
            rx.machine=mach;
            if (mode<MODE_BIPAP) {
                rx.per1=p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,first,last);
                rx.per2=0;
            } else if (mode<MODE_ASV) {
                rx.per1=p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,first,last);
                rx.per2=p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,first,last);
            } else {
                rx.per1=p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,first,last);
                rx.per2=p_profile->calcPercentile(CPAP_IPAPHi,percentile,MT_CPAP,first,last);
            }
            rx.weighted=float(rx.days)/float(cpapdays);
            //rx.weighted=float(days)*rx.ahi;
            rxchange.push_back(rx);
        }

        int rxthresh=5;
        QVector<RXChange *> tmpRX;
        for (int i=0;i<rxchange.size();i++) {
            RXChange & rx=rxchange[i];
            if (rx.days>rxthresh)
                tmpRX.push_back(&rx);
        }

        QString recbox="<html><head><style type='text/css'>"
            "p,a,td,body { font-family: '"+QApplication::font().family()+"'; }"
            "p,a,td,body { font-size: "+QString::number(QApplication::font().pointSize() + 2)+"px; }"
            "a:link,a:visited { color: inherit; text-decoration: none; }" //font-weight: normal;
            "a:hover { background-color: inherit; color: white; text-decoration:none; font-weight: bold; }"
            "</style></head><body>";
        recbox+="<table width=100% cellpadding=1 cellspacing=0>";
        int numdays=AHI.size();
        if (numdays>1) {
            int z=numdays/2;
            if (z>4) z=4;

            recbox+=QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(tr("Usage Information"));
            recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Total Days")).arg(numdays);
            if (PROFILE.cpap->showComplianceInfo()) {
                recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Compliant Days")).arg(compliant);
            }
            int highahi=0;
            for (int i=0;i<numdays;i++) {
                if (AHI.at(i).ahi > 5.0)
                    highahi++;
            }
            recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Days AHI &gt;5.0")).arg(highahi);


            recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");
            recbox+=QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(tr("Best&nbsp;%1").arg(ahitxt));
            for (int i=0;i<z;i++) {
                const UsageData & a=AHI.at(i);
                recbox+=QString("<tr><td><a href='daily=%1'>%2</a></td><td  align=right>%3</td></tr>")
                    .arg(a.date.toString(Qt::ISODate))
                    .arg(a.date.toString(Qt::SystemLocaleShortDate))
                    .arg(a.ahi,0,'f',decimals);
            }
            recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");
            recbox+=QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(tr("Worst&nbsp;%1").arg(ahitxt));
            for (int i=0;i<z;i++) {
                const UsageData & a=AHI.at((numdays-1)-i);
                recbox+=QString("<tr><td><a href='daily=%1'>%2</a></td><td align=right>%3</td></tr>")
                    .arg(a.date.toString(Qt::ISODate))
                    .arg(a.date.toString(Qt::SystemLocaleShortDate))
                    .arg(a.ahi,0,'f',decimals);
            }
            recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");
        }


        if (tmpRX.size()>0) {
            RXsort=RX_ahi;
            QString minstr,maxstr,modestr,maxhistr;
            qSort(tmpRX.begin(),tmpRX.end(),RXSort);
            tmpRX[0]->highlight=4; // worst
            int ls=tmpRX.size()-1;
            tmpRX[ls]->highlight=1; //best
            CPAPMode mode=(CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode,MT_CPAP,tmpRX[ls]->first,tmpRX[ls]->last);

            if (mode<MODE_APAP) { // is CPAP?
                minstr="Pressure";
                maxstr="";
                modestr=tr("CPAP");
            } else if (mode<MODE_BIPAP) { // is AUTO?
                minstr="Min";
                maxstr="Max";
                modestr=tr("APAP");
            } else if (mode<MODE_ASV) { // BIPAP
                minstr="EPAP";
                maxstr="IPAP";
                modestr=tr("Bi-Level");
            } else {
                minstr="EPAP";
                maxstr="IPAPLo";
                maxhistr="IPAPHi";
                modestr=tr("ST/ASV");

            }

            recbox+=QString("<tr><td colspan=2><table width=100% border=0 cellpadding=1 cellspacing=0><tr><td colspan=2 align=center><b>%3</b></td></tr>")
                    .arg(tr("Best RX Setting"));
            recbox+=QString("<tr><td valign=top>Start<br/>End</td><td align=right><a href='overview=%1,%2'>%3<br/>%4</a></td></tr>")
                    .arg(tmpRX[ls]->first.toString(Qt::ISODate))
                    .arg(tmpRX[ls]->last.toString(Qt::ISODate))
                    .arg(tmpRX[ls]->first.toString(Qt::SystemLocaleShortDate))
                    .arg(tmpRX[ls]->last.toString(Qt::SystemLocaleShortDate));
            recbox+=QString("<tr><td><b>%1</b></td><td align=right><b>%2</b></td></tr>").arg(ahitxt).arg(tmpRX[ls]->ahi,0,'f',decimals);
            recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Mode")).arg(modestr);
            recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(minstr).arg(tmpRX[ls]->min,0,'f',1).arg(STR_UNIT_CMH2O);
            if (!maxstr.isEmpty()) recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxstr).arg(tmpRX[ls]->max,0,'f',1).arg(STR_UNIT_CMH2O);
            if (!maxhistr.isEmpty()) recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxhistr).arg(tmpRX[ls]->maxhi,0,'f',1).arg(STR_UNIT_CMH2O);
            recbox+="</table></td></tr>";

            recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");

            mode=(CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode,MT_CPAP,tmpRX[0]->first,tmpRX[0]->last);
            if (mode<MODE_APAP) { // is CPAP?
                minstr="Pressure";
                maxstr="";
                modestr=tr("CPAP");
            } else if (mode<MODE_BIPAP) { // is AUTO?
                minstr="Min";
                maxstr="Max";
                modestr=tr("APAP");
            } else if (mode<MODE_ASV) { // BIPAP or greater
                minstr="EPAP";
                maxstr="IPAP";
                modestr=tr("Bi-Level");
            } else {
                minstr="EPAP";
                maxstr="IPAPLo";
                maxhistr="IPAPHi";
                modestr=tr("ST/ASV");
            }

            recbox+=QString("<tr><td colspan=2><table width=100% border=0 cellpadding=1 cellspacing=0><tr><td colspan=2 align=center><b>%3</b></td></tr>")
                    .arg(tr("Worst RX Setting"));
            recbox+=QString("<tr><td valign=top>Start<br/>End</td><td align=right><a href='overview=%1,%2'>%3<br/>%4</a></td></tr>")
                    .arg(tmpRX[0]->first.toString(Qt::ISODate))
                    .arg(tmpRX[0]->last.toString(Qt::ISODate))
                    .arg(tmpRX[0]->first.toString(Qt::SystemLocaleShortDate))
                    .arg(tmpRX[0]->last.toString(Qt::SystemLocaleShortDate));
            recbox+=QString("<tr><td><b>%1</b></td><td align=right><b>%2</b></td></tr>").arg(ahitxt).arg(tmpRX[0]->ahi,0,'f',decimals);
            recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Mode")).arg(modestr);
            recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(minstr).arg(tmpRX[0]->min,0,'f',1).arg(STR_UNIT_CMH2O);
            if (!maxstr.isEmpty()) recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxstr).arg(tmpRX[0]->max,0,'f',1).arg(STR_UNIT_CMH2O);
            if (!maxhistr.isEmpty()) recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxhistr).arg(tmpRX[0]->maxhi,0,'f',1).arg(STR_UNIT_CMH2O);
            recbox+="</table></td></tr>";

        }
        recbox+="</table>";
        recbox+="</body></html>";
        ui->recordsBox->setHtml(recbox);

        /*RXsort=RX_min;
        RXorder=true;
        qSort(rxchange.begin(),rxchange.end());*/
        html+="<div align=center>";
        html+=QString("<br/><b>Changes to Prescription Settings</b>");
        html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");
        QString extratxt;

        if (cpapmode>=MODE_ASV) {
            extratxt=QString("<td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td>")
                .arg(tr("EPAP")).arg(tr("IPAPLo")).arg(tr("IPAPHi")).arg(tr("PS Min")).arg(tr("PS Max"));
        } else if (cpapmode>=MODE_BIPAP) {
            extratxt=QString("<td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td>")
                .arg(tr("EPAP")).arg(tr("IPAP")).arg(tr("PS"));
        } else if (cpapmode>MODE_CPAP) {
            extratxt=QString("<td><b>%1</b></td><td><b>%2</b></td>")
                .arg(tr("Min Pres.")).arg(tr("Max Pres."));
        } else {
            extratxt=QString("<td><b>%1</b></td>")
                .arg(tr("Pressure"));
        }
        QString tooltip;
        html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</b></td><td><b>%7</td><td><b>%8</td>%9</tr>")
                  .arg(tr("First"))
                  .arg(tr("Last"))
                  .arg(tr("Days"))
                  .arg(ahitxt)
		  .arg(tr("FL"))
                  .arg(tr("Machine"))
                  .arg(tr("Mode"))
                  .arg(tr("Pr. Rel."))
                  .arg(extratxt);

        for (int i=0;i<rxchange.size();i++) {
            RXChange rx=rxchange.at(i);
            QString color;
            if (rx.highlight==1) {
                color="#c0ffc0";
            } else if (rx.highlight==2) {
                color="#e0ffe0";
            } else if (rx.highlight==3) {
                color="#ffe0e0";
            } else if (rx.highlight==4) {
                color="#ffc0c0";
            } else color="";
            QString machstr;
            if (rx.machine->properties.contains(STR_PROP_Brand))
                machstr+=rx.machine->properties[STR_PROP_Brand];
            if (rx.machine->properties.contains(STR_PROP_Model)) {
                 machstr+=" "+rx.machine->properties[STR_PROP_Model];
            }
            if (rx.machine->properties.contains(STR_PROP_Serial)) {
                 machstr+=" ("+rx.machine->properties[STR_PROP_Serial]+")<br/>";
            }

            mode=rx.mode;
            if(mode>=MODE_ASV) {
                extratxt=QString("<td>%1</td><td>%2</td><td>%3</td><td>%4</td>")
                        .arg(rx.max,0,'f',decimals).arg(rx.maxhi,0,'f',decimals).arg(rx.max-rx.min,0,'f',decimals).arg(rx.maxhi-rx.min,0,'f',decimals);

                tooltip=tr("%5 %1% EPAP=%2<br/>%3% IPAP=%4")
                        .arg(percentile*100.0)
                        .arg(rx.per1,0,'f',decimals)
                        .arg(percentile*100.0)
                        .arg(rx.per2,0,'f',decimals)
                        .arg(machstr)
                        ;
            } else if (mode>=MODE_BIPAP) {
                extratxt=QString("<td>%1</td><td>%2</td>")
                       .arg(rx.max,0,'f',decimals).arg(rx.max-rx.min,0,'f',decimals);
                tooltip=tr("%5 %1% EPAP=%2<br/>%3% IPAP=%4")
                        .arg(percentile*100.0)
                        .arg(rx.per1,0,'f',decimals)
                        .arg(percentile*100.0)
                        .arg(rx.per2,0,'f',decimals)
                        .arg(machstr)
                        ;
            } else if (mode>MODE_CPAP) {
                extratxt=QString("<td>%1</td>").arg(rx.max,0,'f',decimals);
                tooltip=tr("%3 %1% Pressure=%2")
                        .arg(percentile*100.0)
                        .arg(rx.per1,0,'f',decimals)
                        .arg(machstr)
                        ;
            } else {
                if (cpapmode>MODE_CPAP) {
                    extratxt="<td>&nbsp;</td>";
                    tooltip=QString("%1").arg(machstr);
                } else {
                    extratxt="";
                    tooltip="";
                }
            }
            QString presrel;
            if (rx.prelset>0) {
                presrel=schema::channel[CPAP_PresReliefType].option(int(rx.prelief));
                presrel+=QString(" x%1").arg(rx.prelset);
            } else presrel="None";
            QString tooltipshow,tooltiphide;
            if (!tooltip.isEmpty()) {
                tooltipshow=QString("tooltip.show(\"%1\");").arg(tooltip);
                tooltiphide="tooltip.hide();";
            }
            html+=QString("<tr bgcolor='"+color+"' onmouseover='ChangeColor(this, \"#eeeeee\"); %13' onmouseout='ChangeColor(this, \""+color+"\"); %14' onclick='alert(\"overview=%1,%2\");'><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8</td><td>%9</td><td>%10</td><td>%11</td>%12</tr>")
                    .arg(rx.first.toString(Qt::ISODate))
                    .arg(rx.last.toString(Qt::ISODate))
                    .arg(rx.first.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.last.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.days)
                    .arg(rx.ahi,0,'f',decimals)
		    .arg(rx.fl,0,'f',decimals)
                    .arg(rx.machine->GetClass())
                    .arg(schema::channel[CPAP_Mode].option(int(rx.mode)-1))
                    .arg(presrel)
                    .arg(rx.min,0,'f',decimals)
                    .arg(extratxt)
                    .arg(tooltipshow)
                    .arg(tooltiphide);
        }
        html+="</table>";
        html+=QString("<i>The above has a threshold which excludes day counts less than %1 from the best/worst highlighting</i><br/>").arg(rxthresh);
        html+="</div>";

    }
    if (mach.size()>0) {
        html+="<div align=center>";

        html+=QString("<br/><b>Machine Information</b>");
        html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");
        html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td></tr>")
                      .arg(tr("Brand"))
                      .arg(tr("Model"))
                      .arg(tr("Serial"))
                      .arg(tr("First Use"))
                      .arg(tr("Last Use"));
        Machine *m;
        for (int i=0;i<mach.size();i++) {
            m=mach.at(i);
            if (m->GetType()==MT_JOURNAL) continue;
            QString mn=m->properties[STR_PROP_ModelNumber];
            //if (mn.isEmpty())
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                    .arg(m->properties[STR_PROP_Brand])
                    .arg(m->properties[STR_PROP_Model]+" "+m->properties[STR_PROP_SubModel]+ (mn.isEmpty() ? "" : QString(" (")+mn+QString(")")))
                    .arg(m->properties[STR_PROP_Serial])
                    .arg(m->FirstDay().toString(Qt::SystemLocaleShortDate))
                    .arg(m->LastDay().toString(Qt::SystemLocaleShortDate));
        }
        html+="</table>";
        html+="</div>";
    }
    html+="<script type='text/javascript' language='javascript' src='qrc:/docs/script.js'></script>";
    updateFavourites();
    html+=htmlFooter();
    //QWebFrame *frame=ui->summaryView->page()->currentFrame();
    //frame->addToJavaScriptWindowObject("mainwin",this);
    //ui->summaryView->setHtml(html);
    MyStatsPage *page=new MyStatsPage(this);
    page->currentFrame()->setHtml(html);
    ui->summaryView->setPage(page);
//    connect(ui->summaryView->page()->currentFrame(),SIGNAL(javaScriptWindowObjectCleared())
//    QString file="qrc:/docs/index.html";
//    QUrl url(file);
//    ui->webView->setUrl(url);
}


void MainWindow::updateFavourites()
{
    QDate date=PROFILE.LastDay(MT_JOURNAL);
    if (!date.isValid())
        return;

    QString html="<html><head><style type='text/css'>"
        "p,a,td,body { font-family: '"+QApplication::font().family()+"'; }"
        "p,a,td,body { font-size: "+QString::number(QApplication::font().pointSize() + 2)+"px; }"
        "a:link,a:visited { color: inherit; text-decoration: none; }" //font-weight: normal;
        "a:hover { background-color: inherit; color: white; text-decoration:none; font-weight: bold; }"
        "</style></head><body>";
    html+="<table width=100% cellpadding=2 cellspacing=0>";

    //ui->favouritesList->blockSignals(true);
    //ui->favouritesList->clear();

    do {
        Day * journal=PROFILE.GetDay(date,MT_JOURNAL);
        if (journal) {
            if (journal->size()>0) {
                Session *sess=(*journal)[0];
                QString tmp;
                bool filtered=!bookmarkFilter.isEmpty();
                bool found=!filtered;
                if (sess->settings.contains(Bookmark_Start)) {
                    //QVariantList start=sess->settings[Bookmark_Start].toList();
                    //QVariantList end=sess->settings[Bookmark_End].toList();
                    QStringList notes=sess->settings[Bookmark_Notes].toStringList();
                    if (notes.size()>0) {
                        tmp+=QString("<tr><td><b><a href='daily=%1'>%2</a></b><br/>")
                                .arg(date.toString(Qt::ISODate))
                                .arg(date.toString());

                        tmp+="<list>";

                        for (int i=0;i<notes.size();i++) {
                            //QDate d=start[i].toDate();
                            QString note=notes[i];
                            if (filtered && note.contains(bookmarkFilter,Qt::CaseInsensitive))
                                found=true;
                            tmp+="<li>"+note+"</li>";
                        }
                        tmp+="</list></td>";
                    }
                }
                if (found) html+=tmp;
            }
        }

        date=date.addDays(-1);
    } while (date>=PROFILE.FirstDay(MT_JOURNAL));
    html+="</table></body></html>";
    ui->bookmarkView->setHtml(html);
    //ui->favouritesList->blockSignals(false);
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
    qstatus2->setText(STR_TR_Daily);
}
void MainWindow::JumpDaily()
{
    on_dailyButton_clicked();
}

void MainWindow::on_overviewButton_clicked()
{
    ui->tabWidget->setCurrentWidget(overview);
    qstatus2->setText(STR_TR_Overview);
}

void MainWindow::on_webView_loadFinished(bool arg1)
{
    Q_UNUSED(arg1);
    qprogress->hide();
    if (first_load) {
        QTimer::singleShot(0,this,SLOT(Startup()));
        first_load=false;
    } else {
        qstatus->setText("");
    }
    ui->backButton->setEnabled(ui->webView->history()->canGoBack());
    ui->forwardButton->setEnabled(ui->webView->history()->canGoForward());

    connect(ui->webView->page(),SIGNAL(linkHovered(QString,QString,QString)),this,SLOT(LinkHovered(QString,QString,QString)));
}

void MainWindow::on_webView_loadStarted()
{
    disconnect(ui->webView->page(),SIGNAL(linkHovered(QString,QString,QString)),this,SLOT(LinkHovered(QString,QString,QString)));
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

void MainWindow::on_action_About_triggered()
{

    QString gitrev=QString(GIT_REVISION);
    if (!gitrev.isEmpty()) gitrev="Revision: "+gitrev;

     QString msg=tr("<html><body><div align='center'><h2>SleepyHead v%1.%2.%3-%4 (%8)</h2>Build Date: %5 %6<br/>%7<br/>Data Folder: %9<hr>"
"Copyright &copy;2011 Mark Watkins (jedimark) <br> \n"
"<a href='http://sleepyhead.sourceforge.net'>http://sleepyhead.sourceforge.net</a> <hr>"
"This software is released under the GNU Public License <br>"
"<i>This software comes with absolutely no warranty, either express of implied. It comes with no guarantee of fitness for any particular purpose. No guarantees are made regarding the accuracy of any data this program displays."
                    "</div></body></html>").arg(major_version).arg(minor_version).arg(revision_number).arg(release_number).arg(__DATE__).arg(__TIME__).arg(gitrev).arg(ReleaseStatus).arg(GetAppRoot());
    QMessageBox msgbox(QMessageBox::Information,tr("About SleepyHead"),"",QMessageBox::Ok,this);
    msgbox.setTextFormat(Qt::RichText);
    msgbox.setText(msg);
    msgbox.exec();
}

void MainWindow::on_actionDebug_toggled(bool checked)
{
    PROFILE.general->setShowDebug(checked);
    if (checked) {
        ui->logText->show();
    } else {
        ui->logText->hide();
    }
}

void MainWindow::on_action_Reset_Graph_Layout_triggered()
{
    if (daily && (ui->tabWidget->currentWidget()==daily)) daily->ResetGraphLayout();
    if (overview && (ui->tabWidget->currentWidget()==overview)) overview->ResetGraphLayout();
}

void MainWindow::on_action_Preferences_triggered()
{
    if (m_inRecalculation) {
        mainwin->Notify("Access to Preferences has been blocked until recalculation completes.");
        return;
    }
    PreferencesDialog pd(this,p_profile);
    prefdialog=&pd;
    if (pd.exec()==PreferencesDialog::Accepted) {
        qDebug() << "Preferences Accepted";
        //pd.Save();
        if (daily) {
            //daily->ReloadGraphs();
            daily->RedrawGraphs();
        }
        if (overview) {
            overview->ReloadGraphs();
            overview->RedrawGraphs();
        }
    }
    prefdialog=NULL;
}
void MainWindow::selectOximetryTab()
{
    on_oximetryButton_clicked();
}

void MainWindow::on_oximetryButton_clicked()
{
    bool first=false;
    if (!oximetry) {
        if (!PROFILE.oxi->oximetryEnabled()) {
            if (QMessageBox::question(this,"Question","Do you have a CMS50[x] Oximeter?\nOne is required to use this section.",QMessageBox::Yes,QMessageBox::No)==QMessageBox::No) return;
            PROFILE.oxi->setOximetryEnabled(true);
        }
        oximetry=new Oximetry(ui->tabWidget,daily->graphView());
        ui->tabWidget->insertTab(3,oximetry,STR_TR_Oximetry);
        first=true;
    }
    ui->tabWidget->setCurrentWidget(oximetry);
    if (!first) oximetry->RedrawGraphs();
    qstatus2->setText(STR_TR_Oximetry);
}

void MainWindow::CheckForUpdates()
{
    on_actionCheck_for_Updates_triggered();
}

void MainWindow::on_actionCheck_for_Updates_triggered()
{
    UpdaterWindow *w=new UpdaterWindow(this);
    w->checkForUpdates();
}

void MainWindow::on_action_Screenshot_triggered()
{
    QTimer::singleShot(250,this,SLOT(DelayedScreenshot()));
}
void MainWindow::DelayedScreenshot()
{
//#ifdef Q_OS_MAC
//    CGImageRef windowImage = CGWindowListCreateImage(imageBounds, singleWindowListOptions, windowID, imageOptions);
//    originalPixmap = new QPixmap(QPixmap::fromMacCGImageRef(windowImage));


//    CGImageRef img = CGDisplayCreateImageForRect(displayID, *rect);
//    CFMutableDataRef imgData = CFDataCreateMutable(0, 0);
//    CGImageDestinationRef imgDst = CGImageDestinationCreateWithData(imgData, kUTTypeJPEG2000, 1, 0);
//    CGImageDestinationAddImage(imgDst, img, 0);
//    CGImageDestinationFinalize(imgDst);
//    CFRelease(imgDst);
//    QPixmap pixmap=QPixmap::fromMacCGImageRef(img);
//#else
    QPixmap pixmap = QPixmap::grabWindow(this->winId());
//#endif
    QString a=PREF.Get("{home}")+"/Screenshots";
    QDir dir(a);
    if (!dir.exists()){
        dir.mkdir(a);
    }
    a+="/screenshot-"+QDateTime::currentDateTime().toString(Qt::ISODate)+".png";
    pixmap.save(a);
}

void MainWindow::on_actionView_O_ximetry_triggered()
{
    on_oximetryButton_clicked();
}
void MainWindow::updatestatusBarMessage (const QString & text)
{
    ui->statusbar->showMessage(text,1000);
}

void MainWindow::on_actionPrint_Report_triggered()
{
    if (ui->tabWidget->currentWidget()==overview) {
        PrintReport(overview->graphView(),STR_TR_Overview);
    } else if (ui->tabWidget->currentWidget()==daily) {
        PrintReport(daily->graphView(),STR_TR_Daily,daily->getDate());
    } else if (ui->tabWidget->currentWidget()==oximetry) {
        if (oximetry)
            PrintReport(oximetry->graphView(),STR_TR_Oximetry);
    } else {
        QPrinter printer;
#ifdef Q_WS_X11
        printer.setPrinterName("Print to File (PDF)");
        printer.setOutputFormat(QPrinter::PdfFormat);
        QString name;
        QString datestr;
        if (ui->tabWidget->currentWidget()==ui->summaryTab) {
            name="Summary";
            datestr=QDate::currentDate().toString(Qt::ISODate);
        } else if (ui->tabWidget->currentWidget()==ui->helpTab) {
            name="Help";
            datestr=QDateTime::currentDateTime().toString(Qt::ISODate);
        } else name="Unknown";

        QString filename=PREF.Get("{home}/"+name+"_"+PROFILE.user->userName()+"_"+datestr+".pdf");

        printer.setOutputFileName(filename);
#endif
        printer.setPrintRange(QPrinter::AllPages);
        printer.setOrientation(QPrinter::Portrait);
        printer.setFullPage(false); // This has nothing to do with scaling
        printer.setNumCopies(1);
        printer.setPageMargins(10,10,10,10,QPrinter::Millimeter);

        QPrintDialog pdlg(&printer,this);
        if (pdlg.exec()==QPrintDialog::Accepted) {

            if (ui->tabWidget->currentWidget()==ui->summaryTab) {
                ui->summaryView->print(&printer);
            } else if (ui->tabWidget->currentWidget()==ui->helpTab) {
                ui->webView->print(&printer);
            }

        }
        //QMessageBox::information(this,tr("Not supported Yet"),tr("Sorry, printing from this page is not supported yet"),QMessageBox::Ok);
    }
}

void MainWindow::on_action_Edit_Profile_triggered()
{
    NewProfile newprof(this);
    newprof.edit(PREF[STR_GEN_Profile].toString());
    newprof.exec();

}

void MainWindow::on_action_Link_Graph_Groups_toggled(bool arg1)
{
    PROFILE.general->setLinkGroups(arg1);
    if (daily) daily->RedrawGraphs();
}

void MainWindow::on_action_CycleTabs_triggered()
{
    int i;
    qDebug() << "Switching Tabs";
    i=ui->tabWidget->currentIndex()+1;
    if (i >= ui->tabWidget->count())
        i=0;
    ui->tabWidget->setCurrentIndex(i);
}

void MainWindow::on_actionExp_ort_triggered()
{
    ExportCSV ex(this);
    if (ex.exec()==ExportCSV::Accepted) {
    }
}

void MainWindow::on_actionOnline_Users_Guide_triggered()
{
    ui->webView->load(QUrl("http://sourceforge.net/apps/mediawiki/sleepyhead/index.php?title=SleepyHead_Users_Guide"));
    ui->tabWidget->setCurrentWidget(ui->helpTab);
}

void MainWindow::on_action_Frequently_Asked_Questions_triggered()
{
    ui->webView->load(QUrl("http://sourceforge.net/apps/mediawiki/sleepyhead/index.php?title=Frequently_Asked_Questions"));
    ui->tabWidget->setCurrentWidget(ui->helpTab);
}

void MainWindow::PrintReport(gGraphView *gv,QString name, QDate date)
{
    if (!gv) return;
    Session * journal=NULL;
    //QDate d=QDate::currentDate();

    int visgraphs=gv->visibleGraphs();
    if (visgraphs==0) {
        Notify(tr("There are no graphs visible to print"));
        return;
    }

    QString username=PROFILE.Get(QString("_{")+QString(STR_UI_UserName)+"}_");

    bool print_bookmarks=false;
    if (name==STR_TR_Daily) {
        QVariantList book_start;
        journal=getDaily()->GetJournalSession(getDaily()->getDate());
        if (journal && journal->settings.contains(Bookmark_Start)) {
            book_start=journal->settings[Bookmark_Start].toList();
            if (book_start.size()>0) {
                if (QMessageBox::question(this,tr("Bookmarks"),tr("Would you like to show bookmarked areas in this report?"),QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes) {
                    print_bookmarks=true;
                }
            }
        }
    }

    QPrinter * printer;

    bool aa_setting=PROFILE.appearance->antiAliasing();

    bool force_antialiasing=aa_setting;

    printer=new QPrinter(QPrinter::HighResolution);

#ifdef Q_WS_X11
    printer->setPrinterName("Print to File (PDF)");
    printer->setOutputFormat(QPrinter::PdfFormat);
    QString filename=PREF.Get("{home}/"+name+username+date.toString(Qt::ISODate)+".pdf");

    printer->setOutputFileName(filename);
#endif
    printer->setPrintRange(QPrinter::AllPages);
    printer->setOrientation(QPrinter::Portrait);
    printer->setFullPage(false); // This has nothing to do with scaling
    printer->setNumCopies(1);
    printer->setPageMargins(10,10,10,10,QPrinter::Millimeter);
    QPrintDialog dialog(printer);
#ifdef Q_OS_MAC
    // QTBUG-17913
    QApplication::processEvents();
#endif
    if (dialog.exec() != QDialog::Accepted) {
        delete printer;
        return;
    }

    Notify(tr("This make take some time to complete..\nPlease don't touch anything until it's done."),tr("Printing %1 Report").arg(name),20000);
    QPainter painter;
    painter.begin(printer);

    GLint gw;
    gw=2048;  // Rough guess.. No GL_MAX_RENDERBUFFER_SIZE in mingw.. :(

    //QSizeF pxres=printer->paperSize(QPrinter::DevicePixel);
    QRect prect=printer->pageRect();
    float ratio=float(prect.height())/float(prect.width());
    float virt_width=gw;
    float virt_height=virt_width*ratio;
    painter.setWindow(0,0,virt_width, virt_height);
    painter.setViewport(0,0,prect.width(),prect.height());
    painter.setViewTransformEnabled(true);

    QFont report_font=*defaultfont;
    QFont medium_font=*mediumfont;
    QFont title_font=*bigfont;
    float normal_height=30; //fm2.ascent();
    report_font.setPixelSize(normal_height);
    medium_font.setPixelSize(40);
    title_font.setPixelSize(90);
    painter.setFont(report_font);

    //QFontMetrics fm2(*defaultfont);

    qDebug() << "Printer Resolution is" << virt_width << "x" << virt_height;

    const int graphs_per_page=6;
    float full_graph_height=(virt_height-(normal_height*graphs_per_page)) / float(graphs_per_page);

    QString title=tr("%1 Report").arg(name);
    painter.setFont(title_font);
    int top=0;
    QRectF bounds=painter.boundingRect(QRectF(0,top,virt_width,0),title,QTextOption(Qt::AlignHCenter | Qt::AlignTop));
    painter.drawText(bounds,title,QTextOption(Qt::AlignHCenter | Qt::AlignTop));
    top+=bounds.height()+normal_height/2.0;
    painter.setFont(report_font);

    int maxy=0;
    if (!PROFILE.user->firstName().isEmpty()) {
        QString userinfo=tr("Name:\t %1, %2\n").arg(PROFILE.user->lastName()).arg(PROFILE.user->firstName());
        userinfo+=tr("DOB:\t%1\n").arg(PROFILE.user->DOB().toString(Qt::SystemLocaleShortDate));
        if (!PROFILE.doctor->patientID().isEmpty()) userinfo+=tr("Patient ID:\t%1\n").arg(PROFILE.doctor->patientID());
        userinfo+=tr("Phone:\t%1\n").arg(PROFILE.user->phone());
        userinfo+=tr("Email:\t%1\n").arg(PROFILE.user->email());
        if (!PROFILE.user->address().isEmpty()) userinfo+=tr("\nAddress:\n%1").arg(PROFILE.user->address());

        QRectF bounds=painter.boundingRect(QRectF(0,top,virt_width,0),userinfo,QTextOption(Qt::AlignLeft | Qt::AlignTop));
        painter.drawText(bounds,userinfo,QTextOption(Qt::AlignLeft | Qt::AlignTop));
        if (bounds.height()>maxy) maxy=bounds.height();
    }
    Day *cpap=NULL, *oxi=NULL;

    int graph_slots=0;
    if (name==STR_TR_Daily) {
        cpap=PROFILE.GetGoodDay(date,MT_CPAP);
        oxi=PROFILE.GetGoodDay(date,MT_OXIMETER);
        QString cpapinfo=date.toString(Qt::SystemLocaleLongDate)+"\n\n";
        if (cpap) {
            time_t f=cpap->first()/1000L;
            time_t l=cpap->last()/1000L;
            int tt=qint64(cpap->total_time())/1000L;
            int h=tt/3600;
            int m=(tt/60)%60;
            int s=tt % 60;

            cpapinfo+=tr("Mask Time: ")+QString().sprintf("%2i hours, %2i minutes, %2i seconds",h,m,s)+"\n";
            cpapinfo+=tr("Bedtime: ")+QDateTime::fromTime_t(f).time().toString("HH:mm:ss")+" ";
            cpapinfo+=tr("Wake-up: ")+QDateTime::fromTime_t(l).time().toString("HH:mm:ss")+"\n\n";
            QString submodel;
            cpapinfo+=tr("Machine: ");
            if (cpap->machine->properties.find(STR_PROP_SubModel)!=cpap->machine->properties.end())
                submodel="\n"+cpap->machine->properties[STR_PROP_SubModel];
            cpapinfo+=cpap->machine->properties[STR_PROP_Brand]+" "+cpap->machine->properties[STR_PROP_Model]+submodel;
            CPAPMode mode=(CPAPMode)(int)cpap->settings_max(CPAP_Mode);
            cpapinfo+=tr("\nMode: ");

            if (mode==MODE_CPAP) {
                EventDataType min=round(cpap->settings_wavg(CPAP_Pressure)*2)/2.0;
                cpapinfo+=tr("CPAP")+" "+QString::number(min)+STR_UNIT_CMH2O;
            } else if (mode==MODE_APAP) {
                EventDataType min=cpap->settings_min(CPAP_PressureMin);
                EventDataType max=cpap->settings_max(CPAP_PressureMax);
                cpapinfo+=tr("APAP")+" "+QString::number(min)+"-"+QString::number(max)+STR_UNIT_CMH2O;
            } else if (mode==MODE_BIPAP) {
                EventDataType epap=cpap->settings_min(CPAP_EPAP);
                EventDataType ipap=cpap->settings_max(CPAP_IPAP);
                EventDataType ps=cpap->settings_max(CPAP_PS);
                cpapinfo+=tr("Bi-Level")+QString("\nEPAP: %1 IPAP: %2 %3\nPS: %4")
                        .arg(epap,0,'f',1).arg(ipap,0,'f',1).arg(STR_UNIT_CMH2O).arg(ps,0,'f',1);
            }
            else if (mode==MODE_ASV) {
                EventDataType epap=cpap->settings_min(CPAP_EPAP);
                EventDataType low=cpap->settings_min(CPAP_IPAPLo);
                EventDataType high=cpap->settings_max(CPAP_IPAPHi);
                EventDataType psl=cpap->settings_min(CPAP_PSMin);
                EventDataType psh=cpap->settings_max(CPAP_PSMax);
                cpapinfo+=tr("ASV")+QString("\nEPAP: %1 IPAP: %2 - %3 %4\nPS: %5 / %6")
                        .arg(epap,0,'f',1)
                        .arg(low,0,'f',1)
                        .arg(high,0,'f',1)
                        .arg(STR_UNIT_CMH2O)
                        .arg(psl,0,'f',1)
                        .arg(psh,0,'f',1);
            }
            else cpapinfo+=tr("Unknown");

            float ahi=(cpap->count(CPAP_Obstructive)+cpap->count(CPAP_Hypopnea)+cpap->count(CPAP_ClearAirway)+cpap->count(CPAP_Apnea));
            if (PROFILE.general->calculateRDI()) ahi+=cpap->count(CPAP_RERA);
            ahi/=cpap->hours();
            float csr=(100.0/cpap->hours())*(cpap->sum(CPAP_CSR)/3600.0);
            float uai=cpap->count(CPAP_Apnea)/cpap->hours();
            float oai=cpap->count(CPAP_Obstructive)/cpap->hours();
            float hi=(cpap->count(CPAP_ExP)+cpap->count(CPAP_Hypopnea))/cpap->hours();
            float cai=cpap->count(CPAP_ClearAirway)/cpap->hours();
            float rei=cpap->count(CPAP_RERA)/cpap->hours();
            float vsi=cpap->count(CPAP_VSnore)/cpap->hours();
            float fli=cpap->count(CPAP_FlowLimit)/cpap->hours();
            float nri=cpap->count(CPAP_NRI)/cpap->hours();
            float lki=cpap->count(CPAP_LeakFlag)/cpap->hours();
            float exp=cpap->count(CPAP_ExP)/cpap->hours();

            int piesize=(2048.0/8.0)*1.3;   // 1.5" in size
            //float fscale=font_scale;
            //if (!highres)
//                fscale=1;

            QString stats;
            painter.setFont(medium_font);
            if (PROFILE.general->calculateRDI())
                stats=tr("RDI\t%1\n").arg(ahi,0,'f',2);
            else
                stats=tr("AHI\t%1\n").arg(ahi,0,'f',2);
            QRectF bounds=painter.boundingRect(QRectF(0,0,virt_width,0),stats,QTextOption(Qt::AlignRight));
            painter.drawText(bounds,stats,QTextOption(Qt::AlignRight));


            getDaily()->eventBreakdownPie()->setShowTitle(false);
            getDaily()->eventBreakdownPie()->setMargins(0,0,0,0);
            QPixmap ebp;
            if (ahi>0) {
                ebp=getDaily()->eventBreakdownPie()->renderPixmap(piesize,piesize,true);
            } else {
                ebp=QPixmap(":/icons/smileyface.png");
            }
            if (!ebp.isNull()) {
                painter.drawPixmap(virt_width-piesize,bounds.height(),piesize,piesize,ebp);
            }
            getDaily()->eventBreakdownPie()->setShowTitle(true);

            cpapinfo+="\n\n";

            painter.setFont(report_font);
            //bounds=painter.boundingRect(QRectF((virt_width/2)-(virt_width/6),top,virt_width/2,0),cpapinfo,QTextOption(Qt::AlignLeft));
            bounds=painter.boundingRect(QRectF(0,top,virt_width,0),cpapinfo,QTextOption(Qt::AlignHCenter));
            painter.drawText(bounds,cpapinfo,QTextOption(Qt::AlignHCenter));

            int ttop=bounds.height();

            stats=tr("AI=%1 HI=%2 CAI=%3 ").arg(oai,0,'f',2).arg(hi,0,'f',2).arg(cai,0,'f',2);
            if (cpap->machine->GetClass()==STR_MACH_PRS1) {
                stats+=tr("REI=%1 VSI=%2 FLI=%3 PB/CSR=%4\%")
                        .arg(rei,0,'f',2).arg(vsi,0,'f',2)
                        .arg(fli,0,'f',2).arg(csr,0,'f',2);
            } else if (cpap->machine->GetClass()==STR_MACH_ResMed) {
                stats+=tr("UAI=%1 ").arg(uai,0,'f',2);
            } else if (cpap->machine->GetClass()==STR_MACH_Intellipap) {
                stats+=tr("NRI=%1 LKI=%2 EPI=%3").arg(nri,0,'f',2).arg(lki,0,'f',2).arg(exp,0,'f',2);
            }
            bounds=painter.boundingRect(QRectF(0,top+ttop,virt_width,0),stats,QTextOption(Qt::AlignHCenter));
            painter.drawText(bounds,stats,QTextOption(Qt::AlignHCenter));
            ttop+=bounds.height();

            if (journal) {
                stats="";
                if (journal->settings.contains(Journal_Weight))
                    stats+=tr("Weight %1 ").arg(weightString(journal->settings[Journal_Weight].toDouble()));
                if (journal->settings.contains(Journal_BMI))
                    stats+=tr("BMI %1 ").arg(journal->settings[Journal_BMI].toDouble(),0,'f',2);
                if (journal->settings.contains(Journal_ZombieMeter))
                    stats+=tr("Zombie %1/10 ").arg(journal->settings[Journal_ZombieMeter].toDouble(),0,'f',0);

                if (!stats.isEmpty()) {
                    bounds=painter.boundingRect(QRectF(0,top+ttop,virt_width,0),stats,QTextOption(Qt::AlignHCenter));

                    painter.drawText(bounds,stats,QTextOption(Qt::AlignHCenter));
                    ttop+=bounds.height();
                }
                ttop+=normal_height;
                if (journal->settings.contains(Journal_Notes)) {
                    QTextDocument doc;
                    doc.setHtml(journal->settings[Journal_Notes].toString());
                    stats=doc.toPlainText();
                    //doc.drawContents(&painter); // doesn't work as intended..

                    bounds=painter.boundingRect(QRectF(0,top+ttop,virt_width,0),stats,QTextOption(Qt::AlignHCenter));
                    painter.drawText(bounds,stats,QTextOption(Qt::AlignHCenter));
                    bounds.setLeft(virt_width/4);
                    bounds.setRight(virt_width-(virt_width/4));

                    QPen pen(Qt::black);
                    pen.setWidth(4);
                    painter.setPen(pen);
                    painter.drawRect(bounds);
                    ttop+=bounds.height()+normal_height;
                }
            }


            if (ttop>maxy) maxy=ttop;
        } else {
            bounds=painter.boundingRect(QRectF(0,top+maxy,virt_width,0),cpapinfo,QTextOption(Qt::AlignCenter));
            painter.drawText(bounds,cpapinfo,QTextOption(Qt::AlignCenter));
            if (maxy+bounds.height()>maxy) maxy=maxy+bounds.height();
        }
    } else if (name==STR_TR_Overview) {
        QDateTime first=QDateTime::fromTime_t((*gv)[0]->min_x/1000L);
        QDateTime last=QDateTime::fromTime_t((*gv)[0]->max_x/1000L);
        QString ovinfo=tr("Reporting from %1 to %2").arg(first.date().toString(Qt::SystemLocaleShortDate)).arg(last.date().toString(Qt::SystemLocaleShortDate));
        QRectF bounds=painter.boundingRect(QRectF(0,top,virt_width,0),ovinfo,QTextOption(Qt::AlignHCenter));
        painter.drawText(bounds,ovinfo,QTextOption(Qt::AlignHCenter));

        if (bounds.height()>maxy) maxy=bounds.height();
    } else if (name==STR_TR_Oximetry) {
        QString ovinfo=tr("Reporting data goes here");
        QRectF bounds=painter.boundingRect(QRectF(0,top,virt_width,0),ovinfo,QTextOption(Qt::AlignHCenter));
        painter.drawText(bounds,ovinfo,QTextOption(Qt::AlignHCenter));

        if (bounds.height()>maxy) maxy=bounds.height();
    }
    top+=maxy;

    graph_slots=graphs_per_page-((virt_height-top)/(full_graph_height+normal_height));

    bool first=true;
    QStringList labels;
    QVector<gGraph *> graphs;
    QVector<qint64> start,end;
    qint64 savest,saveet;
    gv->GetXBounds(savest,saveet);
    qint64 st=savest,et=saveet;

    gGraph *g;
    if (name==STR_TR_Daily) {
        if (!print_bookmarks) {
            for (int i=0;i<gv->size();i++) {
                g=(*gv)[i];
                if (g->isEmpty()) continue;
                if (!g->visible()) continue;

                if (cpap) {
                    st=cpap->first();
                    et=cpap->last();
                } else if (oxi) {
                    st=oxi->first();
                    et=oxi->last();
                }

                if (g->title()==STR_TR_FlowRate) {
                    if (!((qAbs(savest-st)<2000) && (qAbs(saveet-et)<2000))) {
                        start.push_back(st);
                        end.push_back(et);
                        graphs.push_back(g);
                        labels.push_back(tr("Entire Day's Flow Waveform"));
                    }
                    start.push_back(savest);
                    end.push_back(saveet);
                    graphs.push_back(g);
                    labels.push_back("Current Selection");
                } else {
                    start.push_back(savest);
                    end.push_back(saveet);
                    graphs.push_back(g);
                    labels.push_back("");
                }

            }
        } else {
            if (journal) {
                if (journal->settings.contains(Bookmark_Start)) {
                    QVariantList st1=journal->settings[Bookmark_Start].toList();
                    QVariantList et1=journal->settings[Bookmark_End].toList();
                    QStringList notes=journal->settings[Bookmark_Notes].toStringList();
                    gGraph *flow=gv->findGraph(STR_TR_FlowRate),
                           *spo2=gv->findGraph(STR_TR_SpO2),
                           *pulse=gv->findGraph(STR_TR_PulseRate);


                    if (cpap && flow && !flow->isEmpty() && flow->visible()) {
                        labels.push_back("Entire Day");
                        start.push_back(cpap->first());
                        end.push_back(cpap->last());
                        graphs.push_back(flow);
                    }
                    if (oxi && spo2 && !spo2->isEmpty() && spo2->visible()) {
                        labels.push_back("Entire Day");
                        start.push_back(oxi->first());
                        end.push_back(oxi->last());
                        graphs.push_back(spo2);
                    }
                    if (oxi && pulse && !pulse->isEmpty() && pulse->visible()) {
                        labels.push_back("Entire Day");
                        start.push_back(oxi->first());
                        end.push_back(oxi->last());
                        graphs.push_back(pulse);
                    }
                    for (int i=0;i<notes.size();i++) {
                        if (flow && !flow->isEmpty() && flow->visible()) {
                            labels.push_back(notes.at(i));
                            start.push_back(st1.at(i).toLongLong());
                            end.push_back(et1.at(i).toLongLong());
                            graphs.push_back(flow);
                        }
                        if (spo2 && !spo2->isEmpty() && spo2->visible()) {
                            labels.push_back(notes.at(i));
                            start.push_back(st1.at(i).toLongLong());
                            end.push_back(et1.at(i).toLongLong());
                            graphs.push_back(spo2);
                        }
                        if (pulse && !pulse->isEmpty() && pulse->visible()) {
                            labels.push_back(notes.at(i));
                            start.push_back(st1.at(i).toLongLong());
                            end.push_back(et1.at(i).toLongLong());
                            graphs.push_back(pulse);
                        }
                    }
                }
            }
            for (int i=0;i<gv->size();i++) {
                gGraph *g=(*gv)[i];
                if (g->isEmpty()) continue;
                if (!g->visible()) continue;
                if ((g->title()!=STR_TR_FlowRate ) && (g->title()!=STR_TR_SpO2) && (g->title()!=STR_TR_PulseRate)) {
                    start.push_back(st);
                    end.push_back(et);
                    graphs.push_back(g);
                    labels.push_back(tr(""));
                }
            }
        }
    } else {
        for (int i=0;i<gv->size();i++) {
            gGraph *g=(*gv)[i];
            if (g->isEmpty()) continue;
            if (!g->visible()) continue;

            start.push_back(st);
            end.push_back(et);
            graphs.push_back(g);
            labels.push_back(""); // date range?
        }
    }
    int pages=ceil(float(graphs.size()+graph_slots)/float(graphs_per_page));

    if (qprogress) {
        qprogress->setValue(0);
        qprogress->setMaximum(graphs.size());
        qprogress->show();
    }

    int page=1;
    int gcnt=0;
    for (int i=0;i<graphs.size();i++) {

        if ((top+full_graph_height+normal_height) > virt_height) {
            top=0;
            gcnt=0;
            first=true;
            if (page > pages)
                break;

            if (!printer->newPage()) {
                qWarning("failed in flushing page to disk, disk full?");
                break;
            }


        }
        if (first) {
            QString footer=tr("SleepyHead v%1 - http://sleepyhead.sourceforge.net").arg(VersionString);

            QRectF bounds=painter.boundingRect(QRectF(0,virt_height,virt_width,normal_height),footer,QTextOption(Qt::AlignHCenter));
            painter.drawText(bounds,footer,QTextOption(Qt::AlignHCenter));

            QString pagestr=tr("Page %1 of %2").arg(page).arg(pages);
            QRectF pagebnds=painter.boundingRect(QRectF(0,virt_height,virt_width,normal_height),pagestr,QTextOption(Qt::AlignRight));
            painter.drawText(pagebnds,pagestr,QTextOption(Qt::AlignRight));
            first=false;
            page++;
        }

        gGraph *g=graphs[i];
        g->SetXBounds(start[i],end[i]);
        g->deselect();

        QString label=labels[i];
        if (!label.isEmpty()) {
            //label+=":";
            top+=normal_height/3;
            QRectF bounds=painter.boundingRect(QRectF(0,top,virt_width,0),label,QTextOption(Qt::AlignHCenter));
            //QRectF pagebnds=QRectF(0,top,virt_width,normal_height);
            painter.drawText(bounds,label,QTextOption(Qt::AlignHCenter));
            top+=bounds.height();
        } else top+=normal_height/2;

        PROFILE.appearance->setAntiAliasing(force_antialiasing);
        int tmb=g->m_marginbottom;
        g->m_marginbottom=0;


        //painter.beginNativePainting();
        //g->showTitle(false);
        int hhh=full_graph_height-normal_height;
        QPixmap pm2=g->renderPixmap(virt_width,hhh,1);
        QImage pm=pm2.toImage();//fscale);
        pm2.detach();
        //g->showTitle(true);
        //painter.endNativePainting();
        g->m_marginbottom=tmb;
        PROFILE.appearance->setAntiAliasing(aa_setting);


        if (!pm.isNull()) {
            painter.drawImage(0,top,pm);;



            //painter.drawImage(0,top,virt_width,full_graph_height-normal_height,pm);
        }
        top+=full_graph_height;

        gcnt++;
        if (qprogress) {
            qprogress->setValue(i);
            QApplication::processEvents();
        }
    }

    gv->SetXBounds(savest,saveet);
    qprogress->hide();
    painter.end();
    delete printer;
    Notify("SleepyHead has finished sending the job to the printer.");
}

void packEventList(EventList *el, EventDataType minval=0)
{
    if (el->count()<2) return;
    EventList nel(EVL_Waveform);
    EventDataType t,lastt=0; //el->data(0);
    qint64 ti;//=el->time(0);
    //nel.AddEvent(ti,lastt);
    bool f;
    qint64 lasttime=0;
    EventDataType min=999,max=0;
    for (quint32 i=0;i<el->count();i++) {
        t=el->data(i);
        ti=el->time(i);
        f=false;
        if (t>minval) {
            if (t!=lastt) {
                if (!lasttime) {
                    nel.setFirst(ti);
                }
                nel.AddEvent(ti,t);
                if (t < min) min=t;
                if (t > max) max=t;
                lasttime=ti;
                f=true;
            }
        } else {
            if (lastt>minval) {
                nel.AddEvent(ti,lastt);
                lasttime=ti;
                f=true;
            }
        }
        lastt=t;
    }
    if (!f) {
        if (t>minval) {
            nel.AddEvent(ti,t);
            lasttime=ti;
        }
    }
    el->setFirst(nel.first());
    el->setLast(nel.last());
    el->setMin(min);
    el->setMax(max);

    el->getData().clear();
    el->getTime().clear();
    el->setCount(nel.count());

    el->getData()=nel.getData();
    el->getTime()=nel.getTime();
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

    QList<Machine *> machines=PROFILE.GetMachines(MT_OXIMETER);

    qint64 f=0,l=0;

    int discard_threshold=PROFILE.oxi->oxiDiscardThreshold();
    Machine *m;
    for (int z=0;z<machines.size();z++) {
        m=machines.at(z);
        //m->sessionlist.erase(m->sessionlist.find(0));

        // For each Session
        for (QHash<SessionID,Session *>::iterator s=m->sessionlist.begin();s!=m->sessionlist.end();s++) {
            Session *sess=s.value();
            if (!sess) continue;
            sess->OpenEvents();

            // For each EventList contained in session
            invalid.clear();
            f=0;
            l=0;
            for (QHash<ChannelID,QVector<EventList *> >::iterator e=sess->eventlist.begin();e!=sess->eventlist.end();e++) {

                // Discard any non data events.
                if (!valid.contains(e.key())) {
                    // delete and push aside for later to clean up
                    for (int i=0;i<e.value().size();i++)  {
                        delete e.value()[i];
                    }
                    e.value().clear();
                    invalid.push_back(e.key());
                } else {
                    // Valid event
                    QVector<EventList *> newlist;
                    for (int i=0;i<e.value().size();i++)  {
                        if (e.value()[i]->count() > (unsigned)discard_threshold) {
                            newlist.push_back(e.value()[i]);
                        } else {
                            delete e.value()[i];
                        }
                    }
                    for (int i=0;i<newlist.size();i++) {
                        packEventList(newlist[i],8);

                        EventList * el=newlist[i];
                        if (!f || f > el->first()) f=el->first();
                        if (!l || l < el->last()) l=el->last();
                    }
                    e.value()=newlist;
                }
            }
            for (int i=0;i<invalid.size();i++) {
                sess->eventlist.erase(sess->eventlist.find(invalid[i]));
            }
            if (f) sess->really_set_first(f);
            if (l) sess->really_set_last(l);
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
    for (int i=0;i<machines.size();i++) {
        Machine *m=machines[i];
        m->Save();
    }
    getDaily()->ReloadGraphs();
    getOverview()->ReloadGraphs();
}

void MainWindow::RestartApplication(bool force_login)
{
    QString apppath;
#ifdef Q_OS_MAC
        // In Mac OS the full path of aplication binary is:
        //    <base-path>/myApp.app/Contents/MacOS/myApp

        // prune the extra bits to just get the app bundle path
        apppath=QApplication::instance()->applicationDirPath().section("/",0,-3);

        QStringList args;
        args << "-n"; // -n option is important, as it opens a new process
        args << apppath;

        args << "--args"; // SleepyHead binary options after this
        args << "-p"; // -p starts with 1 second delay, to give this process time to save..


        if (force_login) args << "-l";

        if (QProcess::startDetached("/usr/bin/open",args)) {
            QApplication::instance()->exit();
        } else QMessageBox::warning(this,tr("Gah!"),tr("If you can read this, the restart command didn't work. Your going to have to do it yourself manually."),QMessageBox::Ok);

#else
        apppath=QApplication::instance()->applicationFilePath();

        // If this doesn't work on windoze, try uncommenting this method
        // Technically should be the same thing..

        //if (QDesktopServices::openUrl(apppath)) {
        //    QApplication::instance()->exit();
        //} else
        QStringList args;
        args << "-p";
        if (force_login) args << "-l";
        if (QProcess::startDetached(apppath,args)) {
            QApplication::instance()->exit();
        } else QMessageBox::warning(this,tr("Gah!"),tr("If you can read this, the restart command didn't work. Your going to have to do it yourself manually."),QMessageBox::Ok);
#endif
}

void MainWindow::on_actionChange_User_triggered()
{
    PROFILE.Save();
    PREF.Save();
    RestartApplication(true);
}

void MainWindow::on_actionPurge_Current_Day_triggered()
{
    QDate date=getDaily()->getDate();
    Day *day=PROFILE.GetDay(date,MT_CPAP);
    Machine *m;
    if (day) {
        m=day->machine;
        QString path=PROFILE.Get("{"+STR_GEN_DataFolder+"}/")+m->GetClass()+"_"+m->hexid()+"/";

        QVector<Session *>::iterator s;

        for (s=day->begin();s!=day->end();s++) {
            SessionID id=(*s)->session();
            QString filename0=path+QString().sprintf("%08lx.000",id);
            QString filename1=path+QString().sprintf("%08lx.001",id);
            qDebug() << "Removing" << filename0;
            qDebug() << "Removing" << filename1;
            QFile::remove(filename0);
            QFile::remove(filename1);
            m->sessionlist.erase(m->sessionlist.find(id)); // remove from machines session list
        }
        QList<Day *> & dl=PROFILE.daylist[date];
        QList<Day *>::iterator it;//=dl.begin();
        for (it=dl.begin();it!=dl.end();it++) {
            if ((*it)==day) break;
        }
        if (it!=dl.end()) {
            dl.erase(it);
            //PROFILE.daylist[date].  // ??
            delete day;
        }
    }
    getDaily()->ReloadGraphs();
}

void MainWindow::on_actionAll_Data_for_current_CPAP_machine_triggered()
{
    QDate date=getDaily()->getDate();
    Day *day=PROFILE.GetDay(date,MT_CPAP);
    Machine *m;
    if (day) {
        m=day->machine;
        if (!m) {
            qDebug() << "Gah!! no machine to purge";
            return;
        }
        if (QMessageBox::question(this,tr("Are you sure?"),tr("Are you sure you want to purge all CPAP data for the following machine:\n")+m->properties[STR_PROP_Brand]+" "+m->properties[STR_PROP_Model]+" "+m->properties[STR_PROP_ModelNumber]+" ("+m->properties[STR_PROP_Serial]+")",QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes) {
            m->Purge(3478216);
            PROFILE.machlist.erase(PROFILE.machlist.find(m->id()));
            // delete or not to delete.. this needs to delete later.. :/
            //delete m;
            RestartApplication();
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent * event)
{
    Q_UNUSED(event)
    //qDebug() << "Keypress:" << event->key();
}

void MainWindow::on_summaryButton_2_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->summaryTab);
    on_summaryButton_clicked();
}

void MainWindow::on_action_Sidebar_Toggle_toggled(bool visible)
{
    ui->toolBox->setVisible(visible);
}

void MainWindow::on_recordsBox_linkClicked(const QUrl &linkurl)
{
    QString link=linkurl.toString().section("=",0,0).toLower();
    QString datestr=linkurl.toString().section("=",1).toLower();
    qDebug() << linkurl.toString() << link << datestr;
    if (link=="daily") {
        QDate date=QDate::fromString(datestr,Qt::ISODate);
        daily->LoadDate(date);
        ui->tabWidget->setCurrentWidget(daily);
    } else if (link=="overview") {
        QString date1=datestr.section(",",0,0);
        QString date2=datestr.section(",",1);

        QDate d1=QDate::fromString(date1,Qt::ISODate);
        QDate d2=QDate::fromString(date2,Qt::ISODate);
        overview->setRange(d1,d2);
        ui->tabWidget->setCurrentWidget(overview);
    }

}

void MainWindow::on_helpButton_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->helpTab);
}

void MainWindow::on_actionView_S_ummary_triggered()
{
    ui->tabWidget->setCurrentWidget(ui->summaryTab);
}

void MainWindow::on_webView_linkClicked(const QUrl &url)
{
    QString s=url.toString();
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

void MainWindow::LinkHovered(const QString & link, const QString & title, const QString & textContent)
{
    Q_UNUSED(title);
    Q_UNUSED(textContent);
    ui->statusbar->showMessage(link);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index);
    QWidget *widget=ui->tabWidget->currentWidget();
    if ((widget==ui->summaryTab) || (widget==ui->helpTab)) {
        qstatus2->setVisible(false);
    } else if (widget==daily) {
        qstatus2->setVisible(true);
        daily->graphView()->selectionTime();
    } else if (widget==overview) {
        qstatus2->setVisible(true);
        overview->graphView()->selectionTime();
    } else if (widget==oximetry) {
        qstatus2->setVisible(true);
        oximetry->graphView()->selectionTime();
    }
}

void MainWindow::on_summaryView_linkClicked(const QUrl &arg1)
{
    //qDebug() << arg1;
    on_recordsBox_linkClicked(arg1);
}

void MainWindow::on_bookmarkView_linkClicked(const QUrl &arg1)
{
    on_recordsBox_linkClicked(arg1);
}

void MainWindow::on_filterBookmarks_editingFinished()
{
    bookmarkFilter=ui->filterBookmarks->text();
    updateFavourites();
}

void MainWindow::on_filterBookmarksButton_clicked()
{
    if (!bookmarkFilter.isEmpty()) {
        ui->filterBookmarks->setText("");
        bookmarkFilter="";
        updateFavourites();
    }
}

void MainWindow::reprocessEvents(bool restart)
{
    m_restartRequired=restart;
    QTimer::singleShot(0,this,SLOT(doReprocessEvents()));
}


void MainWindow::FreeSessions()
{
    QDate first=PROFILE.FirstDay();
    QDate date=PROFILE.LastDay();
    Day *day;
    QDate current=daily->getDate();
    do {
        day=PROFILE.GetDay(date,MT_CPAP);
        if (day) {
            if (date!=current) {
                day->CloseEvents();
            }
        }
        date=date.addDays(-1);
    }  while (date>=first);
}

void MainWindow::doReprocessEvents()
{
    if (PROFILE.countDays(MT_CPAP,PROFILE.FirstDay(),PROFILE.LastDay())==0)
        return;

    m_inRecalculation=true;
    QDate first=PROFILE.FirstDay();
    QDate date=PROFILE.LastDay();
    Session *sess;
    Day *day;
    //FlowParser flowparser;

    mainwin->Notify("Performance will be degraded during these recalculations.","Recalculating Indices");

    // For each day in history
    int daycount=first.daysTo(date);
    int idx=0;

    QList<Machine *> machines=PROFILE.GetMachines(MT_CPAP);

    // Disabling multithreaded save as it appears it's causing problems
    bool cache_sessions=false; //PROFILE.session->cacheSessions();
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
        day=PROFILE.GetDay(date,MT_CPAP);
        if (day) {
            for (int i=0;i<day->size();i++) {
                sess=(*day)[i];
                isopen=sess->eventsLoaded();

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

                sess->SetChanged(true);
                if (!cache_sessions) {
                    sess->UpdateSummaries();
                    sess->machine()->SaveSession(sess);
                    if (!isopen)
                        sess->TrashEvents();
                }
            }
        }
        date=date.addDays(-1);
       // if (qprogress && (++idx % 10) ==0) {
            qprogress->setValue(0+(float(++idx)/float(daycount)*100.0));
            QApplication::processEvents();
       // }

    } while (date>=first);

    if (cache_sessions) {
        qstatus->setText(tr("Recalculating Summaries"));
        for (int i=0;i<machines.size();i++) {
            machines.at(i)->Save();
        }
    }

    qstatus->setText(tr(""));
    qprogress->setVisible(false);
    m_inRecalculation=false;
    if (m_restartRequired) {
        QMessageBox::information(this,"Restart Required",QString("Recalculations are complete, the application now needs to restart to display the changes."),QMessageBox::Ok);
        RestartApplication();
        return;
    } else {
        Notify("Recalculations are now complete.","Task Completed");

        FreeSessions();
        QDate current=daily->getDate();
        daily->LoadDate(current);
        if (overview) overview->ReloadGraphs();
    }
}

void MainWindow::on_actionImport_ZEO_Data_triggered()
{
    QFileDialog w;
    w.setFileMode(QFileDialog::ExistingFiles);
    w.setOption(QFileDialog::ShowDirsOnly, false);
    w.setOption(QFileDialog::DontUseNativeDialog,true);
    w.setFilters(QStringList("Zeo CSV File (*.csv)"));

    ZEOLoader zeo;
    if (w.exec()==QFileDialog::Accepted) {
        QString filename=w.selectedFiles()[0];
        if (!zeo.OpenFile(filename)) {
            Notify("There was a problem opening ZEO File: "+filename);
            return;
        }
        Notify("Zeo CSV Import complete");
        daily->LoadDate(daily->getDate());
    }


}

void MainWindow::on_actionImport_RemStar_MSeries_Data_triggered()
{
    QFileDialog w;
    w.setFileMode(QFileDialog::ExistingFiles);
    w.setOption(QFileDialog::ShowDirsOnly, false);
    w.setOption(QFileDialog::DontUseNativeDialog,true);
    w.setFilters(QStringList("M-Series data file (*.bin)"));

    MSeriesLoader mseries;
    if (w.exec()==QFileDialog::Accepted) {
        QString filename=w.selectedFiles()[0];
        if (!mseries.Open(filename,p_profile)) {
            Notify("There was a problem opening MSeries block File: "+filename);
            return;
        }
        Notify("MSeries Import complete");
        daily->LoadDate(daily->getDate());
    }
}

void MainWindow::on_actionSleep_Disorder_Terms_Glossary_triggered()
{
    ui->webView->load(QUrl("http://sourceforge.net/apps/mediawiki/sleepyhead/index.php?title=Glossary"));
    ui->tabWidget->setCurrentWidget(ui->helpTab);
}

void MainWindow::on_actionHelp_Support_Sleepyhead_Development_triggered()
{
    QUrl url=QUrl("http://sourceforge.net/apps/mediawiki/sleepyhead/index.php?title=Support_SleepyHead_Development");
    QDesktopServices().openUrl(url);
//    ui->webView->load(url);
//    ui->tabWidget->setCurrentWidget(ui->helpTab);
}
