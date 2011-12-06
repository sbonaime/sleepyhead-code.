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
    AHI=createGraph("AHI","Apnea\nHypopnea\nIndex");
    UC=createGraph("Usage","Usage\n(time)");

    int default_height=PROFILE["GraphHeight"].toInt();
    US=new gGraph(GraphView,"Session Times","Session Times\n(time)",default_height,0);
    US->AddLayer(new gYAxisTime(),LayerLeft,gYAxis::Margin);
    gXAxis *x=new gXAxis();
    x->setUtcFix(true);
    US->AddLayer(x,LayerBottom,0,gXAxis::Margin);
    US->AddLayer(new gXGrid());

    PR=createGraph("Pressure","Pressure\n(cmH2O)");
    SET=createGraph("Settings","Settings");
    LK=createGraph("Leaks","Leak Rate\n(L/min)");
    NPB=createGraph("% in PB","Periodic\nBreathing\n(% of night)");
    AHIHR=createGraph("AHI/Hour","AHI Events/Hour\n(ahi/hr)");
    RR=createGraph("Resp. Rate","Respiratory\nRate\n(breaths/min)");
    TV=createGraph("Tidal Volume","Tidal\nVolume\n(ml)");
    MV=createGraph("Minute Vent.","Minute\nVentilation\n(L/min)");
    PTB=createGraph("Pat. Trig. Br.","Patient\nTriggered\nBreaths\n(%)");
    SES=createGraph("Sessions","Sessions\n(count)");
    PULSE=createGraph("Pulse Rate","Pulse Rate\n(bpm)");
    SPO2=createGraph("SpO2","Oxygen Saturation\n(%)");
    WEIGHT=createGraph("Weight","Weight\n(kg)");
    BMI=createGraph("BMI","Body\nMass\nIndex");
    ZOMBIE=createGraph("Zombie","How you felt\n(% awesome)");

    ahihr=new SummaryChart("AHI/Hr",GT_LINE);
    ahihr->addSlice(CPAP_AHI,QColor("blue"),ST_MAX,true);
    ahihr->addSlice(CPAP_AHI,QColor("orange"),ST_WAVG,true);
    AHIHR->AddLayer(ahihr);

    weight=new SummaryChart("Weight",GT_LINE);
    weight->setMachineType(MT_JOURNAL);
    weight->addSlice("Weight",QColor("black"),ST_SETAVG,true);
    WEIGHT->AddLayer(weight);

    bmi=new SummaryChart("BMI",GT_LINE);
    bmi->setMachineType(MT_JOURNAL);
    bmi->addSlice("BMI",QColor("dark blue"),ST_SETAVG,true);
    BMI->AddLayer(bmi);

    zombie=new SummaryChart("Zombie Meter",GT_LINE);
    zombie->setMachineType(MT_JOURNAL);
    zombie->addSlice("ZombieMeter",QColor("dark red"),ST_SETAVG,true);
    ZOMBIE->AddLayer(zombie);

    pulse=new SummaryChart("Pulse Rate",GT_LINE);
    pulse->setMachineType(MT_OXIMETER);
    pulse->addSlice(OXI_Pulse,QColor("red"),ST_WAVG,true);
    pulse->addSlice(OXI_Pulse,QColor("pink"),ST_MIN,true);
    pulse->addSlice(OXI_Pulse,QColor("orange"),ST_MAX,true);
    PULSE->AddLayer(pulse);

    spo2=new SummaryChart("SpO2",GT_LINE);
    spo2->setMachineType(MT_OXIMETER);
    spo2->addSlice(OXI_SPO2,QColor("cyan"),ST_WAVG,true);
    spo2->addSlice(OXI_SPO2,QColor("light blue"),ST_90P,true);
    spo2->addSlice(OXI_SPO2,QColor("blue"),ST_MIN,true);
    SPO2->AddLayer(spo2);

    uc=new SummaryChart("Hours",GT_BAR);
    uc->addSlice("",QColor("green"),ST_HOURS,true);
    UC->AddLayer(uc);

    us=new SummaryChart("Hours",GT_SESSIONS);
    us->addSlice("",QColor("dark blue"),ST_HOURS,true);
    us->addSlice("",QColor("blue"),ST_SESSIONS,true);
    US->AddLayer(us);

    ses=new SummaryChart("Sessions",GT_LINE);
    ses->addSlice("",QColor("blue"),ST_SESSIONS,true);
    SES->AddLayer(ses);

    bc=new SummaryChart("AHI",GT_BAR);
    bc->addSlice(CPAP_Hypopnea,QColor("blue"),ST_CPH,false);
    bc->addSlice(CPAP_Apnea,QColor("dark green"),ST_CPH,false);
    bc->addSlice(CPAP_Obstructive,QColor("#40c0ff"),ST_CPH,false);
    bc->addSlice(CPAP_ClearAirway,QColor("purple"),ST_CPH,false);
    AHI->AddLayer(bc);

    set=new SummaryChart("",GT_LINE);
    //set->addSlice("SysOneResistSet",QColor("grey"),ST_SETAVG);
    set->addSlice("HumidSet",QColor("blue"),ST_SETWAVG,true);
    set->addSlice("FlexSet",QColor("red"),ST_SETWAVG,true);
    set->addSlice("EPR",QColor("green"),ST_SETWAVG,true);
    set->addSlice("SmartFlex",QColor("purple"),ST_SETWAVG,true);
    SET->setRecMinY(0);
    SET->setRecMaxY(5);
    SET->AddLayer(set);

    rr=new SummaryChart("breaths/min",GT_LINE);
    rr->addSlice(CPAP_RespRate,QColor("light blue"),ST_MIN,true);
    rr->addSlice(CPAP_RespRate,QColor("light green"),ST_90P,true);
    rr->addSlice(CPAP_RespRate,QColor("blue"),ST_WAVG,true);
    RR->AddLayer(rr);

    tv=new SummaryChart("L/b",GT_LINE);
    tv->addSlice(CPAP_TidalVolume,QColor("light blue"),ST_MIN,true);
    tv->addSlice(CPAP_TidalVolume,QColor("light green"),ST_90P,true);
    tv->addSlice(CPAP_TidalVolume,QColor("blue"),ST_WAVG,true);
    TV->AddLayer(tv);

    mv=new SummaryChart("L/m",GT_LINE);
    mv->addSlice(CPAP_MinuteVent,QColor("light blue"),ST_MIN,true);
    mv->addSlice(CPAP_MinuteVent,QColor("light green"),ST_90P,true);
    mv->addSlice(CPAP_MinuteVent,QColor("blue"),ST_WAVG,true);
    MV->AddLayer(mv);


    ptb=new SummaryChart("%PTB",GT_LINE);
    ptb->addSlice(CPAP_PTB,QColor("yellow"),ST_MIN,true);
    ptb->addSlice(CPAP_PTB,QColor("light gray"),ST_90P,true);
    ptb->addSlice(CPAP_PTB,QColor("orange"),ST_WAVG,true);
    PTB->AddLayer(ptb);

    pr=new SummaryChart("cmH2O",GT_LINE);
    //PR->setRecMinY(4.0);
    //PR->setRecMaxY(12.0);
    pr->addSlice(CPAP_Pressure,QColor("dark green"),ST_WAVG,true);
    pr->addSlice(CPAP_Pressure,QColor("orange"),ST_MIN,true);
    pr->addSlice(CPAP_Pressure,QColor("red"),ST_MAX,true);
    pr->addSlice(CPAP_Pressure,QColor("grey"),ST_90P,true);
    pr->addSlice(CPAP_EPAP,QColor("light green"),ST_MIN,true);
    pr->addSlice(CPAP_IPAP,QColor("light blue"),ST_MAX,true);
    PR->AddLayer(pr);

    lk=new SummaryChart("Avg Leak",GT_LINE);
    lk->addSlice(CPAP_Leak,QColor("dark grey"),ST_90P,false);
    lk->addSlice(CPAP_Leak,QColor("dark blue"),ST_WAVG,false);
    //lk->addSlice(CPAP_Leak,QColor("dark yellow"));
    //pr->addSlice(CPAP_IPAP,QColor("red"));
    LK->AddLayer(lk);

    NPB->AddLayer(npb=new SummaryChart("% PB",GT_BAR));
    npb->addSlice(CPAP_CSR,QColor("light green"),ST_SPH,false);
    // <--- The code to the previous marker is crap

    GraphView->LoadSettings("Overview");
}
Overview::~Overview()
{
    GraphView->SaveSettings("Overview");
    disconnect(this,SLOT(dateStart_currentPageChanged(int,int)));
    disconnect(this,SLOT(dateEnd_currentPageChanged(int,int)));
    delete ui;
}
gGraph * Overview::createGraph(QString name,QString units)
{
    int default_height=PROFILE["GraphHeight"].toInt();
    gGraph *g=new gGraph(GraphView,name,units,default_height,0);
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

void Overview::on_printButton_clicked()
{
    mainwin->PrintReport(GraphView,"Overview");
}

void Overview::ResetGraphLayout()
{
    GraphView->resetLayout();
}

