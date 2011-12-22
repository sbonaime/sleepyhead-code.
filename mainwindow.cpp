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
    ui->tabWidget->setCurrentIndex(0);

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

    // Start with the Welcome Tab
    ui->tabWidget->setCurrentWidget(ui->welcome);

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

    daily=new Daily(ui->tabWidget,NULL);
    ui->tabWidget->insertTab(1,daily,STR_TR_Daily);

    overview=new Overview(ui->tabWidget,daily->graphView());
    ui->tabWidget->insertTab(2,overview,STR_TR_Overview);
    if (PROFILE.oxi->oximetryEnabled()) {
        oximetry=new Oximetry(ui->tabWidget,daily->graphView());
        ui->tabWidget->insertTab(3,oximetry,STR_TR_Oximetry);
    }


    if (daily) daily->ReloadGraphs();
    if (overview) overview->ReloadGraphs();
    qprogress->hide();
    qstatus->setText("");
    on_homeButton_clicked();

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

void MainWindow::on_actionView_Welcome_triggered()
{
    ui->tabWidget->setCurrentWidget(ui->welcome);
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
"<h2><img src='qrc:/docs/sheep.png' width=100px height=100px>SleepyHead v"+VersionString+" "+ReleaseStatus+"</h2>"
"<p><i>This page is being redesigned to be more useful... Please send me your ideas on what you'd like to see here :)</i></p>"
"<p>The plan is to get the content happening first, then make the layout pretty...</p>"
"<p>Percentile calcs aren't done, as they require ALL data in memory, or very slow calcs, loading and unloading the data.. I really don't want to be forced to have to do either of those. There is simply too much variance to cache this data. :(</br>"
"If an average of the daily percentile values were useful, maybe.. ? </p>"
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
              +p_profile->calcCount(CPAP_Apnea,MT_CPAP,start,end))
             /p_profile->calcHours(MT_CPAP,start,end);

    return val;
}

struct RXChange
{
    RXChange() {}
    RXChange(const RXChange & copy) {
        first=copy.first;
        last=copy.last;
        days=copy.days;
        ahi=copy.ahi;
        mode=copy.mode;
        min=copy.min;
        max=copy.min;
        p90=copy.p90;
    }
    QDate first;
    QDate last;
    int days;
    EventDataType ahi;
    CPAPMode mode;
    EventDataType min;
    EventDataType max;
    EventDataType p90;
};

bool operator<(const RXChange & comp1, const RXChange & comp2) {
    return comp1.ahi < comp2.ahi;
}

void MainWindow::on_homeButton_clicked()
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
    QList<Machine *> mach=PROFILE.GetMachines(MT_CPAP);

    int days=firstcpap.daysTo(lastcpap);
    if (days==0) {
        html+="<p>No Machine Data Imported</p>";
    } else {
        html+=QString("<p>%1 day%2 of CPAP Data</p>").arg(days).arg(days>1 ? "s" : "");

        html+=QString("<b>Summary Information as of %1</b>").arg(lastcpap.toString(Qt::SystemLocaleLongDate));
        html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=100%>"
        "<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</td></tr>")
        .arg(tr("Details")).arg(tr("Most Recent")).arg(tr("Last 7 Days")).arg(tr("Last 30 Days")).arg(tr("Last 6 months")).arg(tr("Last Year"));

        html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
        .arg(tr("AHI"))
        .arg(calcAHI(lastcpap,lastcpap),0,'f',2)
        .arg(calcAHI(cpapweek,lastcpap),0,'f',2)
        .arg(calcAHI(cpapmonth,lastcpap),0,'f',2)
        .arg(calcAHI(cpap6month,lastcpap),0,'f',2)
        .arg(calcAHI(cpapyear,lastcpap),0,'f',2);
        html+="<tr><td colspan=6>Note, these are different to overview calcs.. Overview shows a simple average AHI, this shows combined counts divide by combined hours</td></tr>";

        html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
        .arg(tr("Usage (Average)"))
        .arg(p_profile->calcHours(MT_CPAP),0,'f',2)
        .arg(p_profile->calcHours(MT_CPAP,cpapweek,lastcpap)/float(cpapweekdays),0,'f',2)
        .arg(p_profile->calcHours(MT_CPAP,cpapmonth,lastcpap)/float(cpapmonthdays),0,'f',2)
        .arg(p_profile->calcHours(MT_CPAP,cpap6month,lastcpap)/float(cpap6monthdays),0,'f',2)
        .arg(p_profile->calcHours(MT_CPAP,cpapyear,lastcpap)/float(cpapyeardays),0,'f',2);

        html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
        .arg(tr("Average Pressure"))
        .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP))
        .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapweek,lastcpap),0,'f',3)
        .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
        .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpap6month,lastcpap),0,'f',3)
        .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapyear,lastcpap),0,'f',3);
        html+="<tr><td colspan=6>TODO: 90% pressure.. Any point showing if this is all CPAP?</td></tr>";


        html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
        .arg(tr("Average Leaks"))
        .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP))
        .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP,cpapweek,lastcpap),0,'f',3)
        .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP,cpapmonth,lastcpap),0,'f',3)
        .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP,cpap6month,lastcpap),0,'f',3)
        .arg(p_profile->calcWavg(CPAP_Leak,MT_CPAP,cpapyear,lastcpap),0,'f',3);
        html+="<tr><td colspan=6>What about median leak values? 90% Leaks?</td></tr>";


        html+="</table>";
        html+=QString("<br/><b>Changes to Prescription Settings</b>");
        html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=100%>");
        html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</b></td><td><b>%7</b></td><td><b>%8</b></td></tr>")
                      .arg(tr("First"))
                      .arg(tr("Last"))
                      .arg(tr("Days"))
                      .arg(tr("AHI"))
                      .arg(tr("Mode"))
                      .arg(tr("Min Pressure"))
                      .arg(tr("Max Pressure"))
                      .arg(tr("90% Pressure"));

        QDate first,last=lastcpap;
        CPAPMode mode,cmode=MODE_UNKNOWN;
        EventDataType cmin=0,cmax=0,min,max;
        QDate date=lastcpap;
        Day * day;
        bool lastchanged;
        int cnt=0;
        QList<RXChange> rxchange;

        do {
            day=PROFILE.GetDay(date,MT_CPAP);
            lastchanged=false;
            if (day) {
                mode=(CPAPMode)round(day->settings_wavg(CPAP_Mode));
                min=day->settings_min(CPAP_PressureMin);
                if (mode==MODE_CPAP) {
                    max=day->settings_max(CPAP_PressureMin);
                } else max=day->settings_max(CPAP_PressureMax);

                if ((mode!=cmode) || (min!=cmin) || (max!=cmax))  {
                    if (cmode!=MODE_UNKNOWN) {
                        first=date.addDays(1);
                        RXChange rx;
                        rx.first=first;
                        rx.last=last;
                        rx.days=first.daysTo(last)+1;
                        rx.ahi=calcAHI(first,last);
                        rx.mode=cmode;
                        rx.min=cmin;
                        rx.max=cmax;
                        rx.p90=0;
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
            RXChange rx;
            rx.first=first;
            rx.last=last;
            rx.days=first.daysTo(last)+1;
            rx.ahi=calcAHI(first,last);
            rx.mode=mode;
            rx.min=min;
            rx.max=max;
            rx.p90=0;
            rxchange.push_back(rx);
        }
        qSort(rxchange);
        for (int i=0;i<rxchange.size();i++) {
            RXChange rx=rxchange.at(i);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8</td></tr>")
                    .arg(rx.first.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.last.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.days)
                    .arg(rx.ahi,0,'f',2)
                    .arg(schema::channel[CPAP_Mode].option(int(rx.mode)-1))
                    .arg(rx.min)
                    .arg(rx.max)
                    .arg(rx.p90);
        }

        html+="</table>";

    }
    if (mach.size()>0) {

        html+=QString("<br/><b>Machine Information</b>");
        html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=100%>");
        html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td></tr>")
                      .arg(tr("Brand"))
                      .arg(tr("Model"))
                      .arg(tr("Serial"))
                      .arg(tr("First Use"))
                      .arg(tr("Last Use"));
        Machine *m;
        for (int i=0;i<mach.size();i++) {
            m=mach.at(i);
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
    }
    html+=htmlFooter();
    ui->webView->setHtml(html);
//    QString file="qrc:/docs/index.html";
//    QUrl url(file);
//    ui->webView->setUrl(url);
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
}

void MainWindow::on_webView_loadStarted()
{
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
        //QPrinter printer();
        //ui->webView->print(printer)
        QMessageBox::information(this,tr("Not supported Yet"),tr("Sorry, printing from this page is not supported yet"),QMessageBox::Ok);
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
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::on_action_Frequently_Asked_Questions_triggered()
{
    ui->webView->load(QUrl("http://sourceforge.net/apps/mediawiki/sleepyhead/index.php?title=Frequently_Asked_Questions"));
    ui->tabWidget->setCurrentIndex(0);
}

EventList *packEventList(EventList *ev)
{
    EventList *nev=new EventList(EVL_Event);

    EventDataType val,lastval=0;
    qint64 time,lasttime=0;//,lasttime2=0;

    lastval=ev->data(0);
    lasttime=ev->time(0);
    nev->AddEvent(lasttime,lastval);

    for (unsigned i=1;i<ev->count();i++) {
        val=ev->data(i);
        time=ev->time(i);
        if (val!=lastval) {
            nev->AddEvent(time,val);
            //lasttime2=time;
        }
        lastval=val;
        lasttime=time;
    }
    if (val==lastval) {
        nev->AddEvent(lasttime,val);
    }
    return nev;
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

    int graph_slots=0;
    if (name==STR_TR_Daily) {
        Day *cpap=PROFILE.GetDay(date,MT_CPAP);
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

            float ahi=(cpap->count(CPAP_Obstructive)+cpap->count(CPAP_Hypopnea)+cpap->count(CPAP_ClearAirway)+cpap->count(CPAP_Apnea))/cpap->hours();
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

            int piesize=(2048.0/8.0)*1.5;   // 1.5" in size
            //float fscale=font_scale;
            //if (!highres)
//                fscale=1;

            getDaily()->eventBreakdownPie()->showTitle(false);
            getDaily()->eventBreakdownPie()->setMargins(0,0,0,0);
            QPixmap ebp=getDaily()->eventBreakdownPie()->renderPixmap(piesize,piesize,1);
            painter.drawPixmap(virt_width-piesize,bounds.height()/2,piesize,piesize,ebp);
            getDaily()->eventBreakdownPie()->showTitle(true);

            QString stats;
            painter.setFont(medium_font);
            stats=tr("AHI\t%1\n").arg(ahi,0,'f',2);
            QRectF bounds=painter.boundingRect(QRectF(0,0,virt_width,0),stats,QTextOption(Qt::AlignRight));
            painter.drawText(bounds,stats,QTextOption(Qt::AlignRight));

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
    qint64 st,et;
    gv->GetXBounds(st,et);
    for (int i=0;i<gv->size();i++) {
        bool normal=true;
        gGraph *g=(*gv)[i];
        if (g->isEmpty()) continue;
        if (!g->visible()) continue;
        if (print_bookmarks && (g->title()==tr("Flow Rate"))) {
            normal=false;
            start.push_back(st);
            end.push_back(et);
            graphs.push_back(g);
            labels.push_back(tr("Current Selection"));
            if (journal) {
                if (journal->settings.contains(Bookmark_Start)) {
                    QVariantList st1=journal->settings[Bookmark_Start].toList();
                    QVariantList et1=journal->settings[Bookmark_End].toList();
                    QStringList notes=journal->settings[Bookmark_Notes].toStringList();
                    for (int i=0;i<notes.size();i++) {
                        labels.push_back(notes.at(i));
                        start.push_back(st1.at(i).toLongLong());
                        end.push_back(et1.at(i).toLongLong());
                        graphs.push_back(g);
                    }
                }
            }
        }
        if (normal) {
            start.push_back(st);
            end.push_back(et);
            graphs.push_back(g);
            labels.push_back("");
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

    qprogress->hide();
    painter.end();
    delete printer;
    Notify("SleepyHead has finished sending the job to the printer.");
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
                        EventList *nev=packEventList(newlist[i]);
                        if (nev->count()!=e.value()[i]->count() ) {
                            delete newlist[i];
                            newlist[i]=nev;
                        } else {
                            delete nev;
                        }
                    }
                   e.value()=newlist;
                }
            }
            for (int i=0;i<invalid.size();i++) {
                sess->eventlist.erase(sess->eventlist.find(invalid[i]));
            }
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
