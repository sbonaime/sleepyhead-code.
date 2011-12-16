#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QResource>
#include <QProgressBar>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QDate>
#include <QDebug>
#include <QXmlSimpleReader>
#include <QCryptographicHash>
#include <QDesktopWidget>
#include "SleepLib/preferences.h"
#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "UpdaterWindow.h"
#include "ui_UpdaterWindow.h"
#include "version.h"
#include "mainwindow.h"
#include "common_gui.h"

extern MainWindow * mainwin;

UpdaterWindow::UpdaterWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::UpdaterWindow)
{
    ui->setupUi(this);


    QDesktopWidget *desktop = QApplication::desktop();

    int screenWidth = desktop->width(); // get width of screen
    int screenHeight = desktop->height(); // get height of screen

    QSize windowSize = size(); // size of our application window
    int width = windowSize.width();
    int height = windowSize.height();

    // little computations
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;
    y -= 50;

    // move window to desired coordinates
    move ( x, y );



    requestmode=RM_None;
    netmanager = new QNetworkAccessManager(this);
    connect(netmanager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    update=NULL;
    ui->stackedWidget->setCurrentIndex(0);
}

UpdaterWindow::~UpdaterWindow()
{
    disconnect(netmanager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    delete ui;
}


void UpdaterWindow::checkForUpdates()
{
    QString filename=QApplication::applicationDirPath()+QDir::separator()+"update.xml";
    // Check updates.xml file if it's still recent..
    if (QFile::exists(filename)) {
        QFileInfo fi(filename);
        QDateTime created=fi.created();
        int age=created.secsTo(QDateTime::currentDateTime());

        if (age<7200) {
            QFile file(filename);
            file.open(QFile::ReadOnly);
            ParseUpdateXML(&file);
            file.close();
            return;
        }
    }
    mainwin->Notify("Checking for SleepyHead Updates");

    requestmode=RM_CheckUpdates;

    reply=netmanager->get(QNetworkRequest(QUrl("http://192.168.1.8/update.xml")));
    ui->plainTextEdit->appendPlainText("Requesting "+reply->url().toString());
    netmanager->connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this, SLOT(downloadProgress(qint64,qint64)));
    dltime.start();
}

void UpdaterWindow::dataReceived()
{
    //HttpStatusCodeAttribute
    QString rs=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
    if (rs!="200") return;
    //QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

    QByteArray read=reply->read(reply->bytesAvailable());
    qDebug() << "Received" << read.size() << "bytes";
    file.write(read);
}

void UpdaterWindow::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QString rs=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
    if (rs!="200") return;

    if (ui->tableWidget->rowCount()>0) {
        double f=(double(bytesReceived) / double(bytesTotal)) * 100.0;
        QProgressBar *bar=qobject_cast<QProgressBar *>(ui->tableWidget->cellWidget(current_row,3));
        if (bar)
            bar->setValue(f);

        ui->tableWidget->item(current_row,2)->setText(QString::number(bytesTotal/1048576.0,'f',3)+"MB");
    }
    //ui->progressBar->setValue(f);
    int elapsed=dltime.elapsed();
}

void UpdaterWindow::requestFile()
{
    if (!update) return;
    QProgressBar *bar=qobject_cast<QProgressBar *>(ui->tableWidget->cellWidget(current_row,3));
    QString style="QProgressBar{\
            border: 1px solid gray;\
            border-radius: 3px;\
            text-align: center;\
            text-decoration: bold;\
            color: yellow;\
            }\
            QProgressBar::chunk {\
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 \"light green\", stop: 1 green);\
            width: 10px;\
            margin: 0px;\
            }";

    if (bar)
        bar->setStyleSheet(style);

    QString filename=update->filename;
    ui->plainTextEdit->appendPlainText("Requesting "+update->url);

    requestmode=RM_GetFile;

    QString path=QApplication::applicationDirPath()+QDir::separator()+"Download";
    QDir().mkdir(path);
    path+=QDir::separator()+filename;
    ui->plainTextEdit->appendPlainText("Saving as "+path);
    file.setFileName(path);
    file.open(QFile::WriteOnly);
    dltime.start();

    QNetworkRequest req=QNetworkRequest(QUrl(update->url));
    req.setRawHeader("User-Agent", "Wget/1.12 (linux-gnu)");
    reply=netmanager->get(req);
    connect(reply,SIGNAL(readyRead()),this, SLOT(dataReceived()));
    connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this, SLOT(downloadProgress(qint64,qint64)));
}

void UpdaterWindow::ParseUpdateXML(QIODevice * dev)
{
    QXmlInputSource src(dev);
    QXmlSimpleReader reader;
    reader.setContentHandler(&updateparser);
    if (reader.parse(src)) {
        ui->plainTextEdit->appendPlainText("XML update structure parsed cleanly");

        QStringList versions;
        for (QHash<QString,Release>::iterator it=updateparser.releases.begin();it!=updateparser.releases.end();it++) {
            versions.push_back(it.key());
        }
        qSort(versions);

        QString platform=PlatformString.toLower();
        release=NULL;
        for (int i=versions.size()-1;i>=0;i--) {
            QString verstr=versions[i];
            release=&updateparser.releases[verstr];
            if (release->updates.contains(platform)) {
                break;
            } else release=NULL;
        }
        if (!release || (VersionString() > release->version)) {
            mainwin->Notify("No updates were found for your platform",5000,"SleepyHead Updates");
            delay(4000);
            close();
            return;
        }


        qDebug() << "Version" << release->version << "has updates for" << platform;

        QString latestapp="", latestqt="";
        updates.clear();
        Update *upd=NULL;
        Update *upq=NULL;
        for (int i=0;i<release->updates[platform].size();i++) {
            update=&release->updates[platform][i];
            if (update->type=="qtlibs") {
                qDebug() << "QT Version" << update->version;
                if (update->version > latestqt) {
                    latestqt=update->version;
                    upq=update;
                }
            } else if (update->type=="application") {
                qDebug() << "Application Version" << update->version;
                if (update->version > latestapp) {
                    latestapp=update->version;
                    upd=update;
                }
            }
        }

        if (upq && (upq->version > QT_VERSION_STR)) {
            updates.push_back(upq);
        }
        if (upd && upd->version > VersionString()) {
            updates.push_back(upd);
        }


        if (updates.size()>0) {
            QString html="<html><h3>SleepyHead v"+release->version+" codename \""+release->codename+"\"</h3><p>"+release->notes[""]+"</p><b>";
            html+=platform.left(1).toUpper()+platform.mid(1);
            html+=" platform notes</b><p>"+release->notes[platform]+"</p></html>";
            ui->webView->setHtml(html);
            QString info;
            if (VersionString()< release->version) {
                ui->Title->setText("<font size=+1>A new version of SleepyHead is available!</font>");
                info="Shiny new <b>v"+latestapp+"</b> is available. You're running old and busted v"+VersionString();
                ui->notesTabWidget->setCurrentIndex(0);
            } else {
                ui->Title->setText("<font size=+1>An update for SleepyHead is available.</font>");
                info="Version <b>"+latestapp+"</b> is available. You're currently running v"+VersionString();
                ui->notesTabWidget->setCurrentIndex(1);
            }
            ui->versionInfo->setText(info);

            QString notes;
            for (int i=0;i<release->updates[platform].size();i++) {
                update=&release->updates[platform][i];
                if ((update->type=="application") && (update->version > VersionString())) {
                    notes+="<b>SleepyHead v"+update->version+" build notes</b><br/>"+update->notes.trimmed()+"<br/><br/>";
                } else if ((update->type=="qtlibs") && (update->version > QT_VERSION_STR)) {
                    notes+="<b>Update to QtLibs (v"+update->version+")</b><br/>"+update->notes.trimmed();
                }
            }
            ui->buildNotes->setText(notes);
            setWindowModality(Qt::ApplicationModal);
            show();
        }
    } else {
        mainwin->Notify("There was an error parsing the XML Update file.");
    }
}

void UpdaterWindow::replyFinished(QNetworkReply * reply)
{
    netmanager->disconnect(reply,SIGNAL(downloadProgress(qint64,qint64)),this, SLOT(downloadProgress(qint64,qint64)));
    if (reply->error()==QNetworkReply::NoError) {
        if (requestmode==RM_CheckUpdates) {
            ui->plainTextEdit->appendPlainText(QString::number(reply->size())+" bytes received.");
            QString filename=QApplication::applicationDirPath()+QDir::separator()+reply->url().toString().section("/",-1);
            qDebug() << filename;
            QFile file(filename);
            file.open(QFile::WriteOnly);
            file.write(reply->readAll());
            file.close();
            file.open(QFile::ReadOnly);
            //QTextStream ts(&file);
            ParseUpdateXML(&file);
            file.close();
            reply->deleteLater();
        } else if (requestmode==RM_GetFile) {
            disconnect(reply,SIGNAL(readyRead()),this, SLOT(dataReceived()));
            file.close();
            //HttpStatusCodeAttribute
            QString rs=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();

            bool failed=false;
            if (rs=="404") {
                qDebug() << "File not found";
                failed=true;
            } else {
                qDebug() << "StatCodeAttr" << rs;
                QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                if (!redirectUrl.isEmpty() && (redirectUrl!=reply->url())) {
                    file.open(QFile::WriteOnly); //reopen file..
                    update->url=redirectUrl.toString();
                    ui->plainTextEdit->appendPlainText("Redirected to "+update->url);
                    QTimer::singleShot(100,this,SLOT(requestFile()));
                    reply->deleteLater();
                    return;
                }
                ui->plainTextEdit->appendPlainText("Received "+QString::number(file.size())+" bytes");
                if (update->size>0) {
                    double s1=update->size/1048576.0;
                    double s2=ui->tableWidget->item(current_row,2)->text().toDouble();
                    if (s1!=s2) {
                        failed=true;
                        ui->plainTextEdit->appendPlainText("File size mismatch for "+update->filename);
                    }
                } else {
                    QString path=QApplication::applicationDirPath()+QDir::separator()+"Download";
                    path+=QDir::separator()+update->filename;
                    QFile f(path);
                    f.open(QFile::ReadOnly);
                    QCryptographicHash hash(QCryptographicHash::Sha1);
                    hash.addData(f.readAll());
                    QString res=hash.result().toHex();
                    if (res!=update->hash) {
                        ui->plainTextEdit->appendPlainText("File integrity check failed for "+update->filename);
                        failed=true;
                    }
                }
            }
            reply->deleteLater();
            QProgressBar *bar=qobject_cast<QProgressBar *>(ui->tableWidget->cellWidget(current_row,3));
            if (!failed) {
                //file.open(QFile::ReadOnly);
                QuaZip zip(&file);

                if (!zip.open(QuaZip::mdUnzip)) {
                    failed=true;
                } else {

                    QStringList files=zip.getFileNameList();
                    QFile f;
                    int errors=0;
                    int fsize=files.size();
                    QByteArray ba;
                    QStringList update_txt;

                    QString apppath=QApplication::applicationDirPath()+QDir::separator();
                    QString backups=apppath+"Backups"+QDir::separator();
                    QString downloads=apppath+"Downloads"+QDir::separator();
                    QDir().mkpath(backups);

                    for (int i=0;i<fsize;i++) {
                        ui->plainTextEdit->appendPlainText("Extracting "+files.at(i));
                        QuaZipFile qzf(file.fileName(),files.at(i));
                        qzf.open(QuaZipFile::ReadOnly);

                        QString path=downloads+files.at(i);
                        if (path.endsWith(QDir::separator())) {
                            QDir().mkpath(path);
                            if (update->unzipped_path.isEmpty())
                                update->unzipped_path=path;
                        } else {
                            ba=qzf.readAll();
                            if (qzf.getZipError()) {
                                errors++;
                            } else if (files.at(i)=="update.txt") {
                                QTextStream ts(ba);
                                QString line;
                                do {
                                    line=ts.readLine();
                                    if (!line.isNull()) update_txt.append(line);
                                } while (!line.isNull());
                            } else {
                                QString fn=files.at(i).section("/",-1);
                                if (f.exists(backups+fn)) f.remove(backups+fn);
                                if (!f.rename(apppath+fn,backups+fn)) {
                                    errors++;
                                }

                                f.setFileName(path);
                                f.open(QFile::WriteOnly);
                                f.write(ba);
                                f.close();
                            }
                        }
                        if (bar) {
                            bar->setValue((1.0/float(fsize)*float(i+1))*100.0);
                            QApplication::processEvents();
                        }
                        qzf.close();
                    }
                    zip.close();
                    if (!errors) {
                    }
                }


            }
            ui->tableWidget->item(current_row,0)->setCheckState(Qt::Checked);
            if (failed) {
                qDebug() << "File is corrupted";
                if (bar) {
                    bar->setFormat("Failed");
                    QString style="QProgressBar{\
                        border: 1px solid gray;\
                        border-radius: 3px;\
                        text-align: center;\
                        text-decoration: bold;\
                        color: yellow;\
                        }\
                        QProgressBar::chunk {\
                        background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 \"dark red\", stop: 1 red);\
                        width: 10px;\
                        margin: 0px;\
                        }";
                    //: qlineargradient(x1: 0, y1: 0.5, x2: 1, y2: 0.5, stop: 0 red, stop: 1 white);
                    bar->setStyleSheet(style);
                }
            }
            ui->tableWidget->item(current_row,0)->setData(Qt::UserRole+1,failed);
            QTimer::singleShot(100,this,SLOT(upgradeNext()));
            ui->plainTextEdit->appendPlainText("Download Complete");

    } else if (requestmode==RM_UpdateQT) {
            ui->plainTextEdit->appendPlainText("Received "+QString::number(reply->size())+" bytes");
        }
    } else {
        mainwin->Notify("There was an error completing a network request:\n\n("+reply->errorString()+")");
    }
}

void UpdaterWindow::on_CloseButton_clicked()
{
    close();
}

void UpdaterWindow::upgradeNext()
{
    QTableWidgetItem *item;
    bool fnd=false;
    for (current_row=0;current_row<ui->tableWidget->rowCount();current_row++) {
        item=ui->tableWidget->item(current_row,0);
        bool complete=item->checkState()==Qt::Checked;
        if (complete)
            continue;
        update=item->data(Qt::UserRole).value<Update *>();
        qDebug() << "Processing" << update->url;
        fnd=true;
        requestFile();
        break;
    }

    if (!fnd) {
        bool ok=true;
        for (current_row=0;current_row<ui->tableWidget->rowCount();current_row++) {
            bool failed=ui->tableWidget->item(current_row,0)->data(Qt::UserRole+1).toBool();
            if (failed) {
                ok=false;
                break;
            }
        }
        if (ok) {
            success=true;
            //QMessageBox::information(this,"Updates Complete","SleepyHead has been updated and needs to restart.",QMessageBox::Ok);
            ui->downloadTitle->setText("Update Complete!");
            ui->FinishedButton->setVisible(true);
            ui->downloadLabel->setText("Updates Complete. SleepyHead needs to restart now, click Finished to do so.");
        } else {
            ui->downloadTitle->setText("Update Failed :(");
            success=false;
            ui->downloadLabel->setText("Download Error. Sorry, try again later.");
            ui->FinishedButton->setVisible(true);
            //QMessageBox::warning(this,"Download Error","Sorry, could not get all necessary files for upgrade.. Try again later.",QMessageBox::Ok);
            //close();
        }
    }
}


void UpdaterWindow::on_upgradeButton_clicked()
{
    if (!updates.size()) return;
    ui->tableWidget->clearContents();
    ui->tableWidget->setColumnHidden(4,true);
    ui->tableWidget->setColumnHidden(5,true);
    ui->FinishedButton->setVisible(false);
    ui->downloadLabel->setText("Downloading & Installing Updates...");
    ui->downloadTitle->setText("Please wait while downloading and installing updates.");
    success=false;
    for (int i=0;i<updates.size();i++) {
        update=updates.at(i);
        ui->tableWidget->insertRow(i);
        QTableWidgetItem *item=new QTableWidgetItem(update->type);
        QVariant av;
        av.setValue(update);
        item->setData(Qt::UserRole,av);
        item->setCheckState(Qt::Unchecked);
        item->setFlags(Qt::ItemIsEnabled);
        ui->tableWidget->setItem(i,0,item);
        ui->tableWidget->setItem(i,1,new QTableWidgetItem(update->version));
        ui->tableWidget->setItem(i,2,new QTableWidgetItem(QString::number(update->size/1048576.0,'f',3)+"MB"));
        QProgressBar *bar=new QProgressBar(ui->tableWidget);
        bar->setMaximum(100);
        bar->setValue(0);

        ui->tableWidget->setCellWidget(i,3,bar);
        ui->tableWidget->setItem(i,4,new QTableWidgetItem(update->url));
    }
    ui->stackedWidget->setCurrentIndex(1);
    upgradeNext();
}

void UpdaterWindow::on_FinishedButton_clicked()
{
    if (success)
        mainwin->RestartApplication();
    else close();
}
