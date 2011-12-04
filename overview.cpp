/*
 Overview GUI Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QCalendarWidget>
#include <QTextCharFormat>
#include <QSystemLocale>
#include <QDebug>
#include <QDateTimeEdit>
#include <QCalendarWidget>
#include <QFileDialog>
//#include <QProgressBar>

#include "SleepLib/profiles.h"
#include "overview.h"
#include "ui_overview.h"
#include "common_gui.h"
#include "Graphs/gXAxis.h"
#include "Graphs/gLineChart.h"
#include "Graphs/gYAxis.h"
#include "Graphs/gSessionTime.h"

#include "mainwindow.h"
extern MainWindow * mainwin;
//extern QProgressBar * qprogress;

Overview::Overview(QWidget *parent,gGraphView * shared) :
    QWidget(parent),
    ui(new Ui::Overview),
    m_shared(shared)
{
    ui->setupUi(this);

    // Set Date controls locale to 4 digit years
    QLocale locale=QLocale::system();
    QString shortformat=locale.dateFormat(QLocale::ShortFormat);
    if (!shortformat.toLower().contains("yyyy")) {
        shortformat.replace("yy","yyyy");
    }
    ui->dateStart->setDisplayFormat(shortformat);
    ui->dateEnd->setDisplayFormat(shortformat);

    Qt::DayOfWeek dow=firstDayOfWeekFromLocale();

    ui->dateStart->calendarWidget()->setFirstDayOfWeek(dow);
    ui->dateEnd->calendarWidget()->setFirstDayOfWeek(dow);


    // Stop both calendar drop downs highlighting weekends in red
    QTextCharFormat format = ui->dateStart->calendarWidget()->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    ui->dateStart->calendarWidget()->setWeekdayTextFormat(Qt::Saturday, format);
    ui->dateStart->calendarWidget()->setWeekdayTextFormat(Qt::Sunday, format);
    ui->dateEnd->calendarWidget()->setWeekdayTextFormat(Qt::Saturday, format);
    ui->dateEnd->calendarWidget()->setWeekdayTextFormat(Qt::Sunday, format);

    // Connect the signals to update which days have CPAP data when the month is changed
    connect(ui->dateStart->calendarWidget(),SIGNAL(currentPageChanged(int,int)),SLOT(dateStart_currentPageChanged(int,int)));
    connect(ui->dateEnd->calendarWidget(),SIGNAL(currentPageChanged(int,int)),SLOT(dateEnd_currentPageChanged(int,int)));

    // Create the horizontal layout to hold the GraphView object and it's scrollbar
    layout=new QHBoxLayout(ui->graphArea);
    layout->setSpacing(0); // remove the ugly margins/spacing
    layout->setMargin(0);
    layout->setContentsMargins(0,0,0,0);
    ui->graphArea->setLayout(layout);
    ui->graphArea->setAutoFillBackground(false);

    // Create the GraphView Object
    GraphView=new gGraphView(ui->graphArea,m_shared);
    GraphView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    // Create the custom scrollbar and attach to GraphView
    scrollbar=new MyScrollBar(ui->graphArea);
    scrollbar->setOrientation(Qt::Vertical);
    scrollbar->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
    scrollbar->setMaximumWidth(20);
    GraphView->setScrollBar(scrollbar);

    // Add the graphView and scrollbar to the layout.
    layout->addWidget(GraphView,1);
    layout->addWidget(scrollbar,0);
    layout->layout();

    // TODO: Automate graph creation process

    // The following code (to the closing marker) is crap --->
    AHI=createGraph("AHI");
    UC=createGraph("Usage");

    int default_height=PROFILE["GraphHeight"].toInt();
    US=new gGraph(GraphView,"Session Usage",default_height,0);
    US->AddLayer(new gYAxisTime(),LayerLeft,gYAxis::Margin);
    gXAxis *x=new gXAxis();
    x->setUtcFix(true);
    US->AddLayer(x,LayerBottom,0,gXAxis::Margin);
    US->AddLayer(new gXGrid());

    PR=createGraph("Pressure");
    SET=createGraph("Settings");
    LK=createGraph("Leaks");
    SES=createGraph("Sessions");
    NPB=createGraph("% in PB");
    RR=createGraph("Resp. Rate");
    TV=createGraph("Tidal Volume");
    MV=createGraph("Minute Vent.");
    PTB=createGraph("Pat. Trig. Br.");
    PULSE=createGraph("Pulse Rate");
    SPO2=createGraph("SpO2");
    WEIGHT=createGraph("Weight");
    BMI=createGraph("BMI");
    ZOMBIE=createGraph("Zombie");

    weight=new SummaryChart("Weight",GT_LINE);
    weight->setMachineType(MT_JOURNAL);
    weight->addSlice("Weight",QColor("black"),ST_SETAVG);
    WEIGHT->AddLayer(weight);

    bmi=new SummaryChart("BMI",GT_LINE);
    bmi->setMachineType(MT_JOURNAL);
    bmi->addSlice("BMI",QColor("dark blue"),ST_SETAVG);
    BMI->AddLayer(bmi);

    zombie=new SummaryChart("Zombie Meter",GT_LINE);
    zombie->setMachineType(MT_JOURNAL);
    zombie->addSlice("ZombieMeter",QColor("dark red"),ST_SETAVG);
    ZOMBIE->AddLayer(zombie);

    pulse=new SummaryChart("Pulse Rate",GT_LINE);
    pulse->setMachineType(MT_OXIMETER);
    pulse->addSlice(OXI_Pulse,QColor("red"),ST_WAVG);
    pulse->addSlice(OXI_Pulse,QColor("light red"),ST_MIN);
    pulse->addSlice(OXI_Pulse,QColor("orange"),ST_MAX);
    PULSE->AddLayer(pulse);

    spo2=new SummaryChart("SpO2",GT_LINE);
    spo2->setMachineType(MT_OXIMETER);
    spo2->addSlice(OXI_SPO2,QColor("cyan"),ST_WAVG);
    spo2->addSlice(OXI_SPO2,QColor("light blue"),ST_90P);
    spo2->addSlice(OXI_SPO2,QColor("blue"),ST_MIN);
    SPO2->AddLayer(spo2);

    uc=new SummaryChart("Hours",GT_BAR);
    uc->addSlice("",QColor("green"),ST_HOURS);
    UC->AddLayer(uc);

    us=new SummaryChart("Hours",GT_SESSIONS);
    us->addSlice("",QColor("dark blue"),ST_HOURS);
    us->addSlice("",QColor("blue"),ST_SESSIONS);
    US->AddLayer(us);

    ses=new SummaryChart("Sessions",GT_LINE);
    ses->addSlice("",QColor("blue"),ST_SESSIONS);
    SES->AddLayer(ses);

    bc=new SummaryChart("AHI",GT_BAR);
    bc->addSlice(CPAP_Hypopnea,QColor("blue"),ST_CPH);
    bc->addSlice(CPAP_Apnea,QColor("dark green"),ST_CPH);
    bc->addSlice(CPAP_Obstructive,QColor("#40c0ff"),ST_CPH);
    bc->addSlice(CPAP_ClearAirway,QColor("purple"),ST_CPH);
    AHI->AddLayer(bc);

    set=new SummaryChart("",GT_LINE);
    //set->addSlice("SysOneResistSet",QColor("grey"),ST_SETAVG);
    set->addSlice("HumidSet",QColor("blue"),ST_SETWAVG);
    set->addSlice("FlexSet",QColor("red"),ST_SETWAVG);
    set->addSlice("EPR",QColor("red"),ST_SETWAVG);
    set->addSlice("SmartFlex",QColor("red"),ST_SETWAVG);
    SET->setRecMinY(0);
    SET->setRecMaxY(5);
    SET->AddLayer(set);

    rr=new SummaryChart("breaths/min",GT_LINE);
    rr->addSlice(CPAP_RespRate,QColor("light blue"),ST_MIN);
    rr->addSlice(CPAP_RespRate,QColor("light green"),ST_90P);
    rr->addSlice(CPAP_RespRate,QColor("blue"),ST_WAVG);
    RR->AddLayer(rr);

    tv=new SummaryChart("L/b",GT_LINE);
    tv->addSlice(CPAP_TidalVolume,QColor("light blue"),ST_MIN);
    tv->addSlice(CPAP_TidalVolume,QColor("light green"),ST_90P);
    tv->addSlice(CPAP_TidalVolume,QColor("blue"),ST_WAVG);
    TV->AddLayer(tv);

    mv=new SummaryChart("L/m",GT_LINE);
    mv->addSlice(CPAP_MinuteVent,QColor("light blue"),ST_MIN);
    mv->addSlice(CPAP_MinuteVent,QColor("light green"),ST_90P);
    mv->addSlice(CPAP_MinuteVent,QColor("blue"),ST_WAVG);
    MV->AddLayer(mv);


    ptb=new SummaryChart("%PTB",GT_LINE);
    ptb->addSlice(CPAP_PTB,QColor("yellow"),ST_MIN);
    ptb->addSlice(CPAP_PTB,QColor("light gray"),ST_90P);
    ptb->addSlice(CPAP_PTB,QColor("orange"),ST_WAVG);
    PTB->AddLayer(ptb);

    pr=new SummaryChart("cmH2O",GT_LINE);
    //PR->setRecMinY(4.0);
    //PR->setRecMaxY(12.0);
    pr->addSlice(CPAP_Pressure,QColor("dark green"),ST_WAVG);
    pr->addSlice(CPAP_Pressure,QColor("orange"),ST_MIN);
    pr->addSlice(CPAP_Pressure,QColor("red"),ST_MAX);
    pr->addSlice(CPAP_Pressure,QColor("grey"),ST_90P);
    pr->addSlice(CPAP_EPAP,QColor("light green"),ST_MIN);
    pr->addSlice(CPAP_IPAP,QColor("light blue"),ST_MAX);
    PR->AddLayer(pr);

    lk=new SummaryChart("Avg Leak",GT_LINE);
    lk->addSlice(CPAP_Leak,QColor("dark grey"),ST_90P);
    lk->addSlice(CPAP_Leak,QColor("dark blue"),ST_WAVG);
    //lk->addSlice(CPAP_Leak,QColor("dark yellow"));
    //pr->addSlice(CPAP_IPAP,QColor("red"));
    LK->AddLayer(lk);

    NPB->AddLayer(npb=new SummaryChart("% PB",GT_BAR));
    npb->addSlice(CPAP_CSR,QColor("light green"),ST_SPH);
    // <--- The code to the previous marker is crap

    report=NULL;

    GraphView->LoadSettings("Overview");
}
Overview::~Overview()
{
    GraphView->SaveSettings("Overview");
    disconnect(this,SLOT(dateStart_currentPageChanged(int,int)));
    disconnect(this,SLOT(dateEnd_currentPageChanged(int,int)));
    if (report) {
        report->close();
        delete report;
    }
    delete ui;
}
gGraph * Overview::createGraph(QString name)
{
    int default_height=PROFILE["GraphHeight"].toInt();
    gGraph *g=new gGraph(GraphView,name,default_height,0);
    g->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gXAxis *x=new gXAxis();
    x->setUtcFix(true);
    g->AddLayer(x,LayerBottom,0,gXAxis::Margin);
    g->AddLayer(new gXGrid());
    return g;
}

void Overview::ReloadGraphs()
{
    ui->dateStart->setDate(p_profile->FirstDay());
    ui->dateEnd->setDate(p_profile->LastDay());
    GraphView->setDay(NULL);

}

void Overview::RedrawGraphs()
{
    GraphView->updateGL();
}

void Overview::UpdateCalendarDay(QDateEdit * dateedit,QDate date)
{
    QCalendarWidget *calendar=dateedit->calendarWidget();
    QTextCharFormat bold;
    QTextCharFormat cpapcol;
    QTextCharFormat normal;
    QTextCharFormat oxiday;
    bold.setFontWeight(QFont::Bold);
    cpapcol.setForeground(QBrush(Qt::blue, Qt::SolidPattern));
    cpapcol.setFontWeight(QFont::Bold);
    oxiday.setForeground(QBrush(Qt::red, Qt::SolidPattern));
    oxiday.setFontWeight(QFont::Bold);
    bool hascpap=p_profile->GetDay(date,MT_CPAP)!=NULL;
    bool hasoxi=p_profile->GetDay(date,MT_OXIMETER)!=NULL;
    //bool hasjournal=p_profile->GetDay(date,MT_JOURNAL)!=NULL;

    if (hascpap) {
        if (hasoxi) {
            calendar->setDateTextFormat(date,oxiday);
        } else {
            calendar->setDateTextFormat(date,cpapcol);
        }
    } else if (p_profile->GetDay(date)) {
        calendar->setDateTextFormat(date,bold);
    } else {
        calendar->setDateTextFormat(date,normal);
    }
    calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
}
void Overview::dateStart_currentPageChanged(int year, int month)
{
    QDate d(year,month,1);
    int dom=d.daysInMonth();

    for (int i=1;i<=dom;i++) {
        d=QDate(year,month,i);
        UpdateCalendarDay(ui->dateStart,d);
    }
}
void Overview::dateEnd_currentPageChanged(int year, int month)
{
    QDate d(year,month,1);
    int dom=d.daysInMonth();

    for (int i=1;i<=dom;i++) {
        d=QDate(year,month,i);
        UpdateCalendarDay(ui->dateEnd,d);
    }
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

QString Overview::GetHTML()
{
    if (!report) {
        report=new Report(this,m_shared,this);
        report->hide();
    }

    QString html;
    if (report) {
        GraphView->deselect();

        report->ReloadGraphs();
        QString reportname="overview";
        html=report->GenerateReport(reportname,ui->dateStart->date(),ui->dateEnd->date());
        if (html.isEmpty()) {
            qDebug() << "Faulty Report" << reportname;
        }
    }
    return html;
}
void Overview::on_printButton_clicked()
{
    mainwin->PrintReport(GraphView,"Overview");
    //report->Print(GetHTML());
}

void Overview::ResetGraphLayout()
{
    GraphView->resetLayout();
}

