/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * MainWindow Headers
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

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

extern Profile *profile;

namespace Ui {
class MainWindow;
}

/*! \mainpage SleepyHead

 \section intro_sec Introduction

 SleepyHead is Cross-Platform Open-Source software for reviewing data from %CPAP machines, which are used in the treatment of Sleep Disorders.

 SleepyHead has been created by <a href="http://jedimark64.blogspot.com">Mark Watkins</a> (JediMark), an Australian software developer.

 This document is an attempt to provide a little technical insight into SleepyHead's program internals.

 \section project_info Further Information
 The project is hosted on sourceforge, and it's project page can be reached at <a href="http://sourceforge.net/projects/sleepyhead">http://sourceforge.net/projects/sleepyhead</a>.

 There is also the <a href="http://sleepyhead.sourceforge.net">SleepyHead Wiki</a> containing further information

 \section structure Program Structure
 SleepyHead is written in C++ using Qt Toolkit library, and comprises of 3 main components
 \list
 \li The SleepLib Database, a specialized database for working with multiple sources of Sleep machine data.
 \li A custom designed, high performance and interactive OpenGL Graphing Library.
 \li and the main Application user interface.
 \endlist

 This document is still a work in progress, right now all the classes and sections are jumbled together.

 */

// * \section install_sec Installation

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

    //! \brief Update the list of Favourites (Bookmarks) in the right sidebar.
    void updateFavourites();

    //! \brief Update statistics report
    void GenerateStatistics();

    //! \brief Create a new menu object in the main menubar.
    QMenu *CreateMenu(QString title);

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
    void Notify(QString s, QString title = "SleepyHead v" + VersionString, int ms = 5000);

    /*! \fn gGraphView *snapshotGraph()
        \brief Returns the current snapshotGraph object used by the report printing system */
    gGraphView *snapshotGraph() { return SnapshotGraph; }

    //! \brief Returns the Daily Tab object
    Daily *getDaily() { return daily; }

    //! \brief Returns the Overview Tab object
    Overview *getOverview() { return overview; }

    //! \brief Returns the Oximetry Tab object
    Oximetry *getOximetry() { return oximetry; }

    /*! \fn void RestartApplication(bool force_login=false);
        \brief Closes down SleepyHead and restarts it
        \param bool force_login

        If force_login is set, it will return to the login menu even if it's set to skip
        */
    static void RestartApplication(bool force_login = false, bool change_datafolder = false);

    //! \brief Self explainitory, selects the Oximetry Tab
    void selectOximetryTab();

    void JumpDaily();

    void sendStatsUrl(QString msg) { on_recordsBox_linkClicked(QUrl(msg)); }

    //! \brief Sets up recalculation of all event summaries and flags
    void reprocessEvents(bool restart = false);


    //! \brief Internal function to set Records Box html from statistics module
    void setRecBoxHTML(QString html);

    void closeEvent(QCloseEvent *);

  public slots:
    //! \brief Recalculate all event summaries and flags
    void doReprocessEvents();

  protected:
    virtual void keyPressEvent(QKeyEvent *event);

  private slots:
    /*! \fn void on_action_Import_Data_triggered();
        \brief Provide the file dialog for selecting import location, and start the import process
        This is called when the Import button is clicked
        */
    void on_action_Import_Data_triggered();

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
    void on_actionView_Oximetry_triggered();

    //! \brief Updates the Statusbar message with the QString message contained in Text
    void updatestatusBarMessage(const QString &text);

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

    void on_action_Sidebar_Toggle_toggled(bool arg1);

    void on_recordsBox_linkClicked(const QUrl &arg1);

    void on_helpButton_clicked();

    void on_actionView_Statistics_triggered();

    void on_webView_linkClicked(const QUrl &arg1);

    //void on_favouritesList_itemSelectionChanged();

    //void on_favouritesList_itemClicked(QListWidgetItem *item);

    void on_webView_statusBarMessage(const QString &text);

    //! \brief Display Help WebView Link in statusbar.
    void LinkHovered(const QString &link, const QString &title, const QString &textContent);
    void on_tabWidget_currentChanged(int index);

    void on_bookmarkView_linkClicked(const QUrl &arg1);

    void on_filterBookmarks_editingFinished();

    void on_filterBookmarksButton_clicked();

    void on_actionImport_ZEO_Data_triggered();

    void on_actionImport_RemStar_MSeries_Data_triggered();

    void on_actionSleep_Disorder_Terms_Glossary_triggered();

    void on_actionHelp_Support_SleepyHead_Development_triggered();

    void aboutBoxLinkClicked(const QUrl &url);

    void on_actionChange_Language_triggered();

    void on_actionChange_Data_Folder_triggered();

    void on_actionImport_Somnopose_Data_triggered();

    //! \brief Populates the statistics with information.
    void on_statisticsButton_clicked();

    void on_statisticsView_linkClicked(const QUrl &arg1);

    void on_reportModeMonthly_clicked();

    void on_reportModeStandard_clicked();

private:
    int importCPAP(const QString &path, const QString &message);
    void importCPAPBackups();
    void finishCPAPImport();
    QStringList detectCPAPCards();

    QString getWelcomeHTML();
    void FreeSessions();

    Ui::MainWindow *ui;
    Daily *daily;
    Overview *overview;
    Oximetry *oximetry;
    bool first_load;
    PreferencesDialog *prefdialog;
    QMutex loglock, strlock;
    QStringList logbuffer;
    QTime logtime;
    QSystemTrayIcon *systray;
    QMenu *systraymenu;
    gGraphView *SnapshotGraph;
    QString bookmarkFilter;
    bool m_restartRequired;
    volatile bool m_inRecalculation;
};

#endif // MAINWINDOW_H
