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
#include <Graphs/gFlagsLine.h>
namespace Ui {
    class Daily;
}

class Daily : public QWidget
{
    Q_OBJECT

public:
    explicit Daily(QWidget *parent,QGLContext *context);
    ~Daily();
    void SetGLContext(QGLContext *context) { shared_context=context; };
    void ReloadGraphs();
    void RedrawGraphs();

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

private:
    Session * CreateJournalSession(QDate date);
    Session * GetJournalSession(QDate date);

    void Load(QDate date);
    void Unload(QDate date);
    void UpdateCalendarDay(QDate date);
    void UpdateEventsTree(QTreeWidget * tree,Day *day);

    gPointData *tap,*tap_eap,*tap_iap,*g_ahi,*frw,*prd,*leak,*pressure_iap,*pressure_eap,*snore;
    gPointData *pulse,*spo2,*rr,*mv,*tv,*mp,*flg,*ptb;

    gFlagsGroup *fg;
    gGraphWindow *PRD,*FRW,*G_AHI,*TAP,*LEAK,*SF,*TAP_EAP,*TAP_IAP,*PULSE,*SPO2,*SNORE,*RR,*MP,*MV,*TV,*FLG,*PTB;

    list<gPointData *> OXIData;
    list<gPointData *> CPAPData;
    list<gGraphWindow *> Graphs;


    void AddCPAPData(gPointData *d) { CPAPData.push_back(d); };
    void AddOXIData(gPointData *d) { OXIData.push_back(d); };
    void AddGraph(gGraphWindow *w);
    void UpdateCPAPGraphs(Day *day);
    void UpdateOXIGraphs(Day *day);

    gPointData *flags[10];

    Ui::Daily *ui;
    Profile *profile;
    QDate previous_date;
    QGLContext *shared_context;
    QScrollArea *scrollArea;
    QSplitter *gSplitter;
    QLabel *NoData;
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
