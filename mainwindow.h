/*
 MainWindow Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGLContext>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "daily.h"
#include "overview.h"
#include "oximetry.h"
#include "report.h"

const int major_version=0;
const int minor_version=8;
const int revision_number=5;

namespace Ui {
    class MainWindow;
}

extern QStatusBar *qstatusbar;

class Daily;
class Report;
class Overview;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void Log(QString s);
    QMenu * CreateMenu(QString title);

private slots:
    void on_action_Import_Data_triggered();

    void on_actionView_Welcome_triggered();

    void on_action_Fullscreen_triggered();

    void on_homeButton_clicked();

    void on_backButton_clicked();

    void on_forwardButton_clicked();

    void on_webView_urlChanged(const QUrl &arg1);

    void on_urlBar_activated(const QString &arg1);

    void on_dailyButton_clicked();

    void on_overviewButton_clicked();

    void on_webView_loadFinished(bool arg1);

    void on_webView_loadStarted();

    void on_webView_loadProgress(int progress);

    void on_action_About_triggered();

    void Startup();

    void on_action_Link_Graphs_triggered(bool checked);

    void on_actionUse_AntiAliasing_triggered(bool checked);

    void on_actionDebug_toggled(bool arg1);

    void on_actionOverlay_Bars_toggled(bool arg1);

    void on_action_Reset_Graph_Layout_triggered();

    void on_action_Preferences_triggered();

    void on_oximetryButton_clicked();

    void on_actionEnable_Multithreading_toggled(bool arg1);

    void on_actionCheck_for_Updates_triggered();

    void replyFinished(QNetworkReply*);
    void on_action_Screenshot_triggered();
    void DelayedScreenshot();

    void on_actionView_O_ximetry_triggered();
    void updatestatusBarMessage (const QString & text);
    void on_actionPrint_Report_triggered();

private:
    Ui::MainWindow *ui;
    Daily * daily;
    Overview * overview;
    Oximetry * oximetry;
    bool first_load;
    Profile *profile;
    QNetworkAccessManager *netmanager;

    QMutex loglock,strlock;
    QStringList logbuffer;
    QTime logtime;

};

#endif // MAINWINDOW_H
