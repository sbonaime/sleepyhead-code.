/* Daily Panel
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

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
#include <QFontMetrics>
#include <QLabel>

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
#include "Graphs/gdailysummary.h"
#include "Graphs/MinutesAtPressure.h"

//extern QProgressBar *qprogress;
extern MainWindow * mainwin;

// This was Sean Stangl's idea.. but I couldn't apply that patch.
inline QString channelInfo(ChannelID code) {
    return schema::channel[code].fullname()+"\n"+schema::channel[code].description()+"\n("+schema::channel[code].units()+")";
}

void Daily::setCalendarVisible(bool visible)
{
    on_calButton_toggled(visible);
}

void Daily::setSidebarVisible(bool visible)
{
    QList<int> a;

    int panel_width = visible ? 370 : 0;
    a.push_back(panel_width);
    a.push_back(this->width() - panel_width);
    ui->splitter_2->setStretchFactor(1,1);
    ui->splitter_2->setSizes(a);
    ui->splitter_2->setStretchFactor(1,1);
}

Daily::Daily(QWidget *parent,gGraphView * shared)
    :QWidget(parent), ui(new Ui::Daily)
{
    ui->setupUi(this);

    // Remove Incomplete Extras Tab
    //ui->tabWidget->removeTab(3);

    ZombieMeterMoved=false;
    BookmarksChanged=false;

    lastcpapday=nullptr;

    setSidebarVisible(true);

    layout=new QHBoxLayout();
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setContentsMargins(0,0,0,0);

    dateDisplay=new MyLabel(this);
    dateDisplay->setAlignment(Qt::AlignCenter);
    QFont font = dateDisplay->font();
    font.setPointSizeF(font.pointSizeF()*1.3F);
    dateDisplay->setFont(font);
    QPalette palette = dateDisplay->palette();
    palette.setColor(QPalette::Base, Qt::blue);
    dateDisplay->setPalette(palette);
    //dateDisplay->setTextFormat(Qt::RichText);
    ui->sessionBarLayout->addWidget(dateDisplay,1);

//    const bool sessbar_under_graphs=false;
//    if (sessbar_under_graphs) {
//        ui->sessionBarLayout->addWidget(sessbar,1);
//    } else {
//        ui->splitter->insertWidget(2,sessbar);
//        sessbar->setMaximumHeight(sessbar->height());
//        sessbar->setMinimumHeight(sessbar->height());
//    }


    ui->calNavWidget->setMaximumHeight(ui->calNavWidget->height());
    ui->calNavWidget->setMinimumHeight(ui->calNavWidget->height());
    sessbar=nullptr;

    webView=new MyWebView(this);



    QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, true);

    ui->tabWidget->insertTab(0,webView,QIcon(),"Details");

    ui->graphFrame->setLayout(layout);
    //ui->graphMainArea->setLayout(layout);

    ui->graphMainArea->setAutoFillBackground(false);

    GraphView=new gGraphView(ui->graphFrame,shared);
    GraphView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    snapGV=new gGraphView(GraphView);
    snapGV->setMinimumSize(172,172);
    snapGV->hideSplitter();
    snapGV->hide();

    scrollbar=new MyScrollBar(ui->graphFrame);
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

    int default_height = p_profile->appearance->graphHeight();

    gGraph *GAHI = nullptr,
//            *TAP = nullptr,
            *SF = nullptr,
            *AHI = nullptr;

    const QString STR_GRAPH_DailySummary = "DailySummary";
    const QString STR_GRAPH_TAP = "TimeAtPressure";

//    gGraph * SG;
//    graphlist[STR_GRAPH_DailySummary] = SG = new gGraph(STR_GRAPH_DailySummary, GraphView, QObject::tr("Summary"), QObject::tr("Summary of this daily information"), default_height);
//    SG->AddLayer(new gLabelArea(nullptr),LayerLeft,gYAxis::Margin);
//    SG->AddLayer(new gDailySummary());

    graphlist[STR_GRAPH_SleepFlags] = SF = new gGraph(STR_GRAPH_SleepFlags, GraphView, STR_TR_EventFlags, STR_TR_EventFlags, default_height);
    SF->setPinned(true);

    ChannelID cpapcodes[] = {
        CPAP_FlowRate, CPAP_MaskPressure, CPAP_Pressure, CPAP_Leak, CPAP_Snore,  CPAP_FLG, CPAP_RespRate,
        CPAP_TidalVolume, CPAP_MinuteVent,CPAP_PTB, CPAP_RespEvent, CPAP_Ti, CPAP_Te,
        CPAP_IE, ZEO_SleepStage, POS_Inclination, POS_Orientation, CPAP_Test1
    };

    int cpapsize = sizeof(cpapcodes) / sizeof(ChannelID);

    ChannelID oxicodes[] = {
        OXI_Pulse, OXI_SPO2, OXI_Perf, OXI_Plethy
    };
    int oxisize = sizeof(oxicodes) / sizeof(ChannelID);


    for (int i=0; i < cpapsize; ++i) {
        ChannelID code = cpapcodes[i];
        graphlist[schema::channel[code].code()] = new gGraph(schema::channel[code].code(), GraphView, schema::channel[code].label(), channelInfo(code), default_height);
    }

    //int oxigrp=p_profile->ExistsAndTrue("SyncOximetry") ? 0 : 1; // Contemplating killing this setting...
    for (int i=0; i < oxisize; ++i) {
        ChannelID code = oxicodes[i];
        graphlist[schema::channel[code].code()] = new gGraph(schema::channel[code].code(), GraphView, schema::channel[code].label(), channelInfo(code), default_height);
    }

    if (p_profile->general->calculateRDI()) {
        AHI=new gGraph("AHI", GraphView,STR_TR_RDI, channelInfo(CPAP_RDI), default_height);
    } else {
        AHI=new gGraph("AHI", GraphView,STR_TR_AHI, channelInfo(CPAP_AHI), default_height);
    }

    graphlist["AHI"] = AHI;

    graphlist[STR_GRAPH_EventBreakdown] = GAHI = new gGraph(STR_GRAPH_EventBreakdown, snapGV,tr("Breakdown"),tr("events"),172);
    gSegmentChart * evseg=new gSegmentChart(GST_Pie);
    evseg->AddSlice(CPAP_Hypopnea,QColor(0x40,0x40,0xff,0xff),STR_TR_H);
    evseg->AddSlice(CPAP_Apnea,QColor(0x20,0x80,0x20,0xff),STR_TR_UA);
    evseg->AddSlice(CPAP_Obstructive,QColor(0x40,0xaf,0xbf,0xff),STR_TR_OA);
    evseg->AddSlice(CPAP_ClearAirway,QColor(0xb2,0x54,0xcd,0xff),STR_TR_CA);
    evseg->AddSlice(CPAP_RERA,QColor(0xff,0xff,0x80,0xff),STR_TR_RE);
    evseg->AddSlice(CPAP_NRI,QColor(0x00,0x80,0x40,0xff),STR_TR_NR);
    evseg->AddSlice(CPAP_FlowLimit,QColor(0x40,0x40,0x40,0xff),STR_TR_FL);
    evseg->AddSlice(CPAP_SensAwake,QColor(0x40,0xC0,0x40,0xff),STR_TR_SA);
    if (p_profile->cpap->userEventPieChart()) {
        evseg->AddSlice(CPAP_UserFlag1,QColor(0xe0,0xe0,0xe0,0xff),tr("UF1"));
        evseg->AddSlice(CPAP_UserFlag2,QColor(0xc0,0xc0,0xe0,0xff),tr("UF2"));
    }



    GAHI->AddLayer(evseg);
    GAHI->setMargins(0,0,0,0);

    gFlagsGroup *fg=new gFlagsGroup();
    SF->AddLayer(fg);


    SF->setBlockZoom(true);
    SF->AddLayer(new gShadowArea());

    SF->AddLayer(new gLabelArea(fg),LayerLeft,gYAxis::Margin);

    //SF->AddLayer(new gFooBar(),LayerBottom,0,1);
    SF->AddLayer(new gXAxis(COLOR_Text,false),LayerBottom,0,gXAxis::Margin);


    // The following list contains graphs that don't have standard xgrid/yaxis labels
    QStringList skipgraph;
    skipgraph.push_back(STR_GRAPH_EventBreakdown);
    skipgraph.push_back(STR_GRAPH_SleepFlags);
    skipgraph.push_back(STR_GRAPH_DailySummary);
    skipgraph.push_back(STR_GRAPH_TAP);

    QHash<QString, gGraph *>::iterator it;

    for (it = graphlist.begin(); it != graphlist.end(); ++it) {
        if (skipgraph.contains(it.key())) continue;
        it.value()->AddLayer(new gXGrid());
    }


    gLineChart *l;
    l=new gLineChart(CPAP_FlowRate,false,false);

    gGraph *FRW = graphlist[schema::channel[CPAP_FlowRate].code()];

    // Then the graph itself
    FRW->AddLayer(l);


//    FRW->AddLayer(AddOXI(new gLineOverlayBar(OXI_SPO2Drop, COLOR_SPO2Drop, STR_TR_O2)));

    bool square=p_profile->appearance->squareWavePlots();
    gLineChart *pc=new gLineChart(CPAP_Pressure, square);
    graphlist[schema::channel[CPAP_Pressure].code()]->AddLayer(pc);

  //  graphlist[schema::channel[CPAP_Pressure].code()]->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_Ramp, COLOR_Ramp, schema::channel[CPAP_Ramp].label(), FT_Span)));

    pc->addPlot(CPAP_EPAP, square);
    pc->addPlot(CPAP_IPAPLo, square);
    pc->addPlot(CPAP_IPAP, square);
    pc->addPlot(CPAP_IPAPHi, square);

    gGraph * TAP2;
    graphlist[STR_GRAPH_TAP] = TAP2 = new gGraph(STR_GRAPH_TAP, GraphView, QObject::tr("By Pressure"), QObject::tr("Statistics at Pressure"), default_height);
    MinutesAtPressure * map;
    TAP2->AddLayer(map = new MinutesAtPressure());
    TAP2->AddLayer(new gLabelArea(map),LayerLeft,gYAxis::Margin);
    TAP2->setBlockSelect(true);

    if (p_profile->general->calculateRDI()) {
        AHI->AddLayer(new gLineChart(CPAP_RDI, square));
//        AHI->AddLayer(AddCPAP(new AHIChart(QColor("#37a24b"))));
    } else {
        AHI->AddLayer(new gLineChart(CPAP_AHI, square));
    }

    // this is class wide because the leak redline can be reset in preferences..
    // Better way would be having a search for linechart layers in graphlist[...]
    gLineChart *leakchart=new gLineChart(CPAP_Leak, square);
  //  graphlist[schema::channel[CPAP_Leak].code()]->AddLayer(AddCPAP(new gLineOverlayBar(CPAP_LargeLeak, COLOR_LargeLeak, STR_TR_LL, FT_Span)));

    leakchart->addPlot(CPAP_LeakTotal, square);
    leakchart->addPlot(CPAP_MaxLeak, square);
//    schema::channel[CPAP_Leak].setUpperThresholdColor(Qt::red);
//    schema::channel[CPAP_Leak].setLowerThresholdColor(Qt::green);

    graphlist[schema::channel[CPAP_Leak].code()]->AddLayer(leakchart);
    //LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_Leak, COLOR_Leak,square)));
    //LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_MaxLeak, COLOR_MaxLeak,square)));
    graphlist[schema::channel[CPAP_Snore].code()]->AddLayer(new gLineChart(CPAP_Snore, true));

    graphlist[schema::channel[CPAP_PTB].code()]->AddLayer(new gLineChart(CPAP_PTB, square));
    graphlist[schema::channel[CPAP_Test1].code()]->AddLayer(new gLineChart(CPAP_Test1, false));


    gLineChart *lc = nullptr;
    graphlist[schema::channel[CPAP_MaskPressure].code()]->AddLayer(new gLineChart(CPAP_MaskPressure, false));
    graphlist[schema::channel[CPAP_RespRate].code()]->AddLayer(lc=new gLineChart(CPAP_RespRate, false));

    graphlist[schema::channel[POS_Inclination].code()]->AddLayer(new gLineChart(POS_Inclination));
    graphlist[schema::channel[POS_Orientation].code()]->AddLayer(new gLineChart(POS_Orientation));

    graphlist[schema::channel[CPAP_MinuteVent].code()]->AddLayer(lc=new gLineChart(CPAP_MinuteVent, false));
    lc->addPlot(CPAP_TgMV, square);

    graphlist[schema::channel[CPAP_TidalVolume].code()]->AddLayer(lc=new gLineChart(CPAP_TidalVolume, false));
    //lc->addPlot(CPAP_Test2,COLOR_DarkYellow,square);

    //graphlist[schema::channel[CPAP_TidalVolume].code()]->AddLayer(AddCPAP(new gLineChart("TidalVolume2", square)));
    graphlist[schema::channel[CPAP_FLG].code()]->AddLayer(new gLineChart(CPAP_FLG, true));
    //graphlist[schema::channel[CPAP_RespiratoryEvent].code()]->AddLayer(AddCPAP(new gLineChart(CPAP_RespiratoryEvent, true)));
    graphlist[schema::channel[CPAP_IE].code()]->AddLayer(lc=new gLineChart(CPAP_IE, false));
    graphlist[schema::channel[CPAP_Te].code()]->AddLayer(lc=new gLineChart(CPAP_Te, false));
    graphlist[schema::channel[CPAP_Ti].code()]->AddLayer(lc=new gLineChart(CPAP_Ti, false));
    //lc->addPlot(CPAP_Test2,COLOR:DarkYellow,square);

    graphlist[schema::channel[ZEO_SleepStage].code()]->AddLayer(new gLineChart(ZEO_SleepStage, true));

//    gLineOverlaySummary *los1=new gLineOverlaySummary(STR_UNIT_EventsPerHour,5,-4);
//    gLineOverlaySummary *los2=new gLineOverlaySummary(STR_UNIT_EventsPerHour,5,-4);
//    graphlist[schema::channel[OXI_Pulse].code()]->AddLayer(AddOXI(los1->add(new gLineOverlayBar(OXI_PulseChange, COLOR_PulseChange, STR_TR_PC,FT_Span))));
//    graphlist[schema::channel[OXI_Pulse].code()]->AddLayer(AddOXI(los1));
//    graphlist[schema::channel[OXI_SPO2].code()]->AddLayer(AddOXI(los2->add(new gLineOverlayBar(OXI_SPO2Drop, COLOR_SPO2Drop, STR_TR_O2,FT_Span))));
//    graphlist[schema::channel[OXI_SPO2].code()]->AddLayer(AddOXI(los2));

    graphlist[schema::channel[OXI_Pulse].code()]->AddLayer(new gLineChart(OXI_Pulse, square));
    graphlist[schema::channel[OXI_SPO2].code()]->AddLayer(new gLineChart(OXI_SPO2, true));
    graphlist[schema::channel[OXI_Perf].code()]->AddLayer(new gLineChart(OXI_Perf, false));
    graphlist[schema::channel[OXI_Plethy].code()]->AddLayer(new gLineChart(OXI_Plethy, false));


    // Fix me
//    gLineOverlaySummary *los3=new gLineOverlaySummary(STR_UNIT_EventsPerHour,5,-4);
//    graphlist["INTPULSE"]->AddLayer(AddCPAP(los3->add(new gLineOverlayBar(OXI_PulseChange, COLOR_PulseChange, STR_TR_PC,FT_Span))));
//    graphlist["INTPULSE"]->AddLayer(AddCPAP(los3));
//    graphlist["INTPULSE"]->AddLayer(AddCPAP(new gLineChart(OXI_Pulse, square)));
//    gLineOverlaySummary *los4=new gLineOverlaySummary(STR_UNIT_EventsPerHour,5,-4);
//    graphlist["INTSPO2"]->AddLayer(AddCPAP(los4->add(new gLineOverlayBar(OXI_SPO2Drop, COLOR_SPO2Drop, STR_TR_O2,FT_Span))));
//    graphlist["INTSPO2"]->AddLayer(AddCPAP(los4));
//    graphlist["INTSPO2"]->AddLayer(AddCPAP(new gLineChart(OXI_SPO2, true)));

    graphlist[schema::channel[CPAP_PTB].code()]->setForceMaxY(100);
    graphlist[schema::channel[OXI_SPO2].code()]->setForceMaxY(100);

    for (it = graphlist.begin(); it != graphlist.end(); ++it) {
        if (skipgraph.contains(it.key())) continue;

        it.value()->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
        it.value()->AddLayer(new gXAxis(),LayerBottom,0,gXAxis::Margin);
    }

    if (p_profile->cpap->showLeakRedline()) {
        schema::channel[CPAP_Leak].setUpperThreshold(p_profile->cpap->leakRedline());
    } else {
        schema::channel[CPAP_Leak].setUpperThreshold(0); // switch it off
    }

    layout->layout();

    QTextCharFormat format = ui->calendar->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(COLOR_Black, Qt::SolidPattern));
    ui->calendar->setWeekdayTextFormat(Qt::Saturday, format);
    ui->calendar->setWeekdayTextFormat(Qt::Sunday, format);

    Qt::DayOfWeek dow=firstDayOfWeekFromLocale();

    ui->calendar->setFirstDayOfWeek(dow);

    ui->tabWidget->setCurrentWidget(webView);

    webView->settings()->setFontSize(QWebSettings::DefaultFontSize,QApplication::font().pointSize());
    webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    connect(webView,SIGNAL(linkClicked(QUrl)),this,SLOT(Link_clicked(QUrl)));

    int ews=p_profile->general->eventWindowSize();
    ui->evViewSlider->setValue(ews);
    ui->evViewLCD->display(ews);


    icon_on=new QIcon(":/icons/session-on.png");
    icon_off=new QIcon(":/icons/session-off.png");

    ui->splitter->setVisible(false);

    if (p_profile->general->unitSystem()==US_Archiac) {
        ui->weightSpinBox->setSuffix(STR_UNIT_POUND);
        ui->weightSpinBox->setDecimals(0);
        ui->ouncesSpinBox->setVisible(true);
        ui->ouncesSpinBox->setSuffix(STR_UNIT_OUNCE);
    } else {
        ui->ouncesSpinBox->setVisible(false);
        ui->weightSpinBox->setDecimals(3);
        ui->weightSpinBox->setSuffix(STR_UNIT_KG);
    }
    GraphView->setEmptyText(STR_Empty_NoData);
    previous_date=QDate();

    ui->calButton->setChecked(p_profile->appearance->calendarVisible() ? Qt::Checked : Qt::Unchecked);
    on_calButton_toggled(p_profile->appearance->calendarVisible());

    GraphView->resetLayout();
    GraphView->LoadSettings("Daily");

    connect(GraphView, SIGNAL(updateCurrentTime(double)), this, SLOT(on_LineCursorUpdate(double)));
    connect(GraphView, SIGNAL(updateRange(double,double)), this, SLOT(on_RangeUpdate(double,double)));
    connect(GraphView, SIGNAL(GraphsChanged()), this, SLOT(updateGraphCombo()));

    GraphView->setEmptyImage(QPixmap(":/docs/sheep.png"));
}


//void Daily::populateSessionWidget()
//{

//    ui->sessionWidget->clearContents();
//    ui->sessionWidget->setColumnCount(2);

//    QMap<QDate, Day *>::iterator it;
//    QMap<QDate, Day *>::iterator it_end = p_profile->daylist.end();

//    int row = 0;
//    for (it = p_profile->daylist.begin(); it != it_end; ++it) {
//        const QDate & date = it.key();
//        Day * day = it.value();
//        QList<Session *> sessions = day->getSessions(MT_CPAP);
//        int size = sessions.size();
//        if (size > 0) {
//            QTableWidgetItem * item = new QTableWidgetItem(date.toString(Qt::SystemLocaleShortDate));
//            item->setData(Qt::UserRole, date);
//            ui->sessionWidget->setItem(row, 0, item);
//            SessionBar * sb = new SessionBar();

//            for (int i=0; i < size; i++) {
//                Session * sess = sessions[i];
//                QColor col = Qt::blue;
//                sb->add(sess, col);
//            }
//            ui->sessionWidget->setCellWidget(row, 1, sb);
//            row++;
//        }
//    }
//    ui->sessionWidget->setRowCount(row-1);
//    ui->sessionWidget->setCurrentCell(row-1, 0);
//    ui->sessionWidget->scrollToBottom();
//}
Daily::~Daily()
{
//    disconnect(sessbar, SIGNAL(toggledSession(Session*)), this, SLOT(doToggleSession(Session*)));

    // Save any last minute changes..

    delete ui;
    delete icon_on;
    delete icon_off;
}

void Daily::showEvent(QShowEvent *)
{
    RedrawGraphs();
}

void Daily::closeEvent(QCloseEvent *event)
{

    disconnect(webView,SIGNAL(linkClicked(QUrl)),this,SLOT(Link_clicked(QUrl)));

    if (previous_date.isValid())
        Unload(previous_date);

    GraphView->SaveSettings("Daily");
    QWidget::closeEvent(event);
    event->accept();
}


void Daily::doToggleSession(Session * sess)
{
    sess->setEnabled(!sess->enabled());

    LoadDate(previous_date);
    mainwin->getOverview()->graphView()->dataChanged();
}

void Daily::Link_clicked(const QUrl &url)
{
    QString code=url.toString().section("=",0,0).toLower();
    QString data=url.toString().section("=",1);
    int sid=data.toInt();
    Day *day=nullptr;

    if (code=="togglecpapsession") { // Enable/Disable CPAP session
        day=p_profile->GetDay(previous_date,MT_CPAP);
        if (!day) return;
        Session *sess=day->find(sid);
        if (!sess)
            return;
        int i=webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)-webView->page()->mainFrame()->scrollBarValue(Qt::Vertical);
        sess->setEnabled(!sess->enabled());

        // Reload day
        LoadDate(previous_date);
        webView->page()->mainFrame()->setScrollBarValue(Qt::Vertical, webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)-i);
    } else  if (code=="toggleoxisession") { // Enable/Disable Oximetry session
        day=p_profile->GetDay(previous_date,MT_OXIMETER);
        Session *sess=day->find(sid);
        if (!sess)
            return;
        int i=webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)-webView->page()->mainFrame()->scrollBarValue(Qt::Vertical);
        sess->setEnabled(!sess->enabled());

        // Reload day
        LoadDate(previous_date);
        webView->page()->mainFrame()->setScrollBarValue(Qt::Vertical, webView->page()->mainFrame()->scrollBarMaximum(Qt::Vertical)-i);
    } else if (code=="cpap")  {
        day=p_profile->GetDay(previous_date,MT_CPAP);
        if (day) {
            Session *sess=day->machine(MT_CPAP)->sessionlist[sid];
            if (sess && sess->enabled()) {
                GraphView->SetXBounds(sess->first(),sess->last());
            }
        }
    } else if (code=="oxi") {
        day=p_profile->GetDay(previous_date,MT_OXIMETER);
        if (day) {
            Session *sess=day->machine(MT_OXIMETER)->sessionlist[sid];
            if (sess && sess->enabled()) {
                GraphView->SetXBounds(sess->first(),sess->last());
            }
        }

    } else if (code=="event")  {
        QList<QTreeWidgetItem *> list=ui->treeWidget->findItems(schema::channel[sid].fullname(),Qt::MatchContains);
        if (list.size()>0) {
            ui->treeWidget->collapseAll();
            ui->treeWidget->expandItem(list.at(0));
            QTreeWidgetItem *wi=list.at(0)->child(0);
            ui->treeWidget->setCurrentItem(wi);
            ui->tabWidget->setCurrentIndex(1);
        } else {
            mainwin->Notify(tr("No %1 events are recorded this day").arg(schema::channel[sid].fullname()),"",1500);
        }
    } else if (code=="graph") {
        qDebug() << "Select graph " << data;
    } else {
        qDebug() << "Clicked on" << code << data;
    }
}

void Daily::ReloadGraphs()
{
    GraphView->setDay(nullptr);

    ui->splitter->setVisible(true);
    QDate d;

    if (previous_date.isValid()) {
        d=previous_date;
        //Unload(d);
    }
    d=p_profile->LastDay();
    if (!d.isValid()) {
        d=ui->calendar->selectedDate();
    }
    on_calendar_currentPageChanged(d.year(),d.month());
    // this fires a signal which unloads the old and loads the new
    ui->calendar->blockSignals(true);
    ui->calendar->setSelectedDate(d);
    ui->calendar->blockSignals(false);
    Load(d);
    ui->calButton->setText(ui->calendar->selectedDate().toString(Qt::TextDate));
    graphView()->redraw();
}

void Daily::hideSpaceHogs()
{
    if (p_profile->appearance->calendarVisible()) {
        ui->calendarFrame->setVisible(false);
    }
}
void Daily::showSpaceHogs()
{
    if (p_profile->appearance->calendarVisible()) {
        ui->calendarFrame->setVisible(true);
    }
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

    QTreeWidgetItem *root=nullptr;
    QHash<ChannelID,QTreeWidgetItem *> mcroot;
    QHash<ChannelID,int> mccnt;
    int total_events=0;

    qint64 drift=0, clockdrift=p_profile->cpap->clockDrift()*1000L;

    quint32 chantype = schema::FLAG | schema::SPAN | schema::MINOR_FLAG;
    if (p_profile->general->showUnknownFlags()) chantype |= schema::UNKNOWN;
    QList<ChannelID> chans = day->getSortedMachineChannels(chantype);

    for (QList<Session *>::iterator s=day->begin();s!=day->end();++s) {
        Session * sess = *s;
        if (!sess->enabled()) continue;

        QHash<ChannelID,QVector<EventList *> >::iterator m;
        for (int c=0; c < chans.size(); ++c) {
            ChannelID code = chans.at(c);
            m = sess->eventlist.find(code);
            if (m == sess->eventlist.end()) continue;

            drift=(sess->type() == MT_CPAP) ? clockdrift : 0;

            QTreeWidgetItem *mcr;
            if (mcroot.find(code)==mcroot.end()) {
                int cnt=day->count(code);
                if (!cnt) continue; // If no events than don't bother showing..
                total_events+=cnt;
                QString st=schema::channel[code].fullname();
                if (st.isEmpty())  {
                    st=QString("Fixme %1").arg(code);
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
                    QDateTime d=QDateTime::fromMSecsSinceEpoch(t);
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
    QTreeWidgetItem * start = new QTreeWidgetItem(QStringList(tr("Session Start Times")));
    QTreeWidgetItem * end = new QTreeWidgetItem(QStringList(tr("Session End Times")));
    tree->insertTopLevelItem(cnt++ , start);
    tree->insertTopLevelItem(cnt++ , end);
    for (QList<Session *>::iterator s=day->begin(); s!=day->end(); ++s) {
        QDateTime st = QDateTime::fromMSecsSinceEpoch((*s)->first());
        QDateTime et = QDateTime::fromMSecsSinceEpoch((*s)->last());

        QTreeWidgetItem * item = new QTreeWidgetItem(QStringList(st.toString("HH:mm:ss")));
        item->setData(0,Qt::UserRole, (*s)->first());
        start->addChild(item);


        item = new QTreeWidgetItem(QStringList(et.toString("HH:mm:ss")));
        item->setData(0,Qt::UserRole, (*s)->last());
        end->addChild(item);
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

    bool hascpap=p_profile->FindDay(date,MT_CPAP)!=nullptr;
    bool hasoxi=p_profile->FindDay(date,MT_OXIMETER)!=nullptr;
    bool hasjournal=p_profile->FindDay(date,MT_JOURNAL)!=nullptr;
    bool hasstage=p_profile->FindDay(date,MT_SLEEPSTAGE)!=nullptr;
    bool haspos=p_profile->FindDay(date,MT_POSITION)!=nullptr;
    if (hascpap) {
        if (hasoxi) {
            ui->calendar->setDateTextFormat(date,oxicpap);
        } else if (hasjournal) {
            ui->calendar->setDateTextFormat(date,cpapjour);
        } else if (hasstage || haspos) {
            ui->calendar->setDateTextFormat(date,stageday);
        } else {
            ui->calendar->setDateTextFormat(date,cpaponly);
        }
    } else if (hasoxi) {
        ui->calendar->setDateTextFormat(date,oxiday);
    } else if (hasjournal) {
        ui->calendar->setDateTextFormat(date,jourday);
    } else if (hasstage) {
        ui->calendar->setDateTextFormat(date,oxiday);
    } else if (haspos) {
        ui->calendar->setDateTextFormat(date,oxiday);
    } else {
        ui->calendar->setDateTextFormat(date,nodata);
    }
    ui->calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
}
void Daily::LoadDate(QDate date)
{
    if (!date.isValid()) {
        qDebug() << "LoadDate called with invalid date";
        return;
    }
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
    QTimer::singleShot(0, this, SLOT(on_ReloadDay()));
}

void Daily::on_ReloadDay()
{
    graphView()->releaseKeyboard();
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

    if (p_profile->general->unitSystem()==US_Archiac) {
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

MyWebPage::MyWebPage(QObject *parent):
   QWebPage(parent)
{
   // Enable plugin support
   settings()->setAttribute(QWebSettings::PluginsEnabled, true);
}

QObject *MyWebPage::createPlugin(const QString &classid, const QUrl &url, const QStringList & paramNames, const QStringList & paramValues)
{
    Q_UNUSED(paramNames)
    Q_UNUSED(paramValues)
    Q_UNUSED(url)

    if (classid=="SessionBar") {
        return mainwin->getDaily()->sessionBar();
    }
    qDebug() << "Request for unknown MyWebPage plugin";
    return new QWidget();
}

MyWebView::MyWebView(QWidget *parent):
   QWebView(parent),
   m_page(this)
{
   // Set the page of our own PageView class, MyPageView,
   // because only objects of this class will handle
   // object-tags correctly.
   setPage(&m_page);
}


QString Daily::getSessionInformation(Day * day)
{
    QString html;
    if (!day) return html;

    html="<table cellpadding=0 cellspacing=0 border=0 width=100%>";
    html+=QString("<tr><td colspan=5 align=center><b>"+tr("Session Information")+"</b></td></tr>");
    html+="<tr><td colspan=5 align=center>&nbsp;</td></tr>";
    QFontMetrics FM(*defaultfont);
    QRect r=FM.boundingRect('@');

    Machine * cpap = day->machine(MT_CPAP);

    if (cpap) {
        html+=QString("<tr><td colspan=5 align=center>"
        "<object type=\"application/x-qt-plugin\" classid=\"SessionBar\" name=\"sessbar\" height=%1 width=100%></object>"
//        "<script>"
//        "sessbar.show();"
//        "</script>"
        "</td></tr>").arg(r.height()*3,0,10);
        html+="<tr><td colspan=5 align=center>&nbsp; </td></tr>";
    }


    QDateTime fd,ld;
    bool corrupted_waveform=false;
    QString tooltip;

    QString type;

    QHash<MachineType, Machine *>::iterator mach_end = day->machines.end();
    QHash<MachineType, Machine *>::iterator mi;

    for (mi = day->machines.begin(); mi != mach_end; ++mi) {
        if (mi.key() == MT_JOURNAL) continue;
        html += "<tr><td colspan=5 align=center><i>";
        switch (mi.key()) {
            case MT_CPAP: type="cpap";
                html+=tr("CPAP Sessions");
                break;
            case MT_OXIMETER: type="oxi";
                html+=tr("Oximetery Sessions");
                break;
            case MT_SLEEPSTAGE: type="stage";
                html+=tr("Sleep Stage Sessions");
                break;
            case MT_POSITION: type="stage";
                html+=tr("Position Sensor Sessions");
                break;

            default:
                type="unknown";
                html+=tr("Unknown Session");
                break;
        }
        html+="</i></td></tr>\n";
        html+=QString("<tr>"
                      "<td>"+STR_TR_On+"</td>"
                      "<td>"+STR_TR_Date+"</td>"
                      "<td>"+STR_TR_Start+"</td>"
                      "<td>"+STR_TR_End+"</td>"
                      "<td>"+tr("Duration")+"</td></tr>");

        QList<Session *> sesslist = day->getSessions(mi.key(), true);

        for (QList<Session *>::iterator s=sesslist.begin(); s != sesslist.end(); ++s) {
            if (((*s)->type() == MT_CPAP) &&
                ((*s)->settings.find(CPAP_BrokenWaveform) != (*s)->settings.end()))
                    corrupted_waveform=true;

            fd=QDateTime::fromTime_t((*s)->first()/1000L);
            ld=QDateTime::fromTime_t((*s)->last()/1000L);
            int len=(*s)->length()/1000L;
            int h=len/3600;
            int m=(len/60) % 60;
            int s1=len % 60;

            Session *sess=*s;
            bool b=sess->enabled();

            html+=QString("<tr class='datarow2'><td colspan=5 align=center>%2</td></tr>"
                          "<tr class='datarow2'>"
                          "<td width=26><a href='toggle"+type+"session=%1'>"
                          "<img src='qrc:/icons/session-%4.png' width=24px></a></td>"
                          "<td align=left>%5</td>"
                          "<td align=left>%6</td>"
                          "<td align=left>%7</td>"
                          "<td align=left>%3</td></tr>"
                          )
                    .arg((*s)->session())
                    .arg(QObject::tr("%1 Session #%2").arg((*s)->machine()->loaderName()).arg((*s)->session(),8,10,QChar('0')))
                    .arg(QString("%1h %2m %3s").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s1,2,10,QChar('0')))
                    .arg((b ? "on" : "off"))
                    .arg(fd.date().toString(Qt::SystemLocaleShortDate))
                    .arg(fd.toString("HH:mm:ss"))
                    .arg(ld.toString("HH:mm:ss"));
#ifdef SESSION_DEBUG
            for (int i=0; i< sess->session_files.size(); ++i) {
                html+=QString("<tr><td colspan=5 align=center>%1</td></tr>").arg(sess->session_files[i].section("/",-1));
            }
#endif
        }
    }

    if (corrupted_waveform) {
        html+=QString("<tr><td colspan=5 align=center><i>%1</i></td></tr>").arg(tr("One or more waveform record for this session had faulty source data. Some waveform overlay points may not match up correctly."));
    }
    html+="</table>";
    return html;
}

QString Daily::getMachineSettings(Day * day) {
    QString html;

    Machine * cpap = day->machine(MT_CPAP);
    if (cpap && day->hasEnabledSessions(MT_CPAP)) {
        html="<table cellpadding=0 cellspacing=0 border=0 width=100%>";
        html+=QString("<tr><td colspan=5 align=center><b>%1</b></td></tr>").arg(tr("Machine Settings"));
        html+="<tr><td colspan=5>&nbsp;</td></tr>";

        if ((day->settingExists(CPAP_BrokenSummary))) {
            html+="<tr><td colspan=5 align=center><i>"+tr("Machine Settings Unavailable")+"</i></td></tr></table><hr/>\n";
            return html;
        }

        QMap<QString, QString> other;
        Session * sess = day->firstSession(MT_CPAP);

        QHash<ChannelID, QVariant>::iterator it;
        QHash<ChannelID, QVariant>::iterator it_end;
        if (sess) {
            it_end = sess->settings.end();
            it = sess->settings.begin();
        }
        QMap<int, QString> first;

        CPAPLoader * loader = qobject_cast<CPAPLoader *>(cpap->loader());

        ChannelID cpapmode = loader->CPAPModeChannel();
        schema::Channel & chan = schema::channel[cpapmode];
        first[cpapmode] = QString("<tr class='datarow'><td><a class='info' href='#'>%1<span>%2</span></a></td><td colspan=4>%3</td></tr>")
                .arg(chan.label())
                .arg(chan.description())
                .arg(day->getCPAPMode());


        if (sess) for (; it != it_end; ++it) {
            ChannelID code = it.key();

            if ((code <= 1) || (code == RMS9_MaskOnTime) || (code == CPAP_Mode) || (code == cpapmode) || (code == CPAP_SummaryOnly)) continue;

            schema::Channel & chan = schema::channel[code];

            QString data;

            if (chan.datatype() == schema::LOOKUP) {
                data = chan.option(it.value().toInt());
            } else if (chan.datatype() == schema::BOOL) {
                data = (it.value().toBool() ? STR_TR_Yes : STR_TR_No);
            } else if (chan.datatype() == schema::DOUBLE) {
                data = QString().number(it.value().toDouble(),'f',1) + " "+chan.units();
            } else {
                data = it.value().toString() + " "+ chan.units();
            }
            QString tmp = QString("<tr class='datarow'><td><a class='info' href='#'>%1<span>%2</span></a></td><td colspan=4>%3</td></tr>")
                    .arg(schema::channel[code].label())
                    .arg(schema::channel[code].description())
                    .arg(data);


            if ((code == CPAP_IPAP)
            || (code == CPAP_EPAP)
            || (code == CPAP_IPAPHi)
            || (code == CPAP_EPAPHi)
            || (code == CPAP_IPAPLo)
            || (code == CPAP_EPAPLo)
            || (code == CPAP_PressureMin)
            || (code == CPAP_PressureMax)
            || (code == CPAP_Pressure)
            || (code == CPAP_PSMin)
            || (code == CPAP_PSMax)
            || (code == CPAP_PS)) {
                first[code] = tmp;
            } else {
                other[schema::channel[code].label()] = tmp;
            }
        }

        ChannelID order[] = { cpapmode, CPAP_Pressure, CPAP_PressureMin, CPAP_PressureMax, CPAP_EPAP, CPAP_EPAPLo, CPAP_EPAPHi, CPAP_IPAP, CPAP_IPAPLo, CPAP_IPAPHi, CPAP_PS, CPAP_PSMin, CPAP_PSMax };
        int os = sizeof(order) / sizeof(ChannelID);
        for (int i=0 ;i < os; ++i) {
            if (first.contains(order[i])) html += first[order[i]];
        }

        for (QMap<QString,QString>::iterator it = other.begin(); it != other.end(); ++it) {
            html += it.value();
        }

  /*      ChannelID pr_level_chan = NoChannel;
        ChannelID pr_mode_chan = NoChannel;
        ChannelID hum_stat_chan = NoChannel;
        ChannelID hum_level_chan = NoChannel;
        CPAPLoader * loader = dynamic_cast<CPAPLoader *>(cpap->machine->loader());
        if (loader) {
            pr_level_chan = loader->PresReliefLevel();
            pr_mode_chan = loader->PresReliefMode();
            hum_stat_chan = loader->HumidifierConnected();
            hum_level_chan = loader->HumidifierLevel();
        }

        if ((pr_level_chan != NoChannel) && (cpap->settingExists(pr_level_chan))) {
            QString flexstr = cpap->getPressureRelief();

            html+=QString("<tr><td><a class='info' href='#'>%1<span>%2</span></a></td><td colspan=4>%3</td></tr>")
                    .arg(schema::channel[pr_mode_chan].label())
                    .arg(schema::channel[pr_mode_chan].description())
                    .arg(flexstr);
        }


        if (cpap->settingExists(hum_level_chan)) {
            int humid=round(cpap->settings_wavg(hum_level_chan));
            html+=QString("<tr><td><a class='info' href='#'>"+schema::channel[hum_level_chan].label()+"<span>%1</span></a></td><td colspan=4>%2</td></tr>")
                .arg(schema::channel[hum_level_chan].description())
                .arg(humid == 0 ? STR_GEN_Off : "x"+QString::number(humid));
        } */
        html+="</table>";
        html+="<hr/>\n";
    }
    return html;
}

QString Daily::getOximeterInformation(Day * day)
{
    QString html;
    Machine * oxi = day->machine(MT_OXIMETER);
    if (oxi && day->hasEnabledSessions(MT_OXIMETER)) {
        html="<table cellpadding=0 cellspacing=0 border=0 width=100%>";
        html+=QString("<tr><td colspan=5 align=center><b>%1</b></td></tr>\n").arg(tr("Oximeter Information"));
        html+="<tr><td colspan=5 align=center>&nbsp;</td></tr>";
        html+="<tr><td colspan=5 align=center>"+oxi->brand()+" "+oxi->model()+"</td></tr>\n";
        html+="<tr><td colspan=5 align=center>&nbsp;</td></tr>";
        html+=QString("<tr><td colspan=5 align=center>%1: %2 (%3%)</td></tr>").arg(tr("SpO2 Desaturations")).arg(day->count(OXI_SPO2Drop)).arg((100.0/day->hours(MT_OXIMETER)) * (day->sum(OXI_SPO2Drop)/3600.0),0,'f',2);
        html+=QString("<tr><td colspan=5 align=center>%1: %2 (%3%)</td></tr>").arg(tr("Pulse Change events")).arg(day->count(OXI_PulseChange)).arg((100.0/day->hours(MT_OXIMETER)) * (day->sum(OXI_PulseChange)/3600.0),0,'f',2);
        html+=QString("<tr><td colspan=5 align=center>%1: %2%</td></tr>").arg(tr("SpO2 Baseline Used")).arg(day->settings_wavg(OXI_SPO2Drop),0,'f',2); // CHECKME: Should this value be wavg OXI_SPO2 isntead?
        html+="</table>\n";
        html+="<hr/>\n";
    }
    return html;
}

QString Daily::getCPAPInformation(Day * day)
{
    QString html;
    if (!day)
        return html;

    Machine * cpap = day->machine(MT_CPAP);
    if (!cpap) return html;


    MachineInfo info = cpap->getInfo();

    html="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";

    html+="<tr><td align=center><a class=info2 href='#'>"+info.brand + " "+ info.series+"<br/> "+info.model+"<span>";
    QString tooltip=("Model "+info.modelnumber+" - "+info.serial);
    tooltip=tooltip.replace(" ","&nbsp;");

    html+=tooltip;
    html+="</span></td></tr>\n";
    //CPAPMode mode=(CPAPMode)(int)cpap->settings_max(CPAP_Mode);
    html+="<tr><td align=center>";

    html+=tr("PAP Mode: %1<br/>").arg(day->getCPAPMode());
    html+= day->getPressureSettings();
    html+="</td></tr>\n";
    if ((day->settingExists(CPAP_BrokenSummary))) {
        html+="<tr><td>&nbsp;</td></tr>\n";
        html+=QString("<tr><td colspan=2><i>%1</i></td></tr>").arg("<b>"+STR_MessageBox_PleaseNote+":</b> "+ tr("This day has missing pressure, mode and settings data."));
    }

    html+="</table>\n";
    html+="<hr/>\n";
    return html;
}


QString Daily::getStatisticsInfo(Day * day)
{
    if (!day) return QString();

    Machine *cpap = day->machine(MT_CPAP),
            *oxi = day->machine(MT_OXIMETER),
            *pos = day->machine(MT_POSITION);


    int mididx=p_profile->general->prefCalcMiddle();
    SummaryType ST_mid;
    if (mididx==0) ST_mid=ST_PERC;
    if (mididx==1) ST_mid=ST_WAVG;
    if (mididx==2) ST_mid=ST_AVG;

    float percentile=p_profile->general->prefCalcPercentile()/100.0;

    SummaryType ST_max=p_profile->general->prefCalcMax() ? ST_PERC : ST_MAX;
    const EventDataType maxperc=0.995F;

    QString midname;
    if (ST_mid==ST_WAVG) midname=STR_TR_WAvg;
    else if (ST_mid==ST_AVG) midname=STR_TR_Avg;
    else if (ST_mid==ST_PERC) midname=STR_TR_Med;

    QString html;

    html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";
    html+=QString("<tr><td colspan=5 align=center><b>%1</b></td></tr>\n").arg(tr("Statistics"));
    html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td></tr>")
            .arg(STR_TR_Channel)
            .arg(STR_TR_Min)
            .arg(midname)
            .arg(tr("%1%2").arg(percentile*100,0,'f',0).arg(STR_UNIT_Percentage))
            .arg(STR_TR_Max);

    ChannelID chans[]={
        CPAP_Pressure,CPAP_EPAP,CPAP_IPAP,CPAP_PS,CPAP_PTB,
        CPAP_MinuteVent, CPAP_RespRate, CPAP_RespEvent,CPAP_FLG,
        CPAP_Leak, CPAP_LeakTotal, CPAP_Snore,CPAP_IE,CPAP_Ti,CPAP_Te, CPAP_TgMV,
        CPAP_TidalVolume, OXI_Pulse, OXI_SPO2, POS_Inclination, POS_Orientation
    };
    int numchans=sizeof(chans)/sizeof(ChannelID);
    int ccnt=0;
    EventDataType tmp,med,perc,mx,mn;

    for (int i=0;i<numchans;i++) {

        ChannelID code=chans[i];

        if (!day->channelHasData(code))
            continue;

        QString tooltip=schema::channel[code].description();

        if (!schema::channel[code].units().isEmpty()) tooltip+=" ("+schema::channel[code].units()+")";

        if (ST_max == ST_MAX) {
            mx=day->Max(code);
        } else {
            mx=day->percentile(code,maxperc);
        }

        mn=day->Min(code);
        perc=day->percentile(code,percentile);

        if (ST_mid == ST_PERC) {
            med=day->percentile(code,0.5);
            tmp=day->wavg(code);
            if (tmp>0 || mx==0) {
                tooltip+=QString("<br/>"+STR_TR_WAvg+": %1").arg(tmp,0,'f',2);
            }
        } else if (ST_mid == ST_WAVG) {
            med=day->wavg(code);
            tmp=day->percentile(code,0.5);
            if (tmp>0 || mx==0) {
                tooltip+=QString("<br/>"+STR_TR_Median+": %1").arg(tmp,0,'f',2);
            }
        } else if (ST_mid == ST_AVG) {
            med=day->avg(code);
            tmp=day->percentile(code,0.5);
            if (tmp>0 || mx==0) {
                tooltip+=QString("<br/>"+STR_TR_Median+": %1").arg(tmp,0,'f',2);
            }
        }

        html+=QString("<tr class='datarow'><td align=left class='info' onmouseover=\"style.color='blue';\" onmouseout=\"style.color='"+COLOR_Text.name()+"';\">%1<span>%6</span></td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
            .arg(schema::channel[code].label())
            .arg(mn,0,'f',2)
            .arg(med,0,'f',2)
            .arg(perc,0,'f',2)
            .arg(mx,0,'f',2)
            .arg(tooltip);
        ccnt++;

    }

    if (GraphView->isEmpty() && ((ccnt>0) || (cpap && day->summaryOnly()))) {
        html+="<tr><td colspan=5>&nbsp;</td></tr>\n";
        html+=QString("<tr><td colspan=5 align=center><i>%1</i></td></tr>").arg("<b>"+STR_MessageBox_PleaseNote+"</b> "+ tr("This day just contains summary data, only limited information is available ."));
    } else if (cpap) {
        html+="<tr><td colspan=5>&nbsp;</td></tr>";

        if ((cpap->loaderName() == STR_MACH_ResMed) || ((cpap->loaderName() == STR_MACH_PRS1) && (p_profile->cpap->resyncFromUserFlagging()))) {
            int ttia = day->sum(CPAP_Obstructive) + day->sum(CPAP_ClearAirway) + day->sum(CPAP_Apnea) + day->sum(CPAP_Hypopnea);
            int h = ttia / 3600;
            int m = ttia / 60 % 60;
            int s = ttia % 60;
            if (ttia > 0) {
                html+="<tr><td colspan=3 align='left' bgcolor='white'><b>"+tr("Total time in apnea")+
                    QString("</b></td><td colspan=2 bgcolor='white'>%1</td></tr>").arg(QString().sprintf("%02i:%02i:%02i",h,m,s));
            }

        }
        float hours = day->hours(MT_CPAP);

        if (p_profile->cpap->showLeakRedline()) {
            float rlt = day->timeAboveThreshold(CPAP_Leak, p_profile->cpap->leakRedline()) / 60.0;
            float pc = 100.0 / hours * rlt;
            html+="<tr><td colspan=3 align='left' bgcolor='white'><b>"+tr("Time over leak redline")+
                    QString("</b></td><td colspan=2 bgcolor='white'>%1%</td></tr>").arg(pc, 0, 'f', 3);
        }
        int l = day->sum(CPAP_Ramp);

        if (l > 0) {
            html+="<tr><td colspan=3 align='left' bgcolor='white'>"+tr("Total ramp time")+
                    QString("</td><td colspan=2 bgcolor='white'>%1:%2:%3</td></tr>").arg(l / 3600, 2, 10, QChar('0')).arg((l / 60) % 60, 2, 10, QChar('0')).arg(l % 60, 2, 10, QChar('0'));
            float v = (hours - (float(l) / 3600.0));
            int q = v * 3600.0;
            html+="<tr><td colspan=3 align='left' bgcolor='white'>"+tr("Time outside of ramp")+
                    QString("</td><td colspan=2 bgcolor='white'>%1:%2:%3</td></tr>").arg(q / 3600, 2, 10, QChar('0')).arg((q / 60) % 60, 2, 10, QChar('0')).arg(q % 60, 2, 10, QChar('0'));

//            EventDataType hc = day->count(CPAP_Hypopnea) - day->countInsideSpan(CPAP_Ramp, CPAP_Hypopnea);
//            EventDataType oc = day->count(CPAP_Obstructive) - day->countInsideSpan(CPAP_Ramp, CPAP_Obstructive);

            //EventDataType tc = day->count(CPAP_Hypopnea) + day->count(CPAP_Obstructive);
//            EventDataType ahi = (hc+oc) / v;
            // Not sure if i was trying to be funny, and left on my replication of Devilbiss's bug here...  :P

//            html+="<tr><td colspan=3 align='left' bgcolor='white'>"+tr("AHI excluding ramp")+
//                    QString("</td><td colspan=2 bgcolor='white'>%1</td></tr>").arg(ahi, 0, 'f', 2);
        }

    }


    html+="</table>\n";
    html+="<hr/>\n";
    return html;
}

QString Daily::getEventBreakdown(Day * cpap)
{
    Q_UNUSED(cpap)
    QString html;
    html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";

    html+="</table>";
    return html;
}

QString Daily::getSleepTime(Day * day)
{
    //cpap, Day * oxi
    QString html;

    if (!day || (day->hours() < 0.0000001))
        return html;

    html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";
    html+="<tr><td align='center'><b>"+STR_TR_Date+"</b></td><td align='center'><b>"+tr("Sleep")+"</b></td><td align='center'><b>"+tr("Wake")+"</b></td><td align='center'><b>"+STR_UNIT_Hours+"</b></td></tr>";
    int tt=qint64(day->total_time())/1000L;
    QDateTime date=QDateTime::fromTime_t(day->first()/1000L);
    QDateTime date2=QDateTime::fromTime_t(day->last()/1000L);

    int h=tt/3600;
    int m=(tt/60)%60;
    int s=tt % 60;
    html+=QString("<tr><td align='center'>%1</td><td align='center'>%2</td><td align='center'>%3</td><td align='center'>%4</td></tr>\n")
            .arg(date.date().toString(Qt::SystemLocaleShortDate))
            .arg(date.toString("HH:mm:ss"))
            .arg(date2.toString("HH:mm:ss"))
            .arg(QString().sprintf("%02i:%02i:%02i",h,m,s));
    html+="</table>\n";
//    html+="<hr/>";

    return html;
}



void Daily::Load(QDate date)
{
    dateDisplay->setText("<i>"+date.toString(Qt::SystemLocaleLongDate)+"</i>");
    previous_date=date;

    Day * day = p_profile->GetDay(date);
    Machine *cpap = nullptr,
            *oxi = nullptr,
            *stage = nullptr,
            *posit = nullptr;

    if (day) {
        cpap = day->machine(MT_CPAP);
        oxi = day->machine(MT_OXIMETER);
        stage = day->machine(MT_SLEEPSTAGE);
        posit = day->machine(MT_POSITION);
    }

    if (!p_profile->session->cacheSessions()) {
        // Getting trashed on purge last day...

        // lastcpapday can get purged and be invalid
        if (lastcpapday && (lastcpapday!=day)) {
            for (QMap<QDate, Day *>::iterator di = p_profile->daylist.begin(); di!= p_profile->daylist.end(); ++di) {
                Day * d = di.value();
                if (d->eventsLoaded()) {
                    if (d->useCounter() == 0) {
                        d->CloseEvents();
                    }
                }
            }
        }
    }

    // Don't really see a point in unlinked oximetery sessions anymore... All I can say is BLEH...
//    if ((cpap && oxi) && day->hasEnabledSessions(MT_OXIMETER)) {
//        int gr;

//        if (qAbs(day->first(MT_CPAP) - day->first(MT_OXIMETER)) > 30000) {
//            mainwin->Notify(tr("Oximetry data exists for this day, but its timestamps are too different, so the Graphs will not be linked."),"",3000);
//            gr=1;
//        } else
//            gr=0;

//        (*GraphView)[schema::channel[OXI_Pulse].code()]->setGroup(gr);
//        (*GraphView)[schema::channel[OXI_SPO2].code()]->setGroup(gr);
//        (*GraphView)[schema::channel[OXI_Plethy].code()]->setGroup(gr);
//    }
    lastcpapday=day;

    QString html="<html><head><style type='text/css'>"
    "p,a,td,body { font-family: '"+QApplication::font().family()+"'; }"
    "p,a,td,body { font-size: "+QString::number(QApplication::font().pointSize() + 2)+"px; }"
    "tr.datarow:nth-child(even) {"
    "background-color: #f8f8f8;"
    "}"
    "tr.datarow2:nth-child(4n-1) {"
    "background-color: #f8f8f8;"
    "}"
    "tr.datarow2:nth-child(4n+0) {"
    "background-color: #f8f8f8;"
    "}"
    "table.curved {"
    "border: 1px solid gray;"
    "border-radius:10px;"
    "-moz-border-radius:10px;"
    "-webkit-border-radius:10px;"
    "page-break-after:auto;"
    "-fs-table-paginate: paginate;"
    "}"

    "</style>"
    "<link rel='stylesheet' type='text/css' href='qrc:/docs/tooltips.css' />"
    "<script language='javascript'><!--"
            "func dosession(sessid) {"
            ""
            "}"
    "--></script>"
    "</head>"
    "<body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>";
    QString tmp;

    if (day) {
        day->OpenEvents();
    }
    GraphView->setDay(day);


//    UpdateOXIGraphs(oxi);
//    UpdateCPAPGraphs(cpap);
//    UpdateSTAGEGraphs(stage);
//    UpdatePOSGraphs(posit);
    UpdateEventsTree(ui->treeWidget, day);

    // FIXME:
    // Generating entire statistics because bookmarks may have changed.. (This updates the side panel too)
    mainwin->GenerateStatistics();

    snapGV->setDay(day);

   // GraphView->ResetBounds(false);

    // wtf is hiding the scrollbars for???
//    if (!cpap && !oxi) {
//        scrollbar->hide();
//    } else {
//        scrollbar->show();
//    }

    QString modestr;
    CPAPMode mode=MODE_UNKNOWN;
    QString a;
    bool isBrick=false;

    updateGraphCombo();
    ui->eventsCombo->clear();

    quint32 chans = schema::SPAN | schema::FLAG | schema::MINOR_FLAG;
    if (p_profile->general->showUnknownFlags()) chans |= schema::UNKNOWN;

    QList<ChannelID> available;
    if (day) available.append(day->getSortedMachineChannels(chans));

    for (int i=0; i < available.size(); ++i) {
        ChannelID code = available.at(i);
        schema::Channel & chan = schema::channel[code];
        ui->eventsCombo->addItem(chan.enabled() ? *icon_on : * icon_off, chan.label(), code);
        ui->eventsCombo->setItemData(i, chan.fullname(), Qt::ToolTipRole);

    }

    if (!cpap) {
        GraphView->setEmptyImage(QPixmap(":/docs/sheep.png"));
    }
    if (cpap) {
        //QHash<schema::ChanType, QList<schema::Channel *> > list;


        float hours=day->hours(MT_CPAP);
        if (GraphView->isEmpty() && (hours>0)) {
            if (!p_profile->hasChannel(CPAP_Obstructive) && !p_profile->hasChannel(CPAP_Hypopnea)) {
                GraphView->setEmptyText(STR_Empty_Brick);

                GraphView->setEmptyImage(QPixmap(":/icons/sadface.png"));
                isBrick=true;
            } else {
                GraphView->setEmptyImage(QPixmap(":/docs/sheep.png"));
            }
        }

        mode=(CPAPMode)(int)day->settings_max(CPAP_Mode);

        modestr=schema::channel[CPAP_Mode].m_options[mode];

        EventDataType ahi=(day->count(CPAP_Obstructive)+day->count(CPAP_Hypopnea)+day->count(CPAP_ClearAirway)+day->count(CPAP_Apnea));
        if (p_profile->general->calculateRDI()) ahi+=day->count(CPAP_RERA);
        ahi/=hours;

        if (hours>0) {
            html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";
            html+="<tr>";
            if (!isBrick) {
                ChannelID ahichan=CPAP_AHI;
                QString ahiname=STR_TR_AHI;
                if (p_profile->general->calculateRDI()) {
                    ahichan=CPAP_RDI;
                    ahiname=STR_TR_RDI;
                }
                html+=QString("<td colspan=4 bgcolor='%1' align=center><a class=info2 href='#'><font size=+4 color='%2'><b>%3</b></font><span>%4</span></a> &nbsp; <font size=+4 color='%2'><b>%5</b></font></td>\n")
                        .arg("#F88017").arg(COLOR_Text.name()).arg(ahiname).arg(schema::channel[ahichan].fullname()).arg(ahi,0,'f',2);
            } else {
                html+=QString("<td colspan=5 bgcolor='%1' align=center><font size=+4 color='yellow'>%2</font></td>\n")
                        .arg("#F88017").arg(tr("BRICK! :("));
            }
            html+="</tr>\n";
            html+="</table>\n";
            html+=getCPAPInformation(day);
            html+=getSleepTime(day);

            html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";


            quint32 zchans = schema::SPAN | schema::FLAG;
            bool show_minors = true;
            if (p_profile->general->showUnknownFlags()) zchans |= schema::UNKNOWN;

            if (show_minors) zchans |= schema::MINOR_FLAG;
            QList<ChannelID> available = day->getSortedMachineChannels(zchans);

            EventDataType val;
            QHash<ChannelID, EventDataType> values;
            for (int i=0; i < available.size(); ++i) {
                ChannelID code = available.at(i);
                schema::Channel & chan = schema::channel[code];
                if (!chan.enabled()) continue;
                QString data;
                if (chan.type() == schema::SPAN) {
                    val = (100.0 / hours)*(day->sum(code)/3600.0);
                    data = QString("%1%").arg(val,0,'f',2);
                } else {
                    val = day->count(code) / hours;
                    data = QString("%1").arg(val,0,'f',2);
                }
                values[code] = val;
                QColor altcolor = (brightness(chan.defaultColor()) < 0.3) ? Qt::white : Qt::black; // pick a contrasting color
                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a href='event=%5'>%3</a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%4</font></b></td></tr>\n")
                        .arg(chan.defaultColor().name()).arg(altcolor.name()).arg(chan.fullname()).arg(data).arg(code);
            }


//            for (int i=0;i<numchans;i++) {
//                if (!cpap->channelHasData(chans[i].id))
//                    continue;
//                if ((cpap->machine->loaderName() == STR_MACH_PRS1) && (chans[i].id == CPAP_VSnore))
//                    continue;
//                html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a href='event=%5'>%3</a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%4</font></b></td></tr>\n")
//                        .arg(schema::channel[chans[i].id].defaultColor().name()).arg(chans[i].color2.name()).arg(schema::channel[chans[i].id].fullname()).arg(chans[i].value,0,'f',2).arg(chans[i].id);

//                // keep in case tooltips are needed
//                //html+=QString("<tr><td align='left' bgcolor='%1'><b><font color='%2'><a class=info href='event=%6'>%3<span>%4</span></a></font></b></td><td width=20% bgcolor='%1'><b><font color='%2'>%5</font></b></td></tr>\n")
//                // .arg(chans[i].color.name()).arg(chans[i].color2.name()).arg(chans[i].name).arg(schema::channel[chans[i].id].description()).arg(chans[i].value,0,'f',2).arg(chans[i].id);
//            }
            html+="</table>";

            html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";
            // Show Event Breakdown pie chart
            if ((hours > 0) && p_profile->appearance->graphSnapshots()) {  // AHI Pie Chart
                if ((values[CPAP_Obstructive] + values[CPAP_Hypopnea] + values[CPAP_ClearAirway] + values[CPAP_Apnea] + values[CPAP_RERA] + values[CPAP_FlowLimit] + values[CPAP_SensAwake])>0) {
                    html+="<tr><td align=center>&nbsp;</td></tr>";
                    html+=QString("<tr><td align=center><b>%1</b></td></tr>").arg(tr("Event Breakdown"));
                    eventBreakdownPie()->setShowTitle(false);

                    int w=155;
                    int h=155;
                    QPixmap pixmap=eventBreakdownPie()->renderPixmap(w,h,false);
                    if (!pixmap.isNull()) {
                        QByteArray byteArray;
                        QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
                        buffer.open(QIODevice::WriteOnly);
                        pixmap.save(&buffer, "PNG");
                        html += "<tr><td align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + QString("\" width='%1' height='%2px'></td></tr>\n").arg(w).arg(h);
                    } else {
                        html += "<tr><td align=center>Unable to display Pie Chart on this system</td></tr>\n";
                    }
                } else if (day->channelHasData(CPAP_Obstructive)
                           || day->channelHasData(CPAP_Hypopnea)
                           || day->channelHasData(CPAP_ClearAirway)
                           || day->channelHasData(CPAP_RERA)
                           || day->channelHasData(CPAP_Apnea)
                           || day->channelHasData(CPAP_FlowLimit)
                           || day->channelHasData(CPAP_SensAwake)
                           ) {
                        html += "<tr><td align=center><img src=\"qrc:/docs/0.0.gif\"></td></tr>\n";
                }
            }

            html+="</table>\n";
            html+="<hr/>\n";

        } else {
            html+="<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n";
            if (!isBrick) {
                html+="<tr><td colspan='5'>&nbsp;</td></tr>\n";
                if (day->size()>0) {
                    html+="<tr><td colspan=5 align='center'><font size='+3'>"+tr("Sessions all off!")+"</font></td></tr>";
                    html+="<tr><td align=center><img src='qrc:/docs/sheep.png' width=120px></td></tr>";
                    html+="<tr bgcolor='#89abcd'><td colspan=5 align='center'><i><font color=white size=+1>"+tr("Sessions exist for this day but are switched off.")+"</font></i></td></tr>\n";
                    GraphView->setEmptyText(STR_Empty_NoSessions);
                } else {
                    html+="<tr><td colspan=5 align='center'><b><h2>"+tr("Impossibly short session")+"</h2></b></td></tr>";
                    html+="<tr><td colspan=5 align='center'><i>"+tr("Zero hours??")+"</i></td></tr>\n";
                }
            } else { // machine is a brick
                html+="<tr><td colspan=5 align='center'><b><h2>"+tr("BRICK :(")+"</h2></b></td></tr>";
                html+="<tr><td colspan=5 align='center'><i>"+tr("Sorry, your machine only provides compliance data.")+"</i></td></tr>\n";
                html+="<tr><td colspan=5 align='center'><i>"+tr("Complain to your Equipment Provider!")+"</i></td></tr>\n";
            }
            html+="<tr><td colspan='5'>&nbsp;</td></tr>\n";
            html+="</table>\n";
        }

    } // if (!CPAP)
    else html+=getSleepTime(day);

    if ((cpap && !isBrick && (day->hours()>0)) || oxi || posit) {

        html+=getStatisticsInfo(day);

    } else {
        if (cpap && day->hours(MT_CPAP)<0.0000001) {
        } else if (!isBrick) {
            html+="<table cellspacing=0 cellpadding=0 border=0 height=100% width=100%>";
            html+="<tr height=25%><td align=center></td></tr>";
            html+="<tr><td align=center><font size='+3'>"+tr("\"Nothing's here!\"")+"</font></td></tr>";
            html+="<tr><td align=center><img src='qrc:/docs/sheep.png' width=120px></td></tr>";
            html+="<tr height=5px><td align=center></td></tr>";
            html+="<tr bgcolor='#89abcd'><td align=center><i><font size=+1 color=white>"+tr("Bob is bored with this days lack of data.")+"</font></i></td></tr>";
            html+="<tr height=25%><td align=center></td></tr>";
            html+="</table>\n";
        }

    }
    if (day) {
        html+=getOximeterInformation(day);
        html+=getMachineSettings(day);
        html+=getSessionInformation(day);
    }

    html+="</body></html>";

    QColor cols[]={
        COLOR_Gold,
        QColor("light blue"),
    };
    const int maxcolors=sizeof(cols)/sizeof(QColor);
    QList<Session *>::iterator i;

    // WebView trashes it without asking.. :(
    if (cpap) {
        sessbar=new SessionBar(this);
        sessbar->setMouseTracking(true);
        connect(sessbar, SIGNAL(sessionClicked(Session*)), this, SLOT(doToggleSession(Session*)));
        int c=0;

        for (i=day->begin();i!=day->end();++i) {
            Session * s=*i;
            if ((*s).type() == MT_CPAP)
                sessbar->add(s, cols[c % maxcolors]);
            c++;
        }
    } else sessbar=nullptr;
    //sessbar->update();

    webView->setHtml(html);

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

            if (p_profile->general->unitSystem()==US_Metric) {
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
            double height=p_profile->user->height()/100.0;
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


            qint64 clockdrift=p_profile->cpap->clockDrift()*1000L,drift;
            Day * dday=p_profile->GetDay(previous_date,MT_CPAP);
            drift=(dday!=nullptr) ? clockdrift : 0;

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
    if (p_profile->general->unitSystem()==US_Archiac) {
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

void Daily::clearLastDay()
{
    lastcpapday=nullptr;
}


void Daily::Unload(QDate date)
{
    if (!date.isValid()) {
        date = getDate();
        if (!date.isValid()) {
            graphView()->setDay(nullptr);
            return;
        }
    }

    webView->setHtml("");
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
            journal->settings[LastUpdated]=QDateTime::currentDateTime();
            // blah.. was updating overview graphs here.. Was too slow.
        }
        Machine *jm=p_profile->GetMachine(MT_JOURNAL);
        if (jm) jm->SaveSummary(); //(journal);
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
    Machine *m = p_profile->GetMachine(MT_JOURNAL);
    if (!m) {
        m=new Machine(0);
        MachineInfo info;
        info.loadername = "Journal";
        info.serial = m->hexid();
        info.brand = "Journal";
        info.type = MT_JOURNAL;
        m->setInfo(info);
        m->setType(MT_JOURNAL);
        p_profile->AddMachine(m);
    }

    Session *sess=new Session(m,0);
    qint64 st,et;

    Day *cday=p_profile->GetDay(date);
    if (cday) {
        st=cday->first();
        et=cday->last();
    } else {
        QDateTime dt(date,QTime(20,0));
        st=qint64(dt.toTime_t())*1000L;
        et=st+3600000L;
    }
    sess->SetSessionID(st / 1000L);
    sess->set_first(st);
    sess->set_last(et);
    m->AddSession(sess);
    return sess;
}
Session * Daily::GetJournalSession(QDate date) // Get the first journal session
{
    Day *day=p_profile->GetDay(date, MT_JOURNAL);
    if (day) {
        Session * session = day->firstSession(MT_JOURNAL);
        if (!session) {
            session = CreateJournalSession(date);
        }
        return session;
    }
    return nullptr;
}


void Daily::RedrawGraphs()
{
    // setting this here, because it needs to be done when preferences change
    if (p_profile->cpap->showLeakRedline()) {
        schema::channel[CPAP_Leak].setUpperThreshold(p_profile->cpap->leakRedline());
    } else {
        schema::channel[CPAP_Leak].setUpperThreshold(0); // switch it off
    }

    GraphView->redraw();
}

void Daily::on_LineCursorUpdate(double time)
{
    if (time > 1) {
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(time);
        QString txt = dt.toString("MMM dd HH:mm:ss.zzz");
        dateDisplay->setText(txt);
    } else dateDisplay->setText(QString(GraphView->emptyText()));
}

void Daily::on_RangeUpdate(double minx, double maxx)
{
    if (minx > 1) {
        dateDisplay->setText(GraphView->getRangeString());
    } else {
        dateDisplay->setText(QString(GraphView->emptyText()));
    }

/*    // Delay render some stats...
    Day * day = GraphView->day();
    if (day) {
        QTime time;
        time.start();
        QList<ChannelID> list = day->getSortedMachineChannels(schema::WAVEFORM);
        for (int i=0; i< list.size();i++) {
            schema::Channel & chan = schema::channel[list.at(i)];
            ChannelID code = chan.id();
            if (!day->channelExists(code)) continue;
             float avg = day->rangeAvg(code, minx, maxx);
            float wavg = day->rangeWavg(code, minx, maxx);
            float median = day->rangePercentile(code, 0.5, minx, maxx);
           float p90 = day->rangePercentile(code, 0.9, minx, maxx);
//            qDebug() << chan.label()
//                     << "AVG=" << avg
//                     << "WAVG=" << wavg;
                  //   << "MED" << median
                   //  << "90%" << p90;
        }

        qDebug() << time.elapsed() << "ms";
    }*/
}


void Daily::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    QDateTime d;
    if (!item->data(0,Qt::UserRole).isNull()) {
        qint64 winsize=qint64(p_profile->general->eventWindowSize())*60000L;
        qint64 t=item->data(0,Qt::UserRole).toLongLong();

        double st=t-(winsize/2);
        double et=t+(winsize/2);

        gGraph *g=GraphView->findGraph(STR_GRAPH_SleepFlags);
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
    if (!p_profile->ExistsAndTrue("SkipEmptyDays")) {
        LoadDate(previous_date.addDays(-1));
    } else {
        QDate d=previous_date;
        for (int i=0;i<90;i++) {
            d=d.addDays(-1);
            if (p_profile->GetDay(d)) {
                LoadDate(d);
                break;
            }
        }
    }
}

void Daily::on_nextDayButton_clicked()
{
    if (!p_profile->ExistsAndTrue("SkipEmptyDays")) {
        LoadDate(previous_date.addDays(1));
    } else {
        QDate d=previous_date;
        for (int i=0;i<90;i++) {
            d=d.addDays(1);
            if (p_profile->GetDay(d)) {
                LoadDate(d);
                break;
            }
        }
    }
}

void Daily::on_calButton_toggled(bool checked)
{
    bool b=checked;
    ui->calendarFrame->setVisible(b);
    p_profile->appearance->setCalendarVisible(b);

    if (!b) {
        ui->calButton->setArrowType(Qt::DownArrow);
    } else {
        ui->calButton->setArrowType(Qt::UpArrow);
    }
}


void Daily::on_todayButton_clicked()
{
    QDate d=QDate::currentDate();
    if (d > p_profile->LastDay()) d=p_profile->LastDay();
    LoadDate(d);
}

void Daily::on_evViewSlider_valueChanged(int value)
{
    ui->evViewLCD->display(value);
    p_profile->general->setEventWindowSize(value);

    int winsize=value*60;

    gGraph *g=GraphView->findGraph(STR_GRAPH_SleepFlags);
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

//    qint64 clockdrift=p_profile->cpap->clockDrift()*1000L,drift;
//    Day * dday=p_profile->GetDay(previous_date,MT_CPAP);
//    drift=(dday!=nullptr) ? clockdrift : 0;

    QTableWidgetItem *it=ui->bookmarkTable->item(row,1);
    bool ok;
    st=it->data(Qt::UserRole).toLongLong(&ok);
    et=it->data(Qt::UserRole+1).toLongLong(&ok);
    qint64 st2=0,et2=0,st3,et3;
    Day * day=p_profile->GetGoodDay(previous_date,MT_CPAP);
    if (day) {
        st2=day->first();
        et2=day->last();
    }
    Day * oxi=p_profile->GetGoodDay(previous_date,MT_OXIMETER);
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

void Daily::addBookmark(qint64 st, qint64 et, QString text)
{
    ui->bookmarkTable->blockSignals(true);
    QDateTime d=QDateTime::fromTime_t(st/1000L);
    int row=ui->bookmarkTable->rowCount();
    ui->bookmarkTable->insertRow(row);
    QTableWidgetItem *tw=new QTableWidgetItem(text);
    QTableWidgetItem *dw=new QTableWidgetItem(d.time().toString("HH:mm:ss"));
    dw->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    ui->bookmarkTable->setItem(row,0,dw);
    ui->bookmarkTable->setItem(row,1,tw);
    qint64 clockdrift=p_profile->cpap->clockDrift()*1000L,drift;
    Day * day=p_profile->GetDay(previous_date,MT_CPAP);
    drift=(day!=nullptr) ? clockdrift : 0;

    // Counter CPAP clock drift for storage, in case user changes it later on
    // This won't fix the text string names..

    tw->setData(Qt::UserRole,st-drift);
    tw->setData(Qt::UserRole+1,et-drift);

    ui->bookmarkTable->blockSignals(false);
    update_Bookmarks();
    mainwin->updateFavourites();

}

void Daily::on_addBookmarkButton_clicked()
{
    qint64 st,et;
    GraphView->GetXBounds(st,et);
    QDateTime d=QDateTime::fromTime_t(st/1000L);

    addBookmark(st,et, tr("Bookmark at %1").arg(d.time().toString("HH:mm:ss")));
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
    journal->settings[LastUpdated]=QDateTime::currentDateTime();
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

    // shouldn't be needed anymore with new overview model..
    //if (mainwin->getOverview()) mainwin->getOverview()->ResetGraph("Zombie");
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
    if (p_profile->general->unitSystem()==US_Archiac) {
         kg=((arg1*pound_convert) + (ui->ouncesSpinBox->value()*ounce_convert)) / 1000.0;
    } else kg=arg1;
    double height=p_profile->user->height()/100.0;
    if ((height>0) && (kg>0)) {
        double bmi=kg/(height * height);
        ui->BMI->display(bmi);
        ui->BMI->setVisible(true);
    }
}

void Daily::on_weightSpinBox_editingFinished()
{
    double arg1=ui->weightSpinBox->value();

    double height=p_profile->user->height()/100.0;
    Session *journal=GetJournalSession(previous_date);
    if (!journal) {
        journal=CreateJournalSession(previous_date);
    }

    double kg;
    if (p_profile->general->unitSystem()==US_Archiac) {
            kg=((arg1*pound_convert) + (ui->ouncesSpinBox->value()*ounce_convert)) / 1000.0;
    } else {
            kg=arg1;
    }
    journal->settings[Journal_Weight]=kg;
    gGraphView *gv=mainwin->getOverview()->graphView();
    gGraph *g;
    if (gv) {
        g=gv->findGraph(STR_GRAPH_Weight);
        if (g) g->setDay(nullptr);
    }
    if ((height>0) && (kg>0)) {
        double bmi=kg/(height * height);
        ui->BMI->display(bmi);
        ui->BMI->setVisible(true);
        journal->settings[Journal_BMI]=bmi;
        if (gv) {
            g=gv->findGraph(STR_GRAPH_BMI);
            if (g) g->setDay(nullptr);
        }
    }
    journal->SetChanged(true);
}

void Daily::on_ouncesSpinBox_valueChanged(int arg1)
{
    // just update for BMI display
    double height=p_profile->user->height()/100.0;
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
    double height=p_profile->user->height()/100.0;
    double kg=((ui->weightSpinBox->value()*pound_convert) + (arg1*ounce_convert)) / 1000.0;
    journal->settings[Journal_Weight]=kg;

    gGraph *g;
    if (mainwin->getOverview()) {
        g=mainwin->getOverview()->graphView()->findGraph(STR_GRAPH_Weight);
        if (g) g->setDay(nullptr);
    }

    if ((height>0) && (kg>0)) {
        double bmi=kg/(height * height);
        ui->BMI->display(bmi);
        ui->BMI->setVisible(true);

        journal->settings[Journal_BMI]=bmi;
        if (mainwin->getOverview()) {
            g=mainwin->getOverview()->graphView()->findGraph(STR_GRAPH_BMI);
            if (g) g->setDay(nullptr);
        }
    }
    journal->SetChanged(true);

    // shouldn't be needed anymore with new overview model
    //if (mainwin->getOverview()) mainwin->getOverview()->ResetGraph(STR_GRAPH_Weight);
}

QString Daily::GetDetailsText()
{
    webView->triggerPageAction(QWebPage::SelectAll);
    QString text=webView->page()->selectedText();
    webView->triggerPageAction(QWebPage::MoveToEndOfDocument);
    webView->triggerPageAction(QWebPage::SelectEndOfDocument);
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
    g=GraphView->findGraphTitle(s);
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


        if (ui->graphCombo->count() > 0) {
            GraphView->setEmptyText(STR_Empty_NoGraphs);
        } else {
            if (!p_profile->GetGoodDay(getDate(), MT_CPAP)
              && !p_profile->GetGoodDay(getDate(), MT_OXIMETER)
              && !p_profile->GetGoodDay(getDate(), MT_SLEEPSTAGE)
              && !p_profile->GetGoodDay(getDate(), MT_POSITION)) {
                GraphView->setEmptyText(STR_Empty_NoData);

            } else {
                if (GraphView->emptyText() != STR_Empty_Brick)
                    GraphView->setEmptyText(STR_Empty_SummaryOnly);
            }
        }
    } else {
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

void Daily::on_eventsCombo_activated(int index)
{
    if (index<0)
        return;


    ChannelID code = ui->eventsCombo->itemData(index, Qt::UserRole).toUInt();
    schema::Channel * chan = &schema::channel[code];

    bool b = !chan->enabled();
    chan->setEnabled(b);

    ui->eventsCombo->setItemIcon(index,b ? *icon_on : *icon_off);

    GraphView->redraw();
}

void Daily::on_toggleEvents_clicked(bool checked)
{
    QString s;
    QIcon *icon=checked ? icon_on : icon_off;

    ui->toggleEvents->setArrowType(checked ? Qt::DownArrow : Qt::UpArrow);
    ui->toggleEvents->setToolTip(checked ? tr("Hide all events") : tr("Show all events"));
//    ui->toggleEvents->blockSignals(true);
//    ui->toggleEvents->setChecked(false);
//    ui->toggleEvents->blockSignals(false);

    for (int i=0;i<ui->eventsCombo->count();i++) {
//        s=ui->eventsCombo->itemText(i);
        ui->eventsCombo->setItemIcon(i,*icon);
        ChannelID code = ui->eventsCombo->itemData(i).toUInt();
        schema::channel[code].setEnabled(checked);
    }

    updateCube();
    GraphView->redraw();
}

//void Daily::on_sessionWidget_itemSelectionChanged()
//{
//    int row = ui->sessionWidget->currentRow();
//    QTableWidgetItem *item = ui->sessionWidget->item(row, 0);
//    if (item) {
//        QDate date = item->data(Qt::UserRole).toDate();
//        LoadDate(date);
//        qDebug() << "Clicked.. changing date to" << date;

//       // ui->sessionWidget->setCurrentItem(item);
//    }
//}
