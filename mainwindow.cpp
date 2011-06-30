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
#include "daily.h"
#include "overview.h"
#include "Graphs/glcommon.h"

QProgressBar *qprogress;
QLabel *qstatus;

QString subversion="0";
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("SleepyHead")+QString(" v0.8.")+subversion);

    QGLFormat fmt;
    fmt.setDepth(false);
    fmt.setDirectRendering(true);
    fmt.setAlpha(true);
    fmt.setDoubleBuffer(true);
    fmt.setRgba(true);
    fmt.setDefaultFormat(fmt);
    shared_context=new QGLContext(fmt);

    //ui->tabWidget->setCurrentWidget(daily);
    qprogress=new QProgressBar(this);
    qprogress->setMaximum(100);
    qstatus=new QLabel("",this);
    qprogress->hide();
    ui->statusbar->addPermanentWidget(qstatus);
    ui->statusbar->addPermanentWidget(qprogress);
    daily=NULL;
    overview=NULL;
    Profiles::Scan();

    //loader_progress->Show();

    //pref["Version"]=wxString(AutoVersion::_FULLVERSION_STRING,wxConvUTF8);
    if (!pref.Exists("AppName")) pref["AppName"]="SleepyHead";
    if (!pref.Exists("Profile")) pref["Profile"]=getUserName();
    if (!pref.Exists("LinkGraphMovement")) pref["LinkGraphMovement"]=true;
    else ui->action_Link_Graphs->setChecked(pref["LinkGraphMovement"].toBool());

    if (!pref.Exists("fruitsalad")) pref["fruitsalad"]=true;

    if (!pref.Exists("UseAntiAliasing")) pref["UseAntiAliasing"]=false;
    else ui->actionUse_AntiAliasing->setChecked(pref["UseAntiAliasing"].toBool());
    first_load=true;
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
    DoneGraphs();
    delete ui;
    Profiles::Done();
}
void MainWindow::showEvent(QShowEvent * event)
{
    if (daily)
        daily->RedrawGraphs();
}

void MainWindow::Startup()
{

    qstatus->setText(tr("Loading Data"));
    qprogress->show();

    profile=Profiles::Get(pref["Profile"].toString());
    profile->LoadMachineData();

    daily=new Daily(ui->tabWidget,shared_context);
    ui->tabWidget->addTab(daily,tr("Daily"));

    // Disabled Overview until I want to actually look at it again. :)
    //overview=new Overview(ui->tabWidget,shared_context);
    //ui->tabWidget->addTab(overview,tr("Overview"));

    qprogress->hide();
    qstatus->setText(tr("Ready"));
}

void MainWindow::on_action_Import_Data_triggered()
{
    QStringList dirNames;
    QFileDialog qfd(this);
    //qfddir = QFileDialog::getOpenFileName(this,tr("Import Folder"), "", tr("Folders"));
    qfd.setFileMode(QFileDialog::DirectoryOnly);

    if (qfd.exec()) {
        qprogress->setValue(0);
        qprogress->show();
        qstatus->setText(tr("Importing Data"));
        dirNames=qfd.selectedFiles();
        for (int i=0;i<dirNames.size();i++) {
            profile->Import(dirNames[i]);
        }
        profile->Save();
        if (daily) daily->ReloadGraphs();
        if (overview) {
            overview->ReloadGraphs();
            overview->UpdateGraphs();
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
    //QResource res(file);
    //QByteArray html((const char*)res.data(), res.size());
    ui->webView->setUrl(url);
    ui->webView->load(url);
    //ui->webView->setHtml(url); //QString(html));
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
    QString msg=tr("<html><body><div align='center'><h2>SleepyHead v0.8.0</h2><hr>\
Copyright &copy;2011 Mark Watkins (jedimark) <br> \n\
<a href='http://sleepyhead.sourceforge.net'>http://sleepyhead.sourceforge.net</a> <hr>\
This software is released under the GNU Public License <hr> \
<i>This software comes with absolutely no warranty, either express of implied. It comes with no guarantee of fitness for any particular purpose. No guarantees are made regarding the accuracy of any data this program displays.\
</div></body></html>");
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
    if (daily) daily->RedrawGraphs();

}
