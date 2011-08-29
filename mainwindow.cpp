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
#include <QTimer>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "SleepLib/loader_plugins/prs1_loader.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/loader_plugins/zeo_loader.h"
#include "SleepLib/loader_plugins/resmed_loader.h"
#include "preferencesdialog.h"


#include "Graphs/glcommon.h"

QProgressBar *qprogress;
QLabel *qstatus;
QLabel *qstatus2;
QStatusBar *qstatusbar;

void MainWindow::Log(QString s)
{
    static int start=QDateTime::currentDateTime().toTime_t();
    QString tmp=QString("%1: %2").arg(QDateTime::currentDateTime().toTime_t()-start,5,10,QChar('0')).arg(s);
    ui->logText->appendPlainText(tmp);
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("SleepyHead")+QString(" v%1.%2.%3").arg(major_version).arg(minor_version).arg(revision_number));
    ui->tabWidget->setCurrentIndex(0);

    PRS1Loader::Register();
    CMS50Loader::Register();
    ZEOLoader::Register();
    ResmedLoader::Register();

/*    QGLFormat fmt;
    fmt.setDepth(false);
    fmt.setDirectRendering(true);
    fmt.setAlpha(true);
    fmt.setDoubleBuffer(true);
    fmt.setRgba(true);
    fmt.setDefaultFormat(fmt);
    QGLContext smeg(fmt); */

    //new QGLContext(fmt);
    //shared_context->create(shared_context);

    daily=NULL;
    //overview=NULL;
    //oximetry=NULL;
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
    Profiles::Scan();

    pref["AppName"]="SleepyHead";
    QString Version=QString("%1.%2.%3").arg(major_version).arg(minor_version).arg(revision_number);
    if (pref.Exists("VersionString") && pref["VersionString"]!=Version) {
        //QMessageBox::warning(this,"Potential Crash Warning","This is a new version of SleepyHead. If you experience a crash right after clicking Ok, you will need to manually delete the SleepApp folder (it's located in your Documents folder), and things should then work normally.",QMessageBox::Ok);
        QMessageBox::warning(this,"Notice","Hi, I'm currently in the middle of a large rewrite of graphing system components. Things aren't finished and some features are missing.. The mac/windows scrolling glitches show be gone now, but the new mouse handling isn't complete (ie, things will suck). This annoying message will go away when I'm done with it.",QMessageBox::Ok);
    }
    pref["VersionString"]=Version;

    if (!pref.Exists("Profile")) pref["Profile"]=getUserName();

    if (!pref.Exists("LinkGraphMovement")) pref["LinkGraphMovement"]=true;
    ui->action_Link_Graphs->setChecked(pref["LinkGraphMovement"].toBool());

    if (!pref.Exists("ShowDebug")) pref["ShowDebug"]=true;
    ui->actionDebug->setChecked(pref["ShowDebug"].toBool());

    if (!pref["ShowDebug"].toBool()) {
        ui->logText->hide();
    }

    if (!pref.Exists("EnableGraphSnapshots")) pref["EnableGraphSnapshots"]=false;
    ui->actionDisplay_Graph_Snapshots->setChecked(pref["EnableGraphSnapshots"].toBool());


    if (!pref.Exists("MemoryHog")) pref["MemoryHog"]=true;

    if (!pref.Exists("fruitsalad")) pref["fruitsalad"]=true;

    if (!pref.Exists("UseAntiAliasing")) pref["UseAntiAliasing"]=false;
    ui->actionUse_AntiAliasing->setChecked(pref["UseAntiAliasing"].toBool());


    first_load=true;

    if (!pref.Exists("AlwaysShowOverlayBars")) pref["AlwaysShowOverlayBars"]=true;
    ui->actionOverlay_Bars->setChecked(pref["AlwaysShowOverlayBars"].toBool());

    daily=new Daily(ui->tabWidget,NULL,this);
    ui->tabWidget->insertTab(1,daily,tr("Daily"));

    //overview=new Overview(ui->tabWidget,daily->SharedWidget());
    //ui->tabWidget->insertTab(2,overview,tr("Overview"));
    //oximetry=NULL;
    //overview=NULL;
    oximetry=new Oximetry(ui->tabWidget,daily->SharedWidget());
    ui->tabWidget->insertTab(3,oximetry,tr("Oximetry"));

    ui->tabWidget->setCurrentWidget(ui->welcome);

}
extern MainWindow *mainwin;
MainWindow::~MainWindow()
{
    if (daily) {
        daily->close();
        delete daily;
    }
    /*if (overview) {
        overview->close();
        delete overview;
    } */
    if (oximetry) {
        oximetry->close();
        delete oximetry;
    }
    DoneGraphs();
    Profiles::Done();
    mainwin=NULL;
    delete ui;
}

void MainWindow::Startup()
{
    qDebug() << pref["AppName"].toString().toAscii()+" v"+pref["VersionString"].toString().toAscii() << "built with Qt"<< QT_VERSION_STR << "on" << __DATE__ << __TIME__;
    qstatus->setText(tr("Loading Data"));
    qprogress->show();
    //qstatusbar->showMessage(tr("Loading Data"),0);

    profile=Profiles::Get(pref["Profile"].toString());
    profile->LoadMachineData();

    if (daily) daily->ReloadGraphs();

    /*if (overview) {
        overview->ReloadGraphs();
        overview->UpdateGraphs();

    }*/

    qprogress->hide();
    qstatus->setText("");
    //qstatusbar->clearMessage();

}

void MainWindow::on_action_Import_Data_triggered()
{
    //QStringList dirNames;

    //QFileDialog qfd(this);
    //qfd.setFileMode(QFileDialog::Directory);
    //qfd.setOption(QFileDialog::ShowDirsOnly,true);
    QString dir=QFileDialog::getExistingDirectory(this,"Select a folder to import","",QFileDialog::ShowDirsOnly);

    if (!dir.isEmpty()) {
    //if (qfd.exec()) {
        qprogress->setValue(0);
        qprogress->show();
        qstatus->setText(tr("Importing Data"));
        int c=profile->Import(dir);
        if (!c) {
            QMessageBox::warning(this,"Import Problem","Couldn't Find any Machine Data at this location:\n"+dir,QMessageBox::Ok);
        }
        /*dirNames=qfd.selectedFiles();
        int c=0,d;
        for (int i=0;i<dirNames.size();i++) {
            d=profile->Import(dirNames[i]);
            if (!d) {
                QMessageBox::warning(this,"Import Problem","Couldn't Find any Machine Data at this location:\n"+dirNames[i],QMessageBox::Ok);
            }
            c+=d;
        }*/
        qDebug() << "Finished Importing data" << c;
        if (c) {
            profile->Save();
            if (daily) daily->ReloadGraphs();

            /*if (overview) {
                overview->ReloadGraphs();
                overview->UpdateGraphs();

            } */
            //qDebug() << "overview->ReloadGraphs();";
        }
        qstatus->setText("");
        qprogress->hide();

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

void MainWindow::on_homeButton_clicked()
{
    QString file="qrc:/docs/index.html";
    QUrl url(file);
    ui->webView->setUrl(url);
    //ui->webView->load(url);
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
}

void MainWindow::on_overviewButton_clicked()
{
    //ui->tabWidget->setCurrentWidget(overview);
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
     QString msg=tr("<html><body><div align='center'><h2>SleepyHead v%1.%2.%3</h2>Build Date: %4 %5<hr>"
"Copyright &copy;2011 Mark Watkins (jedimark) <br> \n"
"<a href='http://sleepyhead.sourceforge.net'>http://sleepyhead.sourceforge.net</a> <hr>"
"This software is released under the GNU Public License <br>"
"<i>This software comes with absolutely no warranty, either express of implied. It comes with no guarantee of fitness for any particular purpose. No guarantees are made regarding the accuracy of any data this program displays."
"</div></body></html>").arg(major_version).arg(minor_version).arg(revision_number).arg(__DATE__).arg(__TIME__);
    QMessageBox msgbox(QMessageBox::Information,tr("About SleepyHead"),"",QMessageBox::Ok,this);
    msgbox.setTextFormat(Qt::RichText);
    msgbox.setText(msg);
    msgbox.exec();
}

void MainWindow::on_action_Link_Graphs_triggered(bool checked)
{
    pref["LinkGraphMovement"]=checked;
}

void MainWindow::on_actionUse_AntiAliasing_triggered(bool checked)
{
    pref["UseAntiAliasing"]=checked;
    if (daily)
        daily->RedrawGraphs();

}

void MainWindow::on_actionDebug_toggled(bool checked)
{
    pref["ShowDebug"]=checked;
    if (checked) {
        ui->logText->show();
    } else {
        ui->logText->hide();
    }
}

void MainWindow::on_actionOverlay_Bars_toggled(bool checked)
{
    pref["AlwaysShowOverlayBars"]=checked;
    if (daily)
        daily->RedrawGraphs();
}

void MainWindow::on_action_Reset_Graph_Layout_triggered()
{
    if (daily) daily->ResetGraphLayout();
}

void MainWindow::on_action_Preferences_triggered()
{
    PreferencesDialog pd(this);
    if (pd.exec()==PreferencesDialog::Accepted) {
        qDebug() << "Preferences Accepted";
    }
}

void MainWindow::on_actionDisplay_Graph_Snapshots_toggled(bool checked)
{
    //if (QMessageBox::question(this,"Warning","Turning this feature on has caused crashes on some hardware configurations due to OpenGL/Qt bugs.\nIf you have already seen Pie Charts & CandleSticks in the left panel of daily view previously, you're not affected by this bug.\nAre you sure you want to enable this?",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes) {
        pref["EnableGraphSnapshots"]=checked;
    //}
}

void MainWindow::on_oximetryButton_clicked()
{
    if (oximetry) {
        ui->tabWidget->setCurrentWidget(oximetry);
    }
}
