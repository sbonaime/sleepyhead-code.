/********************************************************************
 Overview GUI Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef OVERVIEW_H
#define OVERVIEW_H

#include <QWidget>
#include <QGLContext>
#include <QSplitter>
#include <SleepLib/profiles.h>
#include <Graphs/graphdata_custom.h>
namespace Ui {
    class Overview;
}

class Overview : public QWidget
{
    Q_OBJECT

public:
    explicit Overview(QWidget *parent,QGLContext *context);
    ~Overview();

    void SetGLContext(QGLContext *context) { shared_context=context; };
    void ReloadGraphs();
    void UpdateGraphs();


private slots:

    void on_rbLastWeek_clicked(bool checked);

    void on_rbLastMonth_clicked(bool checked);

    void on_rbEverything_clicked(bool checked);

    void on_rbDateRange_clicked(bool checked);

    void on_drStart_dateChanged(const QDate &date);

    void on_drEnd_dateChanged(const QDate &date);

    void on_rbDateRange_toggled(bool checked);

private:
    Ui::Overview *ui;
    Profile *profile;
    QGLContext *shared_context;

    void AddData(HistoryData *d) { Data.push_back(d);  };

    HistoryData *ahidata,*pressure,*leak,*usage,*bedtime,*waketime,*pressure_iap,*pressure_eap;
    HistoryData *pressure_min,*pressure_max;

    gGraphWindow *AHI,*PRESSURE,*LEAK,*USAGE;

    gLayer *prmax,*prmin,*iap,*eap,*pr;

    list<HistoryData *> Data;
    Day *dummyday;
    QSplitter *gSplitter;
};

#endif // OVERVIEW_H
