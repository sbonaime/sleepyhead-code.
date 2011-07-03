/********************************************************************
 MainWindow Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGLContext>

#include "daily.h"
#include "overview.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

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

    void on_action_Noon_Date_Split_toggled(bool );

private:
    Ui::MainWindow *ui;
    Daily * daily;
    Overview * overview;
    QGLContext *shared_context;
    bool first_load;
    Profile *profile;
};

#endif // MAINWINDOW_H
