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

#include "version.h"
#include "daily.h"
#include "overview.h"
#include "oximetry.h"
#include "preferencesdialog.h"

extern Profile * profile;

namespace Ui {
    class MainWindow;
}

extern QStatusBar *qstatusbar;

class Daily;
class Report;
class Overview;

/*! \class MainWindow
    \author Mark Watkins
    \brief The Main Application window for SleepyHead
    */

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //! \brief Log message s to Application Debug log
    void Log(QString s);


    //! \brief Create a new menu object in the main menubar.
    QMenu * CreateMenu(QString title);

    //! \brief Start the automatic update checker process
    void CheckForUpdates();

    /*! \fn Notify(QString s,int ms=5000, QString title="SleepyHead v"+VersionString());
        \brief Pops up a message box near the system tray
        \param QString string
        \param title
        \param int ms

        Title is shown in bold
        string is the main message content to show
        ms = millisecond delay of how long to show popup

        Mac needs Growl notification system for this to work
        */
    void Notify(QString s, QString title="SleepyHead v"+VersionString(), int ms=5000);

    /*! \fn gGraphView *snapshotGraph()
        \brief Returns the current snapshotGraph object used by the report printing system */
    gGraphView *snapshotGraph() { return SnapshotGraph; }

    Daily *getDaily() { return daily; }
    Overview *getOverview() { return overview; }
    Oximetry *getOximetry() { return oximetry; }

    /*! \fn void RestartApplication(bool force_login=false);
        \brief Closes down SleepyHead and restarts it
        \param bool force_login

        If force_login is set, it will return to the login menu even if it's set to skip
        */
    void RestartApplication(bool force_login=false);

    //! \brief Self explainitory, selects the Oximetry Tab
    void selectOximetryTab();

    void JumpDaily();

    /*! \fn void PrintReport gGraphView *gv,QString name, QDate date=QDate::currentDate());
        \brief Prepares a report using gGraphView object, and sends to a created QPrinter object
        \param gGraphView *gv  GraphView Object containing which graph set to print
        \param QString name   Report Title
        \param QDate date
        */
    void PrintReport(gGraphView *gv,QString name, QDate date=QDate::currentDate());

private slots:
    /*! \fn void on_action_Import_Data_triggered();
        \brief Provide the file dialog for selecting import location, and start the import process
        This is called when the Import button is clicked
        */
    void on_action_Import_Data_triggered();

    //! \brief Selects the welcome page when it's clicked on
    void on_actionView_Welcome_triggered();

    //! \brief Toggle Fullscreen (currently F11)
    void on_action_Fullscreen_triggered();

    //! \brief Loads the default page in the Welcome screens web browser
    void on_homeButton_clicked();

    //! \brief Go back in the welcome browsers history
    void on_backButton_clicked();

    //! \brief Go forward in the welcome browsers history
    void on_forwardButton_clicked();

    //! \brief Updates the URL bar to show changes to the URL
    void on_webView_urlChanged(const QUrl &arg1);

    //! \brief Loads a web page when enter is pressed in the URL bar
    void on_urlBar_activated(const QString &arg1);

    //! \brief Selects the Daily page and redraws the graphs
    void on_dailyButton_clicked();

    //! \brief Selects the Overview page and redraws the graphs
    void on_overviewButton_clicked();

    //! \brief called when webpage has finished loading in welcome browser
    void on_webView_loadFinished(bool arg1);

    //! \brief called when webpage has starts loading in welcome browser
    void on_webView_loadStarted();

    //! \brief Updates the progress bar in the statusbar while a page is loading
    void on_webView_loadProgress(int progress);

    //! \brief Display About Dialog
    void on_action_About_triggered();

    //! \brief Called after a timeout to initiate loading of all summary data for this profile
    void Startup();

    //! \brief Toggle the Debug pane on and off
    void on_actionDebug_toggled(bool arg1);

    //! \brief passes the ResetGraphLayout menu click to the Daily & Overview views
    void on_action_Reset_Graph_Layout_triggered();

    //! \brief Opens the Preferences Dialog, and saving changes if OK is pressed
    void on_action_Preferences_triggered();

    //! \brief Opens and/or shows the Oximetry page
    void on_oximetryButton_clicked();

    //! \brief Creates the UpdaterWindow object that actually does the real check for updates
    void on_actionCheck_for_Updates_triggered();

    //! \brief Attempts to do a screenshot of the application window
    //! \note This is currently broken on Macs
    void on_action_Screenshot_triggered();

    //! \brief This is the actual screenshot code.. It's delayed with a QTimer to give the menu time to close.
    void DelayedScreenshot();

    //! \brief a slot that calls the real Oximetry tab selector
    void on_actionView_O_ximetry_triggered();

    //! \brief Updates the Statusbar message with the QString message contained in Text
    void updatestatusBarMessage (const QString & text);

    //! \brief Passes the Daily, Overview & Oximetry object to Print Report, based on current tab
    void on_actionPrint_Report_triggered();

    //! \brief Opens the Profile Editor
    void on_action_Edit_Profile_triggered();

    //! \brief Toggled the LinkGraphGroups preference, which forces the link between Oximetry & CPAP days
    void on_action_Link_Graph_Groups_toggled(bool arg1);

    //! \brief Selects the next view tab
    void on_action_CycleTabs_triggered();

    //! \brief Opens the CSV Export window
    void on_actionExp_ort_triggered();

    //! \brief Opens the User Guide at the wiki in the welcome browser.
    void on_actionOnline_Users_Guide_triggered();

    //! \brief Opens the Frequently Asked Questions at the wiki in the welcome browser.
    void on_action_Frequently_Asked_Questions_triggered();

    /*! \fn void on_action_Rebuild_Oximetry_Index_triggered();
        \brief This function scans over all oximetry data and reindexes and tries to fix any inconsistencies.
        */
    void on_action_Rebuild_Oximetry_Index_triggered();

    //! \brief Log out, by effectively forcing a restart
    void on_actionChange_User_triggered();

    //! \brief Destroy the CPAP data for the currently selected day, so it can be freshly imported again
    void on_actionPurge_Current_Day_triggered();

    //! \brief Destroy ALL the CPAP data for the currently selected machine, so it can be freshly imported again
    void on_actionAll_Data_for_current_CPAP_machine_triggered();

private:

    Ui::MainWindow *ui;
    Daily * daily;
    Overview * overview;
    Oximetry * oximetry;
    bool first_load;
    PreferencesDialog *prefdialog;
    QMutex loglock,strlock;
    QStringList logbuffer;
    QTime logtime;
    QSystemTrayIcon *systray;
    QMenu *systraymenu;
    gGraphView *SnapshotGraph;
};

#endif // MAINWINDOW_H
