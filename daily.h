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
#include <SleepLib/profiles.h>
#include <Graphs/graphwindow.h>
#include <Graphs/graphdata.h>

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

private slots:

    void on_calendar_currentPageChanged(int year, int month);
    void on_calendar_selectionChanged();
    void on_JournalNotesItalic_clicked();
    void on_JournalNotesBold_clicked();
    void on_JournalNotesFontsize_activated(int index);
    void on_JournalNotesColour_clicked();
    void on_EnergySlider_sliderMoved(int position);

    void on_treeWidget_itemSelectionChanged();

private:
    Session * CreateJournalSession(QDate date);
    Session * GetJournalSession(QDate date);

    void Load(QDate date);
    void Unload(QDate date);
    void UpdateCalendarDay(QDate date);
    void UpdateEventsTree(QTreeWidget * tree,Day *day);

    gPointData *tap,*tap_eap,*tap_iap,*g_ahi,*frw,*prd,*leakdata,*pressure_iap,*pressure_eap,*snore;
    gPointData *pulse,*spo2;

    gGraphWindow *PRD,*FRW,*G_AHI,*TAP,*LEAK,*SF,*TAP_EAP,*TAP_IAP,*PULSE,*SPO2,*SNORE;

    list<gPointData *> OXIData;
    list<gPointData *> CPAPData;
    void AddCPAPData(gPointData *d) { CPAPData.push_back(d); };
    void AddOXIData(gPointData *d) { OXIData.push_back(d); };
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

#endif // DAILY_H
