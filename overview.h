/*
 Overview GUI Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef OVERVIEW_H
#define OVERVIEW_H

#include <QWidget>
#include <QGLContext>
#include <QHBoxLayout>

#include "SleepLib/profiles.h"
#include "Graphs/gGraphView.h"
#include "Graphs/gBarChart.h"

namespace Ui {
    class Overview;
}

class Overview : public QWidget
{
    Q_OBJECT

public:
    explicit Overview(QWidget *parent,gGraphView *shared=NULL);
    ~Overview();

    void ReloadGraphs();
    //void UpdateGraphs();


private slots:
/*    void on_drStart_dateChanged(const QDate &date);
    void on_drEnd_dateChanged(const QDate &date);
    void on_rbDateRange_toggled(bool checked);
    void on_rbLastWeek_clicked();
    void on_rbLastMonth_clicked();
    void on_rbEverything_clicked();
    void on_rbDateRange_clicked(); */

private:
    Ui::Overview *ui;
    Profile *profile;
    gGraphView *GraphView;
    MyScrollBar *scrollbar;
    QHBoxLayout *layout;
    gGraphView * m_shared;

    void UpdateHTML();

    //SessionTimes *session_times;
    gGraph *AHI,*UC,*PR,*LK;
    AHIChart *bc;
    UsageChart *uc;
    AvgChart *pr;
    AvgChart *lk;
    //,*PRESSURE,*LEAK,*SESSTIMES;

    //Layer *prmax,*prmin,*iap,*eap,*pr,*sesstime;

    Day * day;// dummy in this case

};

#endif // OVERVIEW_H
