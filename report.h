#ifndef REPORT_H
#define REPORT_H

#include <QWidget>
#include <QWebView>
#include "SleepLib/profiles.h"
#include "Graphs/gGraphView.h"
#include "overview.h"

namespace Ui {
    class Report;
}

class Daily;
class Overview;
class Report : public QWidget
{
    Q_OBJECT

public:
    explicit Report(QWidget *parent, Profile * _profile, gGraphView * shared, Overview * overview);
    ~Report();
    void GenerateReport(QDate start, QDate end);
    void ReloadGraphs();
    QPixmap Snapshot(gGraph * graph);
public slots:
    void on_printButton_clicked();

protected:
//    virtual void showEvent (QShowEvent * event);

private:
    Ui::Report  *ui;
    Profile * profile;
    Overview * m_overview;
    gGraphView * shared;
    gGraphView * GraphView;
    gGraph *AHI,*UC,*PR,*LK,*NPB;
    SummaryChart *bc,*uc,*pr,*lk,*npb;
    QVector<gGraph *> graphs;

    QDate startDate;
    QDate endDate;

    bool m_ready;
    virtual void resizeEvent(QResizeEvent *);
};

#endif // REPORT_H
