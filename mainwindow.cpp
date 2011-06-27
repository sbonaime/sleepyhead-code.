/********************************************************************
 MainWindow Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <QGLFormat>
#include <QFileDialog>
#include <QMessageBox>
#include <QResource>
#include <QProgressBar>
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
    this->setWindowTitle("SleepyHead v0.8."+subversion);

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
    pref["AppName"]="SleepyHead";
    pref["Profile"]=getUserName();
    pref["LinkGraphMovement"]=true;
    pref["fruitsalad"]=true;

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
    DoneFonts();
    delete ui;
    Profiles::Done();
}

void MainWindow::Startup()
{

    qstatus->setText("Loading Data");
    qprogress->show();

      Profile *profile=Profiles::Get(pref["Profile"].toString());
    profile->LoadMachineData();

    daily=new Daily(ui->tabWidget,shared_context);
    overview=new Overview(ui->tabWidget,shared_context);
    ui->tabWidget->addTab(daily,"Daily");
    ui->tabWidget->addTab(overview,"Overview");

    qprogress->hide();
    qstatus->setText("Ready");
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
        qstatus->setText("Importing Data");
        dirNames=qfd.selectedFiles();
        for (int i=0;i<dirNames.size();i++) {
            profile->Import(dirNames[i]);
        }
        profile->Save();
        daily->ReloadGraphs();
        overview->ReloadGraphs();
        overview->UpdateGraphs();
        qstatus->setText("Ready");
        qprogress->hide();

    }
}

void MainWindow::on_actionView_Daily_triggered()
{

}

void MainWindow::on_actionView_Overview_triggered()
{

}

void MainWindow::on_actionView_Welcome_triggered()
{

}

void MainWindow::on_action_Fullscreen_triggered()
{
    if (ui->action_Fullscreen->isChecked())
        this->showFullScreen();
    else
        this->showNormal();

}

void MainWindow::on_actionUse_AntiAliasing_triggered()
{
    pref["UseAntiAliasing"]=(bool)ui->actionUse_AntiAliasing->isChecked();
    if (daily) daily->update();
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
        qstatus->setText("Ready");
    }

}

void MainWindow::on_webView_loadStarted()
{
    if (!first_load) {
        qstatus->setText("Loading");
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
    QString msg="<html><body><div align='center'><h2>SleepyHead v0.8.0</h2><hr>\
Copyright &copy;2011 Mark Watkins (jedimark) <br> \n\
<a href='http://sleepyhead.sourceforge.net'>http://sleepyhead.sourceforge.net</a> <hr>\
This software is released under the GNU Public License <hr> \
<i>This software comes with absolutely no warranty, either express of implied. It comes with no guarantee of fitness for any particular purpose. No guarantees are made regarding the accuracy of any data this program displays.\
</div></body></html>";
    QMessageBox msgbox(QMessageBox::Information,"About SleepyHead","",QMessageBox::Ok,this);
    msgbox.setTextFormat(Qt::RichText);
    msgbox.setText(msg);
    msgbox.exec();
}
