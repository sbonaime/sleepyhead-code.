/********************************************************************
 Overview GUI Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef OVERVIEW_H
#define OVERVIEW_H

/*#include <QWidget>
#include <QGLContext>
#include <QSplitter>
#include <QLabel>
#include <SleepLib/profiles.h>
#include <Graphs/graphdata_custom.h>

namespace Ui {
    class Overview;
}

class Overview : public QWidget
{
    Q_OBJECT

public:
    explicit Overview(QWidget *parent,QGLWidget *shared=NULL);
    ~Overview();

    void ReloadGraphs();
    void UpdateGraphs();


private slots:
    void on_drStart_dateChanged(const QDate &date);
    void on_drEnd_dateChanged(const QDate &date);
    void on_rbDateRange_toggled(bool checked);
    void on_rbLastWeek_clicked();
    void on_rbLastMonth_clicked();
    void on_rbEverything_clicked();
    void on_rbDateRange_clicked();

private:
    Ui::Overview *ui;
    Profile *profile;

    void AddData(HistoryData *d) { Data.push_back(d);  };
    void AddGraph(gGraphWindow *w) { Graphs.push_back(w); };
    void RedrawGraphs();
    void UpdateHTML();

    HistoryData *ahidata,*pressure,*leak,*usage,*bedtime,*waketime,*pressure_iap,*pressure_eap;
    HistoryData *pressure_min,*pressure_max;
    SessionTimes *session_times;
    gGraphWindow *AHI,*PRESSURE,*LEAK,*USAGE,*SESSTIMES;

    gLayer *prmax,*prmin,*iap,*eap,*pr,*sesstime;

    list<HistoryData *> Data;
    list<gGraphWindow *> Graphs;
    Day *dummyday;
    QSplitter *gSplitter;
    QLabel *NoData;
};
*/
#endif // OVERVIEW_H
