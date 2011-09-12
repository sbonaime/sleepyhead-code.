/*
 Overview GUI Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QCalendarWidget>
#include <QTextCharFormat>
#include <QSystemLocale>
#include <QDebug>
#include "overview.h"
#include "ui_overview.h"
#include "Graphs/gXAxis.h"
#include "Graphs/gLineChart.h"
#include "Graphs/gYAxis.h"

const int default_height=180;

Overview::Overview(QWidget *parent,Profile * _profile,gGraphView * shared) :
    QWidget(parent),
    ui(new Ui::Overview),
    profile(_profile),
    m_shared(shared)
{
    ui->setupUi(this);
    Q_ASSERT(profile!=NULL);


    // Create dummy day & session for holding eventlists.
    //day=new Day(mach);

    layout=new QHBoxLayout(ui->graphArea);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setContentsMargins(0,0,0,0);
    ui->graphArea->setLayout(layout);
    ui->graphArea->setAutoFillBackground(false);

    GraphView=new gGraphView(ui->graphArea,m_shared);
    GraphView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    scrollbar=new MyScrollBar(ui->graphArea);
    scrollbar->setOrientation(Qt::Vertical);
    scrollbar->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
    scrollbar->setMaximumWidth(20);

    GraphView->setScrollBar(scrollbar);
    layout->addWidget(GraphView,1);
    layout->addWidget(scrollbar,0);

    layout->layout();

    AHI=new gGraph(GraphView,"AHI",default_height,0);
    UC=new gGraph(GraphView,"Usage",default_height,0);
    PR=new gGraph(GraphView,"Pressure",default_height,0);
    LK=new gGraph(GraphView,"Leaks",default_height,0);

    uc=new SummaryChart(profile,"Hours",GT_BAR);
    uc->addSlice(EmptyChannel,QColor("green"),ST_HOURS);
    UC->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gXAxis *gx=new gXAxis();
    gx->setUtcFix(true);
    UC->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
    UC->AddLayer(uc);
    UC->AddLayer(new gXGrid());


    bc=new SummaryChart(profile,"AHI",GT_BAR);
    bc->addSlice(CPAP_Hypopnea,QColor("blue"),ST_CPH);
    bc->addSlice(CPAP_Apnea,QColor("dark green"),ST_CPH);
    bc->addSlice(CPAP_Obstructive,QColor("#40c0ff"),ST_CPH);
    bc->addSlice(CPAP_ClearAirway,QColor("purple"),ST_CPH);
    AHI->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gx=new gXAxis();
    gx->setUtcFix(true);
    AHI->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
    AHI->AddLayer(bc);
    AHI->AddLayer(new gXGrid());


    pr=new SummaryChart(profile,"cmH2O",GT_LINE);

    pr->addSlice(CPAP_Pressure,QColor("dark green"),ST_WAVG);
    pr->addSlice(CPAP_Pressure,QColor("orange"),ST_MIN);
    pr->addSlice(CPAP_Pressure,QColor("red"),ST_MAX);
    pr->addSlice(CPAP_EPAP,QColor("light green"),ST_MIN);
    pr->addSlice(CPAP_IPAP,QColor("light blue"),ST_MAX);

    PR->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gx=new gXAxis();
    gx->setUtcFix(true);
    PR->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
    PR->AddLayer(pr);
    PR->AddLayer(new gXGrid());

    lk=new SummaryChart(profile,"Avg Leak",GT_LINE);
    lk->addSlice(CPAP_Leak,QColor("dark grey"),ST_90P);
    lk->addSlice(CPAP_Leak,QColor("dark blue"),ST_WAVG);
    //lk->addSlice(CPAP_Leak,QColor("dark yellow"));
    //pr->addSlice(CPAP_IPAP,QColor("red"));
    LK->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gx=new gXAxis();
    gx->setUtcFix(true);
    LK->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
    LK->AddLayer(lk);
    LK->AddLayer(new gXGrid());

    NPB=new gGraph(GraphView,"% in PB",default_height,0);
    NPB->AddLayer(npb=new SummaryChart(profile,"% PB",GT_BAR));
    npb->addSlice(CPAP_CSR,QColor("light green"),ST_SPH);
    NPB->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gx=new gXAxis();
    gx->setUtcFix(true);
    NPB->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
    NPB->AddLayer(new gXGrid());


    QLocale locale=QLocale::system();
    QString shortformat=locale.dateFormat(QLocale::ShortFormat);
    if (!shortformat.toLower().contains("yyyy")) {
        shortformat.replace("yy","yyyy");
    }
    ui->dateStart->setDisplayFormat(shortformat);
    ui->dateEnd->setDisplayFormat(shortformat);

    report=NULL;
}
Overview::~Overview()
{
    if (report) {
        report->close();
        delete report;
    }
    //delete day;
    delete ui;
}
void Overview::ReloadGraphs()
{
    ui->dateStart->setDate(profile->FirstDay());
    ui->dateEnd->setDate(profile->LastDay());
    GraphView->setDay(NULL);

}

void Overview::on_dateEnd_dateChanged(const QDate &date)
{
    qint64 d1=qint64(QDateTime(ui->dateStart->date(),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    qint64 d2=qint64(QDateTime(date,QTime(23,59,59),Qt::UTC).toTime_t())*1000L;
    GraphView->SetXBounds(d1,d2);

}

void Overview::on_dateStart_dateChanged(const QDate &date)
{
    qint64 d1=qint64(QDateTime(date,QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    qint64 d2=qint64(QDateTime(ui->dateEnd->date(),QTime(23,59,59),Qt::UTC).toTime_t())*1000L;
    GraphView->SetXBounds(d1,d2);

}

void Overview::on_toolButton_clicked()
{
    qint64 d1=qint64(QDateTime(ui->dateStart->date(),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    qint64 d2=qint64(QDateTime(ui->dateEnd->date(),QTime(23,59,59),Qt::UTC).toTime_t())*1000L;
    GraphView->SetXBounds(d1,d2);
}

void Overview::on_printButton_clicked()
{

    if (!report) {
        report=new Report(this,profile,m_shared,this);
        //report->setMinimumSize(ui->graphArea->width(),ui->graphArea->height());
        //report->setMaximumSize(ui->graphArea->width(),ui->graphArea->height());
        //report->setMinimumSize(1280,800);
        //report->setMaximumSize(1280,800);
        report->hide();
    }

    if (report) {
        bc->deselect();
        uc->deselect();
        pr->deselect();
        lk->deselect();
        npb->deselect();

        //GraphView->hide();
        //report->show();
        report->ReloadGraphs();
        report->GenerateReport(ui->dateStart->date(),ui->dateEnd->date());
        report->on_printButton_clicked();
        //GraphView->show();
        //report->connect(report->webview(),SIGNAL(loadFinished(bool)),this,SLOT(readyToPrint(bool)));
    }

    //report->hide();
    //ui->tabWidget->insertTab(4,report,tr("Overview Report"));

}

void Overview::readyToPrint(bool)
{
}
