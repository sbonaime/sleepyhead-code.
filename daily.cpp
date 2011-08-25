/*
 Daily Panel
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "daily.h"
#include "ui_daily.h"

#include <QTextCharFormat>
#include <QPalette>
#include <QTextBlock>
#include <QColorDialog>
#include <QSpacerItem>
#include <QBuffer>
#include <QPixmap>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScrollBar>

#include "SleepLib/session.h"
#include "Graphs/graphdata_custom.h"
#include "Graphs/gLineOverlay.h"
#include "Graphs/gFlagsLine.h"
#include "Graphs/gFooBar.h"
#include "Graphs/gXAxis.h"
#include "Graphs/gYAxis.h"
#include "Graphs/gBarChart.h"
#include "Graphs/gSegmentChart.h"

const int min_height=150;
const int default_height=150;

Daily::Daily(QWidget *parent,QGLWidget * shared, MainWindow *mw)
    :QWidget(parent),mainwin(mw), ui(new Ui::Daily)
{
    ui->setupUi(this);

    QString prof=pref["Profile"].toString();
    profile=Profiles::Get(prof);
    if (!profile) {
        QMessageBox::critical(this,"Profile Error",QString("Couldn't get profile '%1'.. Have to abort!").arg(pref["Profile"].toString()));
        exit(-1);
    }

    layout=new QHBoxLayout(ui->graphMainArea);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setContentsMargins(0,0,0,0);
    ui->graphMainArea->setLayout(layout);
    //ui->graphMainArea->setLayout(layout);

    ui->graphMainArea->setAutoFillBackground(false);

    GraphView=new gGraphView(ui->graphMainArea);
    GraphView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    scrollbar=new MyScrollBar(ui->graphMainArea);
    scrollbar->setOrientation(Qt::Vertical);
    scrollbar->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
    scrollbar->setMaximumWidth(20);

    GraphView->setScrollBar(scrollbar);
    layout->addWidget(GraphView,1);
    layout->addWidget(scrollbar,0);

    SF=new gGraph(GraphView,"Event Flags",180);
    FRW=new gGraph(GraphView,"Flow Rate",180);
    MP=new gGraph(GraphView,"Mask Pressure",180);
    PRD=new gGraph(GraphView,"Pressure",180);
    LEAK=new gGraph(GraphView,"Leak",180);
    SNORE=new gGraph(GraphView,"Snore",180);
    RR=new gGraph(GraphView,"Respiratory Rate",180);
    TV=new gGraph(GraphView,"Tidal Volume",180);
    MV=new gGraph(GraphView,"Minute Ventilation",180);
    FLG=new gGraph(GraphView,"Flow Limitation Graph",180);

    gFlagsGroup *fg=new gFlagsGroup();
    fg->AddLayer((new gFlagsLine(CPAP_CSR,QColor("light green"),"CSR",false,FT_Span)));
    fg->AddLayer((new gFlagsLine(CPAP_ClearAirway,QColor("purple"),"CA",true)));
    fg->AddLayer((new gFlagsLine(CPAP_Obstructive,QColor("#40c0ff"),"OA",true)));
    fg->AddLayer((new gFlagsLine(CPAP_Apnea,QColor("dark green"),"A")));
    fg->AddLayer((new gFlagsLine(CPAP_Hypopnea,QColor("blue"),"H",true)));
    fg->AddLayer((new gFlagsLine(CPAP_FlowLimit,QColor("black"),"FL")));
    fg->AddLayer((new gFlagsLine(CPAP_RERA,QColor("gold"),"RE")));
    fg->AddLayer((new gFlagsLine(CPAP_VSnore,QColor("red"),"VS")));
    //fg->AddLayer(AddCPAP(new gFlagsLine(flags[8],QColor("dark green"),"U0E")));
    //fg->AddLayer(AddCPAP(new gFlagsLine(flags[10],QColor("red"),"VS2"));
    SF->setBlockZoom(true);
    SF->AddLayer(AddCPAP(fg));
    SF->AddLayer(new gShadowArea());
    SF->AddLayer(new gYSpacer(),LayerLeft,gYAxis::Margin);
    SF->AddLayer(new gFooBar(),LayerBottom,0,10);
    SF->AddLayer(new gXAxis(),LayerBottom,0,gXAxis::Margin);

    PRD->AddLayer(new gXGrid());
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_Pressure,QColor("dark green"),true)));
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_EPAP,Qt::blue,true)));
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_IPAP,Qt::red,true)));
    PRD->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    PRD->AddLayer(new gXAxis(),LayerBottom,0,20);

    gLineChart *l;
    l=new gLineChart(CPAP_FlowRate,Qt::black,false,false);
    AddCPAP(l);
    FRW->AddLayer(new gXGrid());
    FRW->AddLayer(l);
    FRW->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    FRW->AddLayer(new gXAxis(),LayerBottom,0,20);
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_CSR,QColor("light green"),"CSR",FT_Span)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Hypopnea,QColor("blue"),"H")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_PressurePulse,QColor("red"),"PR",FT_Dot)));
    //FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Pressure,QColor("white"),"P",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_Unknown0B,QColor("blue"),"0B",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_Unknown10,QColor("orange"),"10",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_Unknown0E,QColor("yellow"),"0E",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_RERA,QColor("gold"),"RE")));
    //FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Unknown0E,QColor("dark green"),"U0E")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Apnea,QColor("dark green"),"A")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_VSnore,QColor("red"),"VS")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_FlowLimit,QColor("black"),"FL")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Obstructive,QColor("#40c0ff"),"OA")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_ClearAirway,QColor("purple"),"CA")));

    LEAK->AddLayer(new gXGrid());
    LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_Leak,Qt::darkYellow,true)));
    LEAK->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    LEAK->AddLayer(new gXAxis(),LayerBottom,0,20);

    SNORE->AddLayer(new gXGrid());
    SNORE->AddLayer(AddCPAP(new gLineChart(CPAP_Snore,Qt::darkGray,true)));
    SNORE->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    SNORE->AddLayer(new gXAxis(),LayerBottom,0,20);

    MP->AddLayer(new gXGrid());
    MP->AddLayer(AddCPAP(new gLineChart(CPAP_MaskPressure,Qt::blue,false)));
    MP->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    MP->AddLayer(new gXAxis(),LayerBottom,0,20);

    RR->AddLayer(new gXGrid());
    RR->AddLayer(AddCPAP(new gLineChart(CPAP_RespiratoryRate,Qt::darkMagenta,true)));
    RR->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    RR->AddLayer(new gXAxis(),LayerBottom,0,20);

    MV->AddLayer(new gXGrid());
    MV->AddLayer(AddCPAP(new gLineChart(CPAP_MinuteVentilation,Qt::darkCyan,true)));
    MV->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    MV->AddLayer(new gXAxis(),LayerBottom,0,20);

    TV->AddLayer(new gXGrid());
    TV->AddLayer(AddCPAP(new gLineChart(CPAP_TidalVolume,Qt::magenta,true)));
    TV->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    TV->AddLayer(new gXAxis(),LayerBottom,0,20);

    FLG->AddLayer(new gXGrid());
    FLG->AddLayer(AddCPAP(new gLineChart(CPAP_FlowLimitGraph,Qt::darkBlue,true)));
    FLG->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    FLG->AddLayer(new gXAxis(),LayerBottom,0,20);

    //AddGraph(SF);
    //AddGraph(FRW);
    //AddGraph(PRD);

    NoData=new QLabel(tr("No data"),ui->graphMainArea);
    NoData->setAlignment(Qt::AlignCenter);
    QFont font("FreeSans",20); //NoData->font();
    //font.setBold(true);
    NoData->setFont(font);
    layout->addWidget(NoData,1);
    NoData->hide();

    layout->layout();


/*    scrollArea=new MyScrollArea(ui->graphMainArea,this);
=======
    scrollArea=new MyScrollArea(ui->graphMainArea,this);
    QPalette p;
    p.setColor(QPalette::Window,Qt::white);
    scrollArea->setPalette(p);
    scrollArea->setBackgroundRole(QPalette::Window);
>>>>>>> 70c348c1e196c10bbd34b1ce73bce9dda7fdbd29
    ui->graphLayout->addWidget(scrollArea,1);
    ui->graphLayout->setSpacing(0);
    ui->graphLayout->setMargin(0);
    ui->graphLayout->setContentsMargins(0,0,0,0);
    scrollArea->setWidgetResizable(true);
    scrollArea->setAutoFillBackground(true);

    GraphLayout=new QWidget(scrollArea);
    GraphLayout->setAutoFillBackground(true);
    GraphLayout->setPalette(p);
    GraphLayout->setBackgroundRole(QPalette::Window);
    scrollArea->setWidget(GraphLayout);

    splitter=new QVBoxLayout(GraphLayout);
    GraphLayout->setLayout(splitter);
    splitter->setSpacing(0);
    splitter->setMargin(0);
    splitter->setContentsMargins(0,0,0,0);

    //ui->splitter->layout();
    //gSplitter(Qt::Vertical,ui->scrollArea);
    //splitter->setStyleSheet("QSplitter::handle { background-color: 'light grey'; }");
    //splitter->setHandleWidth(3);

    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //ui->scrollArea->setWidget(splitter);
    //this->connect(ui->scrollArea,
    //ui->graphSizer->addWidget(splitter);
    //scrollArea->setAutoFillBackground(false);
    //splitter->setAutoFillBackground(false);


    //splitter->setMinimumHeight(1600);
    //splitter->setMinimumWidth(600);

    QWidget * parental=GraphLayout;
    SF=new gGraphWindow(parental,tr("Event Flags"),shared);
    FRW=new gGraphWindow(parental,tr("Flow Rate"),SF);
    PRD=new gGraphWindow(parental,tr("Pressure"),SF);
    //EXPR=new gGraphWindow(parental,tr("Exp. Pressure"),SF);
    THPR=new gGraphWindow(parental,tr("Therapy Pressure"),SF);
    LEAK=new gGraphWindow(parental,tr("Leaks"),SF);
    MP=new gGraphWindow(parental,tr("Mask Pressure"),SF);
    SNORE=new gGraphWindow(parental,tr("Snore"),SF);
    FLG=new gGraphWindow(parental,tr("Flow Limitation"),SF);
    MV=new gGraphWindow(parental,tr("Minute Ventilation"),SF);
    TV=new gGraphWindow(parental,tr("Tidal Volume"),SF);
    RR=new gGraphWindow(parental,tr("Respiratory Rate"),SF);
    PTB=new gGraphWindow(parental,tr("Patient Trig Breaths"),SF);
    RE=new gGraphWindow(parental,tr("Respiratory Event"),SF);
    IE=new gGraphWindow(parental,tr("I:E"),SF);
    TI=new gGraphWindow(parental,tr("Ti"),SF);
    TE=new gGraphWindow(parental,tr("Te"),SF);
    //OF=new gGraphWindow(parental,tr("Oxi-Flags"),SF);
    INTPULSE=new gGraphWindow(parental,tr("Pulse"),SF); // Integrated Pulse
    INTSPO2=new gGraphWindow(parental,tr("SPO2"),SF);   // Integrated Pulse
    PULSE=new gGraphWindow(parental,tr("Pulse"),SF);
    SPO2=new gGraphWindow(parental,tr("SPO2"),SF);
    PLETHY=new gGraphWindow(parental,tr("Plethysomogram"),SF);

    TAP=new gGraphWindow(NULL,"",(QGLWidget* )NULL);
    TAP_EAP=new gGraphWindow(NULL,"",(QGLWidget* )NULL);
    TAP_IAP=new gGraphWindow(NULL,"",(QGLWidget* )NULL);
    G_AHI=new gGraphWindow(NULL,"",(QGLWidget* )NULL); */

    /*QGLFormat fmt;
    fmt.setDepth(false);
    fmt.setDirectRendering(false);
    fmt.setAlpha(true);
    fmt.setDoubleBuffer(false);
    fmt.setRgba(true);
    //fmt.setDefaultFormat(fmt);
    offscreen_context=new QGLContext(fmt); */

    /*SF->SetLeftMargin(SF->GetLeftMargin()+gYAxis::Margin);
    SF->SetBlockZoom(true);
    SF->AddLayer(new gXAxis());
    SF->setMinimumHeight(min_height);

    fg=new gFlagsGroup();
    fg->AddLayer((new gFlagsLine(CPAP_CSR,QColor("light green"),"CSR",false,FT_Span)));
    fg->AddLayer((new gFlagsLine(CPAP_ClearAirway,QColor("purple"),"CA",true)));
    fg->AddLayer((new gFlagsLine(CPAP_Obstructive,QColor("#40c0ff"),"OA",true)));
    fg->AddLayer((new gFlagsLine(CPAP_Apnea,QColor("dark green"),"A")));
    fg->AddLayer((new gFlagsLine(CPAP_Hypopnea,QColor("blue"),"H",true)));
    fg->AddLayer((new gFlagsLine(CPAP_FlowLimit,QColor("black"),"FL")));
    fg->AddLayer((new gFlagsLine(CPAP_RERA,QColor("gold"),"RE")));
    fg->AddLayer((new gFlagsLine(CPAP_VSnore,QColor("red"),"VS")));
    //fg->AddLayer(AddCPAP(new gFlagsLine(flags[8],QColor("dark green"),"U0E")));
    //fg->AddLayer(AddCPAP(new gFlagsLine(flags[10],QColor("red"),"VS2"));

    SF->AddLayer(AddCPAP(fg));
    // SF Foobar must go last
    SF->AddLayer(new gFooBar(10,QColor("orange"),QColor("dark grey"),true));


    bool square=true;
    PRD->AddLayer(new gXAxis());
    PRD->AddLayer(new gYAxis());
    PRD->AddLayer(AddCPAP(pressure=new gLineChart(CPAP_Pressure,QColor("dark green"),square)));
    PRD->AddLayer(AddCPAP(epap=new gLineChart(CPAP_EPAP,Qt::blue,square)));
    PRD->AddLayer(AddCPAP(ipap=new gLineChart(CPAP_IPAP,Qt::red,square)));
    PRD->setMinimumHeight(min_height);

    THPR->AddLayer(new gXAxis());
    THPR->AddLayer(new gYAxis());
    THPR->AddLayer(AddCPAP(new gLineChart(CPAP_TherapyPressure,QColor("dark green"),square)));
    THPR->AddLayer(AddCPAP(new gLineChart(CPAP_ExpiratoryPressure,QColor("dark blue"),square)));
    THPR->setMinimumHeight(min_height);

    //EXPR->AddLayer(new gXAxis());
    //EXPR->AddLayer(new gYAxis());
    //EXPR->setMinimumHeight(min_height);


    LEAK->AddLayer(new gXAxis());
    LEAK->AddLayer(new gYAxis());
    //LEAK->AddLayer(new gFooBar());
    LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_Leak,QColor("purple"),true)));

    LEAK->setMinimumHeight(min_height);


    MP->AddLayer(new gYAxis());
    MP->AddLayer(new gXAxis());
    gLineChart *g=new gLineChart(CPAP_MaskPressure,Qt::blue,false);
    AddCPAP(g);
    g->ReportEmpty(true);
    MP->AddLayer(g);
    //MP->AddLayer(AddCPAP(new gLineChart(CPAP_EPAP,Qt::yellow,square)));
    //MP->AddLayer(AddCPAP(new gLineChart(CPAP_IPAP,Qt::red,square)));
    MP->setMinimumHeight(min_height);

    //FRW->AddLayer(new gFooBar());
    FRW->AddLayer(new gYAxis());
    FRW->AddLayer(new gXAxis());
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_CSR,QColor("light green"),"CSR",FT_Span)));
    g=new gLineChart(CPAP_FlowRate,Qt::black,false);
    g->ReportEmpty(true);
    AddCPAP(g);
    FRW->AddLayer(g);
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_CSR,QColor("light green"),"CSR",FT_Span)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Hypopnea,QColor("blue"),"H")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_PressurePulse,QColor("red"),"PR",FT_Dot)));
    //FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Pressure,QColor("white"),"P",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_Unknown0B,QColor("blue"),"0B",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_Unknown10,QColor("orange"),"10",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_Unknown0E,QColor("yellow"),"0E",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_RERA,QColor("gold"),"RE")));
    //FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Unknown0E,QColor("dark green"),"U0E")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Apnea,QColor("dark green"),"A")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_VSnore,QColor("red"),"VS")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_FlowLimit,QColor("black"),"FL")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Obstructive,QColor("#40c0ff"),"OA")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_ClearAirway,QColor("purple"),"CA")));

    FRW->setMinimumHeight(min_height);

    SNORE->AddLayer(new gXAxis());
    SNORE->AddLayer(new gYAxis());
    SNORE->AddLayer(AddCPAP(new gLineChart(CPAP_Snore,Qt::black,true)));
    SNORE->setMinimumHeight(min_height);

    FLG->AddLayer(new gXAxis());
    FLG->AddLayer(new gYAxis());
    FLG->AddLayer(AddCPAP(new gLineChart(CPAP_FlowLimitGraph,Qt::black,true)));
    FLG->setMinimumHeight(min_height);

    MV->AddLayer(new gXAxis());
    MV->AddLayer(new gYAxis());
    MV->AddLayer(AddCPAP(new gLineChart(CPAP_MinuteVentilation,QColor(0x20,0x20,0x7f),true)));
    MV->setMinimumHeight(min_height);

    TV->AddLayer(new gXAxis());
    TV->AddLayer(new gYAxis());
    TV->AddLayer(AddCPAP(new gLineChart(CPAP_TidalVolume,QColor(0x7f,0x20,0x20),true)));
    TV->setMinimumHeight(min_height);

    RR->AddLayer(new gXAxis());
    RR->AddLayer(new gYAxis());
    RR->AddLayer(AddCPAP(new gLineChart(CPAP_RespiratoryRate,Qt::gray,true)));
    RR->setMinimumHeight(min_height);

    RE->AddLayer(new gXAxis());
    RE->AddLayer(new gYAxis());
    RE->AddLayer(AddCPAP(new gLineChart(CPAP_RespiratoryEvent,Qt::darkYellow,false)));
    RE->setMinimumHeight(min_height);

    IE->AddLayer(new gXAxis());
    IE->AddLayer(new gYAxis());
    IE->AddLayer(AddCPAP(new gLineChart(CPAP_IE,Qt::darkMagenta,false)));
    IE->setMinimumHeight(min_height);

    TE->AddLayer(new gXAxis());
    TE->AddLayer(new gYAxis());
    TE->AddLayer(AddCPAP(new gLineChart(CPAP_Te,Qt::darkBlue,false)));
    TE->setMinimumHeight(min_height);

    TI->AddLayer(new gXAxis());
    TI->AddLayer(new gYAxis());
    TI->AddLayer(AddCPAP(new gLineChart(CPAP_Ti,Qt::darkRed,false)));
    TI->setMinimumHeight(min_height);



    PTB->AddLayer(new gXAxis());
    PTB->AddLayer(new gYAxis());
    PTB->AddLayer(AddCPAP(new gLineChart(CPAP_PatientTriggeredBreaths,Qt::gray,true)));
    PTB->setMinimumHeight(min_height);

    INTPULSE->AddLayer(new gXAxis());
    INTPULSE->AddLayer(new gYAxis());
    INTPULSE->AddLayer(AddCPAP(new gLineChart(CPAP_Pulse,Qt::red,true)));
    INTPULSE->setMinimumHeight(min_height);

    INTSPO2->AddLayer(new gXAxis());
    INTSPO2->AddLayer(new gYAxis());
    INTSPO2->AddLayer(AddCPAP(new gLineChart(CPAP_SPO2,Qt::blue,true)));
    INTSPO2->setMinimumHeight(min_height);

    PLETHY->AddLayer(new gXAxis());
    PLETHY->AddLayer(new gYAxis());
    PLETHY->AddLayer(AddOXI(new gLineChart(OXI_Plethysomogram,Qt::darkCyan,true)));
    PLETHY->setMinimumHeight(min_height);

    PULSE->AddLayer(new gXAxis());
    PULSE->AddLayer(new gYAxis());
    PULSE->AddLayer(AddOXI(new gLineChart(OXI_Pulse,Qt::red,true)));
    PULSE->AddLayer(AddOXI(new gLineOverlayBar(OXI_PulseChange,Qt::green,"PC",FT_Bar)));
    PULSE->setMinimumHeight(min_height);

    SPO2->AddLayer(new gXAxis());
    SPO2->AddLayer(new gYAxis());
    SPO2->AddLayer(AddOXI(new gLineChart(OXI_SPO2,Qt::blue,true)));
    SPO2->AddLayer(AddOXI(new gLineOverlayBar(OXI_SPO2Drop,Qt::red,"SC",FT_Bar)));
    SPO2->setMinimumHeight(min_height); */

    /*fg=new gFlagsGroup();
    fg->AddLayer(AddOXI(new gFlagsLine(OXI_PulseChange,QColor("orange"),"PUL",true)));
    fg->AddLayer(AddOXI(new gFlagsLine(OXI_SPO2Change,QColor("gray"),"SP",true)));
    OF->AddLayer(fg);
    OF->AddLayer(new gFooBar(10,QColor("orange"),QColor("dark grey"),true));
    OF->SetLeftMargin(SF->GetLeftMargin()+gYAxis::Margin);
    OF->SetBlockZoom(true);
    OF->AddLayer(new gXAxis());
    OF->setMinimumHeight(min_height); */

    // SPO2->AddLayer(new gFooBar());
//    SPO2->setMinimumHeight(min_height);
//    SPO2->LinkZoom(PULSE);
//    PULSE->LinkZoom(SPO2);
//    SPO2->hide();
//    PULSE->hide();

    /*gSegmentChart *seg;

    TAP_EAP->SetMargins(0,0,0,0);
    TAP_EAP->AddLayer(AddCPAP(seg=new gTAPGraph(CPAP_EPAP)));

    TAP_EAP->hide();
    TAP_EAP->SetGradientBackground(false);

    TAP_IAP->SetMargins(0,0,0,0);
    TAP_IAP->AddLayer(AddCPAP(seg=new gTAPGraph(CPAP_IPAP)));
    TAP_IAP->hide();
    TAP_IAP->SetGradientBackground(false);

    TAP->SetMargins(0,0,0,0);
    TAP->AddLayer(AddCPAP(seg=new gTAPGraph(CPAP_Pressure,GST_CandleStick)));
    TAP->hide();
    TAP->SetGradientBackground(false);

    G_AHI->SetMargins(0,0,0,0);
    seg=new gSegmentChart(GST_Pie);
    seg->AddSlice(CPAP_Hypopnea,QColor(0x40,0x40,0xff,0xff),"H");
    seg->AddSlice(CPAP_Apnea,QColor(0x20,0x80,0x20,0xff),"A");
    seg->AddSlice(CPAP_Obstructive,QColor(0x40,0xaf,0xbf,0xff),"OA");
    seg->AddSlice(CPAP_ClearAirway,QColor(0xb2,0x54,0xcd,0xff),"CA");
    seg->AddSlice(CPAP_RERA,QColor(0xff,0xff,0x80,0xff),"RE");
    seg->AddSlice(CPAP_FlowLimit,QColor(0x40,0x40,0x40,0xff),"FL");
    G_AHI->AddLayer(AddCPAP(seg));
    G_AHI->SetGradientBackground(false);
    G_AHI->hide();


    splitter->addWidget(NoData);
    //int i=splitter->indexOf(NoData);
    splitter->setStretchFactor(NoData,1);

    gGraphWindow * graphs[]={SF,FRW,MP,RE,MV,TV,PTB,RR,IE,TE,TI,PRD,THPR,LEAK,FLG,SNORE,INTPULSE,INTSPO2};
    int ss=sizeof(graphs)/sizeof(gGraphWindow *);

    for (int i=0;i<ss;i++) {
        AddGraph(graphs[i]);
        //int j=splitter->indexOf(graphs[i]);
        splitter->setStretchFactor(graphs[i],1);
        //splitter->setAlignment(graphs[i],Qt::AlignTop);
        for (int j=0;j<ss;j++) {
            if (graphs[i]!=graphs[j]) {
                graphs[i]->LinkZoom(graphs[j]);
            }
        }
    }

    //AddGraph(OF);
    AddGraph(PULSE);
    AddGraph(SPO2);
    AddGraph(PLETHY);

    //SPO2->LinkZoom(OF);
    //PULSE->LinkZoom(OF);
    SPO2->LinkZoom(PULSE);
    SPO2->LinkZoom(PLETHY);
    PULSE->LinkZoom(SPO2);
    PULSE->LinkZoom(PLETHY);
    PLETHY->LinkZoom(PULSE);
    PLETHY->LinkZoom(SPO2);
    //OF->LinkZoom(PULSE);
    //OF->LinkZoom(SPO2);

    //  AddGraph(SPO2);
    spacer=new gGraphWindow(scrollArea,"",SF);
    spacer->setMinimumHeight(1);
    spacer->setMaximumHeight(1);
    splitter->addWidget(spacer);
    //i=splitter->indexOf(spacer);
    //splitter->setStretchFactor(i,1);
    //i=splitter->indexOf(FRW);
    //splitter->setStretchFactor(i,15);


    //splitter->refresh();

    //splitter->setChildrenCollapsible(false);  // We set this per widget..
    //splitter->setCollapsible(splitter->indexOf(SF),false);
    //splitter->setStretchFactor(splitter->indexOf(SF),0);

    //splitter_sizes=splitter->sizes();
    //splitter->layout();
    //splitter->update();


    QTextCharFormat format = ui->calendar->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    ui->calendar->setWeekdayTextFormat(Qt::Saturday, format);
    ui->calendar->setWeekdayTextFormat(Qt::Sunday, format);

    ui->tabWidget->setCurrentWidget(ui->details);

    if (mainwin) {
        show_graph_menu=mainwin->CreateMenu("Graphs");
        show_graph_menu->clear();
        for (int i=0;i<Graphs.size();i++) {
            QAction * action=show_graph_menu->addAction(Graphs[i]->Title(),NULL,NULL,0);
            action->setCheckable(true);
            action->setChecked(true);
            connect(action, SIGNAL(triggered()), this, SLOT(ShowHideGraphs()));
            GraphAction.push_back(action);
        }

    } else show_graph_menu=NULL; */
 }

Daily::~Daily()
{
    // Save any last minute changes..
    if (previous_date.isValid())
        Unload(previous_date);

//    delete splitter;
    delete ui;
}

void Daily::resizeEvent (QResizeEvent * event)
{
    //const QSize &size=event->size();
  //  splitter->setMinimumWidth(size.width()-280);
}

void Daily::ReloadGraphs()
{
    QDate d=profile->LastDay();
    if (!d.isValid()) {
        d=ui->calendar->selectedDate();
    }
    on_calendar_currentPageChanged(d.year(),d.month());
    ui->calendar->setSelectedDate(d);
    Load(d);
}
void Daily::on_calendar_currentPageChanged(int year, int month)
{
    QDate d(year,month,1);
    int dom=d.daysInMonth();

    for (int i=1;i<=dom;i++) {
        d=QDate(year,month,i);
        this->UpdateCalendarDay(d);
    }
}
void Daily::UpdateEventsTree(QTreeWidget *tree,Day *day)
{
    tree->clear();
    if (!day) return;

    //return;
    tree->setColumnCount(1); // 1 visible common.. (1 hidden)

    QTreeWidgetItem *root=NULL;//new QTreeWidgetItem((QTreeWidget *)0,QStringList("Stuff"));
    QHash<ChannelID,QTreeWidgetItem *> mcroot;
    QHash<ChannelID,int> mccnt;
    int total_events=0;

    for (QVector<Session *>::iterator s=day->begin();s!=day->end();s++) {

        QHash<ChannelID,QVector<EventList *> >::iterator m;

        //QTreeWidgetItem * sroot;

        for (m=(*s)->eventlist.begin();m!=(*s)->eventlist.end();m++) {
            ChannelID code=m.key();
            if ((code!=CPAP_Obstructive)
                && (code!=CPAP_Hypopnea)
                && (code!=CPAP_Apnea)
                && (code!=PRS1_Unknown0B)
                && (code!=CPAP_ClearAirway)
                && (code!=CPAP_CSR)
                && (code!=CPAP_RERA)
                && (code!=CPAP_FlowLimit)
                && (code!=CPAP_PressurePulse)
                && (code!=CPAP_VSnore)) continue;
            QTreeWidgetItem *mcr;
            if (mcroot.find(code)==mcroot.end()) {
                int cnt=day->count(code);
                total_events+=cnt;
                QString st=channel[m.key()].details();
                if (st.isEmpty())  {
                    st="Fixme "+QString::number((int)code);
                }
                st+=" ("+QString::number(cnt)+" event"+((cnt>1)?"s":"")+")";
                QStringList l(st);
                l.append("");
                mcroot[code]=mcr=new QTreeWidgetItem(root,l);
                mccnt[code]=0;
            } else {
                mcr=mcroot[code];
            }
            for (int z=0;z<m.value().size();z++) {
                for (int o=0;o<m.value()[z]->count();o++) {
                    qint64 t=m.value()[z]->time(o);

                    if (code==CPAP_CSR) {
                        t-=float(m.value()[z]->raw(o)/2.0)*1000.0;
                    }
                    QStringList a;
                    QDateTime d=QDateTime::fromMSecsSinceEpoch(t);
                    QString s=QString("#%1: %2 (%3)").arg((int)++mccnt[code],(int)3,(int)10,QChar('0')).arg(d.toString("HH:mm:ss")).arg(m.value()[z]->raw(o));
                    a.append(s);
                    a.append(d.toString("yyyy-MM-dd HH:mm:ss"));
                    mcr->addChild(new QTreeWidgetItem(a));
                }
            }
        }
    }
    int cnt=0;
    for (QHash<ChannelID,QTreeWidgetItem *>::iterator m=mcroot.begin();m!=mcroot.end();m++) {
        tree->insertTopLevelItem(cnt++,m.value());
    }
    //tree->insertTopLevelItem(cnt++,new QTreeWidgetItem(QStringList("[Total Events ("+QString::number(total_events)+")]")));
    tree->sortByColumn(0,Qt::AscendingOrder);
    //tree->expandAll();
}
void Daily::UpdateCalendarDay(QDate date)
{
    QTextCharFormat bold;
    QTextCharFormat cpapcol;
    QTextCharFormat normal;
    QTextCharFormat oxiday;
    bold.setFontWeight(QFont::Bold);
    cpapcol.setForeground(QBrush(Qt::blue, Qt::SolidPattern));
    cpapcol.setFontWeight(QFont::Bold);
    oxiday.setForeground(QBrush(Qt::red, Qt::SolidPattern));
    oxiday.setFontWeight(QFont::Bold);
    bool hascpap=profile->GetDay(date,MT_CPAP)!=NULL;
    bool hasoxi=profile->GetDay(date,MT_OXIMETER)!=NULL;

    if (hascpap) {
        if (hasoxi) {
            ui->calendar->setDateTextFormat(date,oxiday);
        } else {
            ui->calendar->setDateTextFormat(date,cpapcol);
        }
    } else if (profile->GetDay(date)) {
        ui->calendar->setDateTextFormat(date,bold);
    } else {
        ui->calendar->setDateTextFormat(date,normal);
    }
    ui->calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);

}
void Daily::on_calendar_selectionChanged()
{
    if (previous_date.isValid())
        Unload(previous_date);

    Load(ui->calendar->selectedDate());
}
void Daily::ResetGraphLayout()
{
    //splitter->setSizes(splitter_sizes);

}
void Daily::ShowHideGraphs()
{
/*    int vis=0;
    for (int i=0;i<Graphs.size();i++) {
        if (Graphs[i]->isEmpty()) {
            GraphAction[i]->setVisible(false);
            Graphs[i]->hide();
        } else {
            Graphs[i]->ResetBounds();
            GraphAction[i]->setVisible(true);
            if (GraphAction[i]->isChecked()) {
                Graphs[i]->show();
                vis++;
            } else {
                Graphs[i]->hide();
            }
        }
    }
    GraphLayout->setMinimumHeight(vis*default_height+10);
    //splitter->setMaximumHeight(vis*default_height);
    splitter->layout();
    //splitter->update();
    RedrawGraphs(); */
}
void Daily::Load(QDate date)
{
    static Day * lastcpapday=NULL;
    previous_date=date;
    Day *cpap=profile->GetDay(date,MT_CPAP);
    Day *oxi=profile->GetDay(date,MT_OXIMETER);
   // Day *sleepstage=profile->GetDay(date,MT_SLEEPSTAGE);

    if (!pref["MemoryHog"].toBool()) {
        if (lastcpapday && (lastcpapday!=cpap)) {
            for (QVector<Session *>::iterator s=lastcpapday->begin();s!=lastcpapday->end();s++) {
                (*s)->TrashEvents();
            }
        }
    }
    lastcpapday=cpap;
    QString html="<html><head><style type='text/css'>p,a,td,body { font-family: 'FreeSans', 'Sans Serif'; } p,a,td,body { font-size: 12px; } </style>"
    "</head>"
    "<body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>"
    "<table cellspacing=0 cellpadding=2 border=0 width='100%'>\n";
    QString tmp;
    const int gwwidth=240;
    const int gwheight=100;
    UpdateOXIGraphs(oxi);
    UpdateCPAPGraphs(cpap);
    UpdateEventsTree(ui->treeWidget,cpap);

    GraphView->ResetBounds();

    GraphView->updateGL();
    if (!cpap && !oxi) {
        //splitter->setMinimumHeight(0);
        scrollbar->hide();
        GraphView->hide();
        NoData->setText(tr("No data for ")+date.toString(Qt::SystemLocaleLongDate));
        NoData->show();
    } else {
        NoData->hide();
        GraphView->show();
        scrollbar->show();
    }

    //RedrawGraphs();

    QString epr,modestr;
    float iap90,eap90;
    CPAPMode mode=MODE_UNKNOWN;
    PRTypes pr;
    QString a;
    if (cpap) {
        mode=(CPAPMode)cpap->settings_max(CPAP_Mode);
        pr=(PRTypes)cpap->settings_max(PRS1_PressureReliefType);
        if (pr==PR_NONE)
           epr=tr(" No Pressure Relief");
        else {
            epr=channel[PRS1_PressureReliefSetting].optionString(pr)+QString(" x%1").arg((int)cpap->settings_max(PRS1_PressureReliefSetting));
        }
        modestr=channel[CPAP_Mode].optionString(mode);

        float ahi=(cpap->count(CPAP_Obstructive)+cpap->count(CPAP_Hypopnea)+cpap->count(CPAP_ClearAirway)+cpap->count(CPAP_Apnea))/cpap->hours();
        float csr=(100.0/cpap->hours())*(cpap->sum(CPAP_CSR)/3600.0);
        float uai=cpap->count(CPAP_Apnea)/cpap->hours();
        float oai=cpap->count(CPAP_Obstructive)/cpap->hours();
        float hi=cpap->count(CPAP_Hypopnea)/cpap->hours();
        float cai=cpap->count(CPAP_ClearAirway)/cpap->hours();
        float rei=cpap->count(CPAP_RERA)/cpap->hours();
        float vsi=cpap->count(CPAP_VSnore)/cpap->hours();
        float fli=cpap->count(CPAP_FlowLimit)/cpap->hours();

        //float p90=cpap->p90(CPAP_Pressure);
        //eap90=cpap->p90(CPAP_EPAP);
        //iap90=cpap->p90(CPAP_IPAP);
        QString submodel=tr("Unknown Model");

        //html+="<tr><td colspan=4 align=center><i>"+tr("Machine Information")+"</i></td></tr>\n";
        if (cpap->machine->properties.find("SubModel")!=cpap->machine->properties.end())
            submodel=" <br>"+cpap->machine->properties["SubModel"];
        html+="<tr><td colspan=4 align=center><b>"+cpap->machine->properties["Brand"]+"</b> <br>"+cpap->machine->properties["Model"]+" "+cpap->machine->properties["ModelNumber"]+submodel+"</td></tr>\n";
        if (pref.Exists("ShowSerialNumbers") && pref["ShowSerialNumbers"].toBool()) {
            html+="<tr><td colspan=4 align=center>"+cpap->machine->properties["Serial"]+"</td></tr>\n";
        }

        html+="<tr><td align='center'><b>Date</b></td><td align='center'><b>"+tr("Sleep")+"</b></td><td align='center'><b>"+tr("Wake")+"</b></td><td align='center'><b>"+tr("Hours")+"</b></td></tr>";
        int tt=cpap->total_time()/1000.0;
        QDateTime date=QDateTime::fromMSecsSinceEpoch(cpap->first());
        QDateTime date2=QDateTime::fromMSecsSinceEpoch(cpap->last());

        int h=tt/3600.0;
        int m=(tt/60)%60;
        int s=tt % 60;
        html+=QString("<tr><td align='center'>%1</td><td align='center'>%2</td><td align='center'>%3</td><td align='center'>%4</td></tr>\n"
        "<tr><td colspan=4 align=center><hr></td></tr>\n")
                .arg(date.date().toString(Qt::SystemLocaleShortDate))
                .arg(date.toString("HH:mm"))
                .arg(date2.toString("HH:mm"))
                .arg(QString().sprintf("%02i:%02i:%02i",h,m,s));

        QString cs;
        if (cpap->machine->GetClass()!="PRS1") {
            cs="4 align=center>";
        } else cs="2>";
        html+="<tr><td colspan="+cs+"<table cellspacing=0 cellpadding=2 border=0 width='100%'>"
        "<tr><td align='right' bgcolor='#F88017'><b><font color='black'>"+tr("AHI")+"</font></b></td><td  bgcolor='#F88017'><b><font color='black'>"+QString().sprintf("%.2f",ahi)+"</font></b></td></tr>\n"
        "<tr><td align='right' bgcolor='#4040ff'><b><font color='white'>"+tr("Hypopnea")+"</font></b></td><td bgcolor='#4040ff'><font color='white'>"+QString().sprintf("%.2f",hi)+"</font></td></tr>\n";
        if (cpap->machine->GetClass()=="ResMed") {
            html+="<tr><td align='right' bgcolor='#208020'><b>"+tr("Apnea")+"</b></td><td bgcolor='#208020'>"+QString().sprintf("%.2f",uai)+"</td></tr>\n";
        }
        html+="<tr><td align='right' bgcolor='#40afbf'><b>"+tr("Obstructive")+"</b></td><td bgcolor='#40afbf'>"+QString().sprintf("%.2f",oai)+"</td></tr>\n"
        "<tr><td align='right' bgcolor='#b254cd'><b>"+tr("Clear Airway")+"</b></td><td bgcolor='#b254cd'>"+QString().sprintf("%.2f",cai)+"</td></tr>\n"
        "</table></td>";

        if (cpap->machine->GetClass()=="PRS1") {
            html+="<td colspan=2><table cellspacing=0 cellpadding=2 border=0 width='100%'>"
            "<tr><td align='right' bgcolor='#ffff80'><b>"+tr("RERA")+"</b></td><td bgcolor='#ffff80'>"+QString().sprintf("%.2f",rei)+"</td></tr>\n"
            "<tr><td align='right' bgcolor='#404040'><b><font color='white'>"+tr("FlowLimit")+"</font></b></td><td bgcolor='#404040'><font color='white'>"+a.sprintf("%.2f",fli)+"</font></td></tr>\n"
            "<tr><td align='right' bgcolor='#ff4040'><b>"+tr("Vsnore")+"</b></td><td bgcolor='#ff4040'>"+QString().sprintf("%.2f",vsi)+"</td></tr>\n"
            "<tr><td align='right' bgcolor='#80ff80'><b>"+tr("PB/CSR")+"</b></td><td bgcolor='#80ff80'>"+QString().sprintf("%.2f",csr)+"%</td></tr>\n"
            "</table></td>";
        }


        // Note, this may not be a problem since Qt bug 13622 was discovered
        // as it only relates to text drawing, which the Pie chart does not do
        // ^^ Scratch that.. pie now includes text..

        if (pref["EnableGraphSnapshots"].toBool()) {  // AHI Pie Chart
//            if (ahi+rei+fli>0) {
//                html+="</tr>\n<tr><td colspan=4 align=center><i>"+tr("Event Breakdown")+"</i></td></tr>\n";
//                G_AHI->setFixedSize(gwwidth,120);
//                QPixmap pixmap=G_AHI->renderPixmap(gwwidth,120,false); //gwwidth,gwheight,false);
//                QByteArray byteArray;
//                QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
//                buffer.open(QIODevice::WriteOnly);
//                pixmap.save(&buffer, "PNG");
//                html += "<tr><td colspan=4 align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\"></td></tr>\n";
//            } else {
//                html += "<tr><td colspan=4 align=center><img src=\"qrc:/docs/0.0.gif\"></td></tr>\n";
//            }
        }
    }
    html+="</table>";
    html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";
    if (cpap || oxi) {
        html+="<tr height='2'><td colspan=5 height='2'><hr></td></tr>\n";

        //html+=("<tr><td colspan=4 align=center>&nbsp;</td></tr>\n");

        html+=("<tr><td> </td><td><b>Min</b></td><td><b>Avg</b></td><td><b>90%</b></td><td><b>Max</b></td></tr>");
        ChannelID chans[]={
            CPAP_Pressure,CPAP_EPAP,CPAP_IPAP,CPAP_PressureSupport,CPAP_PatientTriggeredBreaths,
            CPAP_MinuteVentilation,CPAP_RespiratoryRate,CPAP_RespiratoryEvent,CPAP_FlowLimitGraph,
            CPAP_Leak,CPAP_Snore,CPAP_IE,CPAP_Ti,CPAP_Te,CPAP_TidalVolume,
            CPAP_Pulse,CPAP_SPO2,OXI_Pulse,OXI_SPO2
        };
        int numchans=sizeof(chans)/sizeof(ChannelID);
        for (int i=0;i<numchans;i++) {
            ChannelID code=chans[i];
            if (cpap && cpap->channelExists(code)) {
                html+="<tr><td align=left>"+channel[code].label();
                html+="</td><td>"+a.sprintf("%.2f",cpap->min(code));
                html+="</td><td>"+a.sprintf("%.2f",cpap->wavg(code));
                html+="</td><td>"+a.sprintf("%.2f",cpap->p90(code));
                html+="</td><td>"+a.sprintf("%.2f",cpap->max(code));
                html+="</td><tr>";
            }
            if (oxi && oxi->channelExists(code)) {
                html+="<tr><td align=left>"+channel[code].label();
                html+="</td><td>"+a.sprintf("%.2f",oxi->min(code));
                html+="</td><td>"+a.sprintf("%.2f",oxi->wavg(code));
                html+="</td><td>"+a.sprintf("%.2f",oxi->p90(code));
                html+="</td><td>"+a.sprintf("%.2f",oxi->max(code));
                html+="</td><tr>";
            }
        }

    } else {
        html+="<tr><td colspan=5 align=center><i>"+tr("No data available")+"</i></td></tr>";
        html+="<tr><td colspan=5>&nbsp;</td></tr>\n";

    }
    html+="</table>";
    html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";

    if (cpap) {
        if (pref["EnableGraphSnapshots"].toBool()) {
      /*      if (cpap->channelExists(CPAP_Pressure)) {
                html+=("<tr><td colspan=4 align=center><i>")+tr("Time@Pressure")+("</i></td></tr>\n");
                TAP->setFixedSize(gwwidth,30);
                QPixmap pixmap=TAP->renderPixmap(gwwidth,30,false);
                QByteArray byteArray;
                QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
                buffer.open(QIODevice::WriteOnly);
                pixmap.save(&buffer, "PNG");
                html+="<tr><td colspan=4 align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\"></td></tr>\n";
            }
            if (cpap->channelExists(CPAP_EPAP)) {
                //html+="<tr height='2'><td colspan=4 height='2'><hr></td></tr>\n";
                html+=("<tr><td colspan=4 align=center><i>")+tr("Time@EPAP")+("</i></td></tr>\n");
                TAP_EAP->setFixedSize(gwwidth,30);
                QPixmap pixmap=TAP_EAP->renderPixmap(gwwidth,30,false);
                QByteArray byteArray;
                QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
                buffer.open(QIODevice::WriteOnly);
                pixmap.save(&buffer, "PNG");
                html+="<tr><td colspan=4 align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\"></td></tr>\n";
            }
            if (cpap->channelExists(CPAP_IPAP)) {
                html+=("<tr><td colspan=4 align=center><i>")+tr("Time@IPAP")+("</i></td></tr>\n");
                TAP_IAP->setFixedSize(gwwidth,30);
                QPixmap pixmap=TAP_IAP->renderPixmap(gwwidth,30,false);
                QByteArray byteArray;
                QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
                buffer.open(QIODevice::WriteOnly);
                pixmap.save(&buffer, "PNG");
                html+="<tr><td colspan=4 align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\"></td></tr>\n";
            } */

        }
        html+="</table><hr height=2><table cellpadding=0 cellspacing=0 border=0 width=100%>";
        html+="<tr><td align=center>SessionID</td><td align=center>Date</td><td align=center>Start</td><td align=center>End</td></tr>";
        QDateTime fd,ld;
        bool corrupted_waveform=false;
        for (QVector<Session *>::iterator s=cpap->begin();s!=cpap->end();s++) {
            fd=QDateTime::fromMSecsSinceEpoch((*s)->first());
            ld=QDateTime::fromMSecsSinceEpoch((*s)->last());
            QHash<ChannelID,QVariant>::iterator i=(*s)->settings.find(CPAP_BrokenWaveform);
            if ((i!=(*s)->settings.end()) && i.value().toBool()) corrupted_waveform=true;
            tmp.sprintf(("<tr><td align=center>%08i</td><td align=center>"+fd.date().toString(Qt::SystemLocaleShortDate)+"</td><td align=center>"+fd.toString("HH:mm ")+"</td><td align=center>"+ld.toString("HH:mm")+"</td></tr>").toLatin1(),(*s)->session());
            html+=tmp;
        }
        html+="</table>";
        if (corrupted_waveform) {
            html+="<hr><div align=center><i>One or more waveform record for this session had faulty source data. Some waveform overlay points may not match up correctly.</i></div>";
        }
    }
    html+="</body></html>";

    ui->webView->setHtml(html);

    ui->JournalNotes->clear();
    Session *journal=GetJournalSession(date);
    if (journal) {
        ui->JournalNotes->setHtml(journal->settings[JOURNAL_Notes].toString());
    }

}
void Daily::Unload(QDate date)
{
    Session *journal=GetJournalSession(date);
    if (!ui->JournalNotes->toPlainText().isEmpty()) {
        QString jhtml=ui->JournalNotes->toHtml();
        if (journal) {
            if (journal->settings[JOURNAL_Notes]!=jhtml) {
                journal->settings[JOURNAL_Notes]=jhtml;
                journal->SetChanged(true);
            }

        } else {
            journal=CreateJournalSession(date);
            journal->settings[JOURNAL_Notes]=jhtml;
            journal->SetChanged(true);
        }

    }
    if (journal) {
        Machine *jm=profile->GetMachine(MT_JOURNAL);
        if (jm) jm->SaveSession(journal);
    }
    UpdateCalendarDay(date);
}

void Daily::on_JournalNotesItalic_clicked()
{
    QTextCursor cursor = ui->JournalNotes->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    QTextCharFormat format=cursor.charFormat();

    format.setFontItalic(!format.fontItalic());


    cursor.mergeCharFormat(format);
   //ui->JournalNotes->mergeCurrentCharFormat(format);

}

void Daily::on_JournalNotesBold_clicked()
{
    QTextCursor cursor = ui->JournalNotes->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    QTextCharFormat format=cursor.charFormat();

    int fw=format.fontWeight();
    if (fw!=QFont::Bold)
        format.setFontWeight(QFont::Bold);
    else
        format.setFontWeight(QFont::Normal);

    cursor.mergeCharFormat(format);
    //ui->JournalNotes->mergeCurrentCharFormat(format);

}

void Daily::on_JournalNotesFontsize_activated(int index)
{
    QTextCursor cursor = ui->JournalNotes->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    QTextCharFormat format=cursor.charFormat();

    QFont font=format.font();
    int fontsize=10;

    if (index==1) fontsize=15;
    else if (index==2) fontsize=25;

    font.setPointSize(fontsize);
    format.setFont(font);

    cursor.mergeCharFormat(format);
}

void Daily::on_JournalNotesColour_clicked()
{
    QColor col=QColorDialog::getColor(Qt::black,this,tr("Pick a Colour")); //,QColorDialog::NoButtons);
    if (!col.isValid()) return;

    QTextCursor cursor = ui->JournalNotes->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    QBrush b(col);
    QPalette newPalette = palette();
    newPalette.setColor(QPalette::ButtonText, col);
    ui->JournalNotesColour->setPalette(newPalette);

    QTextCharFormat format=cursor.charFormat();

    format.setForeground(b);

    cursor.setCharFormat(format);
}
Session * Daily::CreateJournalSession(QDate date)
{
    Machine *m=profile->GetMachine(MT_JOURNAL);
    if (!m) {
        m=new Machine(profile,0);
        m->SetClass("Journal");
        m->properties["Brand"]="Virtual";
        m->SetType(MT_JOURNAL);
        profile->AddMachine(m);
    }
    Session *sess=new Session(m,0);
    QDateTime dt;
    dt.setDate(date);
    dt.setTime(QTime(17,0)); //5pm to make sure it goes in the right day
    sess->set_first(dt.toMSecsSinceEpoch());
    dt=dt.addSecs(3600);
    sess->set_last(dt.toMSecsSinceEpoch());
    sess->SetChanged(true);
    m->AddSession(sess,profile);
    return sess;
}
Session * Daily::GetJournalSession(QDate date) // Get the first journal session
{
    Day *journal=profile->GetDay(date,MT_JOURNAL);
    if (!journal)
        return NULL; //CreateJournalSession(date);
    QVector<Session *>::iterator s;
    s=journal->begin();
    if (s!=journal->end())
        return *s;
    return NULL;
}
void Daily::on_EnergySlider_sliderMoved(int position)
{
    //Session *s=GetJournalSession(previous_date);
    //if (!s)
      //  s=CreateJournalSession(previous_date);

    //s->summary[JOURNAL_Energy]=position;
    //s->SetChanged(true);
}

void Daily::UpdateCPAPGraphs(Day *day)
{
    //if (!day) return;
    if (day) {
        day->OpenEvents();
    }
    for (QList<Layer *>::iterator g=CPAPData.begin();g!=CPAPData.end();g++) {
        (*g)->SetDay(day);
    }
};

void Daily::UpdateOXIGraphs(Day *day)
{
    //if (!day) return;

    if (day) {
        day->OpenEvents();
    }
    for (QList<Layer *>::iterator g=OXIData.begin();g!=OXIData.end();g++) {
        (*g)->SetDay(day);
    }
}

void Daily::RedrawGraphs()
{
    /*for (int i=0;i<Graphs.size();i++) {
       Graphs[i]->updateGL();
    } */
}

void Daily::on_treeWidget_itemSelectionChanged()
{
    if (ui->treeWidget->selectedItems().size()==0) return;
    QTreeWidgetItem *item=ui->treeWidget->selectedItems().at(0);
    if (!item) return;
    QDateTime d;
    if (!item->text(1).isEmpty()) {
        d=d.fromString(item->text(1),"yyyy-MM-dd HH:mm:ss");
        double st=(d.addSecs(-120)).toMSecsSinceEpoch();
        double et=(d.addSecs(120)).toMSecsSinceEpoch();
        GraphView->SetXBounds(st,et);
    }
}

void Daily::on_JournalNotesUnderline_clicked()
{
    QTextCursor cursor = ui->JournalNotes->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    QTextCharFormat format=cursor.charFormat();

    format.setFontUnderline(!format.fontUnderline());

    cursor.mergeCharFormat(format);
   //ui->JournalNotes->mergeCurrentCharFormat(format);
}
