/********************************************************************
 Daily GUI Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef DAILY_H
#define DAILY_H


#include <QScrollArea>
#include <QSplitter>
#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QtOpenGL/QGLContext>
#include <QWebPluginFactory>
#include <SleepLib/profiles.h>
#include <Graphs/graphwindow.h>
#include <Graphs/graphdata.h>
#include "Graphs/gLineChart.h"
#include <Graphs/gFlagsLine.h>
namespace Ui {
    class Daily;
}

class Daily : public QWidget
{
    Q_OBJECT

public:
    explicit Daily(QWidget *parent,QGLWidget *shared=NULL);
    ~Daily();
    void ReloadGraphs();
    void RedrawGraphs();
    QGLWidget *SharedWidget() { return SF; }

private slots:

    void on_calendar_currentPageChanged(int year, int month);
    void on_calendar_selectionChanged();
    void on_JournalNotesItalic_clicked();
    void on_JournalNotesBold_clicked();
    void on_JournalNotesFontsize_activated(int index);
    void on_JournalNotesColour_clicked();
    void on_EnergySlider_sliderMoved(int position);

    void on_treeWidget_itemSelectionChanged();

    void on_JournalNotesUnderline_clicked();
protected:
    virtual void resizeEvent (QResizeEvent * event);

private:
    Session * CreateJournalSession(QDate date);
    Session * GetJournalSession(QDate date);
    void Load(QDate date);
    void Unload(QDate date);
    void UpdateCalendarDay(QDate date);
    void UpdateEventsTree(QTreeWidget * tree,Day *day);

    //gLineChart *frw,*prd,*leak,*pr_ipap,*pr_epap,*snore,*pulse,*spo2,*rr,*mv,*tv,*mp,*flg,*ptb;

    //gPointData *tap,*tap_eap,*tap_iap,*g_ahi,*frw,*prd,*leak,*pressure_iap,*pressure_eap,*snore;
    //gPointData *pulse,*spo2,*rr,*mv,*tv,*mp,*flg,*ptb;

    gFlagsGroup *fg;
    gGraphWindow *PRD,*FRW,*G_AHI,*TAP,*LEAK,*SF,*TAP_EAP,*TAP_IAP,*PULSE,*SPO2,*SNORE,*RR,*MP,*MV,*TV,*FLG,*PTB;

    list<gLayer *> OXIData;
    list<gLayer *> CPAPData;
    vector<gGraphWindow *> Graphs;
    QGLContext *offscreen_context;

    gLayer * AddCPAP(gLayer *d) { CPAPData.push_back(d); return d; }
    gLayer * AddOXI(gLayer *d) { OXIData.push_back(d); return d; }
    void AddGraph(gGraphWindow *w);
    void UpdateCPAPGraphs(Day *day);
    void UpdateOXIGraphs(Day *day);

    Ui::Daily *ui;
    Profile *profile;
    QDate previous_date;
    QScrollArea *scrollArea;
    QSplitter *gSplitter;
    QLabel *NoData;
    QWidget *spacer;
};

/*class AHIGraph:public QWebPluginFactory
{
public:
    AHIGraph(QObject * parent = 0);
    virtual ~AHIGraph();
    virtual QObject * 	create ( const QString & mimeType, const QUrl & url, const QStringList & argumentNames, const QStringList & argumentValues) const;
    virtual QList<Plugin> plugins () const;
    //virtual void refreshPlugins ();
}; */

#endif // DAILY_H
