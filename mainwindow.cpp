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


#include "daily.h"
#include "overview.h"
#include "Graphs/glcommon.h"

QProgressBar *qprogress;
QLabel *qstatus;
QLabel *qstatus2;
QStatusBar *qstatusbar;

void MainWindow::Log(QString s)
{
    ui->logText->appendPlainText(s);
    if (s.startsWith("Warning")) {
        int i=5;
    }
}


QString subversion="0";
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

//#ifndef Q_WS_MAC
//    this->showMaximized();
//#endif
    ui->setupUi(this);
    this->setWindowTitle(tr("SleepyHead")+QString(" v0.8.")+subversion);
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
    overview=NULL;
    oximetry=NULL;

    //ui->tabWidget->setCurrentWidget(daily);
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
    ui->statusbar->addPermanentWidget(qstatus2,0);
    ui->statusbar->addPermanentWidget(qstatus,0);
    ui->statusbar->addPermanentWidget(qprogress,10);
    Profiles::Scan();

    //loader_progress->Show();

    //pref["Version"]=wxString(AutoVersion::_FULLVERSION_STRING,wxConvUTF8);
    if (!pref.Exists("AppName")) pref["AppName"]="SleepyHead";
    if (!pref.Exists("Profile")) pref["Profile"]=getUserName();

    if (!pref.Exists("LinkGraphMovement")) pref["LinkGraphMovement"]=true;
    else ui->action_Link_Graphs->setChecked(pref["LinkGraphMovement"].toBool());

    if (!pref.Exists("ShowDebug")) pref["ShowDebug"]=true;
    else ui->actionDebug->setChecked(pref["ShowDebug"].toBool());

    if (!pref["ShowDebug"].toBool()) {
        ui->logText->hide();
    }

    if (!pref.Exists("NoonDateSplit")) pref["NoonDateSplit"]=false;
    else ui->action_Noon_Date_Split->setChecked(pref["NoonDateSplit"].toBool());

    if (!pref.Exists("fruitsalad")) pref["fruitsalad"]=true;

    if (!pref.Exists("UseAntiAliasing")) pref["UseAntiAliasing"]=false;
    else ui->actionUse_AntiAliasing->setChecked(pref["UseAntiAliasing"].toBool());
    first_load=true;

    if (!pref.Exists("AlwaysShowOverlayBars")) pref["AlwaysShowOverlayBars"]=true;
    else ui->actionOverlay_Bars->setChecked(pref["AlwaysShowOverlayBars"].toBool());

}

MainWindow::~MainWindow()
{
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
    DoneGraphs();
    delete ui;
    Profiles::Done();
}

void MainWindow::Startup()
{

    qstatus->setText(tr("Loading Data"));
    qprogress->show();
    qstatusbar->showMessage("Your computer loads faster than JediMark's",1900);

    profile=Profiles::Get(pref["Profile"].toString());
    profile->LoadMachineData();

    daily=new Daily(ui->tabWidget);
    ui->tabWidget->insertTab(1,daily,tr("Daily"));

    overview=new Overview(ui->tabWidget,daily->SharedWidget());
    ui->tabWidget->insertTab(2,overview,tr("Overview"));

    oximetry=new Oximetry(ui->tabWidget,daily->SharedWidget());
    ui->tabWidget->insertTab(3,oximetry,tr("Oximetry"));

    qprogress->hide();
    qstatus->setText(tr("Ready"));

}

void MainWindow::on_action_Import_Data_triggered()
{
    QStringList dirNames;
    QFileDialog qfd(this);
    qfd.setFileMode(QFileDialog::DirectoryOnly);

    if (qfd.exec()) {
        qprogress->setValue(0);
        qprogress->show();
        //qstatus->setText(tr("Importing Data"));
        dirNames=qfd.selectedFiles();
        int c=0,d;
        for (int i=0;i<dirNames.size();i++) {
            d=profile->Import(dirNames[i]);
            if (!d) {
                QMessageBox::warning(this,"Import Problem","Couldn't Find any Machine Data at this location:\n"+dirNames[i],QMessageBox::Ok);
            }
            c+=d;
        }
        qDebug() << "Finished Importing data" << c;
        if (c) {
            profile->Save();
            //qDebug() << " profile->Save();";
            if (daily) daily->ReloadGraphs();

            //qDebug() << " daily->ReloadGraphs();";
            if (overview) {
                overview->ReloadGraphs();
                overview->UpdateGraphs();

            }
            //qDebug() << "overview->ReloadGraphs();";
        }
        qstatus->setText(tr("Ready"));
        qprogress->hide();

    }
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
    ui->tabWidget->setCurrentWidget(overview);
}

void MainWindow::on_webView_loadFinished(bool arg1)
{
    arg1=arg1;
    qprogress->hide();
    if (first_load) {
        QTimer::singleShot(0,this,SLOT(Startup()));
        first_load=false;
    } else {
        qstatus->setText(tr("Ready"));
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
    QString msg=tr("<html><body><div align='center'><h2>SleepyHead v0.8.0</h2><hr>"
"Copyright &copy;2011 Mark Watkins (jedimark) <br> \n"
"<a href='http://sleepyhead.sourceforge.net'>http://sleepyhead.sourceforge.net</a> <hr>"
"This software is released under the GNU Public License <br>"
"<i>This software comes with absolutely no warranty, either express of implied. It comes with no guarantee of fitness for any particular purpose. No guarantees are made regarding the accuracy of any data this program displays."
"</div></body></html>");
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

void MainWindow::on_action_Noon_Date_Split_toggled(bool checked)
{
    pref["NoonDateSplit"]=checked;
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
