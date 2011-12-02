/*
 Daily GUI Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef DAILY_H
#define DAILY_H


#include <QMenu>
#include <QAction>
#include <QWidget>
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QtOpenGL/QGLContext>
#include <QScrollBar>
#include <QTableWidgetItem>
#include "Graphs/gSummaryChart.h"

#include <SleepLib/profiles.h>
#include "mainwindow.h"
#include "Graphs/gGraphView.h"

#include "Graphs/gLineChart.h"

namespace Ui {
    class Daily;
}


class MainWindow;
class Daily : public QWidget
{
    Q_OBJECT

public:
    explicit Daily(QWidget *parent, gGraphView *shared,MainWindow *mw);
    ~Daily();
    void ReloadGraphs();
    void ResetGraphLayout();
    gGraphView *graphView() { return GraphView; }
    void RedrawGraphs();
    void LoadDate(QDate date);
    QDate getDate() { return previous_date; }

    void PrintReport();
    void UnitsChanged();

private slots:

    void on_calendar_currentPageChanged(int year, int month);
    void on_calendar_selectionChanged();
    void on_JournalNotesItalic_clicked();
    void on_JournalNotesBold_clicked();
    void on_JournalNotesFontsize_activated(int index);
    void on_JournalNotesColour_clicked();

    void on_treeWidget_itemSelectionChanged();

    void on_JournalNotesUnderline_clicked();
    void ShowHideGraphs();
    void on_prevDayButton_clicked();

    void on_nextDayButton_clicked();

    void on_calButton_toggled(bool checked);

    void on_todayButton_clicked();

    void on_Link_clicked(const QUrl &url);
    void on_evViewSlider_valueChanged(int value);

    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

    void on_graphtogglebutton_toggled(bool);
    void on_ZombieMeter_actionTriggered(int action);

    void on_addBookmarkButton_clicked();

    void on_removeBookmarkButton_clicked();

    void on_bookmarkTable_itemActivated(QTableWidgetItem *item);

    void on_bookmarkTable_itemClicked(QTableWidgetItem *item);

    void on_bookmarkTable_itemChanged(QTableWidgetItem *item);

protected:

private:
    Session * CreateJournalSession(QDate date);
    Session * GetJournalSession(QDate date);
    void Load(QDate date);
    void Unload(QDate date);
    void UpdateCalendarDay(QDate date);
    void UpdateEventsTree(QTreeWidget * tree,Day *day);

    gGraph *PRD,*FRW,*GAHI,*TAP,*LEAK,*SF,*TAP_EAP,*TAP_IAP,*PULSE,*SPO2,
           *SNORE,*RR,*MP,*MV,*TV,*FLG,*PTB,*OF,*INTPULSE,*INTSPO2, *THPR,
           *PLETHY,*TI,*TE, *RE, *IE, *TgMV, *AHI;

    QList<Layer *> OXIData;
    QList<Layer *> CPAPData;
    QHash<QString,QPushButton *> GraphToggles;
    QVector<QAction *> GraphAction;
    QGLContext *offscreen_context;

    QList<int> splitter_sizes;
    Layer * AddCPAP(Layer *d) { CPAPData.push_back(d); return d; }
    Layer * AddOXI(Layer *d) { OXIData.push_back(d); return d; }

    void UpdateCPAPGraphs(Day *day);
    void UpdateOXIGraphs(Day *day);

    MainWindow * mainwin;
    Ui::Daily *ui;
    QDate previous_date;
    QMenu *show_graph_menu;

    gGraphView *GraphView,*snapGV;
    MyScrollBar *scrollbar;
    QHBoxLayout *layout;
    bool ZombieMeterMoved;
    bool BookmarksChanged;
};

#endif // DAILY_H
