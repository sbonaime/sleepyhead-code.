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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "newprofile.h"
#include "exportcsv.h"
#include "SleepLib/schema.h"
#include "Graphs/glcommon.h"
#include "UpdaterWindow.h"
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

    QString version=VersionString;
    if (QString(GIT_BRANCH)!="master") version+=QString(" ")+QString(GIT_BRANCH);
    this->setWindowTitle(tr("SleepyHead")+QString(" v%1 (Profile: %2)").arg(version).arg(PREF[STR_GEN_Profile].toString()));
    //ui->tabWidget->setCurrentIndex(1);

    overview=NULL;
    daily=NULL;
    oximetry=NULL;
    prefdialog=NULL;

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
        systray=new QSystemTrayIcon(QIcon(":/docs/sheep.png"),this);
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

    ui->recordsBox->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
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
    qDebug() << PREF["AppName"].toString().toAscii()+" v"+VersionString.toAscii() << "built with Qt"<< QT_VERSION_STR << "on" << __DATE__ << __TIME__;
    qstatus->setText(tr("Loading Data"));
    qprogress->show();
    //qstatusbar->showMessage(tr("Loading Data"),0);

    // profile is a global variable set in main after login
    PROFILE.LoadMachineData();

    SnapshotGraph=new gGraphView(this); //daily->graphView());
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

    QStringList importFrom=importLocations;

    if (asknew) {
        QFileDialog w;
        w.setFileMode(QFileDialog::Directory);
        w.setOption(QFileDialog::ShowDirsOnly, true);

#if defined(Q_WS_MAC) && (QT_VERSION_CHECK(4,8,0) > QT_VERSION)
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
    }

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
        if (daily) daily->ReloadGraphs();
        if (overview) overview->ReloadGraphs();
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
        on_summaryButton_clicked();
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
    return QString("<html><head>"
"</head>"
"<style type='text/css'>"
"<!--h1,p,a,td,body { font-family: 'FreeSans', 'Sans Serif' } --/>"
"p,a,td,body { font-size: 14px }"
"a:link,a:visited { color: '#000020'; text-decoration: none; font-weight: bold;}"
"a:hover { background-color: inherit; color: red; text-decoration:none; font-weight: bold; }"
"</style>"
"</head>"
"<body leftmargin=0 topmargin=0 rightmargin=0>"
"<div align=center><table cellpadding=3 cellspacing=0 border=0>"
"<tr><td><img src='qrc:/docs/sheep.png' width=100px height=100px><td valign=center align=center><h1>SleepyHead v"+VersionString+"</h1><i>This is a beta software and some functionality may not work as intended yet.<br/>Please report any bugs you find to SleepyHead's SourceForge page.</i></td></tr></table>"
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
    val/=p_profile->calcHours(MT_CPAP,start,end);

    return val;
}

struct RXChange
{
    RXChange() { highlight=0; }
    RXChange(const RXChange & copy) {
        first=copy.first;
        last=copy.last;
        days=copy.days;
        ahi=copy.ahi;
        mode=copy.mode;
        min=copy.min;
        max=copy.max;
        per1=copy.per1;
        per2=copy.per2;
        highlight=copy.highlight;
        weighted=copy.weighted;
    }
    QDate first;
    QDate last;
    int days;
    EventDataType ahi;
    CPAPMode mode;
    EventDataType min;
    EventDataType max;
    EventDataType per1;
    EventDataType per2;
    EventDataType weighted;
    short highlight;
};

enum RXSortMode { RX_first, RX_last, RX_days, RX_ahi, RX_mode, RX_min, RX_max, RX_per1, RX_per2, RX_weighted };
RXSortMode RXsort=RX_first;
bool RXorder=false;

bool operator<(const RXChange & comp1, const RXChange & comp2) {
    if (RXorder) {
        switch (RXsort) {
        case RX_ahi: return comp1.ahi < comp2.ahi;
        case RX_days: return comp1.days < comp2.days;
        case RX_first: return comp1.first < comp2.first;
        case RX_last: return comp1.last < comp2.last;
        case RX_mode: return comp1.mode < comp2.mode;
        case RX_min:  return comp1.min < comp2.min;
        case RX_max:  return comp1.max < comp2.max;
        case RX_per1:  return comp1.per1 < comp2.per1;
        case RX_per2:  return comp1.per2 < comp2.per2;
        case RX_weighted:  return comp1.weighted < comp2.weighted;
        };
    } else {
        switch (RXsort) {
        case RX_ahi: return comp1.ahi > comp2.ahi;
        case RX_days: return comp1.days > comp2.days;
        case RX_first: return comp1.first > comp2.first;
        case RX_last: return comp1.last > comp2.last;
        case RX_mode: return comp1.mode > comp2.mode;
        case RX_min:  return comp1.min > comp2.min;
        case RX_max:  return comp1.max > comp2.max;
        case RX_per1:  return comp1.per1 > comp2.per1;
        case RX_per2:  return comp1.per2 > comp2.per2;
        case RX_weighted:  return comp1.weighted > comp2.weighted;
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
        case RX_per1:  return comp1->per1 > comp2->per1;
        case RX_per2:  return comp1->per2 > comp2->per2;
        case RX_weighted:  return comp1->weighted > comp2->weighted;
        };
    }
    return true;
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

    QDate lastcpap=p_profile->LastDay(MT_CPAP);
    QDate firstcpap=p_profile->FirstDay(MT_CPAP);
    QDate cpapweek=lastcpap.addDays(-7);
    QDate cpapmonth=lastcpap.addDays(-30);
    QDate cpap6month=lastcpap.addMonths(-6);
    QDate cpapyear=lastcpap.addYears(-12);
    if (cpapweek<firstcpap) cpapweek=firstcpap;
    if (cpapmonth<firstcpap) cpapmonth=firstcpap;
    if (cpap6month<firstcpap) cpap6month=firstcpap;
    if (cpapyear<firstcpap) cpapyear=firstcpap;

    int cpapweekdays=cpapweek.daysTo(lastcpap);
    int cpapmonthdays=cpapmonth.daysTo(lastcpap);
    int cpapyeardays=cpapyear.daysTo(lastcpap);
    int cpap6monthdays=cpap6month.daysTo(lastcpap);
    QList<Machine *> cpap_machines=PROFILE.GetMachines(MT_CPAP);
    QList<Machine *> oximeters=PROFILE.GetMachines(MT_OXIMETER);
    QList<Machine *> mach;
    mach.append(cpap_machines);
    mach.append(oximeters);


    if (mach.size()==0) {
        html+="<table cellpadding=2 cellspacing=0 border=0 width=100% height=70%>";
        html+="<tr><td align=center><h1>Please Import Some Data</h1><br/><i>SleepyHead is pretty much useless without it.</i></td></tr></table>";
        html+=htmlFooter();
        ui->summaryView->setHtml(html);
        return;
    }
    int cpapdays=PROFILE.countDays(MT_CPAP,firstcpap,lastcpap);
    CPAPMode cpapmode=(CPAPMode)p_profile->calcSettingsMax(CPAP_Mode,MT_CPAP,firstcpap,lastcpap);

    float percentile=0.95;

    QString ahitxt;
    if (PROFILE.general->calculateRDI()) {
        ahitxt=tr("RDI");
    } else {
        ahitxt=tr("AHI");
    }
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
            .arg(calcAHI(lastcpap,lastcpap),0,'f',3)
            .arg(calcAHI(cpapweek,lastcpap),0,'f',3)
            .arg(calcAHI(cpapmonth,lastcpap),0,'f',3)
            .arg(calcAHI(cpap6month,lastcpap),0,'f',3)
            .arg(calcAHI(cpapyear,lastcpap),0,'f',3);

            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(tr("Hours per Night"))
            .arg(formatTime(p_profile->calcHours(MT_CPAP)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpapweek,lastcpap)/float(cpapweekdays)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpapmonth,lastcpap)/float(cpapmonthdays)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpap6month,lastcpap)/float(cpap6monthdays)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpapyear,lastcpap)/float(cpapyeardays)));

            if (cpapmode<MODE_BIPAP) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Average Pressure"))
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP),0,'f',3)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapweek,lastcpap),0,'f',3)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpap6month,lastcpap),0,'f',3)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapyear,lastcpap),0,'f',3);
            } else if (cpapmode>MODE_CPAP) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("95% Pressure"))
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',3);
            } else {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Min EPAP"))
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP),0,'f',3)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpapweek,lastcpap),0,'f',3)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpap6month,lastcpap),0,'f',3)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpapyear,lastcpap),0,'f',3);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("95% EPAP"))
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',3);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Max IPAP"))
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP),0,'f',3)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpapweek,lastcpap),0,'f',3)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpap6month,lastcpap),0,'f',3)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpapyear,lastcpap),0,'f',3);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("95% IPAP"))
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',3)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',3);
            }
            //html+="<tr><td colspan=6>TODO: 90% pressure.. Any point showing if this is all CPAP?</td></tr>";


            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(tr("Average Leaks"))
            .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP),0,'f',3)
            .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP,cpapweek,lastcpap),0,'f',3)
            .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
            .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP,cpap6month,lastcpap),0,'f',3)
            .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP,cpapyear,lastcpap),0,'f',3);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(tr("Median Leaks"))
            .arg(p_profile->calcPercentile(CPAP_Leak,0.5,MT_CPAP),0,'f',3)
            .arg(p_profile->calcPercentile(CPAP_Leak,0.5,MT_CPAP,cpapweek,lastcpap),0,'f',3)
            .arg(p_profile->calcPercentile(CPAP_Leak,0.5,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
            .arg(p_profile->calcPercentile(CPAP_Leak,0.5,MT_CPAP,cpap6month,lastcpap),0,'f',3)
            .arg(p_profile->calcPercentile(CPAP_Leak,0.5,MT_CPAP,cpapyear,lastcpap),0,'f',3);
            html+="<tr><td colspan=6>Note, AHI calcs here are different to overview calcs.. Overview shows a average of the dialy AHI's, this shows combined counts divide by combined hours</td></tr>";
        }
    }
    int oxisize=oximeters.size();
    if (oxisize>0) {
        QDate lastoxi=p_profile->LastDay(MT_OXIMETER);
        QDate firstoxi=p_profile->FirstDay(MT_OXIMETER);
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
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER),0,'f',3)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oxiweek,lastoxi),0,'f',3)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oximonth,lastoxi),0,'f',3)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oxi6month,lastoxi),0,'f',3)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oxiyear,lastoxi),0,'f',3);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Minimum SpO2"))
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER),0,'f',3)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oxiweek,lastoxi),0,'f',3)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oximonth,lastoxi),0,'f',3)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oxi6month,lastoxi),0,'f',3)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oxiyear,lastoxi),0,'f',3);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("SpO2 Events / Hour"))
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER)/p_profile->calcHours(MT_OXIMETER),0,'f',3)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oxiweek,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiweek,lastoxi),0,'f',3)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oximonth,lastoxi)/p_profile->calcHours(MT_OXIMETER,oximonth,lastoxi),0,'f',3)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oxi6month,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxi6month,lastoxi),0,'f',3)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oxiyear,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiyear,lastoxi),0,'f',3);
            html+=QString("<tr><td>%1</td><td>%2\%</td><td>%3\%</td><td>%4\%</td><td>%5\%</td><td>%6\%</td></tr>")
                .arg(tr("% of time in SpO2 Events"))
                .arg(100.0/p_profile->calcHours(MT_OXIMETER)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER)/3600.0,0,'f',3)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oxiweek,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oxiweek,lastoxi)/3600.0,0,'f',3)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oximonth,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oximonth,lastoxi)/3600.0,0,'f',3)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oxi6month,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oxi6month,lastoxi)/3600.0,0,'f',3)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oxiyear,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oxiyear,lastoxi)/3600.0,0,'f',3);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Average Pulse Rate"))
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER),0,'f',3)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oxiweek,lastoxi),0,'f',3)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oximonth,lastoxi),0,'f',3)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oxi6month,lastoxi),0,'f',3)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oxiyear,lastoxi),0,'f',3);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Minimum Pulse Rate"))
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER),0,'f',3)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oxiweek,lastoxi),0,'f',3)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oximonth,lastoxi),0,'f',3)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oxi6month,lastoxi),0,'f',3)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oxiyear,lastoxi),0,'f',3);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Maximum Pulse Rate"))
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER),0,'f',3)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oxiweek,lastoxi),0,'f',3)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oximonth,lastoxi),0,'f',3)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oxi6month,lastoxi),0,'f',3)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oxiyear,lastoxi),0,'f',3);
        }
    }

    html+="</table>";
    html+="</div>";

    QDate bestAHIdate, worstAHIdate;
    EventDataType bestAHI=999.0, worstAHI=0;
    if (cpapdays>0) {
        QDate first,last=lastcpap;
        CPAPMode mode,cmode=MODE_UNKNOWN;
        EventDataType cmin=0,cmax=0,min,max;
        QDate date=lastcpap;
        Day * day;
        bool lastchanged;
        int cnt=0;
        QVector<RXChange> rxchange;
        do {
            day=PROFILE.GetDay(date,MT_CPAP);

            lastchanged=false;
            if (day) {

                EventDataType hours=day->hours();

                if (hours>PROFILE.cpap->complianceHours()) {
                    EventDataType ahi=day->count(CPAP_Obstructive)+day->count(CPAP_Hypopnea)+day->count(CPAP_Apnea)+day->count(CPAP_ClearAirway);
                    if (PROFILE.general->calculateRDI()) ahi+=day->count(CPAP_RERA);
                    ahi/=day->hours();
                    if (ahi > worstAHI) {
                        worstAHI=ahi;
                        worstAHIdate=date;
                    }
                    if (ahi < bestAHI) {
                        bestAHI=ahi;
                        bestAHIdate=date;
                    }
                }
                mode=(CPAPMode)round(day->settings_wavg(CPAP_Mode));
                min=day->settings_min(CPAP_PressureMin);
                if (mode==MODE_CPAP) {
                    max=day->settings_max(CPAP_PressureMin);
                } else max=day->settings_max(CPAP_PressureMax);

                if ((mode!=cmode) || (min!=cmin) || (max!=cmax))  {
                    if (cmode!=MODE_UNKNOWN) {
                        first=date.addDays(1);
                        int days=PROFILE.countDays(MT_CPAP,first,last);
                        RXChange rx;
                        rx.first=first;
                        rx.last=last;
                        rx.days=days;
                        rx.ahi=calcAHI(first,last);
                        rx.mode=cmode;
                        rx.min=cmin;
                        rx.max=cmax;
                        if (mode<MODE_BIPAP) {
                            rx.per1=p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,first,last);
                            rx.per2=0;
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
                    last=date;
                    lastchanged=true;
                }

            }
            date=date.addDays(-1);
        } while (date>=firstcpap);

        if (!lastchanged) {
            last=date.addDays(1);
            first=firstcpap;
            int days=PROFILE.countDays(MT_CPAP,first,last);
            RXChange rx;
            rx.first=first;
            rx.last=last;
            rx.days=days;
            rx.ahi=calcAHI(first,last);
            rx.mode=mode;
            rx.min=min;
            rx.max=max;
            if (mode<MODE_BIPAP) {
                rx.per1=p_profile->calcPercentile(CPAP_Pressure,0.9,MT_CPAP,first,last);
                rx.per2=0;
            } else {
                rx.per1=p_profile->calcPercentile(CPAP_EPAP,0.9,MT_CPAP,first,last);
                rx.per2=p_profile->calcPercentile(CPAP_IPAP,0.9,MT_CPAP,first,last);
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
        RXsort=RX_ahi;
        qSort(tmpRX.begin(),tmpRX.end(),RXSort);
        tmpRX[0]->highlight=4; // worst
        int ls=tmpRX.size()-1;
        tmpRX[ls]->highlight=1; //best

        QString recbox="<html><head><style type='text/css'>"
            "p,a,td,body { font-family: '"+QApplication::font().family()+"'; }"
            "p,a,td,body { font-size: "+QString::number(QApplication::font().pointSize() + 2)+"px; }"
            "a:link,a:visited { color: inherit; text-decoration: none; }" //font-weight: normal;
            "a:hover { background-color: inherit; color: white; text-decoration:none; font-weight: bold; }"
            "</style></head><body>";
        recbox+="<table width=100% cellpadding=2 cellspacing=0>";
        recbox+=QString("<tr><td><b><a href='daily=%1'>%2</b></td><td><b>%3</b></td></tr>").arg(bestAHIdate.toString(Qt::ISODate)).arg("Best&nbsp;AHI").arg(bestAHI,0,'f',2);
        recbox+=QString("<tr><td colspan=2>%1</td></tr>").arg(bestAHIdate.toString(Qt::SystemLocaleShortDate));
        recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");
        recbox+=QString("<tr><td><b><a href='daily=%1'>%2</a></b></td><td><b>%3</b></td></tr>").arg(worstAHIdate.toString(Qt::ISODate)).arg("Worst&nbsp;AHI").arg(worstAHI,0,'f',2);
        recbox+=QString("<tr><td colspan=2>%1</td></tr>").arg(worstAHIdate.toString(Qt::SystemLocaleShortDate));
        recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");
        QString minstr,maxstr,modestr;


        {
        CPAPMode mode=(CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode,MT_CPAP,tmpRX[ls]->first,tmpRX[ls]->first);

        if (mode<MODE_APAP) { // is CPAP?
            minstr="Pressure";
            maxstr="";
            modestr=tr("CPAP");
        } else if (mode<MODE_BIPAP) { // is AUTO?
            minstr="Min";
            maxstr="Max";
            modestr=tr("APAP");
        } else { // BIPAP or greater
            minstr="EPAP";
            maxstr="IPAP";
            modestr=tr("Bi-Level/ASV");
        }

        recbox+=QString("<tr><td colspan=2><b><a href='overview=%1,%2'>%3</a></b></td></tr>")
                .arg(tmpRX[ls]->first.toString(Qt::ISODate))
                .arg(tmpRX[ls]->last.toString(Qt::ISODate))
                .arg(tr("Best RX Setting"));
        recbox+=QString("<tr><td colspan=2>%1: %2</td></tr>").arg(tr("AHI")).arg(tmpRX[ls]->ahi,0,'f',2);
        recbox+=QString("<tr><td colspan=2>%1: %2</td></tr>").arg(tr("Mode")).arg(modestr);
        recbox+=QString("<tr><td colspan=2>%1: %2").arg(minstr).arg(tmpRX[ls]->min,0,'f',1);
        if (!maxstr.isEmpty()) recbox+=QString(" %1: %2").arg(maxstr).arg(tmpRX[ls]->max,0,'f',1);
        recbox+="</td></tr>";

        recbox+=QString("<tr><td colspan=2>%1: %2</td></tr>").arg(tr("Start")).arg(tmpRX[ls]->first.toString(Qt::SystemLocaleShortDate));
        recbox+=QString("<tr><td colspan=2>%1: %2</td></tr>").arg(tr("End")).arg(tmpRX[ls]->last.toString(Qt::SystemLocaleShortDate));
        recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");

        mode=(CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode,MT_CPAP,tmpRX[0]->first,tmpRX[0]->first);
        if (mode<MODE_APAP) { // is CPAP?
            minstr="Pressure";
            maxstr="";
            modestr=tr("CPAP");
        } else if (mode<MODE_BIPAP) { // is AUTO?
            minstr="Min";
            maxstr="Max";
            modestr=tr("APAP");
        } else { // BIPAP or greater
            minstr="EPAP";
            maxstr="IPAP";
            modestr=tr("Bi-Level/ASV");
        }

        recbox+=QString("<tr><td colspan=2><b><a href='overview=%1,%2'>%3</a></b></td></tr>")
                .arg(tmpRX[0]->first.toString(Qt::ISODate))
                .arg(tmpRX[0]->last.toString(Qt::ISODate))
                .arg(tr("Worst RX Setting"));
        recbox+=QString("<tr><td colspan=2>%1: %2</td></tr>").arg(tr("AHI")).arg(tmpRX[0]->ahi,0,'f',2);
        recbox+=QString("<tr><td colspan=2>%1: %2</td></tr>").arg(tr("Mode")).arg(modestr);
        recbox+=QString("<tr><td colspan=2>%1: %2").arg(minstr).arg(tmpRX[0]->min,0,'f',1);
        if (!maxstr.isEmpty()) recbox+=QString(" %1: %2").arg(maxstr).arg(tmpRX[0]->max,0,'f',1);
        recbox+="</td></tr>";

        recbox+=QString("<tr><td colspan=2>%1: %2</td></tr>").arg(tr("Start")).arg(tmpRX[0]->first.toString(Qt::SystemLocaleShortDate));
        recbox+=QString("<tr><td colspan=2>%1: %2</td></tr>").arg(tr("End")).arg(tmpRX[0]->last.toString(Qt::SystemLocaleShortDate));
        }
        recbox+=QString("</table>");
        recbox+="</body></html>";
        ui->recordsBox->setHtml(recbox);

//        ui->recordsBox->append("<a href='overview'><b>Best RX Setting</b></a>");
//        ui->recordsBox->append("Start: "+tmpRX[ls]->first.toString(Qt::SystemLocaleShortDate)+"<br/>End: "+tmpRX[ls]->last.toString(Qt::SystemLocaleShortDate)+"\n\n");
//        ui->recordsBox->append("<a href='overview'><b>Worst RX Setting</b></a>");
//        ui->recordsBox->append("Start: "+tmpRX[0]->first.toString(Qt::SystemLocaleShortDate)+"<br/>End: "+tmpRX[0]->last.toString(Qt::SystemLocaleShortDate)+"\n\n");

        //show the second best and worst..
        //if (tmpRX.size()>4) {
        //                tmpRX[1]->highlight=3; // worst
        //                tmpRX[tmpRX.size()-2]->highlight=2; //best
        //            }
        //RXsort=RX_first;
        //qSort(rxchange);
        html+="<div align=center>";
        html+=QString("<br/><b>Changes to Prescription Settings</b>");
        html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");
        QString extratxt;
        if (cpapmode>=MODE_BIPAP) {
            extratxt=QString("<td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td>")
                .arg(tr("EPAP")).arg(tr("EPAP")).arg(tr("%1% EPAP").arg(percentile*100.0)).arg(tr("%1% IPAP").arg(percentile*100.0));
        } else if (cpapmode>MODE_CPAP) {
            extratxt=QString("<td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td>")
                .arg(tr("Min Pressure")).arg(tr("Max Pressure")).arg(tr("%1% Pressure").arg(percentile*100.0));
        } else {
            extratxt=QString("<td><b>%1</b></td>")
                .arg(tr("Pressure"));
        }
        html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td>%6</tr>")
                  .arg(tr("First"))
                  .arg(tr("Last"))
                  .arg(tr("Days"))
                  .arg(ahitxt)
                  .arg(tr("Mode"))
                  .arg(extratxt);

        for (int i=0;i<rxchange.size();i++) {
            RXChange rx=rxchange.at(i);
            QString color;
            if (rx.highlight==1) {
                color=" bgcolor='#c0ffc0'";
            } else if (rx.highlight==2) {
                color=" bgcolor='#e0ffe0'";
            } else if (rx.highlight==3) {
                color=" bgcolor='#ffe0e0'";
            } else if (rx.highlight==4) {
                color=" bgcolor='#ffc0c0'";
            } else color="";
            if (cpapmode>=MODE_BIPAP) {
                extratxt=QString("<td>%1</td><td>%2</td><td>%3</td>").arg(rx.max,0,'f',2).arg(rx.per1,0,'f',2).arg(rx.per2,0,'f',2);
            } else if (cpapmode>MODE_CPAP) {
                extratxt=QString("<td>%1</td><td>%2</td>").arg(rx.max,0,'f',2).arg(rx.per1,0,'f',2);
            } else extratxt="";
            html+=QString("<tr"+color+"><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td>%7</tr>")
                    .arg(rx.first.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.last.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.days)
                    .arg(rx.ahi,0,'f',2)
                    .arg(schema::channel[CPAP_Mode].option(int(rx.mode)-1))
                    .arg(rx.min,0,'f',2)
                    .arg(extratxt);
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
    updateFavourites();
    html+=htmlFooter();
    ui->summaryView->setHtml(html);
//    QString file="qrc:/docs/index.html";
//    QUrl url(file);
//    ui->webView->setUrl(url);
}
void MainWindow::updateFavourites()
{
    ui->favouritesList->blockSignals(true);
    ui->favouritesList->clear();

    QDate date=PROFILE.LastDay();

    do {
        Day * journal=PROFILE.GetDay(date,MT_JOURNAL);
        if (journal) {
            if (journal->size()>0) {
                Session *sess=(*journal)[0];
                if (sess->settings.contains(Bookmark_Start)) {
                    QVariantList start=sess->settings[Bookmark_Start].toList();
                    QVariantList end=sess->settings[Bookmark_End].toList();
                    QStringList notes=sess->settings[Bookmark_Notes].toStringList();
                    if (notes.size()>0) {
                        QListWidgetItem *item=new QListWidgetItem(date.toString());
                        item->setData(Qt::UserRole,date);
                        ui->favouritesList->addItem(item);
                    }
                }
            }
        }

        date=date.addDays(-1);
    } while (date>=PROFILE.FirstDay());
    ui->favouritesList->blockSignals(false);
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
    arg1=arg1;
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

     QString msg=tr("<html><body><div align='center'><h2>SleepyHead v%1.%2.%3 (%7)</h2>Build Date: %4 %5<br/>%6<hr>"
"Copyright &copy;2011 Mark Watkins (jedimark) <br> \n"
"<a href='http://sleepyhead.sourceforge.net'>http://sleepyhead.sourceforge.net</a> <hr>"
"This software is released under the GNU Public License <br>"
"<i>This software comes with absolutely no warranty, either express of implied. It comes with no guarantee of fitness for any particular purpose. No guarantees are made regarding the accuracy of any data this program displays."
                    "</div></body></html>").arg(major_version).arg(minor_version).arg(revision_number).arg(__DATE__).arg(__TIME__).arg(gitrev).arg(ReleaseStatus);
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
    PreferencesDialog pd(this,p_profile);
    prefdialog=&pd;
    if (pd.exec()==PreferencesDialog::Accepted) {
        qDebug() << "Preferences Accepted";
        pd.Save();
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
//#ifdef Q_WS_MAC
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

    QString username=PROFILE.Get(QString("_{")+QString(UI_STR_UserName)+"}_");

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

    //bool highres;
    bool aa_setting=PROFILE.appearance->antiAliasing();

#ifdef Q_WS_MAC
    PROFILE.appearance->setHighResPrinting(true); // forced on
//    bool force_antialiasing=true;
//#else
#endif
    bool force_antialiasing=aa_setting;

    if (PROFILE.appearance->highResPrinting()) {
        printer=new QPrinter(QPrinter::HighResolution);
        //highres=true;
    } else {
        printer=new QPrinter(QPrinter::ScreenResolution);
        //highres=false;
    }

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
#ifdef Q_WS_MAC
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
//#ifdef Q_WS_WIN32
    gw=2048;  // Rough guess.. No GL_MAX_RENDERBUFFER_SIZE in mingw.. :(
//#else
//    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,&gw);
//#endif

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
        cpap=PROFILE.GetDay(date,MT_CPAP);
        oxi=PROFILE.GetDay(date,MT_OXIMETER);
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

            EventDataType min=cpap->settings_min(CPAP_PressureMin);
            EventDataType max=cpap->settings_max(CPAP_PressureMax);

            if (mode==MODE_CPAP) cpapinfo+=tr("CPAP %1cmH2O").arg(min);
            else if (mode==MODE_APAP) cpapinfo+=tr("APAP %1-%2cmH2O").arg(min).arg(max);
            else if (mode==MODE_BIPAP) cpapinfo+=tr("Bi-Level %1-%2cmH2O").arg(min).arg(max);
            else if (mode==MODE_ASV) cpapinfo+=tr("ASV");

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
            painter.drawPixmap(virt_width-piesize,bounds.height(),piesize,piesize,ebp);
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
            bounds=painter.boundingRect(QRectF(0,top+ttop,virt_width,0),stats,QTextOption(Qt::AlignCenter));
            painter.drawText(bounds,stats,QTextOption(Qt::AlignCenter));
            ttop+=bounds.height();

            if (ttop>maxy) maxy=ttop;
        } else {
            bounds=painter.boundingRect(QRectF(0,top+maxy,virt_width,0),cpapinfo,QTextOption(Qt::AlignCenter));
            painter.drawText(bounds,cpapinfo,QTextOption(Qt::AlignCenter));
            if (maxy+bounds.height()>maxy) maxy=maxy+bounds.height();
        }

        graph_slots=2;
    } else if (name==STR_TR_Overview) {
        QDateTime first=QDateTime::fromTime_t((*gv)[0]->min_x/1000L);
        QDateTime last=QDateTime::fromTime_t((*gv)[0]->max_x/1000L);
        QString ovinfo=tr("Reporting from %1 to %2").arg(first.date().toString(Qt::SystemLocaleShortDate)).arg(last.date().toString(Qt::SystemLocaleShortDate));
        QRectF bounds=painter.boundingRect(QRectF(0,top,virt_width,0),ovinfo,QTextOption(Qt::AlignCenter));
        painter.drawText(bounds,ovinfo,QTextOption(Qt::AlignCenter));

        if (bounds.height()>maxy) maxy=bounds.height();
        graph_slots=1;
    } else if (name==STR_TR_Oximetry) {
        QString ovinfo=tr("Reporting data goes here");
        QRectF bounds=painter.boundingRect(QRectF(0,top,virt_width,0),ovinfo,QTextOption(Qt::AlignCenter));
        painter.drawText(bounds,ovinfo,QTextOption(Qt::AlignCenter));

        if (bounds.height()>maxy) maxy=bounds.height();
        graph_slots=1;
    }
    top+=maxy;

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
                gGraph *g=(*gv)[i];
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
                }

                start.push_back(savest);
                end.push_back(saveet);
                graphs.push_back(g);
                labels.push_back("");

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
            QRectF pagebnds=QRectF(0,top,virt_width,normal_height);
            painter.drawText(pagebnds,label,QTextOption(Qt::AlignHCenter | Qt::AlignTop));
            top+=normal_height;
        } else top+=normal_height/2;

        PROFILE.appearance->setAntiAliasing(force_antialiasing);
        int tmb=g->m_marginbottom;
        g->m_marginbottom=0;

        //g->showTitle(false);
        QPixmap pm=g->renderPixmap(virt_width,full_graph_height-normal_height,1);//fscale);
        //g->showTitle(true);

        g->m_marginbottom=tmb;
        PROFILE.appearance->setAntiAliasing(aa_setting);

        painter.drawPixmap(0,top,virt_width,full_graph_height-normal_height,pm);
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
        for (QHash<SessionID,Session *>::iterator s=m->sessionlist.begin();s!=m->sessionlist.end();s++) {
            Session *sess=s.value();
            if (!sess) continue;
            sess->OpenEvents();
            invalid.clear();
            for (QHash<ChannelID,QVector<EventList *> >::iterator e=sess->eventlist.begin();e!=sess->eventlist.end();e++) {
                if (!valid.contains(e.key())) {
                    for (int i=0;i<e.value().size();i++)  {
                        delete e.value()[i];
                    }
                    e.value().clear();
                    invalid.push_back(e.key());
                } else {
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

                        /*EventList *nev=packEventList(newlist[i]);
                        if (nev->count()!=e.value()[i]->count() ) {
                            delete newlist[i];
                            newlist[i]=nev;
                        } else {
                            delete nev;
                        }*/
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
            sess->m_90p.clear();
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
            PROFILE.daylist[date].erase(it);
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
            RestartApplication();
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent * event)
{
    qDebug() << "Keypress:" << event->key();
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

void MainWindow::on_favouritesList_itemSelectionChanged()
{
    QListWidgetItem *item=ui->favouritesList->currentItem();
    if (!item) return;
    QDate date=item->data(Qt::UserRole).toDate();
    if (date.isValid()) {
        daily->LoadDate(date);
        ui->tabWidget->setCurrentWidget(daily);
    }
}

void MainWindow::on_favouritesList_itemClicked(QListWidgetItem *item)
{
    if (!item) return;
    QDate date=item->data(Qt::UserRole).toDate();
    if (date.isValid()) {
        if (date==daily->getDate()) {
            ui->tabWidget->setCurrentWidget(daily);
            daily->graphView()->ResetBounds();
            daily->graphView()->redraw();
        }
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
