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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "preferencesdialog.h"
#include "newprofile.h"
#include "SleepLib/schema.h"


#include "Graphs/glcommon.h"

QProgressBar *qprogress;
QLabel *qstatus;
QLabel *qstatus2;
QStatusBar *qstatusbar;

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
    logtime.start();
    ui->setupUi(this);
    this->setWindowTitle(tr("SleepyHead")+QString(" v%1.%2.%3 (%4)").arg(major_version).arg(minor_version).arg(revision_number).arg(pref["Profile"].toString()));
    ui->tabWidget->setCurrentIndex(0);
    //move(0,0);
    overview=NULL;
    daily=NULL;
    oximetry=NULL;

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

    if (!pref.Exists("ShowDebug")) pref["ShowDebug"]=true;
    ui->actionDebug->setChecked(pref["ShowDebug"].toBool());

    if (!pref["ShowDebug"].toBool()) {
        ui->logText->hide();
    }

    // This speeds up the second part of importing craploads.. later it will speed up the first part too.
    if (!pref.Exists("EnableMultithreading")) pref["EnableMultithreading"]=QThread::idealThreadCount()>1;
    if (!pref.Exists("MemoryHog")) pref["MemoryHog"]=false;
    if (!pref.Exists("EnableGraphSnapshots")) pref["EnableGraphSnapshots"]=false;
    if (!pref.Exists("AlwaysShowOverlayBars")) pref["AlwaysShowOverlayBars"]=0;
    if (!pref.Exists("UseAntiAliasing")) pref["UseAntiAliasing"]=false;
    if (!pref.Exists("IntentionalLeak")) pref["IntentionalLeak"]=(double)0.0;
    if (!pref.Exists("IgnoreShorterSessions")) pref["IgnoreShorterSessions"]=0;
    if (!pref.Exists("CombineCloserSessions")) pref["CombineCloserSessions"]=0;
    if (!pref.Exists("DaySplitTime")) pref["DaySplitTime"]=QTime(12,0,0,0);
    //DateTime(QDate::currentDate(),QTime(12,0,0,0),Qt::UTC).time();


    //ui->actionUse_AntiAliasing->setChecked(pref["UseAntiAliasing"].toBool());


    first_load=true;
    QSettings settings("Jedimark", "SleepyHead");
    this->restoreGeometry(settings.value("MainWindow/geometry").toByteArray());

    ui->tabWidget->setCurrentWidget(ui->welcome);

    netmanager = new QNetworkAccessManager(this);
    connect(netmanager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    connect(ui->webView, SIGNAL(statusBarMessage(QString)), this, SLOT(updatestatusBarMessage(QString)));

}
extern MainWindow *mainwin;
MainWindow::~MainWindow()
{
    if (!isMaximized()) {
        QSettings settings("Jedimark", "SleepyHead");
        settings.setValue("MainWindow/geometry", saveGeometry());
    }
    //QWidget::closeEvent(event);
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
    Q_ASSERT(profile!=NULL);

    profile->LoadMachineData();

    daily=new Daily(ui->tabWidget,profile,NULL,this);
    ui->tabWidget->insertTab(1,daily,tr("Daily"));

    overview=new Overview(ui->tabWidget,profile,daily->SharedWidget());
    ui->tabWidget->insertTab(2,overview,tr("Overview"));

    if (daily) daily->ReloadGraphs();
    if (overview) overview->ReloadGraphs();
    qprogress->hide();
    qstatus->setText("");
    /*schema::Channel & item=schema::channel["SysOneResistSet"];
    if (!item.isNull()) {
        for (QHash<int,QString>::iterator i=item.m_options.begin();i!=item.m_options.end();i++) {
            qDebug() << i.key() << i.value();
        }
    }*/
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
            if (overview) overview->ReloadGraphs();
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
    daily->RedrawGraphs();
    qstatus2->setText("Daily");
}

void MainWindow::on_overviewButton_clicked()
{
    ui->tabWidget->setCurrentWidget(overview);
    qstatus2->setText("Overview");
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

void MainWindow::on_actionDebug_toggled(bool checked)
{
    pref["ShowDebug"]=checked;
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
    PreferencesDialog pd(this,profile);
    if (pd.exec()==PreferencesDialog::Accepted) {
        qDebug() << "Preferences Accepted";
        pd.Save();
        if (daily) {
            daily->ReloadGraphs();
            daily->RedrawGraphs();
        }
        if (overview) {
            overview->ReloadGraphs();
            overview->RedrawGraphs();
        }
    }
}

void MainWindow::on_oximetryButton_clicked()
{
    bool first=false;
    if (!oximetry) {
        if (!pref.Exists("HaveCMS50") || !pref["HaveCMS50"].toBool()) {
            if (QMessageBox::question(this,"Question","Do you have a CMS50[x] Oximeter?\nOne is required to use this section.\nNote: This section is not fully completed yet.",QMessageBox::Yes,QMessageBox::No)==QMessageBox::No) return;
            pref["HaveCMS50"]=true;
        }
        oximetry=new Oximetry(ui->tabWidget,profile,daily->SharedWidget());
        ui->tabWidget->insertTab(3,oximetry,tr("Oximetry"));
        first=true;
    }
    ui->tabWidget->setCurrentWidget(oximetry);
    if (!first) oximetry->RedrawGraphs();
    qstatus2->setText("Oximetry");

}

void MainWindow::on_actionCheck_for_Updates_triggered()
{
    netmanager->get(QNetworkRequest(QUrl("http://sleepyhead.sourceforge.net/current_version.txt")));
}
void MainWindow::replyFinished(QNetworkReply * reply)
{
    if (reply->error()==QNetworkReply::NoError) {
        // Wrap this crap in XML/JSON so can do other stuff.
        if (reply->size()>20) {
            qDebug() << "Doesn't look like a version file... :(";
        } else {
            // check in size
            QByteArray data=reply->readAll();
            QString a=data;
            a=a.trimmed();
            if (a>pref["VersionString"].toString()) {
                if (QMessageBox::question(this,"New Version","A newer version of SleepyHead is available, v"+a+".\nWould you like to update?",QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes) {
                    QMessageBox::information(this,"Laziness Warning","I'd love to do it for you automatically, but it's not implemented yet.. :)",QMessageBox::Ok);
                }
            } else {
                QMessageBox::information(this,"SleepyHead v"+pref["VersionString"].toString(),"You're already up to date!\nLatest version on sourceforge is v"+a,QMessageBox::Ok);
            }
       }
    } else {
        qDebug() << "Network Error:" << reply->errorString();
    }
    reply->deleteLater();
}

#include <QPixmap>
#include <QDesktopWidget>
void MainWindow::on_action_Screenshot_triggered()
{

    QTimer::singleShot(250,this,SLOT(DelayedScreenshot()));
}
void MainWindow::DelayedScreenshot()
{
    QPixmap pixmap = QPixmap::grabWindow(this->winId());
    QString a=pref.Get("{home}")+"/Screenshots";
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
        overview->on_printButton_clicked();
    //} else if (ui->tabWidget->currentWidget()==daily) {
    } else {
        QMessageBox::information(this,"Not supported Yet","Sorry, printing from this page is not supported yet",QMessageBox::Ok);
    }
}

void MainWindow::on_action_Edit_Profile_triggered()
{
    NewProfile newprof(this);
    newprof.edit(pref["Profile"].toString());
    newprof.exec();

}
