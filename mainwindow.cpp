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

    daily=new Daily(ui->tabWidget,shared_context);
    overview=new Overview(ui->tabWidget,shared_context);
    ui->tabWidget->addTab(daily,"Daily");
    ui->tabWidget->addTab(overview,"Overview");

    //ui->tabWidget->setCurrentWidget(daily);
    qprogress=new QProgressBar(this);
    qprogress->setMaximum(100);
    qstatus=new QLabel("Ready",this);

    ui->statusbar->addPermanentWidget(qstatus);
    ui->statusbar->addPermanentWidget(qprogress);
    qprogress->hide();
    //on_homeButton_clicked();

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

void MainWindow::on_action_Import_Data_triggered()
{
    QStringList dirNames;
    QFileDialog qfd(this);
    //qfddir = QFileDialog::getOpenFileName(this,tr("Import Folder"), "", tr("Folders"));
    qfd.setFileMode(QFileDialog::DirectoryOnly);

    if (qfd.exec()) {
        qprogress->setValue(0);
        qprogress->show();
        qstatus->setText("Busy");
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
    qstatus->setText("Ready");
}

void MainWindow::on_webView_loadStarted()
{
    qprogress->reset();
    qprogress->show();
    qstatus->setText("Loading");
}

void MainWindow::on_webView_loadProgress(int progress)
{
    qprogress->setValue(progress);
}
