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
#include <QSystemTrayIcon>

#include "daily.h"
#include "overview.h"
#include "oximetry.h"
#include "report.h"
#include "preferencesdialog.h"

const int major_version=0;
const int minor_version=8;
const int revision_number=8;

extern Profile * profile;

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
    void CheckForUpdates();
    void Notify(QString s);
    gGraphView *snapshotGraph() { return SnapshotGraph; }
    Daily *getDaily() { return daily; }
    Overview *getOverview() { return overview; }
    Oximetry *getOximetry() { return oximetry; }
    void JumpDaily();
    void PrintReport(gGraphView *gv,QString name, QDate date=QDate::currentDate());
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

    void on_actionDebug_toggled(bool arg1);

    void on_action_Reset_Graph_Layout_triggered();

    void on_action_Preferences_triggered();

    void on_oximetryButton_clicked();

    void on_actionCheck_for_Updates_triggered();

    void replyFinished(QNetworkReply*);
    void on_action_Screenshot_triggered();
    void DelayedScreenshot();

    void on_actionView_O_ximetry_triggered();
    void updatestatusBarMessage (const QString & text);
    void on_actionPrint_Report_triggered();

    void on_action_Edit_Profile_triggered();

    void on_action_Link_Graph_Groups_toggled(bool arg1);

    void on_action_CycleTabs_triggered();

    void on_actionExp_ort_triggered();

    void on_actionOnline_Users_Guide_triggered();

    void on_action_Frequently_Asked_Questions_triggered();

    void on_action_Rebuild_Oximetry_Index_triggered();

    void on_actionChange_User_triggered();

private:

    Ui::MainWindow *ui;
    Daily * daily;
    Overview * overview;
    Oximetry * oximetry;
    bool first_load;
    //Profile *profile;
    QNetworkAccessManager *netmanager;
    PreferencesDialog *prefdialog;
    QMutex loglock,strlock;
    QStringList logbuffer;
    QTime logtime;
    QSystemTrayIcon *systray;
    QMenu *systraymenu;
    gGraphView *SnapshotGraph;
};

#endif // MAINWINDOW_H
