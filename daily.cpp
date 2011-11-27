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

const int min_height=150;

Daily::Daily(QWidget *parent,gGraphView * shared, MainWindow *mw)
    :QWidget(parent),mainwin(mw), ui(new Ui::Daily)
{
    ui->setupUi(this);

    // Remove Incomplete Extras Tab
    ui->tabWidget->removeTab(3);

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

    snapGV=new gGraphView(ui->graphMainArea,shared);
    snapGV->setMinimumSize(172,172);
    snapGV->hideSplitter();
    snapGV->hide();
    scrollbar=new MyScrollBar(ui->graphMainArea);
    scrollbar->setOrientation(Qt::Vertical);
    scrollbar->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
    scrollbar->setMaximumWidth(20);

    GraphView->setScrollBar(scrollbar);
    layout->addWidget(GraphView,1);
    layout->addWidget(scrollbar,0);

    SF=new gGraph(GraphView,"Event Flags",default_height);
    FRW=new gGraph(GraphView,"Flow Rate",default_height);
    AHI=new gGraph(GraphView,"AHI",default_height);
    MP=new gGraph(GraphView,"Mask Pressure",default_height);
    PRD=new gGraph(GraphView,"Pressure",default_height);
    LEAK=new gGraph(GraphView,"Leak",default_height);
    SNORE=new gGraph(GraphView,"Snore",default_height);
    RR=new gGraph(GraphView,"Respiratory Rate",default_height);
    TV=new gGraph(GraphView,"Tidal Volume",default_height);
    MV=new gGraph(GraphView,"Minute Ventilation",default_height);
    FLG=new gGraph(GraphView,"Flow Limitation",default_height);
    PTB=new gGraph(GraphView,"Patient Trig. Breath",default_height);
    RE=new gGraph(GraphView,"Respiratory Event",default_height);
    IE=new gGraph(GraphView,"I:E",default_height);
    TE=new gGraph(GraphView,"Te",default_height);
    TI=new gGraph(GraphView,"Ti",default_height);
    TgMV=new gGraph(GraphView,"TgMV",default_height);
    INTPULSE=new gGraph(GraphView,"R-Pulse",default_height,1);
    INTSPO2=new gGraph(GraphView,"R-SPO2",default_height,1);
    PULSE=new gGraph(GraphView,"Pulse",default_height,1);
    SPO2=new gGraph(GraphView,"SPO2",default_height,1);
    PLETHY=new gGraph(GraphView,"Plethy",default_height,1);

    // Event Pie Chart (for snapshot purposes)
    // TODO: Convert snapGV to generic for snapshotting multiple graphs (like reports does)
    GAHI=new gGraph(snapGV,"Breakdown",172);
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
    fg->AddLayer((new gFlagsLine(CPAP_CSR,QColor("light green"),"CSR",false,FT_Span)));
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
    //fg->AddLayer(AddCPAP(new gFlagsLine(flags[8],QColor("dark green"),"U0E")));
    //fg->AddLayer(AddCPAP(new gFlagsLine(flags[10],QColor("red"),"VS2"));
    SF->setBlockZoom(true);
    SF->AddLayer(AddCPAP(fg));
    SF->AddLayer(new gShadowArea());
    SF->AddLayer(new gYSpacer(),LayerLeft,gYAxis::Margin);
    //SF->AddLayer(new gFooBar(),LayerBottom,0,1);
    SF->AddLayer(new gXAxis(Qt::black,false),LayerBottom,0,gXAxis::Margin);


    gLineChart *l;
    l=new gLineChart(CPAP_FlowRate,Qt::black,false,false);
    gLineOverlaySummary *los=new gLineOverlaySummary("Selection AHI",5,-3);
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
    FRW->AddLayer(AddCPAP(los));


    gGraph *graphs[]={ PRD, LEAK, AHI, SNORE, PTB, MP, RR, MV, TV, FLG, IE, TI, TE, TgMV, SPO2, PLETHY, PULSE,INTPULSE, INTSPO2 };
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
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_Pressure,QColor("dark green"),square)));
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_EPAP,Qt::blue,square)));
    PRD->AddLayer(AddCPAP(new gLineChart(CPAP_IPAP,Qt::red,square)));

    AHI->AddLayer(AddCPAP(new AHIChart(Qt::darkYellow)));
    LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_Leak,Qt::darkYellow,square)));
    LEAK->AddLayer(AddCPAP(new gLineChart(CPAP_MaxLeak,Qt::darkRed,square)));
    SNORE->AddLayer(AddCPAP(new gLineChart(CPAP_Snore,Qt::darkGray,true)));

    PTB->AddLayer(AddCPAP(new gLineChart(CPAP_PTB,Qt::gray,square)));
    MP->AddLayer(AddCPAP(new gLineChart(CPAP_MaskPressure,Qt::blue,false)));
    RR->AddLayer(AddCPAP(new gLineChart(CPAP_RespRate,Qt::darkMagenta,square)));
    MV->AddLayer(AddCPAP(new gLineChart(CPAP_MinuteVent,Qt::darkCyan,square)));
    TV->AddLayer(AddCPAP(new gLineChart(CPAP_TidalVolume,Qt::magenta,square)));
    FLG->AddLayer(AddCPAP(new gLineChart(CPAP_FLG,Qt::darkBlue,true)));
    //RE->AddLayer(AddCPAP(new gLineChart(CPAP_RespiratoryEvent,Qt::magenta,true)));
    IE->AddLayer(AddCPAP(new gLineChart(CPAP_IE,Qt::darkRed,square)));
    TE->AddLayer(AddCPAP(new gLineChart(CPAP_Te,Qt::darkGreen,square)));
    TI->AddLayer(AddCPAP(new gLineChart(CPAP_Ti,Qt::darkBlue,square)));
    TgMV->AddLayer(AddCPAP(new gLineChart(CPAP_TgMV,Qt::darkCyan,square)));
    INTPULSE->AddLayer(AddCPAP(new gLineChart(OXI_Pulse,Qt::red,square)));
    INTSPO2->AddLayer(AddCPAP(new gLineChart(OXI_SPO2,Qt::blue,square)));
    PULSE->AddLayer(AddOXI(new gLineChart(OXI_Pulse,Qt::red,square)));
    SPO2->AddLayer(AddOXI(new gLineChart(OXI_SPO2,Qt::blue,square)));
    PLETHY->AddLayer(AddOXI(new gLineChart(OXI_Plethy,Qt::darkBlue,false)));

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
    connect(ui->webView,SIGNAL(linkClicked(QUrl)),this,SLOT(on_Link_clicked(QUrl)));

    if (!PROFILE.Exists("EventViewSize")) PROFILE["EventViewSize"]=4;
    int ews=PROFILE["EventViewSize"].toInt();
    ui->evViewSlider->setValue(ews);
    ui->evViewLCD->display(ews);

    GraphView->LoadSettings("Daily");

    // TODO: Add preference to hide do this for Widget Haters..
    //ui->calNavWidget->hide();
}

Daily::~Daily()
{
    GraphView->SaveSettings("Daily");

    disconnect(ui->webView,SIGNAL(linkClicked(QUrl)),this,SLOT(on_Link_clicked(QUrl)));
    // Save any last minute changes..
    if (previous_date.isValid())
        Unload(previous_date);

//    delete splitter;
    delete ui;
}
void Daily::on_Link_clicked(const QUrl &url)
{
    QString code=url.toString().section("=",0,0).toLower();
    QString data=url.toString().section("=",1);
    int sid=data.toInt();
    Day *day=NULL;
    if (code=="cpap")  {
        day=PROFILE.GetDay(previous_date,MT_CPAP);
    } else if (code=="oxi") {
        day=PROFILE.GetDay(previous_date,MT_OXIMETER);
    } else if (code=="event")  {
        QList<QTreeWidgetItem *> list=ui->treeWidget->findItems(data,Qt::MatchContains);
        if (list.size()>0) {
            ui->treeWidget->collapseAll();
            ui->treeWidget->expandItem(list.at(0));
            QTreeWidgetItem *wi=list.at(0)->child(0);
            ui->treeWidget->setCurrentItem(wi);
            ui->tabWidget->setCurrentIndex(1);
        }
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
    QDate d=PROFILE.LastDay();
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
                QString st=schema::channel[m.key()].description();
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
    QTextCharFormat bold;
    QTextCharFormat cpapcol;
    QTextCharFormat normal;
    QTextCharFormat oxiday;
    bold.setFontWeight(QFont::Bold);
    cpapcol.setForeground(QBrush(Qt::blue, Qt::SolidPattern));
    cpapcol.setFontWeight(QFont::Bold);
    oxiday.setForeground(QBrush(Qt::red, Qt::SolidPattern));
    oxiday.setFontWeight(QFont::Bold);
    bool hascpap=PROFILE.GetDay(date,MT_CPAP)!=NULL;
    bool hasoxi=PROFILE.GetDay(date,MT_OXIMETER)!=NULL;

    if (hascpap) {
        if (hasoxi) {
            ui->calendar->setDateTextFormat(date,oxiday);
        } else {
            ui->calendar->setDateTextFormat(date,cpapcol);
        }
    } else if (PROFILE.GetDay(date)) {
        ui->calendar->setDateTextFormat(date,bold);
    } else {
        ui->calendar->setDateTextFormat(date,normal);
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

    Load(ui->calendar->selectedDate());
    ui->calButton->setText(ui->calendar->selectedDate().toString(Qt::TextDate));
    ui->calendar->setFocus(Qt::ActiveWindowFocusReason);
}
void Daily::ResetGraphLayout()
{
    GraphView->resetLayout();
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
    lastcpapday=cpap;
    QString html="<html><head><style type='text/css'>"
    "p,a,td,body { font-family: 'FreeSans', 'Sans Serif'; }"
    "p,a,td,body { font-size: 12px; }"
    "a:link,a:visited { color: inherit; text-decoration: none; font-weight: normal;}"
    "a:hover { background-color: inherit; color: inherit; text-decoration:none; font-weight: bold; }"
    "</style>"
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

    QString epr,modestr;
    //float iap90,eap90;
    CPAPMode mode=MODE_UNKNOWN;
    PRTypes pr;
    QString a;
    if (cpap) {
        mode=(CPAPMode)cpap->settings_max(CPAP_Mode);
        pr=(PRTypes)cpap->settings_max(PRS1_FlexMode);
        if (pr==PR_NONE)
           epr=tr(" No Pressure Relief");
        else {
            //epr=schema::channel[PRS1_FlexSet].optionString(pr)+QString(" x%1").arg((int)cpap->settings_max(PRS1_FlexSet));
        }
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
        QString submodel=tr("Unknown Model");

        //html+="<tr><td colspan=4 align=center><i>"+tr("Machine Information")+"</i></td></tr>\n";
        if (cpap->machine->properties.find("SubModel")!=cpap->machine->properties.end())
            submodel=" <br>"+cpap->machine->properties["SubModel"];
        html+="<tr><td colspan=4 align=center><b>"+cpap->machine->properties["Brand"]+"</b> <br>"+cpap->machine->properties["Model"]+" "+cpap->machine->properties["ModelNumber"]+submodel+"</td></tr>\n";
        if (PROFILE.Exists("ShowSerialNumbers") && PROFILE["ShowSerialNumbers"].toBool()) {
            html+="<tr><td colspan=4 align=center>"+cpap->machine->properties["Serial"]+"</td></tr>\n";
        }

        html+="<tr><td align='center'><b>Date</b></td><td align='center'><b>"+tr("Sleep")+"</b></td><td align='center'><b>"+tr("Wake")+"</b></td><td align='center'><b>"+tr("Hours")+"</b></td></tr>";
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
        if (cpap->machine->GetClass()!="PRS1") {
            cs="4 width='100%' align=center>";
        } else cs="2 width='50%'>";
        html+="<tr><td colspan="+cs+"<table cellspacing=0 cellpadding=1 border=0 width='100%'>"
        "<tr><td align='right' bgcolor='#F88017'><b><font color='black'>"+tr("AHI")+"</font></b></td><td width=20% bgcolor='#F88017'><b><font color='black'>"+QString().sprintf("%.2f",ahi)+"</font></b></td></tr>\n"
        "<tr><td align='right' bgcolor='#4040ff'><b><font color='white'>&nbsp;<a href='event=Hypopnea'>"+tr("Hypopnea")+"</a></font></b></td><td bgcolor='#4040ff'><font color='white'>"+QString().sprintf("%.2f",hi)+"</font></td></tr>\n";
        if (cpap->machine->GetClass()=="ResMed") {
            html+="<tr><td align='right' bgcolor='#208020'><b>&nbsp;<a href='event=Apnea'>"+tr("Unspecified Apnea")+"</a></b></td><td bgcolor='#208020'>"+QString().sprintf("%.2f",uai)+"</td></tr>\n";
        }
        html+="<tr><td align='right' bgcolor='#40afbf'><b>&nbsp;<a href='event=Obstructive'>"+tr("Obstructive")+"</a></b></td><td bgcolor='#40afbf'>"+QString().sprintf("%.2f",oai)+"</td></tr>\n"
        "<tr><td align='right' bgcolor='#b254cd'><b>&nbsp;<a href='event=Clear Airway'>"+tr("Clear Airway")+"</a></b></td><td bgcolor='#b254cd'>"+QString().sprintf("%.2f",cai)+"</td></tr>\n"
        "</table></td>";

        if (cpap->machine->GetClass()=="PRS1") {
            html+="<td colspan=2><table cellspacing=0 cellpadding=1 border=0 width='100%'>"
            "<tr><td align='right' bgcolor='#ffff80'><b>&nbsp;<a href='event=Respiratory Effort'>"+tr("RERA")+"</a></b></td><td width=20% bgcolor='#ffff80'>"+QString().sprintf("%.2f",rei)+"</td></tr>\n"
            "<tr><td align='right' bgcolor='#404040'><b>&nbsp;<font color='white'><a href='event=Flow Limit'>"+tr("Flow Limit")+"</a></font></b></td><td bgcolor='#404040'><font color='white'>"+a.sprintf("%.2f",fli)+"</font></td></tr>\n"
            "<tr><td align='right' bgcolor='#ff4040'><b>&nbsp;<a href='event=Vibratory snore'>"+tr("Vsnore")+"</a></b></td><td bgcolor='#ff4040'>"+QString().sprintf("%.2f",vsi)+"</td></tr>\n"
            "<tr><td align='right' bgcolor='#80ff80'><b>&nbsp;<a href='event=Cheyne Stokes'>"+tr("PB/CSR")+"</a></b></td><td bgcolor='#80ff80'>"+QString().sprintf("%.2f",csr)+"%</td></tr>\n"
            "</table></td>";
        } else if (cpap->machine->GetClass()=="Intellipap") {
            html+="<td colspan=2><table cellspacing=0 cellpadding=2 border=0 width='100%'>"
            "<tr><td align='right' bgcolor='#ffff80'><b><a href='event=NRI'>"+tr("NRI")+"</a></b></td><td width=20% bgcolor='#ffff80'>"+QString().sprintf("%.2f",nri)+"</td></tr>\n"
            "<tr><td align='right' bgcolor='#404040'><b><font color='white'><a href='event=Leak'>"+tr("Leak Idx")+"</a></font></b></td><td bgcolor='#404040'><font color='white'>"+a.sprintf("%.2f",lki)+"</font></td></tr>\n"
            "<tr><td align='right' bgcolor='#ff4040'><b><a href='event=VSnore'>"+tr("Vibratory Snore")+"</a></b></td><td bgcolor='#ff4040'>"+QString().sprintf("%.2f",vsi)+"</td></tr>\n"
            "<tr><td align='right' bgcolor='#80ff80'><b><a href='event=ExP'>"+tr("Exhalation Puff")+"</a></b></td><td bgcolor='#80ff80'>"+QString().sprintf("%.2f",exp)+"%</td></tr>\n"
            "</table></td>";

        }


        // Note, this may not be a problem since Qt bug 13622 was discovered
        // as it only relates to text drawing, which the Pie chart does not do
        // ^^ Scratch that.. pie now includes text..

        if (PROFILE["EnableGraphSnapshots"].toBool()) {  // AHI Pie Chart
            if (ahi+rei+fli>0) {
                html+="</tr>\n"; //<tr><td colspan=4 align=center><i>"+tr("Event Breakdown")+"</i></td></tr>\n";
                //G_AHI->setFixedSize(gwwidth,120);
                QPixmap pixmap=snapGV->renderPixmap(172,172,false); //gwwidth,gwheight,false);
                QByteArray byteArray;
                QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
                buffer.open(QIODevice::WriteOnly);
                pixmap.save(&buffer, "PNG");
                html += "<tr><td colspan=4 align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\"></td></tr>\n";
            } else {
                html += "<tr><td colspan=4 align=center><img src=\"qrc:/docs/0.0.gif\"></td></tr>\n";
            }
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
            CPAP_MinuteVent,CPAP_RespRate, CPAP_RespEvent,CPAP_FLG,
            CPAP_Leak,CPAP_Snore,CPAP_IE,CPAP_Ti,CPAP_Te, CPAP_TgMV,
            CPAP_TidalVolume, OXI_Pulse, OXI_SPO2
        };
        int numchans=sizeof(chans)/sizeof(ChannelID);
        int suboffset;
        for (int i=0;i<numchans;i++) {

            ChannelID code=chans[i];
            if (cpap && cpap->channelHasData(code)) {
                if (code==CPAP_Leak) suboffset=PROFILE["IntentionalLeak"].toDouble(); else suboffset=0;
                html+="<tr><td align=left>"+schema::channel[code].label();
                html+="</td><td>"+a.sprintf("%.2f",cpap->min(code)-suboffset);
                html+="</td><td>"+a.sprintf("%.2f",cpap->wavg(code)-suboffset);
                html+="</td><td>"+a.sprintf("%.2f",cpap->p90(code)-suboffset);
                html+="</td><td>"+a.sprintf("%.2f",cpap->max(code)-suboffset);
                html+="</td><tr>";
            }
            if (oxi && oxi->channelHasData(code)) {
                html+="<tr><td align=left>"+schema::channel[code].label();
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
      //  if ((*profile)["EnableGraphSnapshots"].toBool()) {
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

        //}
        html+="</table><hr height=2><table cellpadding=0 cellspacing=0 border=0 width=100%>";
        html+="<tr><td align=center>SessionID</td><td align=center>Date</td><td align=center>Start</td><td align=center>End</td></tr>";
        QDateTime fd,ld;
        bool corrupted_waveform=false;
        if (cpap) {
            for (QVector<Session *>::iterator s=cpap->begin();s!=cpap->end();s++) {
                fd=QDateTime::fromTime_t((*s)->first()/1000L);
                ld=QDateTime::fromTime_t((*s)->last()/1000L);
                QHash<ChannelID,QVariant>::iterator i=(*s)->settings.find("BrokenWaveform");
                if ((i!=(*s)->settings.end()) && i.value().toBool()) corrupted_waveform=true;
                tmp.sprintf(("<tr><td align=center><a href='cpap=%i'>%08i</a></td><td align=center>"+fd.date().toString(Qt::SystemLocaleShortDate)+"</td><td align=center>"+fd.toString("HH:mm ")+"</td><td align=center>"+ld.toString("HH:mm")+"</td></tr>").toLatin1(),(*s)->session(),(*s)->session());
                html+=tmp;
            }
        }
        if (oxi) {
            for (QVector<Session *>::iterator s=oxi->begin();s!=oxi->end();s++) {
                fd=QDateTime::fromTime_t((*s)->first()/1000L);
                ld=QDateTime::fromTime_t((*s)->last()/1000L);
                QHash<ChannelID,QVariant>::iterator i=(*s)->settings.find("BrokenWaveform");
                if ((i!=(*s)->settings.end()) && i.value().toBool()) corrupted_waveform=true;
                tmp.sprintf(("<tr><td align=center><a href='oxi=%i'>%08i</a></td><td align=center>"+fd.date().toString(Qt::SystemLocaleShortDate)+"</td><td align=center>"+fd.toString("HH:mm ")+"</td><td align=center>"+ld.toString("HH:mm")+"</td></tr>").toLatin1(),(*s)->session(),(*s)->session());
                html+=tmp;
            }
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
        ui->JournalNotes->setHtml(journal->settings[Journal_Notes].toString());
    }

}
void Daily::Unload(QDate date)
{
    Session *journal=GetJournalSession(date);
    if (!ui->JournalNotes->toPlainText().isEmpty()) {
        QString jhtml=ui->JournalNotes->toHtml();
        if (journal) {
            if (journal->settings[Journal_Notes]!=jhtml) {
                journal->settings[Journal_Notes]=jhtml;
                journal->SetChanged(true);
            }

        } else {
            journal=CreateJournalSession(date);
            journal->settings[Journal_Notes]=jhtml;
            journal->SetChanged(true);
        }

    }
    if (journal) {
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
void Daily::on_EnergySlider_sliderMoved(int position)
{
    position=position;
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

    return;
    if (0) {
/*        if (ui->treeWidget->selectedItems().size()==0) return;
        QTreeWidgetItem *item=ui->treeWidget->selectedItems().at(0);
        if (!item) return;
        QDateTime d;
        if (!item->text(1).isEmpty()) {
            d=d.fromString(item->text(1),"yyyy-MM-dd HH:mm:ss");


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
*/
    } else {
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
}
