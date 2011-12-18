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
const float ounce_convert=28.3495231;
const float pound_convert=ounce_convert*16;

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

    snapGV=new gGraphView(ui->graphMainArea);
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

    int default_height=PROFILE["GraphHeight"].toInt();
    SF=new gGraph(GraphView,"Event Flags","Event Flags",default_height);
    FRW=new gGraph(GraphView,schema::channel[CPAP_FlowRate].label(),schema::channel[CPAP_FlowRate].description()+"\n("+schema::channel[CPAP_FlowRate].units()+")",default_height);
    AHI=new gGraph(GraphView,schema::channel[CPAP_AHI].label(),schema::channel[CPAP_AHI].description()+"\n("+schema::channel[CPAP_AHI].units()+")",default_height);
    MP=new gGraph(GraphView,schema::channel[CPAP_MaskPressure].label(),schema::channel[CPAP_MaskPressure].description()+"\n("+schema::channel[CPAP_MaskPressure].units()+")",default_height);
    PRD=new gGraph(GraphView,schema::channel[CPAP_Pressure].label(),schema::channel[CPAP_Pressure].description()+"\n("+schema::channel[CPAP_Pressure].units()+")",default_height);
    LEAK=new gGraph(GraphView,schema::channel[CPAP_Leak].label(),schema::channel[CPAP_Leak].description()+"\n("+schema::channel[CPAP_Leak].units()+")",default_height);
    SNORE=new gGraph(GraphView,schema::channel[CPAP_Snore].label(),schema::channel[CPAP_Snore].description()+"\n("+schema::channel[CPAP_Snore].units()+")",default_height);
    RR=new gGraph(GraphView,schema::channel[CPAP_RespRate].label(),schema::channel[CPAP_RespRate].description()+"\n("+schema::channel[CPAP_RespRate].units()+")",default_height);
    TV=new gGraph(GraphView,schema::channel[CPAP_TidalVolume].label(),schema::channel[CPAP_TidalVolume].description()+"\n("+schema::channel[CPAP_TidalVolume].units()+")",default_height);
    MV=new gGraph(GraphView,schema::channel[CPAP_MinuteVent].label(),schema::channel[CPAP_MinuteVent].description()+"\n("+schema::channel[CPAP_MinuteVent].units()+")",default_height);
    FLG=new gGraph(GraphView,schema::channel[CPAP_FLG].label(),schema::channel[CPAP_FLG].description()+"\n("+schema::channel[CPAP_FLG].units()+")",default_height);
    PTB=new gGraph(GraphView,schema::channel[CPAP_PTB].label(),schema::channel[CPAP_PTB].description()+"\n("+schema::channel[CPAP_PTB].units()+")",default_height);
    RE=new gGraph(GraphView,schema::channel[CPAP_RespEvent].label(),schema::channel[CPAP_RespEvent].description()+"\n("+schema::channel[CPAP_RespEvent].units()+")",default_height);
    IE=new gGraph(GraphView,schema::channel[CPAP_IE].label(),schema::channel[CPAP_IE].description()+"\n("+schema::channel[CPAP_IE].units()+")",default_height);
    TE=new gGraph(GraphView,schema::channel[CPAP_Te].label(),schema::channel[CPAP_Te].description()+"\n("+schema::channel[CPAP_Te].units()+")",default_height);
    TI=new gGraph(GraphView,schema::channel[CPAP_Ti].label(),schema::channel[CPAP_Ti].description()+"\n("+schema::channel[CPAP_Ti].units()+")",default_height);
    TgMV=new gGraph(GraphView,schema::channel[CPAP_TgMV].label(),schema::channel[CPAP_TgMV].description()+"\n("+schema::channel[CPAP_TgMV].units()+")",default_height);
    //INTPULSE=new gGraph(GraphView,"R-Pulse",schema::channel[CPAP_Te].units(),default_height);
    //INTSPO2=new gGraph(GraphView,"R-SPO2",default_height);

    int oxigrp=PROFILE.ExistsAndTrue("SyncOximetry") ? 0 : 1;
    PULSE=new gGraph(GraphView,schema::channel[OXI_Pulse].label(),schema::channel[OXI_Pulse].description()+"\n("+schema::channel[OXI_Pulse].units()+")",default_height,oxigrp);
    SPO2=new gGraph(GraphView,schema::channel[OXI_SPO2].label(),schema::channel[OXI_SPO2].description()+"\n("+schema::channel[OXI_SPO2].units()+")",default_height,oxigrp);
    PLETHY=new gGraph(GraphView,schema::channel[OXI_Plethy].label(),schema::channel[OXI_Plethy].description()+"\n("+schema::channel[OXI_Plethy].units()+")",default_height,oxigrp);

    // Event Pie Chart (for snapshot purposes)
    // TODO: Convert snapGV to generic for snapshotting multiple graphs (like reports does)
//    TAP=new gGraph(GraphView,"Time@Pressure","cmH2O",100);
//    TAP->showTitle(false);
//    gTAPGraph * tap=new gTAPGraph(CPAP_Pressure,GST_Line);
//    TAP->AddLayer(AddCPAP(tap));
    //TAP->setMargins(0,0,0,0);


    GAHI=new gGraph(snapGV,"Breakdown","events",172);
    gSegmentChart * evseg=new gSegmentChart(GST_Pie);
    evseg->AddSlice(CPAP_Hypopnea,QColor(0x40,0x40,0xff,0xff),"H");
    evseg->AddSlice(CPAP_Apnea,QColor(0x20,0x80,0x20,0xff),"A");
    evseg->AddSlice(CPAP_Obstructive,QColor(0x40,0xaf,0xbf,0xff),"OA");
    evseg->AddSlice(CPAP_ClearAirway,QColor(0xb2,0x54,0xcd,0xff),"CA");
    evseg->AddSlice(CPAP_RERA,QColor(0xff,0xff,0x80,0xff),"RE");
    evseg->AddSlice(CPAP_FlowLimit,QColor(0x40,0x40,0x40,0xff),"FL");

    GAHI->AddLayer(AddCPAP(evseg));
    GAHI->setMargins(0,0,0,0);
    //SF->AddLayer(AddCPAP(evseg),LayerRight,100);

    gFlagsGroup *fg=new gFlagsGroup();
    SF->AddLayer(AddCPAP(fg));
    fg->AddLayer((new gFlagsLine(CPAP_CSR,QColor("light green"),"PB",false,FT_Span)));
    fg->AddLayer((new gFlagsLine(CPAP_ClearAirway,QColor("purple"),"CA",false)));
    fg->AddLayer((new gFlagsLine(CPAP_Obstructive,QColor("#40c0ff"),"OA",true)));
    fg->AddLayer((new gFlagsLine(CPAP_Apnea,QColor("dark green"),"A")));
    fg->AddLayer((new gFlagsLine(CPAP_Hypopnea,QColor("blue"),"H",true)));
    fg->AddLayer((new gFlagsLine(CPAP_ExP,QColor("dark cyan"),"E",false)));
    fg->AddLayer((new gFlagsLine(CPAP_LeakFlag,QColor("dark blue"),"L",false)));
    fg->AddLayer((new gFlagsLine(CPAP_NRI,QColor("dark magenta"),"NRI",false)));
    fg->AddLayer((new gFlagsLine(CPAP_FlowLimit,QColor("black"),"FL")));
    fg->AddLayer((new gFlagsLine(CPAP_RERA,QColor("gold"),"RE")));
    fg->AddLayer((new gFlagsLine(CPAP_VSnore,QColor("red"),"VS")));
    fg->AddLayer((new gFlagsLine("UserFlag1",QColor("yellow"),"UF1")));
    fg->AddLayer((new gFlagsLine("UserFlag2",QColor("green"),"UF2")));
    //fg->AddLayer((new gFlagsLine(PRS1_0B,QColor("dark green"),"U0B")));
    //fg->AddLayer((new gFlagsLine(CPAP_VSnore2,QColor("red"),"VS2")));
    SF->setBlockZoom(true);
    SF->AddLayer(new gShadowArea());
    SF->AddLayer(new gYSpacer(),LayerLeft,gYAxis::Margin);
    //SF->AddLayer(new gFooBar(),LayerBottom,0,1);
    SF->AddLayer(new gXAxis(Qt::black,false),LayerBottom,0,20); //gXAxis::Margin);


    gLineChart *l;
    l=new gLineChart(CPAP_FlowRate,Qt::black,false,false);
    gLineOverlaySummary *los=new gLineOverlaySummary("Selection AHI",5,-4);
    AddCPAP(l);
    FRW->AddLayer(new gXGrid());
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_CSR,QColor("light green"),"CSR",FT_Span)));
    FRW->AddLayer(l);
    FRW->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    FRW->AddLayer(new gXAxis(),LayerBottom,0,20);
    FRW->AddLayer(AddCPAP(los->add(new gLineOverlayBar(CPAP_Hypopnea,QColor("blue"),"H"))));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_PressurePulse,QColor("red"),"PR",FT_Dot)));
    //FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Pressure,QColor("white"),"P",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_0B,QColor("blue"),"0B",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_10,QColor("orange"),"10",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(PRS1_0E,QColor("dark red"),"0E",FT_Dot)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_RERA,QColor("gold"),"RE")));
    FRW->AddLayer(AddCPAP(los->add(new gLineOverlayBar(CPAP_Apnea,QColor("dark green"),"A"))));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_VSnore,QColor("red"),"VS")));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_FlowLimit,QColor("black"),"FL")));
    FRW->AddLayer(AddCPAP(los->add(new gLineOverlayBar(CPAP_Obstructive,QColor("#40c0ff"),"OA"))));
    FRW->AddLayer(AddCPAP(los->add(new gLineOverlayBar(CPAP_ClearAirway,QColor("purple"),"CA"))));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar("UserFlag1",QColor("yellow"),"U1",FT_Bar)));
    FRW->AddLayer(AddCPAP(new gLineOverlayBar("UserFlag2",QColor("orange"),"U2",FT_Bar)));
    FRW->AddLayer(AddOXI(new gLineOverlayBar(OXI_SPO2Drop,QColor("red"),"O2")));
    FRW->AddLayer(AddOXI(new gLineOverlayBar(OXI_PulseChange,QColor("blue"),"PC",FT_Dot)));

    FRW->AddLayer(AddCPAP(los));


    gGraph *graphs[]={ PRD, LEAK, AHI, SNORE, PTB, MP, RR, MV, TV, FLG, IE, TI, TE, TgMV, SPO2, PLETHY, PULSE };
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


    bool square=PROFILE["SquareWavePlots"].toBool();
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_EPAP,Qt::blue,square)));
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_IPAPLo,Qt::darkRed,square)));
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_IPAP,Qt::red,square)));
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_IPAPHi,Qt::darkRed,square)));
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_Pressure,QColor("dark green"),square)));

    AHI->AddLayer(AddCPAP(new gLineChart(CPAP_AHI,QColor("light green"),square)));

    //AHI->AddLayer(AddCPAP(new AHIChart(QColor("#37a24b"))));
    LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_LeakTotal,Qt::darkYellow,square)));
    LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_Leak,Qt::darkMagenta,square)));
    LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_MaxLeak,Qt::darkRed,square)));
    SNORE->AddLayer(AddCPAP(new gLineChart(CPAP_Snore,Qt::darkGray,true)));

    PTB->AddLayer(AddCPAP(new gLineChart(CPAP_PTB,Qt::gray,square)));
    MP->AddLayer(AddCPAP(new gLineChart(CPAP_MaskPressure,Qt::blue,false)));
    RR->AddLayer(AddCPAP(new gLineChart(CPAP_RespRate,Qt::darkMagenta,square)));
    MV->AddLayer(AddCPAP(new gLineChart(CPAP_MinuteVent,Qt::darkCyan,square)));
    TV->AddLayer(AddCPAP(new gLineChart(CPAP_TidalVolume,Qt::magenta,square)));
    //TV->AddLayer(AddCPAP(new gLineChart("TidalVolume2",Qt::magenta,square)));
    FLG->AddLayer(AddCPAP(new gLineChart(CPAP_FLG,Qt::darkBlue,true)));
    //RE->AddLayer(AddCPAP(new gLineChart(CPAP_RespiratoryEvent,Qt::magenta,true)));
    IE->AddLayer(AddCPAP(new gLineChart(CPAP_IE,Qt::darkRed,square)));
    TE->AddLayer(AddCPAP(new gLineChart(CPAP_Te,Qt::darkGreen,square)));
    TI->AddLayer(AddCPAP(new gLineChart(CPAP_Ti,Qt::darkBlue,square)));
    TgMV->AddLayer(AddCPAP(new gLineChart(CPAP_TgMV,Qt::darkCyan,square)));
    //INTPULSE->AddLayer(AddCPAP(new gLineChart(OXI_Pulse,Qt::red,square)));
    //INTSPO2->AddLayer(AddCPAP(new gLineChart(OXI_SPO2,Qt::blue,square)));

    PULSE->AddLayer(AddOXI(new gLineOverlayBar(OXI_PulseChange,QColor("light gray"),"PD",FT_Span)));
    SPO2->AddLayer(AddOXI(new gLineOverlayBar(OXI_SPO2Drop,QColor("light blue"),"O2",FT_Span)));

    PULSE->AddLayer(AddOXI(new gLineChart(OXI_Pulse,Qt::red,square)));
    SPO2->AddLayer(AddOXI(new gLineChart(OXI_SPO2,Qt::blue,true)));
    PLETHY->AddLayer(AddOXI(new gLineChart(OXI_Plethy,Qt::darkBlue,false)));

    PTB->setForceMaxY(100);
    SPO2->setForceMaxY(100);
    //FRW->setRecMinY(-120);
    //FRW->setRecMaxY(0);

    /*SPO2->setRecMaxY(100);
    SPO2->setRecMinY(75);
    PULSE->setRecMinY(40);

    LEAK->setRecMinY(0);
    LEAK->setRecMaxY(80);
    PRD->setRecMinY(4.0);
    PRD->setRecMaxY(15.0); */
    for (int i=0;i<ng;i++){
        graphs[i]->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
        graphs[i]->AddLayer(new gXAxis(),LayerBottom,0,20);
    }

    layout->layout();

    QTextCharFormat format = ui->calendar->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    ui->calendar->setWeekdayTextFormat(Qt::Saturday, format);
    ui->calendar->setWeekdayTextFormat(Qt::Sunday, format);

    //Qt::DayOfWeek dow=QLocale::system().firstDayOfWeek();
    Qt::DayOfWeek dow=firstDayOfWeekFromLocale();

    ui->calendar->setFirstDayOfWeek(dow);

    ui->tabWidget->setCurrentWidget(ui->details);

    ui->webView->settings()->setFontSize(QWebSettings::DefaultFontSize,QApplication::font().pointSize());
    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(ui->webView,SIGNAL(linkClicked(QUrl)),this,SLOT(Link_clicked(QUrl)));

    if (!PROFILE.Exists("EventViewSize")) PROFILE["EventViewSize"]=4;
    int ews=PROFILE["EventViewSize"].toInt();
    ui->evViewSlider->setValue(ews);
    ui->evViewLCD->display(ews);

    GraphView->LoadSettings("Daily");

    for (int i=0;i<GraphView->size();i++) {
        QString title=(*GraphView)[i]->title();
        QPushButton *btn=new QPushButton(title,this);
        btn->setCheckable(true);
        btn->setChecked((*GraphView)[i]->visible());
        btn->setToolTip("Show/Hide "+title);
        GraphToggles[title]=btn;
        btn->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Minimum);
        ui->graphToggleArea->addWidget(btn);
        connect(btn,SIGNAL(toggled(bool)),this,SLOT(graphtogglebutton_toggled(bool)));
    }
    ui->graphToggleArea->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));


    // TODO: Add preference to hide do this for Widget Haters..
    //ui->calNavWidget->hide();
    if (PROFILE["Units"].toString()=="metric") {
        ui->ouncesSpinBox->setVisible(false);
        ui->weightSpinBox->setDecimals(3);
        ui->weightSpinBox->setSuffix("Kg");
    } else {
        ui->weightSpinBox->setSuffix("lb");
        ui->weightSpinBox->setDecimals(0);
        ui->ouncesSpinBox->setVisible(true);
        ui->ouncesSpinBox->setSuffix("oz");
    }
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
}
void Daily::Link_clicked(const QUrl &url)
{
    QString code=url.toString().section("=",0,0).toLower();
    QString data=url.toString().section("=",1);
    int sid=data.toInt();
    Day *day=NULL;
    if (code=="cpap")  {
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
        QList<QTreeWidgetItem *> list=ui->treeWidget->findItems(schema::channel[data].description(),Qt::MatchContains);
        if (list.size()>0) {
            ui->treeWidget->collapseAll();
            ui->treeWidget->expandItem(list.at(0));
            QTreeWidgetItem *wi=list.at(0)->child(0);
            ui->treeWidget->setCurrentItem(wi);
            ui->tabWidget->setCurrentIndex(1);
        } else {
            mainwin->Notify(tr("No %1 events are recorded this day").arg(schema::channel[data].description()),"",1500);
        }
    } else if (code=="graph") {
        qDebug() << "Select graph " << data;
    } else {
        qDebug() << "Clicked on" << code << data;
    }
    if (day) {

        Session *sess=day->machine->sessionlist[sid];
        if (sess) {
            GraphView->SetXBounds(sess->first(),sess->last());
        }
    }
}

void Daily::ReloadGraphs()
{
    QDate d;
    if (previous_date.isValid()) {
        d=previous_date;
        Unload(d);
    } //else
    d=PROFILE.LastDay();
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
                && (code!=PRS1_0B)
                && (code!=CPAP_ClearAirway)
                && (code!=CPAP_CSR)
                && (code!=CPAP_RERA)
                && (code!="UserFlag1")
                && (code!=CPAP_NRI)
                && (code!=CPAP_LeakFlag)
                && (code!=CPAP_ExP)
                && (code!=CPAP_FlowLimit)
                && (code!=CPAP_PressurePulse)
                && (code!=CPAP_VSnore)) continue;
            QTreeWidgetItem *mcr;
            if (mcroot.find(code)==mcroot.end()) {
                int cnt=day->count(code);
                total_events+=cnt;
                QString st=schema::channel[code].description();
                if (st.isEmpty())  {
                    st="Fixme "+code;
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
                for (quint32 o=0;o<m.value()[z]->count();o++) {
                    qint64 t=m.value()[z]->time(o);

                    if (code==CPAP_CSR) {
                        t-=float(m.value()[z]->raw(o)/2.0)*1000.0;
                    }
                    QStringList a;
                    QDateTime d=QDateTime::fromTime_t(t/1000);
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
    QTextCharFormat nodata;
    QTextCharFormat cpaponly;
    QTextCharFormat cpapjour;
    QTextCharFormat oxiday;
    QTextCharFormat oxicpap;
    QTextCharFormat jourday;

    cpaponly.setForeground(QBrush(Qt::blue, Qt::SolidPattern));
    cpaponly.setFontWeight(QFont::Normal);
    cpapjour.setForeground(QBrush(Qt::blue, Qt::SolidPattern));
    cpapjour.setFontWeight(QFont::Bold);
    oxiday.setForeground(QBrush(Qt::red, Qt::SolidPattern));
    oxiday.setFontWeight(QFont::Normal);
    oxicpap.setForeground(QBrush(Qt::red, Qt::SolidPattern));
    oxicpap.setFontWeight(QFont::Bold);
    jourday.setForeground(QBrush(QColor("black"), Qt::SolidPattern));
    jourday.setFontWeight(QFont::Bold);
    nodata.setForeground(QBrush(QColor("black"), Qt::SolidPattern));
    nodata.setFontWeight(QFont::Normal);

    bool hascpap=PROFILE.GetDay(date,MT_CPAP)!=NULL;
    bool hasoxi=PROFILE.GetDay(date,MT_OXIMETER)!=NULL;
    bool hasjournal=PROFILE.GetDay(date,MT_JOURNAL)!=NULL;
    if (hascpap) {
        if (hasoxi) {
            ui->calendar->setDateTextFormat(date,oxicpap);
        } else if (hasjournal) {
            ui->calendar->setDateTextFormat(date,cpapjour);
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
    ui->calendar->setSelectedDate(date);
    on_calendar_selectionChanged();
}

void Daily::on_calendar_selectionChanged()
{
    if (previous_date.isValid())
        Unload(previous_date);

    ZombieMeterMoved=false;
    Load(ui->calendar->selectedDate());
    ui->calButton->setText(ui->calendar->selectedDate().toString(Qt::TextDate));
    ui->calendar->setFocus(Qt::ActiveWindowFocusReason);

    if (PROFILE["Units"].toString()=="metric") {
        ui->ouncesSpinBox->setVisible(false);
        ui->weightSpinBox->setDecimals(3);
        ui->weightSpinBox->setSuffix("Kg");
    } else {
        ui->weightSpinBox->setSuffix("lb");
        ui->weightSpinBox->setDecimals(0);
        ui->ouncesSpinBox->setVisible(true);
        ui->ouncesSpinBox->setSuffix("oz");
    }
}
void Daily::ResetGraphLayout()
{
    GraphView->resetLayout();
    //splitter->setSizes(splitter_sizes);

}
void Daily::graphtogglebutton_toggled(bool b)
{
    Q_UNUSED(b)
    for (int i=0;i<GraphView->size();i++) {
        QString title=(*GraphView)[i]->title();
        (*GraphView)[i]->setVisible(GraphToggles[title]->isChecked());
    }
    GraphView->updateScale();
    GraphView->updateGL();
}
void Daily::Load(QDate date)
{
    static Day * lastcpapday=NULL;
    previous_date=date;
    Day *cpap=PROFILE.GetDay(date,MT_CPAP);
    Day *oxi=PROFILE.GetDay(date,MT_OXIMETER);
   // Day *sleepstage=profile->GetDay(date,MT_SLEEPSTAGE);

    if (!PROFILE["MemoryHog"].toBool()) {
        if (lastcpapday && (lastcpapday!=cpap)) {
            for (QVector<Session *>::iterator s=lastcpapday->begin();s!=lastcpapday->end();s++) {
                (*s)->TrashEvents();
            }
        }
    }

    if (cpap && oxi) {
        qint64 len=qAbs(cpap->first() - oxi->first());
        if (len>30000) {
            GraphView->findGraph(tr("Pulse Rate"))->setGroup(1);
            GraphView->findGraph(tr("SpO2"))->setGroup(1);
            GraphView->findGraph(tr("Plethy"))->setGroup(1);
            mainwin->Notify(tr("Oximetry data exists for this day, however it's timestamps are too different, so the Graphs will not be linked."),"",3000);
        } else {
            //mainwin->Notify(tr("Oximetry & CPAP graphs are linked for this day"),"",2000);
            GraphView->findGraph(tr("Pulse Rate"))->setGroup(0);
            GraphView->findGraph(tr("SpO2"))->setGroup(0);
            GraphView->findGraph(tr("Plethy"))->setGroup(0);
        }
    }
    lastcpapday=cpap;

    QString html="<html><head><style type='text/css'>"
    "p,a,td,body { font-family: '"+QApplication::font().family()+"'; }"
    "p,a,td,body { font-size: "+QString::number(QApplication::font().pointSize() + 2)+"px; }"
    "a:link,a:visited { color: inherit; text-decoration: none; font-weight: normal;}"
    "a:hover { background-color: inherit; color: inherit; text-decoration:none; font-weight: bold; }"
    "</style>"
    "<script language='javascript'><!--"
            "func dosession(sessid) {"
            ""
            "}"
    "--></script>"
    "</head>"
    "<body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>"
    "<table cellspacing=0 cellpadding=1 border=0 width='100%'>\n";
    QString tmp;
    //const int gwwidth=240;
    //const int gwheight=100;
    UpdateOXIGraphs(oxi);
    UpdateCPAPGraphs(cpap);
    UpdateEventsTree(ui->treeWidget,cpap);
    snapGV->setDay(cpap);


    GraphView->ResetBounds();
    //snapGV->ResetBounds();
    //GraphView->ResetBounds(1);

    //GraphView->setEmptyText(tr("No Data")); //tr("No data for ")+date.toString(Qt::SystemLocaleLongDate));
    if (!cpap && !oxi) {
        //splitter->setMinimumHeight(0);
        scrollbar->hide();
 //       GraphView->hide();
    } else {
        //NoData->hide();
   //     GraphView->show();
        scrollbar->show();
    }
    GraphView->updateGL();
    snapGV->updateGL();

    //RedrawGraphs();

    for (int i=0;i<GraphView->size();i++) {
        QString title=(*GraphView)[i]->title();
        GraphToggles[title]->setVisible(!(*GraphView)[i]->isEmpty());
    }

    QString epr,modestr;
    //float iap90,eap90;
    CPAPMode mode=MODE_UNKNOWN;
    PRTypes pr;
    QString a;
    bool isBrick=false;
    if (cpap) {
        if (GraphView->isEmpty()) {
            GraphView->setEmptyText(tr("Brick Machine :("));
            isBrick=true;
        } else {
            GraphView->setEmptyText(tr("No Data"));
        }
        mode=(CPAPMode)(int)cpap->settings_max(CPAP_Mode);

        modestr=schema::channel[CPAP_Mode].m_options[mode];

        float ahi=(cpap->count(CPAP_Obstructive)+cpap->count(CPAP_Hypopnea)+cpap->count(CPAP_ClearAirway)+cpap->count(CPAP_Apnea))/cpap->hours();
        float csr=(100.0/cpap->hours())*(cpap->sum(CPAP_CSR)/3600.0);
        float uai=cpap->count(CPAP_Apnea)/cpap->hours();
        float oai=cpap->count(CPAP_Obstructive)/cpap->hours();
        float hi=(cpap->count(CPAP_ExP)+cpap->count(CPAP_Hypopnea))/cpap->hours();
        float cai=cpap->count(CPAP_ClearAirway)/cpap->hours();
        float rei=cpap->count(CPAP_RERA)/cpap->hours();
        float vsi=cpap->count(CPAP_VSnore)/cpap->hours();
        float fli=cpap->count(CPAP_FlowLimit)/cpap->hours();
        float nri=cpap->count(CPAP_NRI)/cpap->hours();
        float lki=cpap->count(CPAP_LeakFlag)/cpap->hours();
        float exp=cpap->count(CPAP_ExP)/cpap->hours();

        //float p90=cpap->p90(CPAP_Pressure);
        //eap90=cpap->p90(CPAP_EPAP);
        //iap90=cpap->p90(CPAP_IPAP);
        QString submodel; //=tr("Unknown Model");

        //html+="<tr><td colspan=4 align=center><i>"+tr("Machine Information")+"</i></td></tr>\n";
        if (cpap->machine->properties.find("SubModel")!=cpap->machine->properties.end())
            submodel=" <br/>"+cpap->machine->properties["SubModel"];
        html+="<tr><td colspan=4 align=center><b>"+cpap->machine->properties["Brand"]+"</b> <br>"+cpap->machine->properties["Model"]+" "+cpap->machine->properties["ModelNumber"]+submodel+"</td></tr>\n";
        if (PROFILE.Exists("ShowSerialNumbers") && PROFILE["ShowSerialNumbers"].toBool()) {
            html+="<tr><td colspan=4 align=center>"+cpap->machine->properties["Serial"]+"</td></tr>\n";
        }
        CPAPMode mode=(CPAPMode)(int)cpap->settings_max(CPAP_Mode);
        html+="<tr><td colspan=4 align=center>Mode: ";

        EventDataType min=cpap->settings_min(CPAP_PressureMin);
        EventDataType max=cpap->settings_max(CPAP_PressureMax);
        if (mode==MODE_CPAP) html+=tr("CPAP")+" "+QString::number(min)+tr("cmH2O");
        else if (mode==MODE_APAP) html+=tr("APAP")+" "+QString::number(min)+"-"+QString::number(max)+tr("cmH2O");
        else if (mode==MODE_BIPAP) html+=tr("Bi-Level");
        else if (mode==MODE_ASV) html+=tr("ASV");
        else html+=tr("Unknown");
        html+="</td></tr>\n";


        html+="<tr><td align='center'><b>"+tr("Date")+"</b></td><td align='center'><b>"+tr("Sleep")+"</b></td><td align='center'><b>"+tr("Wake")+"</b></td><td align='center'><b>"+tr("Hours")+"</b></td></tr>";
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

        QString cs;
        if (!isBrick) {
            if (cpap->machine->GetClass()=="ResMed") {
                cs="4 width='100%' align=center>";
            } else cs="2 width='50%'>";
            html+="<tr><td colspan="+cs+"<table cellspacing=0 cellpadding=1 border=0 width='100%'>"
            "<tr><td align='right' bgcolor='#F88017'><b><font color='black'><a href='nothing' title='"+schema::channel[CPAP_AHI].description()+"'>"+tr("AHI")+"</a></font></b></td><td width=20% bgcolor='#F88017'><b><font color='black'>"+QString().sprintf("%.2f",ahi)+"</font></b></td></tr>\n"
            "<tr><td align='right' bgcolor='#4040ff'><b><font color='white'>&nbsp;<a href='event="+CPAP_Hypopnea+"' title='"+schema::channel[CPAP_Hypopnea].description()+"'>"+tr("Hypopnea")+"</a></font></b></td><td bgcolor='#4040ff'><font color='white'>"+QString().sprintf("%.2f",hi)+"</font></td></tr>\n";
            if (cpap->machine->GetClass()=="ResMed") {
                html+="<tr><td align='right' bgcolor='#208020'><b>&nbsp;<a href='event="+CPAP_Apnea+"' title='"+schema::channel[CPAP_Apnea].description()+"'>"+tr("Unspecified Apnea")+"</a></b></td><td bgcolor='#208020'>"+QString().sprintf("%.2f",uai)+"</td></tr>\n";
            }
            html+="<tr><td align='right' bgcolor='#40afbf'><b>&nbsp;<a href='event="+CPAP_Obstructive+"' title='"+schema::channel[CPAP_Obstructive].description()+"'>"+tr("Obstructive")+"</a></b></td><td bgcolor='#40afbf'>"+QString().sprintf("%.2f",oai)+"</td></tr>\n"
            "<tr><td align='right' bgcolor='#b254cd'><b>&nbsp;<a href='event="+CPAP_ClearAirway+"' title='"+schema::channel[CPAP_ClearAirway].description()+"'>"+tr("Clear Airway")+"</a></b></td><td bgcolor='#b254cd'>"+QString().sprintf("%.2f",cai)+"</td></tr>\n"
            "</table></td>";

            if (cpap->machine->GetClass()=="PRS1") {
                html+="<td colspan=2><table cellspacing=0 cellpadding=1 border=0 width='100%'>"
                    "<tr><td align='right' bgcolor='#ffff80'><b>&nbsp;<a href='event="+CPAP_RERA+"' title='"+schema::channel[CPAP_RERA].description()+"'>"+tr("RERA")+"</a></b></td><td width=20% bgcolor='#ffff80'>"+QString().sprintf("%.2f",rei)+"</td></tr>\n"
                "<tr><td align='right' bgcolor='#404040'><b>&nbsp;<font color='white'><a href='event="+CPAP_FlowLimit+"' title='"+schema::channel[CPAP_FlowLimit].description()+"'>"+tr("Flow Limit")+"</a></font></b></td><td bgcolor='#404040'><font color='white'>"+a.sprintf("%.2f",fli)+"</font></td></tr>\n"
                "<tr><td align='right' bgcolor='#ff4040'><b>&nbsp;<a href='event="+CPAP_VSnore+"'title=' "+schema::channel[CPAP_VSnore].description()+"'>"+tr("Vsnore")+"</a></b></td><td bgcolor='#ff4040'>"+QString().sprintf("%.2f",vsi)+"</td></tr>\n"
                        "<tr><td align='right' bgcolor='#80ff80'><b>&nbsp;<a href='event="+CPAP_CSR+"' title='"+schema::channel[CPAP_CSR].description()+"'>"+tr("PB/CSR")+"</a></b></td><td bgcolor='#80ff80'>"+QString().sprintf("%.2f",csr)+"%</td></tr>\n"
                "</table></td>";
            } else if (cpap->machine->GetClass()=="Intellipap") {
                html+="<td colspan=2><table cellspacing=0 cellpadding=2 border=0 width='100%'>"
                "<tr><td align='right' bgcolor='#ffff80'><b>&nbsp;<a href='event="+CPAP_NRI+"'>"+tr("NRI")+"</a></b></td><td width=20% bgcolor='#ffff80'>"+QString().sprintf("%.2f",nri)+"</td></tr>\n"
                "<tr><td align='right' bgcolor='#404040'><b>&nbsp;<font color='white'><a href='event="+CPAP_Leak+"'>"+tr("Leak Idx")+"</a></font></b></td><td bgcolor='#404040'><font color='white'>"+a.sprintf("%.2f",lki)+"</font></td></tr>\n"
                "<tr><td align='right' bgcolor='#ff4040'><b>&nbsp;<a href='event="+CPAP_VSnore+"'>"+tr("V.Snore")+"</a></b></td><td bgcolor='#ff4040'>"+QString().sprintf("%.2f",vsi)+"</td></tr>\n"
                "<tr><td align='right' bgcolor='#80ff80'><b>&nbsp;<a href='event="+CPAP_ExP+"'>"+tr("Exh.&nbsp;Puff")+"</a></b></td><td bgcolor='#80ff80'>"+QString().sprintf("%.2f",exp)+"</td></tr>\n"
                "</table></td>";

            }


            // Note, this may not be a problem since Qt bug 13622 was discovered
            // as it only relates to text drawing, which the Pie chart does not do
            // ^^ Scratch that.. pie now includes text..

            if (PROFILE["EnableGraphSnapshots"].toBool()) {  // AHI Pie Chart
                if (ahi+rei+fli>0) {
                    html+="</tr>\n"; //<tr><td colspan=4 align=center><i>"+tr("Event Breakdown")+"</i></td></tr>\n";
                    //G_AHI->setFixedSize(gwwidth,120);
                    //mainwin->snapshotGraph()->setPrintScaleX(1);
                    //mainwin->snapshotGraph()->setPrintScaleY(1);
                    QPixmap pixmap=snapGV->renderPixmap(172,172);
                    QByteArray byteArray;
                    QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
                    buffer.open(QIODevice::WriteOnly);
                    pixmap.save(&buffer, "PNG");
                    html += "<tr><td colspan=4 align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\"></td></tr>\n";
                } else {
                    html += "<tr><td colspan=4 align=center><img src=\"qrc:/docs/0.0.gif\"></td></tr>\n";
                }
            }

            html+="</table>";
            html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";
            if (cpap || oxi) {
                html+="<tr height='2'><td colspan=5 height='2'><hr></td></tr>\n";

                //html+=("<tr><td colspan=4 align=center>&nbsp;</td></tr>\n");

                html+=("<tr><td> </td><td><b>Min</b></td><td><b>Avg</b></td><td><b>90%</b></td><td><b>Max</b></td></tr>");
                ChannelID chans[]={
                    CPAP_Pressure,CPAP_EPAP,CPAP_IPAP,CPAP_PS,CPAP_PTB,
                    CPAP_MinuteVent,CPAP_AHI, CPAP_RespRate, CPAP_RespEvent,CPAP_FLG,
                    CPAP_Leak, CPAP_LeakTotal, CPAP_Snore,CPAP_IE,CPAP_Ti,CPAP_Te, CPAP_TgMV,
                    CPAP_TidalVolume, OXI_Pulse, OXI_SPO2
                };
                int numchans=sizeof(chans)/sizeof(ChannelID);
                int suboffset=0;
                for (int i=0;i<numchans;i++) {

                    ChannelID code=chans[i];
                    if (cpap && cpap->channelHasData(code)) {
                        //if (code==CPAP_LeakTotal) suboffset=PROFILE["IntentionalLeak"].toDouble(); else suboffset=0;
                        QString tooltip=schema::channel[code].description();
                        if (!schema::channel[code].units().isEmpty()) tooltip+=" ("+schema::channel[code].units()+")";
                        html+="<tr><td align=left><a href='graph="+code+"' title='"+tooltip+"'>"+schema::channel[code].label()+"</a>";
                        html+="</td><td>"+a.sprintf("%.2f",cpap->Min(code)-suboffset);
                        html+="</td><td>"+a.sprintf("%.2f",cpap->wavg(code)-suboffset);
                        html+="</td><td>"+a.sprintf("%.2f",cpap->p90(code)-suboffset);
                        html+="</td><td>"+a.sprintf("%.2f",cpap->Max(code)-suboffset);
                        html+="</td><tr>";
                    }
                    if (oxi && oxi->channelHasData(code)) {
                        QString tooltip=schema::channel[code].description();
                        if (!schema::channel[code].units().isEmpty()) tooltip+=" ("+schema::channel[code].units()+")";
                        html+="<tr><td align=left><a href='graph="+code+"' title='"+tooltip+"'>"+schema::channel[code].label()+"</a>";
                        html+="</td><td>"+a.sprintf("%.2f",oxi->Min(code));
                        html+="</td><td>"+a.sprintf("%.2f",oxi->wavg(code));
                        html+="</td><td>"+a.sprintf("%.2f",oxi->p90(code));
                        html+="</td><td>"+a.sprintf("%.2f",oxi->Max(code));
                        html+="</td><tr>";
                    }
                }
            }
        } else {
            html+="<tr><td colspan='5' align='center'><b><h2>"+tr("BRICK :(")+"</h2></b></td></tr>";
            html+="<tr><td colspan='5' align='center'><i>"+tr("Sorry, your machine does not record data.")+"</i></td></tr>\n";
            html+="<tr><td colspan='5' align='center'><i>"+tr("Complain to your Equipment Provider!")+"</i></td></tr>\n";
            html+="<tr><td colspan='5'>&nbsp;</td></tr>\n";
        }
    } else {
        html+="<tr><td colspan=5 align=center><i>"+tr("No data available")+"</i></td></tr>";
        html+="<tr><td colspan=5>&nbsp;</td></tr>\n";

    }
    html+="</table><hr height=2/>";

    if (cpap) {

      //  if ((*profile)["EnableGraphSnapshots"].toBool()) {
            /*if (cpap->channelExists(CPAP_Pressure)) {
                html+=("<tr><td colspan=4 align=center><i>")+tr("Time@Pressure")+("</i></td></tr>\n");
                //TAP->setFixedSize(gwwidth,30);
                QPixmap pixmap=TAP->renderPixmap(200,30);
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
        html+="<table cellpadding=0 cellspacing=0 border=0 width=100%>";
        if (cpap->machine->GetClass()=="PRS1") {
            int i=cpap->settings_max(PRS1_FlexMode);
            int j=cpap->settings_max(PRS1_FlexSet);
            QString flexstr=(i>1) ? schema::channel[PRS1_FlexMode].option(i)+" "+schema::channel[PRS1_FlexSet].option(j) : "None";

            html+="<tr><td colspan=4>"+tr("Pressure Relief:")+" "+flexstr+"</td></tr>";

            i=cpap->settings_max(PRS1_HumidSetting);
            QString humid=(i==0) ? tr("Off") : "x"+QString::number(i);
            html+="<tr><td colspan=4>"+tr("Humidifier Setting:")+" "+humid+"</td></tr>";
        } else if (cpap->machine->GetClass()=="ResMed") {
            int epr=cpap->settings_max("EPR");
            int epr2=cpap->settings_max("EPRSet");
            html+="<tr><td colspan=4>"+tr("EPR Setting:")+" "+QString::number(epr)+" / "+QString::number(epr2)+"</td></tr>";
            //epr=schema::channel[PRS1_FlexSet].optionString(pr)+QString(" x%1").arg((int)cpap->settings_max(PRS1_FlexSet));

        }
        html+="</table><hr height=2>";

        //}
        html+="<table cellpadding=0 cellspacing=0 border=0 width=100%>";
        QDateTime fd,ld;
        bool corrupted_waveform=false;
        QString tooltip;
        if (cpap) {
            html+="<tr><td align=left><b>"+tr("SessionID")+"</b></td><td align=center><b>"+tr("Date")+"</b></td><td align=center><b>"+tr("Start")+"</b></td><td align=center><b>"+tr("End")+"</b></td></tr>";
            html+="<tr><td align=left colspan=4><i>"+tr("CPAP Sessions")+"</i></td></tr>";
            for (QVector<Session *>::iterator s=cpap->begin();s!=cpap->end();s++) {
                fd=QDateTime::fromTime_t((*s)->first()/1000L);
                ld=QDateTime::fromTime_t((*s)->last()/1000L);
                int len=(*s)->length()/1000L;
                int h=len/3600;
                int m=(len/60) % 60;
                int s1=len % 60;
                QHash<ChannelID,QVariant>::iterator i=(*s)->settings.find("BrokenWaveform");
                tooltip=cpap->machine->GetClass()+" "+tr("CPAP")+" "+QString().sprintf("%2ih&nbsp;%2im&nbsp;%2is",h,m,s1);
                // tooltip needs to lookup language.. :-/

                if ((i!=(*s)->settings.end()) && i.value().toBool()) corrupted_waveform=true;
                tmp.sprintf(("<tr><td align=left><a href='cpap=%i' title='"+tooltip+"'>%08i</a></td><td align=center>"+fd.date().toString(Qt::SystemLocaleShortDate)+"</td><td align=center>"+fd.toString("HH:mm ")+"</td><td align=center>"+ld.toString("HH:mm")+"</td></tr>").toLatin1(),(*s)->session(),(*s)->session());
                html+=tmp;
            }
            //if (oxi) html+="<tr><td colspan=4><hr></td></tr>";
        }
        if (oxi) {
            html+="<tr><td align=left colspan=4><i>"+tr("Oximetry Sessions")+"</i></td></tr>";
            for (QVector<Session *>::iterator s=oxi->begin();s!=oxi->end();s++) {
                fd=QDateTime::fromTime_t((*s)->first()/1000L);
                ld=QDateTime::fromTime_t((*s)->last()/1000L);
                int len=(*s)->length()/1000L;
                int h=len/3600;
                int m=(len/60) % 60;
                int s1=len % 60;
                QHash<ChannelID,QVariant>::iterator i=(*s)->settings.find("BrokenWaveform");
                tooltip=oxi->machine->GetClass()+" "+tr("Oximeter")+" "+QString().sprintf("%2ih,&nbsp;%2im,&nbsp;%2is",h,m,s1);


                if ((i!=(*s)->settings.end()) && i.value().toBool()) corrupted_waveform=true;
                tmp.sprintf(("<tr><td align=left><a href='oxi=%i' title='"+tooltip+"'>%08i</a></td><td align=center>"+fd.date().toString(Qt::SystemLocaleShortDate)+"</td><td align=center>"+fd.toString("HH:mm ")+"</td><td align=center>"+ld.toString("HH:mm")+"</td></tr>").toLatin1(),(*s)->session(),(*s)->session());
                html+=tmp;
            }
        }
        html+="</table>";
        if (corrupted_waveform) {
            html+="<hr><div align=center><i>"+tr("One or more waveform record for this session had faulty source data. Some waveform overlay points may not match up correctly.")+"</i></div>";
        }
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

        if (journal->settings.contains("Weight")) {
            double kg=journal->settings["Weight"].toDouble(&ok);
            if (PROFILE["Units"].toString()=="metric") {
                ui->weightSpinBox->setDecimals(3);
                ui->weightSpinBox->blockSignals(true);
                ui->weightSpinBox->setValue(kg);
                ui->weightSpinBox->blockSignals(false);
                ui->ouncesSpinBox->setVisible(false);
                ui->weightSpinBox->setSuffix("Kg");
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

                ui->weightSpinBox->setSuffix("lb");
                ui->weightSpinBox->setDecimals(0);
                ui->ouncesSpinBox->setVisible(true);
                ui->ouncesSpinBox->setSuffix("oz");
            }
            double height=PROFILE["Height"].toDouble(&ok)/100.0;
            if (height>0 && kg>0) {
                double bmi=kg/(height*height);
                ui->BMI->setVisible(true);
                ui->BMIlabel->setVisible(true);
                ui->BMI->display(bmi);
            }
        }

        if (journal->settings.contains("ZombieMeter")) {
            ui->ZombieMeter->blockSignals(true);
            ui->ZombieMeter->setValue(journal->settings["ZombieMeter"].toDouble(&ok));
            ui->ZombieMeter->blockSignals(false);
        }

        if (journal->settings.contains("BookmarkStart")) {
            QVariantList start=journal->settings["BookmarkStart"].toList();
            QVariantList end=journal->settings["BookmarkEnd"].toList();
            QStringList notes=journal->settings["BookmarkNotes"].toStringList();

            ui->bookmarkTable->blockSignals(true);

            bool ok;
            for (int i=0;i<start.size();i++) {
                qint64 st=start.at(i).toLongLong(&ok);
                qint64 et=end.at(i).toLongLong(&ok);

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
            }
            ui->bookmarkTable->blockSignals(false);

        }
    }

}

void Daily::UnitsChanged()
{
    double kg;
    if (PROFILE["Units"].toString()!="metric") {
        kg=ui->weightSpinBox->value();
        float ounces=(kg*1000.0)/ounce_convert;
        int pounds=ounces/16;
        float oz=fmodf(ounces,16);
        ui->weightSpinBox->setValue(pounds);
        ui->ouncesSpinBox->setValue(oz);

        ui->weightSpinBox->setDecimals(0);
        ui->weightSpinBox->setSuffix("lb");
        ui->ouncesSpinBox->setVisible(true);
        ui->ouncesSpinBox->setSuffix("oz");
    } else {
        kg=(ui->weightSpinBox->value()*(ounce_convert*16.0))+(ui->ouncesSpinBox->value()*ounce_convert);
        kg/=1000.0;
        ui->weightSpinBox->setDecimals(3);
        ui->weightSpinBox->setValue(kg);
        ui->ouncesSpinBox->setVisible(false);
        ui->weightSpinBox->setSuffix("Kg");
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
            mainwin->getOverview()->ReloadGraphs();
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
    Machine *m=PROFILE.GetMachine(MT_JOURNAL);
    if (!m) {
        m=new Machine(p_profile,0);
        m->SetClass("Journal");
        m->properties["Brand"]="Virtual";
        m->SetType(MT_JOURNAL);
        PROFILE.AddMachine(m);
    }
    Session *sess=new Session(m,0);
    QDateTime dt(date,QTime(17,0));
    //dt.setDate(date);
    //dt.setTime(QTime(17,0)); //5pm to make sure it goes in the right day
    sess->set_first(qint64(dt.toTime_t())*1000L);
    dt=dt.addSecs(3600);
    sess->set_last(qint64(dt.toTime_t())*1000L);
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
    GraphView->updateGL();
}

void Daily::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    QDateTime d;
    if (!item->text(1).isEmpty()) {
        d=d.fromString(item->text(1),"yyyy-MM-dd HH:mm:ss");
        int winsize=PROFILE["EventViewSize"].toInt()*60;

        double st=qint64((d.addSecs(-(winsize/2))).toTime_t())*1000L;
        double et=qint64((d.addSecs(winsize/2)).toTime_t())*1000L;
        gGraph *g=GraphView->findGraph("Event Flags");
        if (!g) return;
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
    PROFILE["EventViewSize"]=value;

    int winsize=PROFILE["EventViewSize"].toInt()*60;

    gGraph *g=GraphView->findGraph("Event Flags");
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

    QTableWidgetItem *it=ui->bookmarkTable->item(row,1);
    bool ok;
    st=it->data(Qt::UserRole).toLongLong(&ok);
    et=it->data(Qt::UserRole+1).toLongLong(&ok);
    GraphView->SetXBounds(st,et);
    GraphView->updateGL();
}

void Daily::on_addBookmarkButton_clicked()
{
    qint64 st,et;
    ui->bookmarkTable->blockSignals(true);
    GraphView->GetXBounds(st,et);
    QDateTime d=QDateTime::fromTime_t(st/1000L);
    int row=ui->bookmarkTable->rowCount();
    ui->bookmarkTable->insertRow(row);
    QTableWidgetItem *tw=new QTableWidgetItem("Bookmark at "+d.time().toString("HH:mm:ss"));
    QTableWidgetItem *dw=new QTableWidgetItem(d.time().toString("HH:mm:ss"));
    dw->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->bookmarkTable->setItem(row,0,dw);
    ui->bookmarkTable->setItem(row,1,tw);
    tw->setData(Qt::UserRole,st);
    tw->setData(Qt::UserRole+1,et);

    ui->bookmarkTable->blockSignals(false);
    update_Bookmarks();

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

    journal->settings["BookmarkStart"]=start;
    journal->settings["BookmarkEnd"]=end;
    journal->settings["BookmarkNotes"]=notes;
    journal->SetChanged(true);
    BookmarksChanged=true;
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
}
void Daily::on_ZombieMeter_valueChanged(int action)
{
    Q_UNUSED(action);
    ZombieMeterMoved=true;
    Session *journal=GetJournalSession(previous_date);
    if (!journal) {
        journal=CreateJournalSession(previous_date);
    }
    journal->settings["ZombieMeter"]=ui->ZombieMeter->value();
    journal->SetChanged(true);

    gGraph *g;
    if (mainwin->getOverview()) {
        g=mainwin->getOverview()->graphView()->findGraph("Zombie");
        if (g) g->setDay(NULL);
        //mainwin->getOverview()->RedrawGraphs();
    }
}

void Daily::on_bookmarkTable_itemChanged(QTableWidgetItem *item)
{
    Q_UNUSED(item);
    update_Bookmarks();
}
void Daily::on_weightSpinBox_valueChanged(double arg1)
{
    bool ok;
    double height=PROFILE["Height"].toDouble(&ok)/100.0;
    Session *journal=GetJournalSession(previous_date);
    if (!journal) {
        journal=CreateJournalSession(previous_date);
    }

    double kg;
    if (PROFILE["Units"].toString()=="metric")
        kg=arg1;
    else {
        kg=(arg1*pound_convert) + (ui->ouncesSpinBox->value()*ounce_convert);
    }
    journal->settings["Weight"]=kg;
    gGraphView *gv=mainwin->getOverview()->graphView();
    gGraph *g;
    if (gv) {
        g=gv->findGraph("Weight");
        if (g) g->setDay(NULL);
    }
    if ((height>0) && (kg>0)) {
        double bmi=kg/(height * height);
        ui->BMI->display(bmi);
        ui->BMI->setVisible(true);
        journal->settings["BMI"]=bmi;
        if (gv) {
            g=gv->findGraph("BMI");
            if (g) g->setDay(NULL);
        }
    }
    journal->SetChanged(true);
}
void Daily::on_ouncesSpinBox_valueChanged(int arg1)
{
    bool ok;
    Session *journal=GetJournalSession(previous_date);
    if (!journal) {
        journal=CreateJournalSession(previous_date);
    }
    double height=PROFILE["Height"].toDouble(&ok)/100.0;
    double kg=(ui->weightSpinBox->value()*pound_convert) + (arg1*ounce_convert);
    journal->settings["Weight"]=kg;


    gGraph *g;
    if (mainwin->getOverview()) {
        g=mainwin->getOverview()->graphView()->findGraph("Weight");
        if (g) g->setDay(NULL);
    }

    if ((height>0) && (kg>0)) {
        double bmi=kg/(height * height);
        ui->BMI->display(bmi);
        ui->BMI->setVisible(true);

        journal->settings["BMI"]=bmi;
        if (mainwin->getOverview()) {
            g=mainwin->getOverview()->graphView()->findGraph("BMI");
            if (g) g->setDay(NULL);
        }
    }
    journal->SetChanged(true);
}
QString Daily::GetDetailsText()
{
    ui->webView->triggerPageAction(QWebPage::SelectAll);
    QString text=ui->webView->page()->selectedText();
    ui->webView->triggerPageAction(QWebPage::MoveToEndOfDocument);
    ui->webView->triggerPageAction(QWebPage::SelectEndOfDocument);
    return text;
}
