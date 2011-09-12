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

const int graph_print_width=1280;
const int graph_print_height=256;

class Daily;
class Overview;
class Report : public QWidget
{
    Q_OBJECT

public:
    explicit Report(QWidget *parent, Profile * _profile, gGraphView * shared, Overview * overview);
    ~Report();
    bool GenerateReport(QString templ,QDate start, QDate end);
    void ReloadGraphs();
    QString ParseTemplate(QString input);

    QPixmap Snapshot(gGraph * graph);
    void Print();

private:
    Ui::Report  *ui;
    Profile * profile;
    Overview * m_overview;
    gGraphView * shared;
    gGraphView * GraphView;
    gGraph *AHI,*UC,*PR,*LK,*NPB;
    SummaryChart *bc,*uc,*pr,*lk,*npb;
    QHash<QString,QVariant> locals;
    QHash<QString,gGraph *> graphs;

    QDate startDate;
    QDate endDate;

    bool m_ready;
};

#endif // REPORT_H
