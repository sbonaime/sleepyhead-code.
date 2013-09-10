/*
 Daily Panel
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

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
#include <QSpacerItem>
#include <QWebFrame>
#include <cmath>
//#include <QPrinter>
//#include <QProgressBar>

#include "daily.h"
#include "ui_daily.h"

#include "common_gui.h"
#include "SleepLib/profiles.h"
#include "SleepLib/session.h"
#include "Graphs/graphdata_custom.h"
#include "Graphs/gLineOverlay.h"
#include "Graphs/gFlagsLine.h"
#include "Graphs/gFooBar.h"
#include "Graphs/gXAxis.h"
#include "Graphs/gYAxis.h"
#include "Graphs/gSegmentChart.h"
#include "Graphs/gStatsLine.h"

//extern QProgressBar *qprogress;
extern MainWindow * mainwin;

const int min_height=150;

Daily::Daily(QWidget *parent,gGraphView * shared)
    :QWidget(parent), ui(new Ui::Daily)
{
    ui->setupUi(this);

    // Remove Incomplete Extras Tab
    //ui->tabWidget->removeTab(3);

    ZombieMeterMoved=false;
    BookmarksChanged=false;

    QList<int> a;
    a.push_back(300);
    a.push_back(this->width()-300);
    ui->splitter_2->setStretchFactor(1,1);
    ui->splitter_2->setSizes(a);
    ui->splitter_2->setStretchFactor(1,1);

    layout=new QHBoxLayout(ui->graphMainArea);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setContentsMargins(0,0,0,0);
    ui->graphMainArea->setLayout(layout);
    //ui->graphMainArea->setLayout(layout);

    ui->graphMainArea->setAutoFillBackground(false);

    GraphView=new gGraphView(ui->graphMainArea,shared);
    GraphView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    snapGV=new gGraphView(GraphView); //ui->graphMainArea);
    snapGV->setMinimumSize(172,172);
    snapGV->hideSplitter();
    snapGV->hide();

    scrollbar=new MyScrollBar(ui->graphMainArea);
    scrollbar->setOrientation(Qt::Vertical);
    scrollbar->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
    scrollbar->setMaximumWidth(20);

    ui->bookmarkTable->setColumnCount(2);
    ui->bookmarkTable->setColumnWidth(0,70);
    //ui->bookmarkTable->setEditTriggers(QAbstractItemView::SelectedClicked);
    //ui->bookmarkTable->setColumnHidden(2,true);
    //ui->bookmarkTable->setColumnHidden(3,true);
    GraphView->setScrollBar(scrollbar);
    layout->addWidget(GraphView,1);
    layout->addWidget(scrollbar,0);

    int default_height=PROFILE.appearance->graphHeight();
    SF=new gGraph(GraphView,STR_TR_EventFlags,STR_TR_EventFlags,default_height);
    FRW=new gGraph(GraphView,STR_TR_FlowRate,schema::channel[CPAP_FlowRate].description()+"\n("+schema::channel[CPAP_FlowRate].units()+")",default_height);


    if (PROFILE.general->calculateRDI()) {
        AHI=new gGraph(GraphView,STR_TR_RDI,schema::channel[CPAP_RDI].description()+"\n("+schema::channel[CPAP_RDI].units()+")",default_height);
    } else AHI=new gGraph(GraphView,STR_TR_AHI,schema::channel[CPAP_AHI].description()+"\n("+schema::channel[CPAP_AHI].units()+")",default_height);

    MP=new gGraph(GraphView,STR_TR_MaskPressure,schema::channel[CPAP_MaskPressure].description()+"\n("+schema::channel[CPAP_MaskPressure].units()+")",default_height);
    PRD=new gGraph(GraphView,STR_TR_Pressure,schema::channel[CPAP_Pressure].description()+"\n("+schema::channel[CPAP_Pressure].units()+")",default_height);
    LEAK=new gGraph(GraphView,STR_TR_Leak,schema::channel[CPAP_Leak].description()+"\n("+schema::channel[CPAP_Leak].units()+")",default_height);
    SNORE=new gGraph(GraphView,STR_TR_Snore,schema::channel[CPAP_Snore].description()+"\n("+schema::channel[CPAP_Snore].units()+")",default_height);
    RR=new gGraph(GraphView,STR_TR_RespRate,schema::channel[CPAP_RespRate].description()+"\n("+schema::channel[CPAP_RespRate].units()+")",default_height);
    TV=new gGraph(GraphView,STR_TR_TidalVolume,schema::channel[CPAP_TidalVolume].description()+"\n("+schema::channel[CPAP_TidalVolume].units()+")",default_height);
    MV=new gGraph(GraphView,STR_TR_MinuteVent,schema::channel[CPAP_MinuteVent].description()+"\n("+schema::channel[CPAP_MinuteVent].units()+")",default_height);
    //TgMV=new gGraph(GraphView,STR_TR_TgtMinVent,schema::channel[CPAP_TgMV].description()+"\n("+schema::channel[CPAP_TgMV].units()+")",default_height);
    FLG=new gGraph(GraphView,STR_TR_FlowLimit,schema::channel[CPAP_FLG].description()+"\n("+schema::channel[CPAP_FLG].units()+")",default_height);
    PTB=new gGraph(GraphView,STR_TR_PatTrigBreath,schema::channel[CPAP_PTB].description()+"\n("+schema::channel[CPAP_PTB].units()+")",default_height);
    RE=new gGraph(GraphView,STR_TR_RespEvent,schema::channel[CPAP_RespEvent].description()+"\n("+schema::channel[CPAP_RespEvent].units()+")",default_height);
    TI=new gGraph(GraphView,STR_TR_InspTime,schema::channel[CPAP_Ti].description()+"\n("+schema::channel[CPAP_Ti].units()+")",default_height);
    TE=new gGraph(GraphView,STR_TR_ExpTime,schema::channel[CPAP_Te].description()+"\n("+schema::channel[CPAP_Te].units()+")",default_height);
    IE=new gGraph(GraphView,STR_TR_IE,schema::channel[CPAP_IE].description()+"\n("+schema::channel[CPAP_IE].units()+")",default_height);

    STAGE=new gGraph(GraphView,STR_TR_SleepStage,schema::channel[ZEO_SleepStage].description()+"\n("+schema::channel[ZEO_SleepStage].units()+")",default_height);
    int oxigrp=PROFILE.ExistsAndTrue("SyncOximetry") ? 0 : 1;
    PULSE=new gGraph(GraphView,STR_TR_PulseRate,schema::channel[OXI_Pulse].description()+"\n("+schema::channel[OXI_Pulse].units()+")",default_height,oxigrp);
    SPO2=new gGraph(GraphView,STR_TR_SpO2,schema::channel[OXI_SPO2].description()+"\n("+schema::channel[OXI_SPO2].units()+")",default_height,oxigrp);
    PLETHY=new gGraph(GraphView,STR_TR_Plethy,schema::channel[OXI_Plethy].description()+"\n("+schema::channel[OXI_Plethy].units()+")",default_height,oxigrp);

    // Event Pie Chart (for snapshot purposes)
    // TODO: Convert snapGV to generic for snapshotting multiple graphs (like reports does)
//    TAP=new gGraph(GraphView,"Time@Pressure",STR_UNIT_CMH2O,100);
//    TAP->showTitle(false);
//    gTAPGraph * tap=new gTAPGraph(CPAP_Pressure,GST_Line);
//    TAP->AddLayer(AddCPAP(tap));
    //TAP->setMargins(0,0,0,0);


    GAHI=new gGraph(snapGV,tr("Breakdown"),tr("events"),172);
    gSegmentChart * evseg=new gSegmentChart(GST_Pie);
    evseg->AddSlice(CPAP_Hypopnea,QColor(0x40,0x40,0xff,0xff),STR_TR_H);
    evseg->AddSlice(CPAP_Apnea,QColor(0x20,0x80,0x20,0xff),STR_TR_UA);
    evseg->AddSlice(CPAP_Obstructive,QColor(0x40,0xaf,0xbf,0xff),STR_TR_OA);
    evseg->AddSlice(CPAP_ClearAirway,QColor(0xb2,0x54,0xcd,0xff),STR_TR_CA);
    evseg->AddSlice(CPAP_RERA,QColor(0xff,0xff,0x80,0xff),STR_TR_RE);
    evseg->AddSlice(CPAP_NRI,QColor(0x00,0x80,0x40,0xff),STR_TR_NR);
    evseg->AddSlice(CPAP_FlowLimit,QColor(0x40,0x40,0x40,0xff),STR_TR_FL);
    //evseg->AddSlice(CPAP_UserFlag1,QColor(0x40,0x40,0x40,0xff),tr("UF"));

    GAHI->AddLayer(AddCPAP(evseg));
    GAHI->setMargins(0,0,0,0);
    //SF->AddLayer(AddCPAP(evseg),LayerRight,100);

    gFlagsGroup *fg=new gFlagsGroup();
    SF->AddLayer(AddCPAP(fg));
    fg->AddLayer((new gFlagsLine(CPAP_CSR, COLOR_CSR, STR_TR_PB,false,FT_Span)));
    fg->AddLayer((new gFlagsLine(CPAP_ClearAirway, COLOR_ClearAirway, STR_TR_CA,false)));
    fg->AddLayer((new gFlagsLine(CPAP_Obstructive, COLOR_Obstructive, STR_TR_OA,true)));
    fg->AddLayer((new gFlagsLine(CPAP_Apnea, COLOR_Apnea, STR_TR_UA)));
    fg->AddLayer((new gFlagsLine(CPAP_Hypopnea, COLOR_Hypopnea, STR_TR_H,true)));
    fg->AddLayer((new gFlagsLine(CPAP_ExP, COLOR_ExP, STR_TR_EP,false)));
    fg->AddLayer((new gFlagsLine(CPAP_LeakFlag, COLOR_LeakFlag, STR_TR_LE,false)));
    fg->AddLayer((new gFlagsLine(CPAP_NRI, COLOR_NRI, STR_TR_NRI,false)));
    fg->AddLayer((new gFlagsLine(CPAP_FlowLimit, COLOR_FlowLimit, STR_TR_FL)));
    fg->AddLayer((new gFlagsLine(CPAP_RERA, COLOR_RERA, STR_TR_RE)));
    fg->AddLayer((new gFlagsLine(CPAP_VSnore, COLOR_VibratorySnore, STR_TR_VS)));
    fg->AddLayer((new gFlagsLine(CPAP_VSnore2, COLOR_VibratorySnore, STR_TR_VS2)));
    if (PROFILE.cpap->userEventFlagging()) {
        fg->AddLayer((new gFlagsLine(CPAP_UserFlag1, COLOR_Yellow, STR_TR_UF1)));
        fg->AddLayer((new gFlagsLine(CPAP_UserFlag2, COLOR_DarkGreen, STR_TR_UF2)));
        fg->AddLayer((new gFlagsLine(CPAP_UserFlag3, COLOR_Brown, STR_TR_UF3)));
    }
    //fg->AddLayer((new gFlagsLine(PRS1_0B,COLOR_DarkGreen,tr("U0B"))));
    SF->setBlockZoom(true);
    SF->AddLayer(new gShadowArea());
    SF->AddLayer(new gYSpacer(),LayerLeft,gYAxis::Margin);
    //SF->AddLayer(new gFooBar(),LayerBottom,0,1);
    SF->AddLayer(new gXAxis(COLOR_Text,false),LayerBottom,0,20); //gXAxis::Margin);


    gLineChart *l;
    l=new gLineChart(CPAP_FlowRate,COLOR_Black,false,false);
    gLineOverlaySummary *los=new gLineOverlaySummary(tr("Selection AHI"),5,-4);
    AddCPAP(l);
    FRW->AddLayer(new gXGrid());
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_CSR, COLOR_CSR, STR_TR_CSR,FT_Span)));
    FRW->AddLayer(l);
    FRW->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    FRW->AddLayer(new gXAxis(),LayerBottom,0,20);
    FRW->AddLayer(AddCPAP(los->add(new gLineOverlayBar(CPAP_Hypopnea,COLOR_Hypopnea,STR_TR_H))));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_PressurePulse,COLOR_PressurePulse,STR_TR_PP,FT_Dot)));
    //FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Pressure, COLOR_White,STR_TR_P,FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_0B,COLOR_Blue,"0B",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_10,COLOR_Orange,"10",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_0E,COLOR_DarkRed,"0E",FT_Dot)));
    if (PROFILE.general->calculateRDI())
        FRW->AddLayer(AddCPAP(los->add(new gLineOverlayBar(CPAP_RERA, COLOR_RERA, STR_TR_RE))));
    else
        FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_RERA, COLOR_RERA, STR_TR_RE)));

    FRW->AddLayer(AddCPAP(los->add(new gLineOverlayBar(CPAP_Apnea, COLOR_Apnea, STR_TR_UA))));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_VSnore, COLOR_VibratorySnore, STR_TR_VS)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_FlowLimit, COLOR_FlowLimit, STR_TR_FL)));
    FRW->AddLayer(AddCPAP(los->add(new gLineOverlayBar(CPAP_Obstructive, COLOR_Obstructive, STR_TR_OA))));
    FRW->AddLayer(AddCPAP(los->add(new gLineOverlayBar(CPAP_ClearAirway, COLOR_ClearAirway, STR_TR_CA))));
    if (PROFILE.cpap->userEventFlagging()) {
        FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_UserFlag1, COLOR_Yellow, tr("U1"),FT_Bar)));
        FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_UserFlag2, COLOR_Orange, tr("U2"),FT_Bar)));
        FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_UserFlag3, COLOR_Brown, tr("U3"),FT_Bar)));
    }
    FRW->AddLayer(AddOXI(new gLineOverlayBar(OXI_SPO2Drop, COLOR_SPO2Drop, STR_TR_O2)));
    //FRW->AddLayer(AddOXI(new gLineOverlayBar(OXI_PulseChange, COLOR_PulseChange, STR_TR_PC,FT_Dot)));

    FRW->AddLayer(AddCPAP(los));


    gGraph *graphs[]={ PRD, LEAK, AHI, SNORE, PTB, MP, RR, MV, TV, FLG, IE, TI, TE, SPO2, PLETHY, PULSE, STAGE };
    int ng=sizeof(graphs)/sizeof(gGraph*);
    for (int i=0;i<ng;i++){
        graphs[i]->AddLayer(new gXGrid());
    }
    /*PRD->AddLayer(AddCPAP(new gStatsLine(CPAP_Pressure,"Pressure")),LayerBottom,0,20,1);
    PRD->AddLayer(AddCPAP(new gStatsLine(CPAP_EPAP,"EPAP")),LayerBottom,0,20,1);
    PRD->AddLayer(AddCPAP(new gStatsLine(CPAP_IPAP,"IPAP")),LayerBottom,0,20,1);
    LEAK->AddLayer(AddCPAP(new gStatsLine(CPAP_Leak)),LayerBottom,0,20,1);
    SNORE->AddLayer(AddCPAP(new gStatsLine(CPAP_Snore)),LayerBottom,0,20,1);
    PTB->AddLayer(AddCPAP(new gStatsLine(CPAP_PatientTriggeredBreaths)),LayerBottom,0,20,1);
    RR->AddLayer(AddCPAP(new gStatsLine(CPAP_RespiratoryRate)),LayerBottom,0,20,1);
    MV->AddLayer(AddCPAP(new gStatsLine(CPAP_MinuteVentilation)),LayerBottom,0,20,1);
    TV->AddLayer(AddCPAP(new gStatsLine(CPAP_TidalVolume)),LayerBottom,0,20,1);
    FLG->AddLayer(AddCPAP(new gStatsLine(CPAP_FlowLimitGraph)),LayerBottom,0,20,1);
    IE->AddLayer(AddCPAP(new gStatsLine(CPAP_IE)),LayerBottom,0,20,1);
    TE->AddLayer(AddCPAP(new gStatsLine(CPAP_Te)),LayerBottom,0,20,1);
    TI->AddLayer(AddCPAP(new gStatsLine(CPAP_Ti)),LayerBottom,0,20,1); */


    bool square=PROFILE.appearance->squareWavePlots();
    gLineChart *pc=new gLineChart(CPAP_Pressure, COLOR_Pressure, square);
    PRD->AddLayer(AddCPAP(pc));
    pc->addPlot(CPAP_EPAP, COLOR_EPAP, square);
    pc->addPlot(CPAP_IPAPLo, COLOR_IPAPLo, square);
    pc->addPlot(CPAP_IPAP, COLOR_IPAP, square);
    pc->addPlot(CPAP_IPAPHi, COLOR_IPAPHi, square);

    if (PROFILE.general->calculateRDI()) {
        AHI->AddLayer(AddCPAP(new gLineChart(CPAP_RDI, COLOR_RDI, square)));
//        AHI->AddLayer(AddCPAP(new AHIChart(QColor("#37a24b"))));
    } else {
        AHI->AddLayer(AddCPAP(new gLineChart(CPAP_AHI, COLOR_AHI, square)));
    }

    gLineChart *lc=new gLineChart(CPAP_LeakTotal, COLOR_LeakTotal, square);
    lc->addPlot(CPAP_Leak, COLOR_Leak, square);
    lc->addPlot(CPAP_MaxLeak, COLOR_MaxLeak, square);
    LEAK->AddLayer(AddCPAP(lc));
    //LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_Leak, COLOR_Leak,square)));
    //LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_MaxLeak, COLOR_MaxLeak,square)));
    SNORE->AddLayer(AddCPAP(new gLineChart(CPAP_Snore, COLOR_Snore, true)));

    PTB->AddLayer(AddCPAP(new gLineChart(CPAP_PTB, COLOR_PTB, square)));
    MP->AddLayer(AddCPAP(new gLineChart(CPAP_MaskPressure, COLOR_MaskPressure, false)));
    RR->AddLayer(AddCPAP(lc=new gLineChart(CPAP_RespRate, COLOR_RespRate, square)));

    // Delete me!!
//    lc->addPlot(CPAP_Test1, COLOR_DarkRed,square);

    MV->AddLayer(AddCPAP(lc=new gLineChart(CPAP_MinuteVent, COLOR_MinuteVent, square)));
    lc->addPlot(CPAP_TgMV,COLOR_TgMV,square);

    TV->AddLayer(AddCPAP(lc=new gLineChart(CPAP_TidalVolume,COLOR_TidalVolume,square)));
    //lc->addPlot(CPAP_Test2,COLOR_DarkYellow,square);



    //TV->AddLayer(AddCPAP(new gLineChart("TidalVolume2",COLOR_Magenta,square)));
    FLG->AddLayer(AddCPAP(new gLineChart(CPAP_FLG, COLOR_FLG, true)));
    //RE->AddLayer(AddCPAP(new gLineChart(CPAP_RespiratoryEvent,COLOR_Magenta,true)));
    IE->AddLayer(AddCPAP(lc=new gLineChart(CPAP_IE, COLOR_IE, square)));
    TE->AddLayer(AddCPAP(lc=new gLineChart(CPAP_Te, COLOR_Te, square)));
    TI->AddLayer(AddCPAP(lc=new gLineChart(CPAP_Ti, COLOR_Ti, square)));
    //lc->addPlot(CPAP_Test2,COLOR:DarkYellow,square);
    //INTPULSE->AddLayer(AddCPAP(new gLineChart(OXI_Pulse, COLOR_Pulse, square)));
    //INTSPO2->AddLayer(AddCPAP(new gLineChart(OXI_SPO2, COLOR_SPO2, square)));

    STAGE->AddLayer(AddSTAGE(new gLineChart(ZEO_SleepStage, COLOR_SleepStage, true)));

    gLineOverlaySummary *los1=new gLineOverlaySummary(tr("Events/hour"),5,-4);
    gLineOverlaySummary *los2=new gLineOverlaySummary(tr("Events/hour"),5,-4);
    PULSE->AddLayer(AddOXI(los1->add(new gLineOverlayBar(OXI_PulseChange, COLOR_PulseChange, STR_TR_PC,FT_Span))));
    PULSE->AddLayer(AddOXI(los1));
    SPO2->AddLayer(AddOXI(los2->add(new gLineOverlayBar(OXI_SPO2Drop, COLOR_SPO2Drop, STR_TR_O2,FT_Span))));
    SPO2->AddLayer(AddOXI(los2));

    PULSE->AddLayer(AddOXI(new gLineChart(OXI_Pulse, COLOR_Pulse, square)));
    SPO2->AddLayer(AddOXI(new gLineChart(OXI_SPO2, COLOR_SPO2, true)));
    PLETHY->AddLayer(AddOXI(new gLineChart(OXI_Plethy, COLOR_Plethy,false)));

    PTB->setForceMaxY(100);
    SPO2->setForceMaxY(100);

    for (int i=0;i<ng;i++){
        graphs[i]->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
        graphs[i]->AddLayer(new gXAxis(),LayerBottom,0,20);
    }

    layout->layout();

    QTextCharFormat format = ui->calendar->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(COLOR_Black, Qt::SolidPattern));
    ui->calendar->setWeekdayTextFormat(Qt::Saturday, format);
    ui->calendar->setWeekdayTextFormat(Qt::Sunday, format);

    Qt::DayOfWeek dow=firstDayOfWeekFromLocale();

    ui->calendar->setFirstDayOfWeek(dow);

    ui->tabWidget->setCurrentWidget(ui->details);

    ui->webView->settings()->setFontSize(QWebSettings::DefaultFontSize,QApplication::font().pointSize());
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(ui->webView,SIGNAL(linkClicked(QUrl)),this,SLOT(Link_clicked(QUrl)));

    int ews=PROFILE.general->eventWindowSize();
    ui->evViewSlider->setValue(ews);
    ui->evViewLCD->display(ews);

    GraphView->LoadSettings("Daily");
    icon_on=new QIcon(":/icons/session-on.png");
    icon_off=new QIcon(":/icons/session-off.png");

    ui->splitter->setVisible(false);

    if (PROFILE.general->unitSystem()==US_Archiac) {
        ui->weightSpinBox->setSuffix(STR_UNIT_POUND);
        ui->weightSpinBox->setDecimals(0);
        ui->ouncesSpinBox->setVisible(true);
        ui->ouncesSpinBox->setSuffix(STR_UNIT_OUNCE);
    } else {
        ui->ouncesSpinBox->setVisible(false);
        ui->weightSpinBox->setDecimals(3);
        ui->weightSpinBox->setSuffix(STR_UNIT_KG);
    }
    GraphView->setCubeImage(images["nodata"]);
    GraphView->setEmptyText(STR_TR_NoData);
    previous_date=QDate();
}

Daily::~Daily()
{
    GraphView->SaveSettings("Daily");

    disconnect(ui->webView,SIGNAL(linkClicked(QUrl)),this,SLOT(Link_clicked(QUrl)));
    // Save any last minute changes..
    if (previous_date.isValid())
        Unload(previous_date);

//    delete splitter;
    delete ui;
    delete icon_on;
    delete icon_off;
}
void Daily::Link_clicked(const QUrl &url)
{
    QString code=url.toString().section("=",0,0).toLower();
    QString data=url.toString().section("=",1);
    int sid=data.toInt();
    Day *day=NULL;
    if (code=="togglecpapsession") { // Enable/Disable CPAP session
        day=PROFILE.GetDay(previous_date,MT_CPAP);
        Session *sess=day->find(sid);
        if (!sess)
            return;
        int i=ui->webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)-ui->webView->page()->mainFrame()->scrollBarValue(Qt::Vertical);
        sess->setEnabled(!sess->enabled());

        // Messy, this rewrites both summary & events.. TODO: Write just the session summary file
        day->machine->Save();

        // Reload day
        this->LoadDate(previous_date);
        ui->webView->page()->mainFrame()->setScrollBarValue(Qt::Vertical, ui->webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)-i);
        return;
    } else  if (code=="toggleoxisession") { // Enable/Disable Oximetry session
        day=PROFILE.GetDay(previous_date,MT_OXIMETER);
        Session *sess=day->find(sid);
        if (!sess)
            return;
        int i=ui->webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)-ui->webView->page()->mainFrame()->scrollBarValue(Qt::Vertical);
        sess->setEnabled(!sess->enabled());
        // Messy, this rewrites both summary & events.. TODO: Write just the session summary file
        day->machine->Save();

        // Reload day
        this->LoadDate(previous_date);
        ui->webView->page()->mainFrame()->setScrollBarValue(Qt::Vertical, ui->webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)-i);
        return;
    } else if (code=="cpap")  {
        day=PROFILE.GetDay(previous_date,MT_CPAP);
    } else if (code=="oxi") {
        day=PROFILE.GetDay(previous_date,MT_OXIMETER);
        Session *sess=day->machine->sessionlist[sid];
        if (mainwin->getOximetry()) {
            mainwin->getOximetry()->openSession(sess);
            mainwin->selectOximetryTab();
        }
        return;
    } else if (code=="event")  {
        QList<QTreeWidgetItem *> list=ui->treeWidget->findItems(schema::channel[sid].description(),Qt::MatchContains);
        if (list.size()>0) {
            ui->treeWidget->collapseAll();
            ui->treeWidget->expandItem(list.at(0));
            QTreeWidgetItem *wi=list.at(0)->child(0);
            ui->treeWidget->setCurrentItem(wi);
            ui->tabWidget->setCurrentIndex(1);
        } else {
            mainwin->Notify(tr("No %1 events are recorded this day").arg(schema::channel[sid].description()),"",1500);
        }
    } else if (code=="graph") {
        qDebug() << "Select graph " << data;
    } else {
        qDebug() << "Clicked on" << code << data;
    }
    if (day) {

        Session *sess=day->machine->sessionlist[sid];
        if (sess && sess->enabled()) {
            GraphView->SetXBounds(sess->first(),sess->last());
        }
    }
}

void Daily::ReloadGraphs()
{
    ui->splitter->setVisible(true);
    QDate d;

    if (previous_date.isValid()) {
        d=previous_date;
//        Unload(d);
    }
    d=PROFILE.LastDay();
    if (!d.isValid()) {
        d=ui->calendar->selectedDate();
    }
    on_calendar_currentPageChanged(d.year(),d.month());
    // this fires a signal which unloads the old and loads the new
    ui->calendar->setSelectedDate(d);
    //Load(d);
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

    tree->setColumnCount(1); // 1 visible common.. (1 hidden)

    QTreeWidgetItem *root=NULL;
    QHash<ChannelID,QTreeWidgetItem *> mcroot;
    QHash<ChannelID,int> mccnt;
    int total_events=0;
    bool userflags=p_profile->cpap->userEventFlagging();

    qint64 drift=0, clockdrift=PROFILE.cpap->clockDrift()*1000L;
    for (QVector<Session *>::iterator s=day->begin();s!=day->end();s++) {
        if (!(*s)->enabled()) continue;

        QHash<ChannelID,QVector<EventList *> >::iterator m;

        for (m=(*s)->eventlist.begin();m!=(*s)->eventlist.end();m++) {
            ChannelID code=m.key();
            if ((code!=CPAP_Obstructive)
                && (code!=CPAP_Hypopnea)
                && (code!=CPAP_Apnea)
                && (code!=PRS1_0B)
                && (code!=CPAP_ClearAirway)
                && (code!=CPAP_CSR)
                && (code!=CPAP_RERA)
                && (code!=CPAP_UserFlag1)
                && (code!=CPAP_UserFlag2)
                && (code!=CPAP_UserFlag3)
                && (code!=CPAP_NRI)
                && (code!=CPAP_LeakFlag)
                && (code!=CPAP_ExP)
                && (code!=CPAP_FlowLimit)
                && (code!=CPAP_PressurePulse)
                && (code!=CPAP_VSnore2)
                && (code!=CPAP_VSnore)) continue;

            if (!userflags && ((code==CPAP_UserFlag1) || (code==CPAP_UserFlag2) || (code==CPAP_UserFlag3))) continue;
            drift=(*s)->machine()->GetType()==MT_CPAP ? clockdrift : 0;

            QTreeWidgetItem *mcr;
            if (mcroot.find(code)==mcroot.end()) {
                int cnt=day->count(code);
                if (!cnt) continue; // If no events than don't bother showing..
                total_events+=cnt;
                QString st=schema::channel[code].description();
                if (st.isEmpty())  {
                    st="Fixme %1"+code;
                }
                st+=" ";
                if (cnt==1) st+=tr("%1 event").arg(cnt);
                else st+=tr("%1 events").arg(cnt);

                QStringList l(st);
                l.append("");
                mcroot[code]=mcr=new QTreeWidgetItem(root,l);
                mccnt[code]=0;
            } else {
                mcr=mcroot[code];
            }

            for (int z=0;z<m.value().size();z++) {
                EventList & ev=*(m.value()[z]);

                for (quint32 o=0;o<ev.count();o++) {
                    qint64 t=ev.time(o)+drift;

                    if (code==CPAP_CSR) { // center it in the middle of span
                        t-=float(ev.raw(o)/2.0)*1000.0;
                    }
                    QStringList a;
                    QDateTime d=QDateTime::fromTime_t(t/1000L);
                    QString s=QString("#%1: %2 (%3)").arg((int)(++mccnt[code]),(int)3,(int)10,QChar('0')).arg(d.toString("HH:mm:ss")).arg(m.value()[z]->raw(o));
                    a.append(s);
                    QTreeWidgetItem *item=new QTreeWidgetItem(a);
                    item->setData(0,Qt::UserRole,t);
                    //a.append(d.toString("yyyy-MM-dd HH:mm:ss"));
                    mcr->addChild(item);
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
    QTextCharFormat nodata;
    QTextCharFormat cpaponly;
    QTextCharFormat cpapjour;
    QTextCharFormat oxiday;
    QTextCharFormat oxicpap;
    QTextCharFormat jourday;
    QTextCharFormat stageday;

    cpaponly.setForeground(QBrush(COLOR_Blue, Qt::SolidPattern));
    cpaponly.setFontWeight(QFont::Normal);
    cpapjour.setForeground(QBrush(COLOR_Blue, Qt::SolidPattern));
    cpapjour.setFontWeight(QFont::Bold);
    oxiday.setForeground(QBrush(COLOR_Red, Qt::SolidPattern));
    oxiday.setFontWeight(QFont::Normal);
    oxicpap.setForeground(QBrush(COLOR_Red, Qt::SolidPattern));
    oxicpap.setFontWeight(QFont::Bold);
    stageday.setForeground(QBrush(COLOR_Magenta, Qt::SolidPattern));
    stageday.setFontWeight(QFont::Bold);
    jourday.setForeground(QBrush(COLOR_Black, Qt::SolidPattern));
    jourday.setFontWeight(QFont::Bold);
    nodata.setForeground(QBrush(COLOR_Black, Qt::SolidPattern));
    nodata.setFontWeight(QFont::Normal);

    bool hascpap=PROFILE.GetDay(date,MT_CPAP)!=NULL;
    bool hasoxi=PROFILE.GetDay(date,MT_OXIMETER)!=NULL;
    bool hasjournal=PROFILE.GetDay(date,MT_JOURNAL)!=NULL;
    bool hasstage=PROFILE.GetDay(date,MT_SLEEPSTAGE)!=NULL;
    if (hascpap) {
        if (hasoxi) {
            ui->calendar->setDateTextFormat(date,oxicpap);
        } else if (hasjournal) {
            ui->calendar->setDateTextFormat(date,cpapjour);
        } else if (hasstage) {
            ui->calendar->setDateTextFormat(date,stageday);
        } else {
            ui->calendar->setDateTextFormat(date,cpaponly);
        }
    } else if (hasoxi) {
        ui->calendar->setDateTextFormat(date,oxiday);
    } else if (hasjournal) {
        ui->calendar->setDateTextFormat(date,jourday);
    } else {
        ui->calendar->setDateTextFormat(date,nodata);
    }
    ui->calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);

}
void Daily::LoadDate(QDate date)
{
    ui->calendar->blockSignals(true);
    if (date.month()!=previous_date.month()) {
        on_calendar_currentPageChanged(date.year(),date.month());
    }
    ui->calendar->setSelectedDate(date);
    ui->calendar->blockSignals(false);
    on_calendar_selectionChanged();
}

void Daily::on_calendar_selectionChanged()
{
    QTime time;
    time_t unload_time, load_time, other_time;
    time.start();

    this->setCursor(Qt::BusyCursor);
    if (previous_date.isValid()) {
       // GraphView->fadeOut();
        Unload(previous_date);
    }
    unload_time=time.restart();
    //bool fadedir=previous_date < ui->calendar->selectedDate();
    ZombieMeterMoved=false;
    Load(ui->calendar->selectedDate());
    load_time=time.restart();

    //GraphView->fadeIn(fadedir);
    GraphView->redraw();
    ui->calButton->setText(ui->calendar->selectedDate().toString(Qt::TextDate));
    ui->calendar->setFocus(Qt::ActiveWindowFocusReason);

    if (PROFILE.general->unitSystem()==US_Archiac) {
        ui->weightSpinBox->setSuffix(STR_UNIT_POUND);
        ui->weightSpinBox->setDecimals(0);
        ui->ouncesSpinBox->setVisible(true);
        ui->ouncesSpinBox->setSuffix(STR_UNIT_OUNCE);
    } else {
        ui->ouncesSpinBox->setVisible(false);
        ui->weightSpinBox->setDecimals(3);
        ui->weightSpinBox->setSuffix(STR_UNIT_KG);
    }
    this->setCursor(Qt::ArrowCursor);
    other_time=time.restart();

    qDebug() << "Page change time (in ms): Unload ="<<unload_time<<"Load =" << load_time << "Other =" << other_time;
}
void Daily::ResetGraphLayout()
{
    GraphView->resetLayout();
}
void Daily::graphtogglebutton_toggled(bool b)
{
    Q_UNUSED(b)
    for (int i=0;i<GraphView->size();i++) {
        QString title=(*GraphView)[i]->title();
        (*GraphView)[i]->setVisible(GraphToggles[title]->isChecked());
    }
    GraphView->updateScale();
    GraphView->redraw();
}
void Daily::Load(QDate date)
{
    static Day * lastcpapday=NULL;
    previous_date=date;
    Day *cpap=PROFILE.GetDay(date,MT_CPAP);
    Day *oxi=PROFILE.GetDay(date,MT_OXIMETER);
    Day *stage=PROFILE.GetDay(date,MT_SLEEPSTAGE);

    if (!PROFILE.session->cacheSessions()) {
        if (lastcpapday && (lastcpapday!=cpap)) {
            for (QVector<Session *>::iterator s=lastcpapday->begin();s!=lastcpapday->end();s++) {
                (*s)->TrashEvents();
            }
        }
    }

    if ((cpap && oxi) && oxi->hasEnabledSessions()) {
        int gr;

        if (qAbs(cpap->first() - oxi->first())>30000) {
            mainwin->Notify(tr("Oximetry data exists for this day, however it's timestamps are too different, so the Graphs will not be linked."),"",3000);
            gr=1;
        } else
            gr=0;

        GraphView->findGraph(STR_TR_PulseRate)->setGroup(gr);
        GraphView->findGraph(STR_TR_SpO2)->setGroup(gr);
        GraphView->findGraph(STR_TR_Plethy)->setGroup(gr);
    }
    lastcpapday=cpap;

    QString html="<html><head><style type='text/css'>"
    "p,a,td,body { font-family: '"+QApplication::font().family()+"'; }"
    "p,a,td,body { font-size: "+QString::number(QApplication::font().pointSize() + 2)+"px; }"
    "</style>"
    "<link rel='stylesheet' type='text/css' href='qrc:/docs/tooltips.css' />"
    "<script language='javascript'><!--"
            "func dosession(sessid) {"
            ""
            "}"
    "--></script>"
    "</head>"
    "<body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>"
    "<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";
    QString tmp;

    UpdateOXIGraphs(oxi);
    UpdateCPAPGraphs(cpap);
    UpdateSTAGEGraphs(stage);
    UpdateEventsTree(ui->treeWidget,cpap);

    mainwin->refreshStatistics();

    snapGV->setDay(cpap);

    GraphView->ResetBounds(false);

    if (!cpap && !oxi) {
        scrollbar->hide();
    } else {
        scrollbar->show();
    }

    QString modestr;
    CPAPMode mode=MODE_UNKNOWN;
    QString a;
    bool isBrick=false;

    updateGraphCombo();
    int mididx=PROFILE.general->prefCalcMiddle();
    SummaryType ST_mid;
    if (mididx==0) ST_mid=ST_PERC;
    if (mididx==1) ST_mid=ST_WAVG;
    if (mididx==2) ST_mid=ST_AVG;

    if (cpap) {
        float hours=cpap->hours();
        if (GraphView->isEmpty() && (hours>0)) {
            if (!PROFILE.hasChannel(CPAP_Obstructive) && !PROFILE.hasChannel(CPAP_Hypopnea)) {
                GraphView->setCubeImage(images["brick"]);
                GraphView->setEmptyText(tr("No Graphs :("));

                isBrick=true;
            }
        }

        mode=(CPAPMode)(int)cpap->settings_max(CPAP_Mode);

        modestr=schema::channel[CPAP_Mode].m_options[mode];

        float ahi=(cpap->count(CPAP_Obstructive)+cpap->count(CPAP_Hypopnea)+cpap->count(CPAP_ClearAirway)+cpap->count(CPAP_Apnea));
        if (PROFILE.general->calculateRDI()) ahi+=cpap->count(CPAP_RERA);
        ahi/=hours;
        float csr=(100.0/cpap->hours())*(cpap->sum(CPAP_CSR)/3600.0);
        float uai=cpap->count(CPAP_Apnea)/hours;
        float oai=cpap->count(CPAP_Obstructive)/hours;
        float hi=(cpap->count(CPAP_ExP)+cpap->count(CPAP_Hypopnea))/hours;
        float cai=cpap->count(CPAP_ClearAirway)/hours;
        float rei=cpap->count(CPAP_RERA)/hours;
        float fli=cpap->count(CPAP_FlowLimit)/hours;
        float nri=cpap->count(CPAP_NRI)/hours;
        float lki=cpap->count(CPAP_LeakFlag)/hours;
        float exp=cpap->count(CPAP_ExP)/hours;

        //float p90=cpap->p90(CPAP_Pressure);
        //eap90=cpap->p90(CPAP_EPAP);
        //iap90=cpap->p90(CPAP_IPAP);
        QString submodel; //=tr("Unknown Model");

        //html+="<tr><td colspan=4 align=center><i>"+tr("Machine Information")+"</i></td></tr>\n";
        if (cpap->machine->properties.find(STR_PROP_SubModel)!=cpap->machine->properties.end())
            submodel=" <br/>"+cpap->machine->properties[STR_PROP_SubModel];
        html+="<tr><td colspan=4 align=center><b>"+cpap->machine->properties[STR_PROP_Brand]+"</b> <br>"+cpap->machine->properties[STR_PROP_Model]+" "+cpap->machine->properties[STR_PROP_ModelNumber]+submodel+"</td></tr>\n";
        if (PROFILE.general->showSerialNumbers()) {
            html+="<tr><td colspan=4 align=center>"+cpap->machine->properties[STR_PROP_Serial]+"</td></tr>\n";
        }
        CPAPMode mode=(CPAPMode)(int)cpap->settings_max(CPAP_Mode);
        html+="<tr><td colspan=4 align=center>Mode: ";

        if (mode==MODE_CPAP) {
            EventDataType min=round(cpap->settings_wavg(CPAP_Pressure)*2)/2.0;
            html+=STR_TR_CPAP+" "+QString::number(min)+STR_UNIT_CMH2O;
        } else if (mode==MODE_APAP) {
            EventDataType min=cpap->settings_min(CPAP_PressureMin);
            EventDataType max=cpap->settings_max(CPAP_PressureMax);
            html+=STR_TR_APAP+" "+QString::number(min)+"-"+QString::number(max)+STR_UNIT_CMH2O;
        } else if (mode==MODE_BIPAP) {
            EventDataType epap=cpap->settings_min(CPAP_EPAP);
            EventDataType ipap=cpap->settings_max(CPAP_IPAP);
            EventDataType ps=cpap->settings_max(CPAP_PS);
            html+=STR_TR_BiLevel+QString("<br/>"+STR_TR_EPAP+": %1 "+STR_TR_IPAP+": %2 %3<br/> "+STR_TR_PS+": %4")
                    .arg(epap,0,'f',1).arg(ipap,0,'f',1).arg(STR_UNIT_CMH2O).arg(ps,0,'f',1);
        }
        else if (mode==MODE_ASV) {
            EventDataType epap=cpap->settings_min(CPAP_EPAP);
            EventDataType low=cpap->settings_min(CPAP_IPAPLo);
            EventDataType high=cpap->settings_max(CPAP_IPAPHi);
            EventDataType psl=cpap->settings_min(CPAP_PSMin);
            EventDataType psh=cpap->settings_max(CPAP_PSMax);
            html+=tr("ASV")+QString("<br/>"+STR_TR_EPAP+": %1 "+STR_TR_IPAP+": %2 - %3 %4<br/> "+STR_TR_PS+": %5 / %6")
                    .arg(epap,0,'f',1)
                    .arg(low,0,'f',1)
                    .arg(high,0,'f',1)
                    .arg(STR_UNIT_CMH2O)
                    .arg(psl,0,'f',1)
                    .arg(psh,0,'f',1);
        }
        else html+=STR_TR_Unknown;
        html+="</td></tr>\n";


        if (hours>0) {
            html+="<tr><td align='center'><b>"+STR_TR_Date+"</b></td><td align='center'><b>"+tr("Sleep")+"</b></td><td align='center'><b>"+tr("Wake")+"</b></td><td align='center'><b>"+STR_UNIT_Hours+"</b></td></tr>";
            int tt=qint64(cpap->total_time())/1000L;
            QDateTime date=QDateTime::fromTime_t(cpap->first()/1000L);
            QDateTime date2=QDateTime::fromTime_t(cpap->last()/1000L);

            int h=tt/3600;
            int m=(tt/60)%60;
            int s=tt % 60;
            html+=QString("<tr><td align='center'>%1</td><td align='center'>%2</td><td align='center'>%3</td><td align='center'>%4</td></tr>\n"
                "<tr><td colspan=4 align=center><hr></td></tr>\n")
                    .arg(date.date().toString(Qt::SystemLocaleShortDate))
                    .arg(date.toString("HH:mm"))
                    .arg(date2.toString("HH:mm"))
                    .arg(QString().sprintf("%02i:%02i:%02i",h,m,s));
        }

        QString cs;

        if (!isBrick && hours>0) {
            if (PROFILE.general->calculateRDI()) {
                html+=QString("<tr><td bgcolor='%1' align=center colspan=4><font size=+2 color='%2'><a class=info2 href='#'><font size=+2><b>%3</b></font><span>%4</span></a> <b>%5</b></font></td></tr>\n")
                        .arg("#F88017").arg(COLOR_Text.name()).arg(STR_TR_RDI).arg(schema::channel[CPAP_RDI].description()).arg(ahi,0,'f',2);
            } else {
                html+=QString("<tr><td bgcolor='%1' align=center colspan=4><font size=+2 color='%2'><a class=info2 href='#'><font size=+2><b>%3</b></font><span>%4</span></a> <b>%5</b></font></td></tr>\n")
                        .arg("#F88017").arg(COLOR_Text.name()).arg(STR_TR_AHI).arg(schema::channel[CPAP_AHI].description()).arg(ahi,0,'f',2);
            }

            if (cpap->machine->GetClass()==STR_MACH_ResMed || cpap->machine->GetClass()==STR_MACH_FPIcon) {
                cs="4 width='70%' align=center>";
            } else cs="2 width='50%'>";
            html+="<tr><td valign=top colspan="+cs+"<table cellspacing=0 cellpadding=1 border=0 width='100%'>";

            html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                    .arg(COLOR_Hypopnea.name()).arg("white").arg(tr("Hypopnea")).arg(schema::channel[CPAP_Hypopnea].description()).arg(hi,0,'f',2).arg(CPAP_Hypopnea);
            if (cpap->machine->GetClass()==STR_MACH_ResMed) {
                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                        .arg(COLOR_Apnea.name()).arg(COLOR_Text.name())
                        .arg(tr("Apnea")).arg(schema::channel[CPAP_Apnea].description()).arg(uai,0,'f',2).arg(CPAP_Apnea);
            }
            html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                    .arg(COLOR_Obstructive.name()).arg(COLOR_Text.name()).arg(tr("Obstructive")).arg(schema::channel[CPAP_Obstructive].description()).arg(oai,0,'f',2).arg(CPAP_Obstructive);

            if (cpap->machine->GetClass()==STR_MACH_FPIcon) {
                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                    .arg(COLOR_FlowLimit.name()).arg("white").arg(tr("Flow Limit")).arg(schema::channel[CPAP_FlowLimit].description()).arg(fli,0,'f',2).arg(CPAP_FlowLimit);
            } else {
                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                    .arg(COLOR_ClearAirway.name()).arg(COLOR_Text.name()).arg(tr("Clear Airway")).arg(schema::channel[CPAP_ClearAirway].description()).arg(cai,0,'f',2).arg(CPAP_ClearAirway);
            }

            if (cpap->machine->GetClass()==STR_MACH_Intellipap) {
                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5%</font></b></td></tr>\n")
                  .arg(schema::channel[CPAP_NRI].defaultColor().name()).arg(COLOR_Text.name()).arg(STR_TR_NRI).arg(schema::channel[CPAP_NRI].description()).arg(nri,0,'f',2).arg(CPAP_NRI);
            }
            if (PROFILE.cpap->userEventFlagging()) {
                EventDataType uf1=cpap->count(CPAP_UserFlag1) / cpap->hours();
                if (uf1>0)
                    html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                        .arg(COLOR_UserFlag1.name()).arg(COLOR_Text.name())
                        .arg(tr("User Flags"))
                        .arg(schema::channel[CPAP_UserFlag1].description())
                        .arg(uf1,0,'f',2).arg(CPAP_UserFlag1);
            }

            html+="</table></td>";

            if (cpap->machine->GetClass()==STR_MACH_PRS1) {
                html+="<td colspan=2 valign=top><table cellspacing=0 cellpadding=1 border=0 width='100%'>";
                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info2 href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                    .arg(COLOR_RERA.name()).arg(COLOR_Text.name()).arg(STR_TR_RERA).arg(schema::channel[CPAP_RERA].description()).arg(rei,0,'f',2).arg(CPAP_RERA);
                if (mode>MODE_CPAP) {
                    html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info2 href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                        .arg(COLOR_FlowLimit.name()).arg("white").arg(tr("Flow Limit")).arg(schema::channel[CPAP_FlowLimit].description()).arg(fli,0,'f',2).arg(CPAP_FlowLimit);

                    html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info2 href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                        .arg(COLOR_VibratorySnore.name()).arg(COLOR_Text.name()).arg(tr("VSnore")).arg(schema::channel[CPAP_VSnore].description()).arg(cpap->count(CPAP_VSnore)/cpap->hours(),0,'f',2).arg(CPAP_VSnore);
                } else {
                    //html+="<tr bgcolor='#404040'><td colspan=2>&nbsp;</td></tr>";
                    html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info2 href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                        .arg(COLOR_VibratorySnore.name()).arg(COLOR_Text.name()).arg(tr("VSnore2")).arg(schema::channel[CPAP_VSnore2].description()).arg(cpap->count(CPAP_VSnore2)/cpap->hours(),0,'f',2).arg(CPAP_VSnore2);
                }
                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5%</font></b></td></tr>\n")
                        .arg(COLOR_CSR.name()).arg(COLOR_Text.name()).arg(tr("PB/CSR")).arg(schema::channel[CPAP_CSR].description()).arg(csr,0,'f',2).arg(CPAP_CSR);

                html+="</table></td>";
            } else if (cpap->machine->GetClass()==STR_MACH_Intellipap) {
                html+="<td colspan=2 valign=top><table cellspacing=0 cellpadding=1 border=0 width='100%'>";
                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info2 href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5%</font></b></td></tr>\n")
                .arg(COLOR_LeakFlag.name()).arg(COLOR_Text.name()).arg(STR_TR_Leak).arg(schema::channel[CPAP_LeakFlag].description()).arg(lki,0,'f',2).arg(CPAP_LeakFlag);

                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info2 href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
                    .arg(COLOR_VibratorySnore.name()).arg(COLOR_Text.name()).arg(tr("VSnore")).arg(schema::channel[CPAP_VSnore].description()).arg(cpap->count(CPAP_VSnore)/cpap->hours(),0,'f',2).arg(CPAP_VSnore);
                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info2 href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5%</font></b></td></tr>\n")
                  .arg(COLOR_ExP.name()).arg("white").arg(tr("Exh&nbsp;Puff")).arg(schema::channel[CPAP_ExP].description()).arg(exp,0,'f',2).arg(CPAP_ExP);

                html+="</table></td>";

            }
            html+="</tr>";

            // Note, this may not be a problem since Qt bug 13622 was discovered
            // as it only relates to text drawing, which the Pie chart does not do
            // ^^ Scratch that.. pie now includes text..

            if ((hours > 0) && PROFILE.appearance->graphSnapshots()) {  // AHI Pie Chart
                if ((oai+hi+cai+uai+rei+fli)>0) {
                    html+="<tr><td colspan=5 align=center>&nbsp;</td></tr>";
                    html+=QString("<tr><td colspan=4 align=center><b>%1</b></td></tr>").arg(tr("Event Breakdown"));
                    html+="<tr><td colspan=5 align=center><hr/></td></tr>";
                    GAHI->setShowTitle(false);

                    QPixmap pixmap=GAHI->renderPixmap(150,150,false);
                    if (!pixmap.isNull()) {
                        QByteArray byteArray;
                        QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
                        buffer.open(QIODevice::WriteOnly);
                        pixmap.save(&buffer, "PNG");
                        html += "<tr><td colspan=4 align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\"></td></tr>\n";
                    } else {
                        html += "<tr><td colspan=4 align=center>Unable to display Pie Chart on this system</td></tr>\n";
                    }
                } else {
                    html += "<tr><td colspan=4 align=center><img src=\"qrc:/docs/0.0.gif\"></td></tr>\n";
                }
            }


        } else { // machine is a brick
            if (!isBrick) {
                html+="<tr><td colspan='5'>&nbsp;</td></tr>\n";
                if (cpap->size()>0) {
                    html+="<tr><td colspan='5' align='center'><b><h2>"+tr("Sessions all off!")+"</h2></b></td></tr>";
                    html+="<tr><td colspan='5' align='center'><i>"+tr("Sessions exist for this day but are switched off.")+"</i></td></tr>\n";
                } else {
                    html+="<tr><td colspan='5' align='center'><b><h2>"+tr("Impossibly short session")+"</h2></b></td></tr>";
                    html+="<tr><td colspan='5' align='center'><i>"+tr("Zero hours??")+"</i></td></tr>\n";
                }
            } else {
                html+="<tr><td colspan='5' align='center'><b><h2>"+tr("BRICK :(")+"</h2></b></td></tr>";
                html+="<tr><td colspan='5' align='center'><i>"+tr("Sorry, your machine does not record data.")+"</i></td></tr>\n";
                html+="<tr><td colspan='5' align='center'><i>"+tr("Complain to your Equipment Provider!")+"</i></td></tr>\n";
            }
            html+="<tr><td colspan='5'>&nbsp;</td></tr>\n";
        }
        html+="</table>";

    } // if (!CPAP)
    html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";

    float percentile=PROFILE.general->prefCalcPercentile()/100.0;

    SummaryType ST_max=PROFILE.general->prefCalcMax() ? ST_MAX : ST_PERC;
    const EventDataType maxperc=0.995F;

    QString midname;
    if (ST_mid==ST_WAVG) midname=tr("Avg");
    else if (ST_mid==ST_AVG) midname=tr("Avg");
    else if (ST_mid==ST_PERC) midname=tr("Med");

    if ((cpap && !isBrick && (cpap->hours()>0)) || oxi) {
        html+="<tr height='2'><td colspan=5>&nbsp;</td></tr>\n";

        html+=QString("<tr><td colspan=5 align=center><b>%1</b></td></tr>\n").arg(tr("Statistics"));
        html+="<tr height='2'><td colspan=5><hr></td></tr>\n";
        html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td></tr>")
                .arg(STR_TR_Channel)
                .arg(STR_TR_Min)
                .arg(midname)
                .arg(tr("%1%").arg(percentile*100,0,'f',0))
                .arg(STR_TR_Max);
        ChannelID chans[]={
            CPAP_Pressure,CPAP_EPAP,CPAP_IPAP,CPAP_PS,CPAP_PTB,
            CPAP_MinuteVent, CPAP_RespRate, CPAP_RespEvent,CPAP_FLG,
            CPAP_Leak, CPAP_LeakTotal, CPAP_Snore,CPAP_IE,CPAP_Ti,CPAP_Te, CPAP_TgMV,
            CPAP_TidalVolume, OXI_Pulse, OXI_SPO2
        };
        int numchans=sizeof(chans)/sizeof(ChannelID);
        //int suboffset=0;
        int ccnt=0;
        EventDataType tmp,med,perc,mx,mn;
        for (int i=0;i<numchans;i++) {

            ChannelID code=chans[i];

            if (cpap && cpap->channelHasData(code)) {
                //if (code==CPAP_LeakTotal) suboffset=PROFILEIntentionalLeak"].toDouble(); else suboffset=0;
                QString tooltip=schema::channel[code].description();
                if (!schema::channel[code].units().isEmpty()) tooltip+=" ("+schema::channel[code].units()+")";
                if (ST_max==ST_MAX) {
                    mx=cpap->Max(code);
                } else {
                    mx=cpap->percentile(code,maxperc);
                }
                mn=cpap->Min(code);
                perc=cpap->percentile(code,percentile);

                if (ST_mid==ST_PERC) {
                    med=cpap->percentile(code,0.5);
                    tmp=cpap->wavg(code);
                    if (tmp>0 || mx==0) {
                        tooltip+=QString("<br/>"+STR_TR_WAvg+": %1").arg(tmp,0,'f',2);
                    }
                } else if (ST_mid==ST_WAVG) {
                    med=cpap->wavg(code);
                    tmp=cpap->percentile(code,0.5);
                    if (tmp>0 || mx==0) {
                        tooltip+=QString("<br/>"+STR_TR_Median+": %1").arg(tmp,0,'f',2);
                    }
                } else if (ST_mid==ST_AVG) {
                    med=cpap->avg(code);
                    tmp=cpap->percentile(code,0.5);
                    if (tmp>0 || mx==0) {
                        tooltip+=QString("<br/>"+STR_TR_Median+": %1").arg(tmp,0,'f',2);
                    }
                }

                html+=QString("<tr><td align=left class='info' onmouseover=\"style.color='blue';\" onmouseout=\"style.color='"+COLOR_Text.name()+"';\">%1<span>%6</span></td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                    //.arg(QString("<a class='info' href='graph=%1'>%3<span>%2</span></a>")  //<a class='tooltip' href='#'>"+STR_TR_RDI+"<span class='classic'>"+
                    //.arg(QString::number(code)).arg(tooltip).arg(schema::channel[code].label()))
                    .arg(schema::channel[code].label())
                    .arg(mn,0,'f',2)
                    .arg(med,0,'f',2)
                    .arg(perc,0,'f',2)
                    .arg(mx,0,'f',2)
                    .arg(tooltip);
                ccnt++;
            }
            if (oxi && oxi->channelHasData(code)) {
                QString tooltip=schema::channel[code].description();
                if (!schema::channel[code].units().isEmpty()) tooltip+=" ("+schema::channel[code].units()+")";
                //wavg=oxi->wavg(code);
                mx=oxi->Max(code);
                mn=oxi->Min(code);
                perc=oxi->percentile(code,percentile);
                //med=oxi->percentile(code,0.5);

                if (ST_mid==ST_PERC) {
                    med=oxi->percentile(code,0.5);
                    tmp=oxi->wavg(code);
                    if (tmp>0 || mx==0) {
                        tooltip+=QString("<br/>"+STR_TR_WAvg+": %1").arg(tmp,0,'f',2);
                    }
                } else if (ST_mid==ST_WAVG) {
                    med=oxi->wavg(code);
                    tmp=oxi->percentile(code,0.5);
                    if (tmp>0 || mx==0) {
                        tooltip+=QString("<br/>"+STR_TR_Median+": %1").arg(tmp,0,'f',2);
                    }
                } else if (ST_mid==ST_AVG) {
                    med=oxi->avg(code);
                    tmp=oxi->percentile(code,0.5);
                    if (tmp>0 || mx==0) {
                        tooltip+=QString("<br/>"+STR_TR_Median+": %1").arg(tmp,0,'f',2);
                    }
                }


//                if ((med>0 && wavg>0) || (med==0)) {
//                    tooltip+=QString("<br/>Avg: %1").arg(wavg,0,'f',2);
//                }

                html+=QString("<tr><td align=left>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                    .arg(QString("<a class=info href='graph=%1'>%2<span>%3</span></a>")
                        .arg(QString::number(code)).arg(schema::channel[code].label()).arg(tooltip))
                    .arg(mn,0,'f',2)
                    .arg(med,0,'f',2)
                    .arg(perc,0,'f',2)
                    .arg(mx,0,'f',2);
                ccnt++;
            }
        }
        if (GraphView->isEmpty() && (ccnt>0)) {
            html+="<tr><td colspan=5>&nbsp;</td></tr>\n";
            html+=QString("<tr><td colspan=5 align=center><i>%1</i></td></tr>").arg(tr("<b>Please Note:</b> This day just contains summary data, only limited information is available ."));
        }
    } else {
        if (cpap && cpap->hours()==0) {
        } else {
            html+="<tr><td colspan=5 align=center><i>"+tr("No data available")+"</i></td></tr>";
            html+="<tr><td colspan=5>&nbsp;</td></tr>\n";
        }

    }
    if (oxi && oxi->hasEnabledSessions()) {
        html+="<tr><td colspan=5 align=center>&nbsp;</td></tr>";
        html+=QString("<tr><td colspan=5 align=center><b>%1</b></td></tr>\n").arg(tr("Oximeter Information"));
        html+="<tr><td colspan=5 align=center><hr/></td></tr>";
        html+="<tr><td colspan=5 align=center>"+oxi->machine->properties[STR_PROP_Brand]+" "+oxi->machine->properties[STR_PROP_Model]+"</td></tr>\n";
        html+="<tr><td colspan=5 align=center>&nbsp;</td></tr>";
        html+=QString("<tr><td colspan=5 align=center>%1: %2 (%3)\%</td></tr>").arg(tr("SpO2 Desaturations")).arg(oxi->count(OXI_SPO2Drop)).arg((100.0/oxi->hours()) * (oxi->sum(OXI_SPO2Drop)/3600.0),0,'f',2);
        html+=QString("<tr><td colspan=5 align=center>%1: %2 (%3)\%</td></tr>").arg(tr("Pulse Change events")).arg(oxi->count(OXI_PulseChange)).arg((100.0/oxi->hours()) * (oxi->sum(OXI_PulseChange)/3600.0),0,'f',2);
        html+=QString("<tr><td colspan=5 align=center>%1: %2\%</td></tr>").arg(tr("SpO2 Baseline Used")).arg(oxi->settings_wavg(OXI_SPO2Drop),0,'f',2);
    }

    if (cpap && cpap->hasEnabledSessions()) {
        html+="<tr><td colspan=5>&nbsp;</td></tr>";
        html+=QString("<tr><td colspan=5 align=center><b>%1</b></td></tr>").arg(tr("Machine Settings"));
        html+="<tr><td colspan=5><hr height=2></td></tr>";
        int i=cpap->settings_max(CPAP_PresReliefType);
        int j=cpap->settings_max(CPAP_PresReliefSet);
        QString flexstr=(i>1) ? schema::channel[CPAP_PresReliefType].option(i)+" x"+QString::number(j) : STR_TR_None;
        html+=QString("<tr><td><a class='info' href='#'>%1<span>%2</span></a></td><td colspan=4>%3</td></tr>")
                .arg(STR_TR_PrRelief)
                .arg(schema::channel[CPAP_PresReliefType].description())
                .arg(flexstr);
        QString mclass=cpap->machine->GetClass();
        if (mclass==STR_MACH_PRS1 || mclass==STR_MACH_FPIcon) {
            int humid=round(cpap->settings_wavg(CPAP_HumidSetting));
            html+=QString("<tr><td><a class='info' href='#'>"+STR_TR_Humidifier+"<span>%1</span></a></td><td colspan=4>%2</td></tr>")
                .arg(schema::channel[CPAP_HumidSetting].description())
                .arg(humid==0 ? STR_GEN_Off : "x"+QString::number(humid));
        }

    }
    html+="</table>";

    if (cpap || oxi) {
        html+="<table cellpadding=0 cellspacing=0 border=0 width=100%>";
        html+="<tr><td colspan=5 align=center>&nbsp;</td></tr>";
        html+=QString("<tr><td colspan=4 align=center><b>"+tr("Session Information")+"</b></td></tr>");
        html+="<tr><td colspan=5><hr/></td></tr>";
        QDateTime fd,ld;
        bool corrupted_waveform=false;
        QString tooltip;
        html+=QString("<tr><td align=left><b>"+tr("SessionID")+"</b></td><td><b>"+STR_TR_On+"</b></td><td align=center><b>"+STR_TR_Date+"</b></td><td align=center><b>"+STR_TR_Start+"</b></td><td align=center><b>"+STR_TR_End+"</b></td></tr>");
        if (cpap) {
            html+=QString("<tr><td align=left colspan=5><i>"+tr("CPAP Sessions")+"</i></td></tr>");
            for (QVector<Session *>::iterator s=cpap->begin();s!=cpap->end();s++) {
                fd=QDateTime::fromTime_t((*s)->first()/1000L);
                ld=QDateTime::fromTime_t((*s)->last()/1000L);
                int len=(*s)->length()/1000L;
                int h=len/3600;
                int m=(len/60) % 60;
                int s1=len % 60;
                tooltip=cpap->machine->GetClass()+"&nbsp;"+STR_TR_CPAP+"&nbsp;"+QString().sprintf("%2ih,&nbsp;%2im,&nbsp;%2is",h,m,s1);
                // tooltip needs to lookup language.. :-/

                QHash<ChannelID,QVariant>::iterator i=(*s)->settings.find(CPAP_BrokenWaveform);
                corrupted_waveform=(i!=(*s)->settings.end()) && i.value().toBool();
                Session *sess=*s;
                if (!sess->settings.contains(SESSION_ENABLED)) {
                    sess->settings[SESSION_ENABLED]=true;
                }
                bool b=sess->settings[SESSION_ENABLED].toBool();
                html+=QString("<tr><td align=left><a class=info href='cpap=%1'>%3<span>%2</span></a></td><td width=26><a href='togglecpapsession=%1'><img src='qrc:/icons/session-%4.png' width=24px></a></td><td align=center>%5</td><td align=center>%6</td><td align=center>%7</td></tr>")
                        .arg((*s)->session())
                        .arg(tooltip)
                        .arg((*s)->session(),8,10,QChar('0'))
                        .arg((b ? "on" : "off"))
                        .arg(fd.date().toString(Qt::SystemLocaleShortDate))
                        .arg(fd.toString("HH:mm"))
                        .arg(ld.toString("HH:mm"));
            }
        }

        if (oxi) {
            html+=QString("<tr><td align=left colspan=5><i>"+tr("Oximetry Sessions")+"</i></td></tr>");
            for (QVector<Session *>::iterator s=oxi->begin();s!=oxi->end();s++) {
                fd=QDateTime::fromTime_t((*s)->first()/1000L);
                ld=QDateTime::fromTime_t((*s)->last()/1000L);
                int len=(*s)->length()/1000L;
                int h=len/3600;
                int m=(len/60) % 60;
                int s1=len % 60;
                tooltip=oxi->machine->GetClass()+" "+STR_TR_Oximeter+" "+QString().sprintf("%2ih,&nbsp;%2im,&nbsp;%2is",h,m,s1);

                Session *sess=*s;
                if (!sess->settings.contains(SESSION_ENABLED)) {
                    sess->settings[SESSION_ENABLED]=true;
                }
                bool b=sess->settings[SESSION_ENABLED].toBool();

                QHash<ChannelID,QVariant>::iterator i=(*s)->settings.find(CPAP_BrokenWaveform);
                corrupted_waveform=(i!=(*s)->settings.end()) && i.value().toBool();
                html+=QString("<tr><td align=left><a class=info href='oxi=%1'>%3<span>%2</span></a></td><td width=26><a href='toggleoxisession=%1'><img src='qrc:/icons/session-%4.png' width=24px></a></td><td align=center>%5</td><td align=center>%6</td><td align=center>%7</td></tr>")
                        .arg((*s)->session())
                        .arg(tooltip)
                        .arg((*s)->session(),8,10,QChar('0'))
                        .arg((b ? "on" : "off"))
                        .arg(fd.date().toString(Qt::SystemLocaleShortDate))
                        .arg(fd.toString("HH:mm"))
                        .arg(ld.toString("HH:mm"));
                //tmp.sprintf(("<tr><td align=left><a href='oxi=%i' title='"+tooltip+"'>%08i</a></td><td align=center>"+fd.date().toString(Qt::SystemLocaleShortDate)+"</td><td align=center>"+fd.toString("HH:mm ")+"</td><td align=center>"+ld.toString("HH:mm")+"</td></tr>").toLatin1(),(*s)->session(),(*s)->session());
                //html+=tmp;
            }
        }
        if (stage) {
            html+=QString("<tr><td align=left colspan=5><i>%1</i></td></tr>").arg(tr("Sleep Stage Sessions"));
            for (QVector<Session *>::iterator s=stage->begin();s!=stage->end();s++) {
                fd=QDateTime::fromTime_t((*s)->first()/1000L);
                ld=QDateTime::fromTime_t((*s)->last()/1000L);
                int len=(*s)->length()/1000L;
                int h=len/3600;
                int m=(len/60) % 60;
                int s1=len % 60;
                tooltip=stage->machine->GetClass()+" "+tr("Sleep Stage")+" "+QString().sprintf("%2ih,&nbsp;%2im,&nbsp;%2is",h,m,s1);

                Session *sess=*s;
                if (!sess->settings.contains(SESSION_ENABLED)) {
                    sess->settings[SESSION_ENABLED]=true;
                }
                bool b=sess->settings[SESSION_ENABLED].toBool();

                QHash<ChannelID,QVariant>::iterator i=(*s)->settings.find(CPAP_BrokenWaveform);
                corrupted_waveform=(i!=(*s)->settings.end()) && i.value().toBool();
                html+=QString("<tr><td align=left><a class=info href='stage=%1'>%3<span>%2</span></a></td><td width=26><a href='toggleoxisession=%1'><img src='qrc:/icons/session-%4.png' width=24px></a></td><td align=center>%5</td><td align=center>%6</td><td align=center>%7</td></tr>")
                        .arg((*s)->session())
                        .arg(tooltip)
                        .arg((*s)->session(),8,10,QChar('0'))
                        .arg((b ? "on" : "off"))
                        .arg(fd.date().toString(Qt::SystemLocaleShortDate))
                        .arg(fd.toString("HH:mm"))
                        .arg(ld.toString("HH:mm"));
                //tmp.sprintf(("<tr><td align=left><a href='stage=%i' title='"+tooltip+"'>%08i</a></td><td align=center>"+fd.date().toString(Qt::SystemLocaleShortDate)+"</td><td align=center>"+fd.toString("HH:mm ")+"</td><td align=center>"+ld.toString("HH:mm")+"</td></tr>").toLatin1(),(*s)->session(),(*s)->session());
                //html+=tmp;
            }
        }
        if (corrupted_waveform) {
            html+=QString("<tr><td colspan=5 align=center><i>%1</i></td></tr>").arg(tr("One or more waveform record for this session had faulty source data. Some waveform overlay points may not match up correctly."));
        }
        html+="</table><br/>";
    }
    html+="</body></html>";

    ui->webView->setHtml(html);

    ui->JournalNotes->clear();

    ui->bookmarkTable->clearContents();
    ui->bookmarkTable->setRowCount(0);
    QStringList sl;
    //sl.append(tr("Starts"));
    //sl.append(tr("Notes"));
    ui->bookmarkTable->setHorizontalHeaderLabels(sl);
    ui->ZombieMeter->blockSignals(true);
    ui->weightSpinBox->blockSignals(true);
    ui->ouncesSpinBox->blockSignals(true);

    ui->weightSpinBox->setValue(0);
    ui->ouncesSpinBox->setValue(0);
    ui->ZombieMeter->setValue(5);
    ui->ouncesSpinBox->blockSignals(false);
    ui->weightSpinBox->blockSignals(false);
    ui->ZombieMeter->blockSignals(false);
    ui->BMI->display(0);
    ui->BMI->setVisible(false);
    ui->BMIlabel->setVisible(false);

    BookmarksChanged=false;
    Session *journal=GetJournalSession(date);
    if (journal) {
        bool ok;
        if (journal->settings.contains(Journal_Notes))
            ui->JournalNotes->setHtml(journal->settings[Journal_Notes].toString());

        if (journal->settings.contains(Journal_Weight)) {
            double kg=journal->settings[Journal_Weight].toDouble(&ok);

            if (PROFILE.general->unitSystem()==US_Metric) {
                ui->weightSpinBox->setDecimals(3);
                ui->weightSpinBox->blockSignals(true);
                ui->weightSpinBox->setValue(kg);
                ui->weightSpinBox->blockSignals(false);
                ui->ouncesSpinBox->setVisible(false);
                ui->weightSpinBox->setSuffix(STR_UNIT_KG);
            } else {
                float ounces=(kg*1000.0)/ounce_convert;
                int pounds=ounces/16.0;
                double oz;
                double frac=modf(ounces,&oz);
                ounces=(int(ounces) % 16)+frac;
                ui->weightSpinBox->blockSignals(true);
                ui->ouncesSpinBox->blockSignals(true);
                ui->weightSpinBox->setValue(pounds);
                ui->ouncesSpinBox->setValue(ounces);
                ui->ouncesSpinBox->blockSignals(false);
                ui->weightSpinBox->blockSignals(false);

                ui->weightSpinBox->setSuffix(STR_UNIT_POUND);
                ui->weightSpinBox->setDecimals(0);
                ui->ouncesSpinBox->setVisible(true);
                ui->ouncesSpinBox->setSuffix(STR_UNIT_OUNCE);
            }
            double height=PROFILE.user->height()/100.0;
            if (height>0 && kg>0) {
                double bmi=kg/(height*height);
                ui->BMI->setVisible(true);
                ui->BMIlabel->setVisible(true);
                ui->BMI->display(bmi);
            }
        }

        if (journal->settings.contains(Journal_ZombieMeter)) {
            ui->ZombieMeter->blockSignals(true);
            ui->ZombieMeter->setValue(journal->settings[Journal_ZombieMeter].toDouble(&ok));
            ui->ZombieMeter->blockSignals(false);
        }

        if (journal->settings.contains(Bookmark_Start)) {
            QVariantList start=journal->settings[Bookmark_Start].toList();
            QVariantList end=journal->settings[Bookmark_End].toList();
            QStringList notes=journal->settings[Bookmark_Notes].toStringList();

            ui->bookmarkTable->blockSignals(true);


            qint64 clockdrift=PROFILE.cpap->clockDrift()*1000L,drift;
            Day * dday=PROFILE.GetDay(previous_date,MT_CPAP);
            drift=(dday!=NULL) ? clockdrift : 0;

            bool ok;
            for (int i=0;i<start.size();i++) {
                qint64 st=start.at(i).toLongLong(&ok)+drift;
                qint64 et=end.at(i).toLongLong(&ok)+drift;

                QDateTime d=QDateTime::fromTime_t(st/1000L);
                //int row=ui->bookmarkTable->rowCount();
                ui->bookmarkTable->insertRow(i);
                QTableWidgetItem *tw=new QTableWidgetItem(notes.at(i));
                QTableWidgetItem *dw=new QTableWidgetItem(d.time().toString("HH:mm:ss"));
                dw->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
                ui->bookmarkTable->setItem(i,0,dw);
                ui->bookmarkTable->setItem(i,1,tw);
                tw->setData(Qt::UserRole,st);
                tw->setData(Qt::UserRole+1,et);
            } // for (int i
            ui->bookmarkTable->blockSignals(false);
        } // if (journal->settings.contains(Bookmark_Start))
    } // if (journal)
}

void Daily::UnitsChanged()
{
    double kg;
    if (PROFILE.general->unitSystem()==US_Archiac) {
        kg=ui->weightSpinBox->value();
        float ounces=(kg*1000.0)/ounce_convert;
        int pounds=ounces/16;
        float oz=fmodf(ounces,16);
        ui->weightSpinBox->setValue(pounds);
        ui->ouncesSpinBox->setValue(oz);

        ui->weightSpinBox->setDecimals(0);
        ui->weightSpinBox->setSuffix(STR_UNIT_POUND);
        ui->ouncesSpinBox->setVisible(true);
        ui->ouncesSpinBox->setSuffix(STR_UNIT_OUNCE);
    } else {
        kg=(ui->weightSpinBox->value()*(ounce_convert*16.0))+(ui->ouncesSpinBox->value()*ounce_convert);
        kg/=1000.0;
        ui->weightSpinBox->setDecimals(3);
        ui->weightSpinBox->setValue(kg);
        ui->ouncesSpinBox->setVisible(false);
        ui->weightSpinBox->setSuffix(STR_UNIT_KG);
    }
}

void Daily::Unload(QDate date)
{
    Session *journal=GetJournalSession(date);

    bool nonotes=ui->JournalNotes->toPlainText().isEmpty();
    if (journal) {
        QString jhtml=ui->JournalNotes->toHtml();
        if ((!journal->settings.contains(Journal_Notes) && !nonotes) || (journal->settings[Journal_Notes]!=jhtml)) {
            journal->settings[Journal_Notes]=jhtml;
            journal->SetChanged(true);
        }
    } else {
        if (!nonotes) {
            journal=CreateJournalSession(date);
            if (!nonotes) {
                journal->settings[Journal_Notes]=ui->JournalNotes->toHtml();
                journal->SetChanged(true);
            }
        }
    }

    if (journal) {
        if (nonotes) {
            QHash<ChannelID,QVariant>::iterator it=journal->settings.find(Journal_Notes);
            if (it!=journal->settings.end()) {
                journal->settings.erase(it);
            }
        }
        if (journal->IsChanged()) {
            // blah.. was updating overview graphs here.. Was too slow.
        }
        Machine *jm=PROFILE.GetMachine(MT_JOURNAL);
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
    QColor col=QColorDialog::getColor(COLOR_Black,this,tr("Pick a Colour")); //,QColorDialog::NoButtons);
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
    Machine *m=PROFILE.GetMachine(MT_JOURNAL);
    if (!m) {
        m=new Machine(p_profile,0);
        m->SetClass("Journal");
        m->properties[STR_PROP_Brand]="Virtual";
        m->SetType(MT_JOURNAL);
        PROFILE.AddMachine(m);
    }
    Session *sess=new Session(m,0);
    qint64 st,et;
    Day *cday=PROFILE.GetDay(date,MT_CPAP);
    if (cday) {
        st=cday->first();
        et=cday->last();
    } else {
        QDateTime dt(date,QTime(20,0));
        st=qint64(dt.toTime_t())*1000L;
        et=st+3600000;
    }
    sess->set_first(st);
    sess->set_last(et);
    sess->SetChanged(true);
    m->AddSession(sess,p_profile);
    return sess;
}
Session * Daily::GetJournalSession(QDate date) // Get the first journal session
{
    Day *journal=PROFILE.GetDay(date,MT_JOURNAL);
    if (!journal)
        return NULL; //CreateJournalSession(date);
    QVector<Session *>::iterator s;
    s=journal->begin();
    if (s!=journal->end())
        return *s;
    return NULL;
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
}
void Daily::UpdateSTAGEGraphs(Day *day)
{
    //if (!day) return;
    if (day) {
        day->OpenEvents();
    }
    for (QList<Layer *>::iterator g=STAGEData.begin();g!=STAGEData.end();g++) {
        (*g)->SetDay(day);
    }
}

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
    GraphView->redraw();
}

void Daily::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    QDateTime d;
    if (!item->data(0,Qt::UserRole).isNull()) {
        qint64 winsize=qint64(PROFILE.general->eventWindowSize())*60000L;
        qint64 t=item->data(0,Qt::UserRole).toLongLong();

        double st=t-(winsize/2);
        double et=t+(winsize/2);


        gGraph *g=GraphView->findGraph(STR_TR_EventFlags);
        if (!g) return;
        if (st<g->rmin_x) {
            st=g->rmin_x;
            et=st+winsize;
        }
        if (et>g->rmax_x) {
            et=g->rmax_x;
            st=et-winsize;
        }
        GraphView->SetXBounds(st,et);
    }
}

void Daily::on_treeWidget_itemSelectionChanged()
{
    if (ui->treeWidget->selectedItems().size()==0) return;
    QTreeWidgetItem *item=ui->treeWidget->selectedItems().at(0);
    if (!item) return;
    on_treeWidget_itemClicked(item, 0);
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

void Daily::on_prevDayButton_clicked()
{
    if (!PROFILE.ExistsAndTrue("SkipEmptyDays")) {
        LoadDate(previous_date.addDays(-1));
    } else {
        QDate d=previous_date;
        for (int i=0;i<90;i++) {
            d=d.addDays(-1);
            if (PROFILE.GetDay(d)) {
                LoadDate(d);
                break;
            }
        }
    }
}

void Daily::on_nextDayButton_clicked()
{
    if (!PROFILE.ExistsAndTrue("SkipEmptyDays")) {
        LoadDate(previous_date.addDays(1));
    } else {
        QDate d=previous_date;
        for (int i=0;i<90;i++) {
            d=d.addDays(1);
            if (PROFILE.GetDay(d)) {
                LoadDate(d);
                break;
            }
        }
    }
}

void Daily::on_calButton_toggled(bool checked)
{
    //bool b=!ui->calendar->isVisible();
    bool b=checked;
    ui->calendar->setVisible(b);
    if (!b) ui->calButton->setArrowType(Qt::DownArrow);
    else ui->calButton->setArrowType(Qt::UpArrow);
}


void Daily::on_todayButton_clicked()
{
    QDate d=QDate::currentDate();
    if (d > PROFILE.LastDay()) d=PROFILE.LastDay();
    LoadDate(d);
}

void Daily::on_evViewSlider_valueChanged(int value)
{
    ui->evViewLCD->display(value);
    PROFILE.general->setEventWindowSize(value);

    int winsize=value*60;

    gGraph *g=GraphView->findGraph(STR_TR_EventFlags);
    if (!g) return;
    qint64 st=g->min_x;
    qint64 et=g->max_x;
    qint64 len=et-st;
    qint64 d=st+len/2.0;

    st=d-(winsize/2)*1000;
    et=d+(winsize/2)*1000;
    if (st<g->rmin_x) {
        st=g->rmin_x;
        et=st+winsize*1000;
    }
    if (et>g->rmax_x) {
        et=g->rmax_x;
        st=et-winsize*1000;
    }
    GraphView->SetXBounds(st,et);
}

void Daily::on_bookmarkTable_itemClicked(QTableWidgetItem *item)
{
    int row=item->row();
    qint64 st,et;

//    qint64 clockdrift=PROFILE.cpap->clockDrift()*1000L,drift;
//    Day * dday=PROFILE.GetDay(previous_date,MT_CPAP);
//    drift=(dday!=NULL) ? clockdrift : 0;

    QTableWidgetItem *it=ui->bookmarkTable->item(row,1);
    bool ok;
    st=it->data(Qt::UserRole).toLongLong(&ok);
    et=it->data(Qt::UserRole+1).toLongLong(&ok);
    qint64 st2=0,et2=0,st3,et3;
    Day * day=PROFILE.GetGoodDay(previous_date,MT_CPAP);
    if (day) {
        st2=day->first();
        et2=day->last();
    }
    Day * oxi=PROFILE.GetGoodDay(previous_date,MT_OXIMETER);
    if (oxi) {
        st3=oxi->first();
        et3=oxi->last();
    }
    if (oxi && day) {
        st2=qMin(st2,st3);
        et2=qMax(et2,et3);
    } else if (oxi) {
        st2=st3;
        et2=et3;
    } else if (!day) return;
    if ((et<st2) || (st>et2)) {
        mainwin->Notify(tr("This bookmarked is in a currently disabled area.."));
        return;
    }

    if (st<st2) st=st2;
    if (et>et2) et=et2;
    GraphView->SetXBounds(st,et);
    GraphView->redraw();
}

void Daily::on_addBookmarkButton_clicked()
{
    qint64 st,et;
    ui->bookmarkTable->blockSignals(true);
    GraphView->GetXBounds(st,et);
    QDateTime d=QDateTime::fromTime_t(st/1000L);
    int row=ui->bookmarkTable->rowCount();
    ui->bookmarkTable->insertRow(row);
    QTableWidgetItem *tw=new QTableWidgetItem(tr("Bookmark at %1").arg(d.time().toString("HH:mm:ss")));
    QTableWidgetItem *dw=new QTableWidgetItem(d.time().toString("HH:mm:ss"));
    dw->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->bookmarkTable->setItem(row,0,dw);
    ui->bookmarkTable->setItem(row,1,tw);
    qint64 clockdrift=PROFILE.cpap->clockDrift()*1000L,drift;
    Day * day=PROFILE.GetDay(previous_date,MT_CPAP);
    drift=(day!=NULL) ? clockdrift : 0;

    // Counter CPAP clock drift for storage, in case user changes it later on
    // This won't fix the text string names..

    tw->setData(Qt::UserRole,st-drift);
    tw->setData(Qt::UserRole+1,et-drift);

    ui->bookmarkTable->blockSignals(false);
    update_Bookmarks();
    mainwin->updateFavourites();

    //ui->bookmarkTable->setItem(row,2,new QTableWidgetItem(QString::number(st)));
    //ui->bookmarkTable->setItem(row,3,new QTableWidgetItem(QString::number(et)));
}
void Daily::update_Bookmarks()
{
    QVariantList start;
    QVariantList end;
    QStringList notes;
    QTableWidgetItem *item;
    for (int row=0;row<ui->bookmarkTable->rowCount();row++) {
        item=ui->bookmarkTable->item(row,1);

        start.push_back(item->data(Qt::UserRole));
        end.push_back(item->data(Qt::UserRole+1));
        notes.push_back(item->text());
    }
    Session *journal=GetJournalSession(previous_date);
    if (!journal) {
        journal=CreateJournalSession(previous_date);
    }

    journal->settings[Bookmark_Start]=start;
    journal->settings[Bookmark_End]=end;
    journal->settings[Bookmark_Notes]=notes;
    journal->SetChanged(true);
    BookmarksChanged=true;
    mainwin->updateFavourites();
}

void Daily::on_removeBookmarkButton_clicked()
{
    int row=ui->bookmarkTable->currentRow();
    if (row>=0) {
        ui->bookmarkTable->blockSignals(true);
        ui->bookmarkTable->removeRow(row);
        ui->bookmarkTable->blockSignals(false);
        update_Bookmarks();
    }
    mainwin->updateFavourites();
}
void Daily::on_ZombieMeter_valueChanged(int action)
{
    Q_UNUSED(action);
    ZombieMeterMoved=true;
    Session *journal=GetJournalSession(previous_date);
    if (!journal) {
        journal=CreateJournalSession(previous_date);
    }
    journal->settings[Journal_ZombieMeter]=ui->ZombieMeter->value();
    journal->SetChanged(true);

    if (mainwin->getOverview()) mainwin->getOverview()->ResetGraph("Zombie");
}

void Daily::on_bookmarkTable_itemChanged(QTableWidgetItem *item)
{
    Q_UNUSED(item);
    update_Bookmarks();
}

void Daily::on_weightSpinBox_valueChanged(double arg1)
{
    // Update the BMI display
    double kg;
    if (PROFILE.general->unitSystem()==US_Archiac) {
         kg=((arg1*pound_convert) + (ui->ouncesSpinBox->value()*ounce_convert)) / 1000.0;
    } else kg=arg1;
    double height=PROFILE.user->height()/100.0;
    if ((height>0) && (kg>0)) {
        double bmi=kg/(height * height);
        ui->BMI->display(bmi);
        ui->BMI->setVisible(true);
    }
}

void Daily::on_weightSpinBox_editingFinished()
{
    double arg1=ui->weightSpinBox->value();

    double height=PROFILE.user->height()/100.0;
    Session *journal=GetJournalSession(previous_date);
    if (!journal) {
        journal=CreateJournalSession(previous_date);
    }

    double kg;
    if (PROFILE.general->unitSystem()==US_Archiac) {
            kg=((arg1*pound_convert) + (ui->ouncesSpinBox->value()*ounce_convert)) / 1000.0;
    } else {
            kg=arg1;
    }
    journal->settings[Journal_Weight]=kg;
    gGraphView *gv=mainwin->getOverview()->graphView();
    gGraph *g;
    if (gv) {
        g=gv->findGraph(STR_TR_Weight);
        if (g) g->setDay(NULL);
    }
    if ((height>0) && (kg>0)) {
        double bmi=kg/(height * height);
        ui->BMI->display(bmi);
        ui->BMI->setVisible(true);
        journal->settings[Journal_BMI]=bmi;
        if (gv) {
            g=gv->findGraph(STR_TR_BMI);
            if (g) g->setDay(NULL);
        }
    }
    journal->SetChanged(true);
}

void Daily::on_ouncesSpinBox_valueChanged(int arg1)
{
    // just update for BMI display
    double height=PROFILE.user->height()/100.0;
    double kg=((ui->weightSpinBox->value()*pound_convert) + (arg1*ounce_convert)) / 1000.0;
    if ((height>0) && (kg>0)) {
        double bmi=kg/(height * height);
        ui->BMI->display(bmi);
        ui->BMI->setVisible(true);
    }
}

void Daily::on_ouncesSpinBox_editingFinished()
{
    double arg1=ui->ouncesSpinBox->value();
    Session *journal=GetJournalSession(previous_date);
    if (!journal) {
        journal=CreateJournalSession(previous_date);
    }
    double height=PROFILE.user->height()/100.0;
    double kg=((ui->weightSpinBox->value()*pound_convert) + (arg1*ounce_convert)) / 1000.0;
    journal->settings[Journal_Weight]=kg;

    gGraph *g;
    if (mainwin->getOverview()) {
        g=mainwin->getOverview()->graphView()->findGraph(STR_TR_Weight);
        if (g) g->setDay(NULL);
    }

    if ((height>0) && (kg>0)) {
        double bmi=kg/(height * height);
        ui->BMI->display(bmi);
        ui->BMI->setVisible(true);

        journal->settings[Journal_BMI]=bmi;
        if (mainwin->getOverview()) {
            g=mainwin->getOverview()->graphView()->findGraph(STR_TR_BMI);
            if (g) g->setDay(NULL);
        }
    }
    journal->SetChanged(true);
    if (mainwin->getOverview()) mainwin->getOverview()->ResetGraph("Weight");
}

QString Daily::GetDetailsText()
{
    ui->webView->triggerPageAction(QWebPage::SelectAll);
    QString text=ui->webView->page()->selectedText();
    ui->webView->triggerPageAction(QWebPage::MoveToEndOfDocument);
    ui->webView->triggerPageAction(QWebPage::SelectEndOfDocument);
    return text;
}

void Daily::on_graphCombo_activated(int index)
{
    if (index<0)
        return;

    gGraph *g;
    QString s;
    s=ui->graphCombo->currentText();
    bool b=!ui->graphCombo->itemData(index,Qt::UserRole).toBool();
    ui->graphCombo->setItemData(index,b,Qt::UserRole);
    if (b) {
        ui->graphCombo->setItemIcon(index,*icon_on);
    } else {
        ui->graphCombo->setItemIcon(index,*icon_off);
    }
    g=GraphView->findGraph(s);
    g->setVisible(b);

    updateCube();
    GraphView->updateScale();
    GraphView->redraw();
}
void Daily::updateCube()
{
    //brick..
    if ((GraphView->visibleGraphs()==0)) {
        ui->toggleGraphs->setArrowType(Qt::UpArrow);
        ui->toggleGraphs->setToolTip(tr("Show all graphs"));
        ui->toggleGraphs->blockSignals(true);
        ui->toggleGraphs->setChecked(true);
        ui->toggleGraphs->blockSignals(false);

        if (ui->graphCombo->count()>0) {
            GraphView->setEmptyText(tr("No Graphs On!"));
            GraphView->setCubeImage(images["nographs"]);

        } else {
            GraphView->setEmptyText(STR_TR_NoData);
            GraphView->setCubeImage(images["nodata"]);
        }
    } else {
        GraphView->setCubeImage(images["sheep"]);

        ui->toggleGraphs->setArrowType(Qt::DownArrow);
        ui->toggleGraphs->setToolTip(tr("Hide all graphs"));
        ui->toggleGraphs->blockSignals(true);
        ui->toggleGraphs->setChecked(false);
        ui->toggleGraphs->blockSignals(false);
    }
}


void Daily::on_toggleGraphs_clicked(bool checked)
{
    QString s;
    QIcon *icon=checked ? icon_off : icon_on;
    for (int i=0;i<ui->graphCombo->count();i++) {
        s=ui->graphCombo->itemText(i);
        ui->graphCombo->setItemIcon(i,*icon);
        ui->graphCombo->setItemData(i,!checked,Qt::UserRole);
    }
    for (int i=0;i<GraphView->size();i++) {
        (*GraphView)[i]->setVisible(!checked);
    }

    updateCube();
    GraphView->updateScale();
    GraphView->redraw();

}

void Daily::updateGraphCombo()
{
    ui->graphCombo->clear();
    gGraph *g;

    for (int i=0;i<GraphView->size();i++) {
        g=(*GraphView)[i];
        if (g->isEmpty()) continue;

        if (g->visible()) {
            ui->graphCombo->addItem(*icon_on,g->title(),true);
        } else {
            ui->graphCombo->addItem(*icon_off,g->title(),false);
        }
    }
    ui->graphCombo->setCurrentIndex(0);
    updateCube();
}

void Daily::on_zoomFullyOut_clicked()
{
    GraphView->ResetBounds(true);
    GraphView->redraw();
}

void Daily::on_resetLayoutButton_clicked()
{
    GraphView->resetLayout();
}
