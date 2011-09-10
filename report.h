#ifndef REPORT_H
#define REPORT_H

#include <QWidget>
#include "SleepLib/profiles.h"
#include "Graphs/gGraphView.h"
#include "daily.h"
#include "overview.h"

namespace Ui {
    class Report;
}

class Daily;
class Report : public QWidget
{
    Q_OBJECT

public:
    explicit Report(QWidget *parent, gGraphView * shared, Daily * daily, Overview * overview);
    ~Report();
    void Reload();
    void ReloadGraphs();
    QPixmap Snapshot(gGraph * graph);

protected:
    virtual void showEvent (QShowEvent * event);
private slots:
    void on_refreshButton_clicked();

    void on_startDate_dateChanged(const QDate &date);

    void on_endDate_dateChanged(const QDate &date);

    void on_printButton_clicked();

private:
    Ui::Report  *ui;
    Profile * profile;
    Daily * m_daily;
    Overview * m_overview;
    gGraphView * shared;
    gGraphView * GraphView;
    gGraph *AHI,*UC,*PR,*LK,*NPB;
    SummaryChart *bc,*uc,*pr,*lk,*npb;
    QVector<gGraph *> graphs;

    bool m_ready;
    virtual void resizeEvent(QResizeEvent *);
};

#endif // REPORT_H
