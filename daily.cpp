/*
 Daily Panel
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "daily.h"
#include "ui_daily.h"

#include <QTextCharFormat>
#include <QTextBlock>
#include <QColorDialog>
#include <QBuffer>
#include <QPixmap>

#include "SleepLib/session.h"
#include "Graphs/graphdata_custom.h"
#include "Graphs/gLineChart.h"
#include "Graphs/gLineOverlay.h"
#include "Graphs/gFlagsLine.h"
#include "Graphs/gFooBar.h"
#include "Graphs/gXAxis.h"
#include "Graphs/gYAxis.h"
#include "Graphs/gCandleStick.h"
#include "Graphs/gBarChart.h"
#include "Graphs/gpiechart.h"

Daily::Daily(QWidget *parent,QGLContext *context) :
    QWidget(parent),
    ui(new Ui::Daily)
{
    ui->setupUi(this);

    shared_context=context;

    QString prof=pref["Profile"].toString();
    profile=Profiles::Get(prof);
    if (!profile) {
        qWarning("Couldn't get profile.. Have to abort!");
        abort();
    }

    gSplitter=new QSplitter(Qt::Vertical,ui->scrollArea);
    gSplitter->setStyleSheet("QSplitter::handle { background-color: 'dark grey'; }");
    gSplitter->setHandleWidth(2);
    //gSplitter->handle
    ui->graphSizer->addWidget(gSplitter);

    //QPalette pal;
    //QColor col("blue");
    //pal.setColor(QPalette::Button, col);
    //gSplitter->setPaletteForegroundColor(QColor("blue"));
    //gSplitter->setBackgroundRole(QPalette::Button);
    //ui->scrollArea->setWidgetResizable(true);
    //gSplitter->setMinimumSize(500,500);
    //gSplitter->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);

    AddCPAPData(flags[3]=new FlagData(CPAP_Hypopnea,4));
    AddCPAPData(flags[0]=new FlagData(CPAP_CSR,7,1,0));
    AddCPAPData(flags[1]=new FlagData(CPAP_ClearAirway,6));
    AddCPAPData(flags[2]=new FlagData(CPAP_Obstructive,5));
    AddCPAPData(flags[4]=new FlagData(CPAP_FlowLimit,3));
    AddCPAPData(flags[5]=new FlagData(CPAP_VSnore,2));
    AddCPAPData(flags[6]=new FlagData(CPAP_RERA,1));
    AddCPAPData(flags[7]=new FlagData(PRS1_PressurePulse,1));
    AddCPAPData(flags[8]=new FlagData(PRS1_Unknown0E,1));
    AddCPAPData(flags[9]=new FlagData(CPAP_Snore,1)); // Snore Index

    AddGraph(SF=new gGraphWindow(gSplitter,tr("Event Flags"),(QGLWidget *)NULL));

    SF->SetLeftMargin(SF->GetLeftMargin()+gYAxis::Margin);
    SF->SetBlockZoom(true);
    SF->AddLayer(new gXAxis());

    bool extras=false; //true;
    int sfc=7;
    if (extras) {
        sfc+=1;
        SF->AddLayer(new gFlagsLine(flags[8],QColor("dark green"),"U0E",7,sfc));
        //SF->AddLayer(new gFlagsLine(flags[8],QColor("red"),"VS2",7,sfc));
    }
    SF->AddLayer(new gFlagsLine(flags[6],QColor("gold"),"RE",6,sfc));
    SF->AddLayer(new gFlagsLine(flags[5],QColor("red"),"VS",5,sfc));
    SF->AddLayer(new gFlagsLine(flags[4],QColor("black"),"FL",4,sfc));
    SF->AddLayer(new gFlagsLine(flags[3],QColor("blue"),"H",3,sfc));
    SF->AddLayer(new gFlagsLine(flags[2],QColor("aqua"),"OA",2,sfc));
    SF->AddLayer(new gFlagsLine(flags[1],QColor("purple"),"CA",1,sfc));
    SF->AddLayer(new gFlagsLine(flags[0],QColor("light green"),"CSR",0,sfc));
    SF->AddLayer(new gFooBar(10,QColor("orange"),QColor("dark grey"),true));
    SF->setMinimumHeight(150+(extras ? 20 : 0));
   // SF->setMaximumHeight(350);

    AddCPAPData(pressure_iap=new EventData(CPAP_IAP));
    AddCPAPData(pressure_eap=new EventData(CPAP_EAP));
    AddCPAPData(prd=new EventData(CPAP_Pressure));
    AddGraph(PRD=new gGraphWindow(gSplitter,tr("Pressure"),SF));
    PRD->AddLayer(new gXAxis());
    PRD->AddLayer(new gYAxis());
    //PRD->AddLayer(new gFooBar());
    bool square=false;
    PRD->AddLayer(new gLineChart(prd,QColor("dark green"),4096,false,false,square));
    PRD->AddLayer(new gLineChart(pressure_iap,Qt::blue,4096,false,true,square));
    PRD->AddLayer(new gLineChart(pressure_eap,Qt::red,4096,false,true,square));
    PRD->setMinimumHeight(150);

    AddCPAPData(leakdata=new EventData(CPAP_Leak,0));
    //leakdata->ForceMinY(0);
    //leakdata->ForceMaxY(120);
    AddGraph(LEAK=new gGraphWindow(gSplitter,tr("Leaks"),SF));
    LEAK->AddLayer(new gXAxis());
    LEAK->AddLayer(new gYAxis());
    //LEAK->AddLayer(new gFooBar());
    LEAK->AddLayer(new gLineChart(leakdata,QColor("purple"),65536,false,false,true));

    LEAK->setMinimumHeight(150);


    AddCPAPData(frw=new WaveData(CPAP_FlowRate,700000)); //FlowRate
   // AddCPAPData(mpw=new WaveData(CPAP_MaskPressure,700000)); //FlowRate
    // Holy crap resmed stuff is huge..
    AddGraph(FRW=new gGraphWindow(gSplitter,tr("Flow Rate"),SF));
    //FRW->AddLayer(new gFooBar());
    FRW->AddLayer(new gYAxis());
    FRW->AddLayer(new gXAxis());
    FRW->AddLayer(new gLineOverlayBar(flags[0],QColor("light green"),"CSR"));
    //FRW->AddLayer(new gLineChart(mpw,Qt::blue,700000,true));
    gLineChart *g=new gLineChart(frw,Qt::black,700000,true);
    g->ReportEmpty(true);
    FRW->AddLayer(g);
    FRW->AddLayer(new gLineOverlayBar(flags[3],QColor("blue"),"H"));
    FRW->AddLayer(new gLineOverlayBar(flags[7],QColor("red"),"PR",LOT_Dot));
    FRW->AddLayer(new gLineOverlayBar(flags[6],QColor("gold"),"RE"));
    //FRW->AddLayer(new gLineOverlayBar(flags[9],QColor("dark green"),"U0E"));
    FRW->AddLayer(new gLineOverlayBar(flags[5],QColor("red"),"VS"));
    FRW->AddLayer(new gLineOverlayBar(flags[4],QColor("black"),"FL"));
    FRW->AddLayer(new gLineOverlayBar(flags[2],QColor("aqua"),"OA"));
    FRW->AddLayer(new gLineOverlayBar(flags[1],QColor("purple"),"CA"));

    FRW->setMinimumHeight(190);

    AddCPAPData(snore=new EventData(CPAP_Snore,0));
    AddGraph(SNORE=new gGraphWindow(gSplitter,tr("Snore"),SF));
    SNORE->AddLayer(new gXAxis());
    SNORE->AddLayer(new gYAxis());
    SNORE->AddLayer(new gLineChart(snore,Qt::black,4096,false,false,true));
    SNORE->setMinimumHeight(150);

    AddCPAPData(flg=new EventData(CPAP_FlowLimitGraph,0));
    AddGraph(FLG=new gGraphWindow(gSplitter,tr("Flow Limitation"),SF));
    FLG->AddLayer(new gXAxis());
    FLG->AddLayer(new gYAxis());
    FLG->AddLayer(new gLineChart(flg,Qt::black,4096,false,false,true));
    FLG->setMinimumHeight(150);


    AddCPAPData(mv=new WaveData(CPAP_MinuteVentilation));
    AddGraph(MV=new gGraphWindow(gSplitter,tr("Minute Ventilation"),SF));
    MV->AddLayer(new gXAxis());
    MV->AddLayer(new gYAxis());
    MV->AddLayer(new gLineChart(mv,QColor(0x20,0x20,0x7f),65536,false,false,false));
    MV->setMinimumHeight(150);

    AddCPAPData(tv=new WaveData(CPAP_TidalVolume));
    AddGraph(TV=new gGraphWindow(gSplitter,tr("Tidal Volume"),SF));
    TV->AddLayer(new gXAxis());
    TV->AddLayer(new gYAxis());
    TV->AddLayer(new gLineChart(tv,QColor(0x7f,0x20,0x20),65536,false,false,false));
    TV->setMinimumHeight(150);

    AddCPAPData(rr=new WaveData(CPAP_RespiratoryRate));
    AddGraph(RR=new gGraphWindow(gSplitter,tr("Respiratory Rate"),SF));
    RR->AddLayer(new gXAxis());
    RR->AddLayer(new gYAxis());
    RR->AddLayer(new gLineChart(rr,Qt::gray,65536,false,false,false));
    RR->setMinimumHeight(150);


    AddOXIData(pulse=new EventData(OXI_Pulse,0,65536,true));
    //pulse->ForceMinY(40);
    //pulse->ForceMaxY(120);
    AddGraph(PULSE=new gGraphWindow(gSplitter,tr("Pulse & SpO2"),SF));
    PULSE->AddLayer(new gXAxis());
    PULSE->AddLayer(new gYAxis());
   // PULSE->AddLayer(new gFooBar());
    PULSE->AddLayer(new gLineChart(pulse,Qt::red,65536,false,false,true));

    PULSE->setMinimumHeight(150);

    AddOXIData(spo2=new EventData(OXI_SPO2,0,65536,true));
    //spo2->ForceMinY(60);
    //spo2->ForceMaxY(100);
//    AddGraph(SPO2=new gGraphWindow(gSplitter,tr("SpO2"),SF));
//    SPO2->AddLayer(new gXAxis());
//    SPO2->AddLayer(new gYAxis());
   // SPO2->AddLayer(new gFooBar());
    PULSE->AddLayer(new gLineChart(spo2,Qt::blue,65536,false,false,true));
//    SPO2->setMinimumHeight(150);
//    SPO2->LinkZoom(PULSE);
//    PULSE->LinkZoom(SPO2);
//    SPO2->hide();
    PULSE->hide();

    AddCPAPData(tap_eap=new TAPData(CPAP_EAP));
    AddCPAPData(tap_iap=new TAPData(CPAP_IAP));
    AddCPAPData(tap=new TAPData(CPAP_Pressure));


    TAP=new gGraphWindow(gSplitter,"",SF);
    //TAP->SetMargins(20,15,5,50);
    TAP->SetMargins(0,0,0,0);
    TAP->AddLayer(new gCandleStick(tap));
    //TAP->AddLayer(new gPieChart(tap));

    TAP_EAP=new gGraphWindow(gSplitter,"",SF);
    TAP_EAP->SetMargins(0,0,0,0);
    TAP_EAP->AddLayer(new gCandleStick(tap_eap));

    TAP_IAP=new gGraphWindow(gSplitter,"",SF);
    TAP_IAP->SetMargins(0,0,0,0);
    TAP_IAP->AddLayer(new gCandleStick(tap_iap));

    G_AHI=new gGraphWindow(gSplitter,"",SF);
    G_AHI->SetMargins(0,0,0,0);
    AddCPAPData(g_ahi=new AHIData());
    //gCandleStick *l=new gCandleStick(g_ahi);
    gPieChart *l=new gPieChart(g_ahi);
    l->AddName(tr("H"));
    l->AddName(tr("OA"));
    l->AddName(tr("CA"));
    l->AddName(tr("RE"));
    l->AddName(tr("FL"));
    l->AddName(tr("CSR"));
    l->color.clear();
    l->color.push_back(QColor(0x40,0x40,0xff,0xff)); // blue
    l->color.push_back(QColor(0x40,0xaf,0xbf,0xff)); // aqua
    l->color.push_back(QColor(0xb2,0x54,0xcd,0xff)); // purple
    l->color.push_back(QColor(0xff,0xff,0x80,0xff));  // yellow
    l->color.push_back(QColor(0x40,0x40,0x40,0xff)); // dark grey
    l->color.push_back(QColor(0x60,0xff,0x60,0xff)); // green
    G_AHI->AddLayer(l);
    G_AHI->SetGradientBackground(false);
    //G_AHI->setMaximumSize(2000,30);
    //TAP->setMaximumSize(2000,30);
    NoData=new QLabel(tr("No CPAP Data"),gSplitter);
    NoData->setAlignment(Qt::AlignCenter);
    QFont font("FreeSans",20); //NoData->font();
    //font.setBold(true);
    NoData->setFont(font);
    NoData->hide();

    G_AHI->hide();
    TAP->hide();
    TAP_IAP->hide();
    TAP_EAP->hide();

    FRW->LinkZoom(SF);
    FRW->LinkZoom(PRD);
    FRW->LinkZoom(LEAK);
    FRW->LinkZoom(SNORE);
    FRW->LinkZoom(MV);
    FRW->LinkZoom(TV);
    FRW->LinkZoom(RR);
    FRW->LinkZoom(FLG);
    SF->LinkZoom(FRW);
    SF->LinkZoom(PRD);
    SF->LinkZoom(LEAK);
    SF->LinkZoom(SNORE);
    SF->LinkZoom(MV);
    SF->LinkZoom(TV);
    SF->LinkZoom(RR);
    SF->LinkZoom(FLG);
    PRD->LinkZoom(SF);
    PRD->LinkZoom(FRW);
    PRD->LinkZoom(LEAK);
    PRD->LinkZoom(SNORE);
    PRD->LinkZoom(MV);
    PRD->LinkZoom(TV);
    PRD->LinkZoom(RR);
    PRD->LinkZoom(FLG);

    LEAK->LinkZoom(SF);
    LEAK->LinkZoom(FRW);
    LEAK->LinkZoom(PRD);
    LEAK->LinkZoom(SNORE);
    LEAK->LinkZoom(MV);
    LEAK->LinkZoom(TV);
    LEAK->LinkZoom(RR);
    LEAK->LinkZoom(FLG);

    SNORE->LinkZoom(SF);
    SNORE->LinkZoom(FRW);
    SNORE->LinkZoom(PRD);
    SNORE->LinkZoom(LEAK);
    SNORE->LinkZoom(MV);
    SNORE->LinkZoom(TV);
    SNORE->LinkZoom(RR);
    SNORE->LinkZoom(FLG);

    MV->LinkZoom(SF);
    MV->LinkZoom(FRW);
    MV->LinkZoom(PRD);
    MV->LinkZoom(LEAK);
    MV->LinkZoom(SNORE);
    MV->LinkZoom(TV);
    MV->LinkZoom(RR);
    MV->LinkZoom(FLG);

    TV->LinkZoom(SF);
    TV->LinkZoom(FRW);
    TV->LinkZoom(PRD);
    TV->LinkZoom(LEAK);
    TV->LinkZoom(SNORE);
    TV->LinkZoom(MV);
    TV->LinkZoom(RR);
    TV->LinkZoom(FLG);

    RR->LinkZoom(SF);
    RR->LinkZoom(FRW);
    RR->LinkZoom(PRD);
    RR->LinkZoom(LEAK);
    RR->LinkZoom(SNORE);
    RR->LinkZoom(MV);
    RR->LinkZoom(TV);
    RR->LinkZoom(FLG);

    FLG->LinkZoom(SF);
    FLG->LinkZoom(FRW);
    FLG->LinkZoom(PRD);
    FLG->LinkZoom(LEAK);
    FLG->LinkZoom(SNORE);
    FLG->LinkZoom(MV);
    FLG->LinkZoom(TV);
    FLG->LinkZoom(RR);


    gSplitter->addWidget(SF);
    gSplitter->addWidget(FRW);
    gSplitter->addWidget(MV);
    gSplitter->addWidget(TV);
    gSplitter->addWidget(RR);
    gSplitter->addWidget(PRD);
    gSplitter->addWidget(LEAK);
    gSplitter->addWidget(FLG);
    gSplitter->addWidget(SNORE);
    gSplitter->addWidget(NoData);
    gSplitter->addWidget(PULSE);
  //  gSplitter->addWidget(SPO2);
    gSplitter->refresh();


    gSplitter->setChildrenCollapsible(true);  // We set this per widget..
    gSplitter->setCollapsible(gSplitter->indexOf(SF),false);
    gSplitter->setStretchFactor(gSplitter->indexOf(SF),0);
    ui->graphSizer->layout();

    QTextCharFormat format = ui->calendar->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    ui->calendar->setWeekdayTextFormat(Qt::Saturday, format);
    ui->calendar->setWeekdayTextFormat(Qt::Sunday, format);

    ui->tabWidget->setCurrentWidget(ui->details);
    ReloadGraphs();
 }

Daily::~Daily()
{
    // Save any last minute changes..
    if (previous_date.isValid())
        Unload(previous_date);

    delete gSplitter;
    delete ui;
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

    tree->setColumnCount(1); // 1 visible common.. (1 hidden)

    QTreeWidgetItem *root=NULL;//new QTreeWidgetItem((QTreeWidget *)0,QStringList("Stuff"));
    map<MachineCode,QTreeWidgetItem *> mcroot;
    map<MachineCode,int> mccnt;

    for (vector<Session *>::iterator s=day->begin();s!=day->end();s++) {

        map<MachineCode,vector<Event *> >::iterator m;

        QTreeWidgetItem * ti,sroot;

        for (m=(*s)->events.begin();m!=(*s)->events.end();m++) {
            MachineCode code=m->first;
            if (code==CPAP_Leak) continue;
            if (code==CPAP_RespiratoryRate) continue;
            if (code==CPAP_TidalVolume) continue;
            if (code==CPAP_MinuteVentilation) continue;
            if (code==CPAP_FlowLimitGraph) continue;

            // Note this is not so evil on PRS1.
            if (code==CPAP_Pressure) continue;

            if (code==CPAP_Snore) continue;
            if (code==PRS1_Unknown12) continue;
            QTreeWidgetItem *mcr;
            if (mcroot.find(code)==mcroot.end()) {
                QString st=DefaultMCLongNames[m->first];
                if (st.isEmpty())  {
                    st=st.sprintf("Fixme: %i",code);
                }
                QStringList l(st);
                l.append("");
                //QString g="";
                mcroot[code]=mcr=new QTreeWidgetItem(root,l);
                mccnt[code]=0;
            } else {
                mcr=mcroot[code];
            }
            for (vector<Event *>::iterator e=(*s)->events[code].begin();e!=(*s)->events[code].end();e++) {
                QDateTime t=(*e)->time();
                if (code==CPAP_CSR) {
                    t=t.addSecs(-((*(*e))[0]/2));
                }
                QStringList a;
                QString c,b;
                c.sprintf("#%03i: ",++mccnt[code]);
                c+=t.toString(" HH:mm:ss");

                //if ((*e)->fields()) { // Perhaps need a dedicated offset field
                    //b.sprintf(" %02.0f",(*(*e))[0]);
                    //c+=b;
                //}
                a.append(c);
                a.append(t.toString("yyyy-MM-dd HH:mm:ss"));
                mcr->addChild(new QTreeWidgetItem(a));
            }
        }
    }
    int cnt=0;
    for (map<MachineCode,QTreeWidgetItem *>::iterator m=mcroot.begin();m!=mcroot.end();m++) {
        tree->insertTopLevelItem(cnt++,m->second);
    }


    tree->sortByColumn(0,Qt::AscendingOrder);
    //tree->expandAll();
}
void Daily::UpdateCalendarDay(QDate date)
{
    QTextCharFormat bold;
    QTextCharFormat cpapcol;
    QTextCharFormat normal;
    bold.setFontWeight(QFont::Bold);
    cpapcol.setForeground(QBrush(Qt::blue, Qt::SolidPattern));
    cpapcol.setFontWeight(QFont::Bold);

    if (profile->GetDay(date,MT_CPAP)) {
        ui->calendar->setDateTextFormat(date,cpapcol);
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
void Daily::Load(QDate date)
{
    previous_date=date;
    Day *cpap=profile->GetDay(date,MT_CPAP);
    Day *oxi=profile->GetDay(date,MT_OXIMETER);
    Day *sleepstage=profile->GetDay(date,MT_SLEEPSTAGE);

    QString html="<html><head><style type='text/css'>p,a,td,body { font-family: 'FreeSans', 'Sans Serif'; } p,a,td,body { font-size: 12px; } </style>";
    html+="</head>";
    html+="<body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>";
    html+="<table cellspacing=0 cellpadding=2 border=0 width='100%'>\n";
    QString tmp;

    const int gwwidth=270;
    const int gwheight=25;
    UpdateCPAPGraphs(cpap);
    UpdateOXIGraphs(oxi);
    UpdateEventsTree(ui->treeWidget,cpap);



    QString epr,modestr;
    float iap90,eap90;
    CPAPMode mode=MODE_UNKNOWN;
    PRTypes pr;
    QString a;
    if (cpap) {
        mode=(CPAPMode)cpap->summary_max(CPAP_Mode);
        pr=(PRTypes)cpap->summary_max(CPAP_PressureReliefType);
        if (pr==PR_NONE)
           epr=tr(" No Pressure Relief");
        else {
            a.sprintf(" x%i",(int)cpap->summary_max(CPAP_PressureReliefSetting));
            epr=PressureReliefNames[pr]+a;
        }
        modestr=CPAPModeNames[mode];

        float ahi=(cpap->count(CPAP_Obstructive)+cpap->count(CPAP_Hypopnea)+cpap->count(CPAP_ClearAirway))/cpap->hours();
        float csr=(100.0/cpap->hours())*(cpap->sum(CPAP_CSR)/3600.0);
        float oai=cpap->count(CPAP_Obstructive)/cpap->hours();
        float hi=cpap->count(CPAP_Hypopnea)/cpap->hours();
        float cai=cpap->count(CPAP_ClearAirway)/cpap->hours();
        float rei=cpap->count(CPAP_RERA)/cpap->hours();
        float vsi=cpap->count(CPAP_VSnore)/cpap->hours();
        float fli=cpap->count(CPAP_FlowLimit)/cpap->hours();
        //        float p90=cpap->percentile(CPAP_Pressure,0,0.9);
        eap90=cpap->percentile(CPAP_EAP,0,0.9);
        iap90=cpap->percentile(CPAP_IAP,0,0.9);
        QString submodel=tr("Unknown Model");


        //html+="<tr><td colspan=4 align=center><i>"+tr("Machine Information")+"</i></td></tr>\n";
        if (cpap->machine->properties.find("SubModel")!=cpap->machine->properties.end())
            submodel=" <br>"+cpap->machine->properties["SubModel"];
        html+="<tr><td colspan=4 align=center><b>"+cpap->machine->properties["Brand"]+"</b> <br>"+cpap->machine->properties["Model"]+" "+cpap->machine->properties["ModelNumber"]+submodel+"</td></tr>\n";
        if (pref.Exists("ShowSerialNumbers") && pref["ShowSerialNumbers"].toBool()) {
            html+="<tr><td colspan=4 align=center>"+cpap->machine->properties["Serial"]+"</td></tr>\n";
        }

        html+="<tr><td align='center'><b>Date</b></td><td align='center'><b>"+tr("Sleep")+"</b></td><td align='center'><b>"+tr("Wake")+"</b></td><td align='center'><b>"+tr("Hours")+"</b></td></tr>";
        int tt=cpap->total_time();
        html+="<tr><td align='center'>"+cpap->first().date().toString(Qt::SystemLocaleShortDate)+"</td><td align='center'>"+cpap->first().toString("HH:mm")+"</td><td align='center'>"+cpap->last().toString("HH:mm")+"</td><td align='center'>"+a.sprintf("%02i:%02i",tt/3600,tt%60)+"</td></tr>\n";
        html+="<tr><td colspan=4 align=center><hr></td></tr>\n";

        QString cs;
        if (cpap->machine->GetClass()!="PRS1") {
            cs="4 align=center>";
        } else cs="2>";
        html+=("<tr><td colspan="+cs+"<table cellspacing=0 cellpadding=2 border=0 width='100%'>");
        html+=("<tr><td align='right' bgcolor='#F88017'><b><font color='black'>")+tr("AHI")+("</font></b></td><td  bgcolor='#F88017'><b><font color='black'>")+a.sprintf("%.2f",ahi)+("</font></b></td></tr>\n");
        html+=("<tr><td align='right' bgcolor='#4040ff'><b><font color='white'>")+tr("Hypopnea")+("</font></b></td><td bgcolor='#4040ff'><font color='white'>")+a.sprintf("%.2f",hi)+("</font></td></tr>\n");
        html+=("<tr><td align='right' bgcolor='#40afbf'><b>")+tr("Obstructive")+("</b></td><td bgcolor='#40afbf'>")+a.sprintf("%.2f",oai)+("</td></tr>\n");
        html+=("<tr><td align='right' bgcolor='#b254cd'><b>")+tr("ClearAirway")+("</b></td><td bgcolor='#b254cd'>")+a.sprintf("%.2f",cai)+("</td></tr>\n");
        html+="</table></td>";
        if (cpap->machine->GetClass()=="PRS1") {
            html+="<td colspan=2><table cellspacing=0 cellpadding=2 border=0 width='100%'>";
            html+="<tr><td align='right' bgcolor='#ffff80'><b>"+tr("RERA")+("</b></td><td bgcolor='#ffff80'>")+a.sprintf("%.2f",rei)+("</td></tr>\n");
            html+="<tr><td align='right' bgcolor='#404040'><b><font color='white'>"+tr("FlowLimit")+("</font></b></td><td bgcolor='#404040'><font color='white'>")+a.sprintf("%.2f",fli)+("</font></td></tr>\n");
            html+="<tr><td align='right' bgcolor='#ff4040'><b>"+tr("Vsnore")+("</b></td><td bgcolor='#ff4040'>")+a.sprintf("%.2f",vsi)+("</td></tr>\n");
            html+="<tr><td align='right' bgcolor='#80ff80'><b>"+tr("PB/CSR")+("</b></td><td bgcolor='#80ff80'>")+a.sprintf("%.2f",csr)+("%</td></tr>\n");
            html+="</table></td>";
        }
        html+="</tr>";

        html+=("<tr><td colspan=4 align=center><i>")+tr("Event Breakdown")+("</i></td></tr>\n");
        {
            G_AHI->setFixedSize(gwwidth,gwheight);
            QPixmap pixmap=G_AHI->renderPixmap(120,120,false); //gwwidth,gwheight,false);
            QByteArray byteArray;
            QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
            buffer.open(QIODevice::WriteOnly);
            pixmap.save(&buffer, "PNG");
            html += "<tr><td colspan=4 align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\"></td></tr>\n";
        }
        html+=("</table>");
        html+=("<table cellspacing=0 cellpadding=0 border=0 width='100%'>\n");
        //html+=("<tr><td colspan=4>&nbsp;</td></tr>\n");
        html+=("<tr height='2'><td colspan=4 height='2'><hr></td></tr>\n");
        //html+=wxT("<tr><td colspan=4 align=center><hr></td></tr>\n");

        if (mode==MODE_BIPAP) {
            html+=("<tr><td colspan=4 align='center'><i>")+tr("90%&nbsp;EPAP ")+a.sprintf("%.2f",eap90)+tr("cmH2O")+"</td></tr>\n";
            html+=("<tr><td colspan=4 align='center'><i>")+tr("90%&nbsp;IPAP ")+a.sprintf("%.2f",iap90)+"</td></tr>\n";
        } else if (mode==MODE_APAP) {
            html+=("<tr><td colspan=4 align='center'><i>")+tr("90%&nbsp;Pressure ")+a.sprintf("%.2f",cpap->summary_weighted_avg(CPAP_PressurePercentValue))+("</i></td></tr>\n");
        } else if (mode==MODE_CPAP) {
            html+=("<tr><td colspan=4 align='center'><i>")+tr("Pressure ")+a.sprintf("%.2f",cpap->summary_min(CPAP_PressureMin))+("</i></td></tr>\n");
        }
        //html+=("<tr><td colspan=4 align=center>&nbsp;</td></tr>\n");

        html+=("<tr><td> </td><td><b>Min</b></td><td><b>Avg</b></td><td><b>Max</b></td></tr>");

        if (mode==MODE_APAP) {
            html+="<tr><td align=left>"+tr("Pressure:")+"</td><td>"+a.sprintf("%.2f",cpap->summary_min(CPAP_PressureMinAchieved));
            html+=(" </td><td>")+a.sprintf("%.2f",cpap->summary_weighted_avg(CPAP_PressureAverage));
            html+=("</td><td>")+a.sprintf("%.2f",cpap->summary_max(CPAP_PressureMaxAchieved))+("</td></tr>");

            //  html+=wxT("<tr><td><b>")+_("90%&nbsp;Pressure")+wxT("</b></td><td>")+wxString::Format(wxT("%.1fcmH2O"),p90)+wxT("</td></tr>\n");
        } else if (mode==MODE_BIPAP) {
            html+=("<tr><td align=left>"+tr("EPAP:")+"</td><td>")+a.sprintf("%.2f",cpap->summary_min(BIPAP_EAPMin));
            html+=(" </td><td>")+a.sprintf("%.2f",cpap->summary_weighted_avg(BIPAP_EAPAverage));
            html+=("</td><td>")+a.sprintf("%.2f",cpap->summary_max(BIPAP_EAPMax))+("</td></tr>");

            html+=("<tr><td> align=left"+tr("IPAP:")+"</td><td>")+a.sprintf("%.2f",cpap->summary_min(BIPAP_IAPMin));
            html+=("</td><td>")+a.sprintf("%.2f",cpap->summary_weighted_avg(BIPAP_IAPAverage));
            html+=("</td><td>")+a.sprintf("%.2f",cpap->summary_max(BIPAP_IAPMax))+("</td></tr>");

        }
        html+="<tr><td align=left>"+tr("Leak:");
        html+="</td><td>"+a.sprintf("%.2f",cpap->summary_min(CPAP_LeakMinimum));
        html+="</td><td>"+a.sprintf("%.2f",cpap->summary_weighted_avg(CPAP_LeakAverage));
        html+="</td><td>"+a.sprintf("%.2f",cpap->summary_max(CPAP_LeakMaximum))+("</td><tr>");

        html+="<tr><td align=left>"+tr("Snore:");
        html+="</td><td>"+a.sprintf("%.2f",cpap->summary_min(CPAP_SnoreMinimum));
        html+="</td><td>"+a.sprintf("%.2f",cpap->summary_avg(CPAP_SnoreAverage));
        html+="</td><td>"+a.sprintf("%.2f",cpap->summary_max(CPAP_SnoreMaximum))+("</td><tr>");
        FRW->show();
        PRD->show();
        LEAK->show();
        SF->show();
        SNORE->show();
    } else {
        html+="<tr><td colspan=4 align=center><i>"+tr("No CPAP data available")+"</i></td></tr>";
        html+="<tr><td colspan=4>&nbsp;</td></tr>\n";

        //TAP_EAP->Show(false);
        //TAP_IAP->Show(false);
        //G_AHI->Show(false);
        FRW->hide();
        PRD->hide();
        LEAK->hide();
        SF->hide();
        SNORE->hide();
    }
    if (cpap && (cpap->machine->GetClass()=="ResMed")) {
        MV->show();
        TV->show();
        RR->show();
        FLG->show();
    } else {
        MV->hide();
        TV->hide();
        RR->hide();
        FLG->hide();
    }

    if (oxi) {
        html+="<tr><td>"+tr("Pulse:");
        html+="</td><td>"+a.sprintf("%.2fbpm",oxi->summary_min(OXI_PulseMin));
        html+="</td><td>"+a.sprintf("%.2fbpm",oxi->summary_avg(OXI_PulseAverage));
        html+="</td><td>"+a.sprintf("%.2fbpm",oxi->summary_max(OXI_PulseMax))+"</td><tr>";

        html+="<tr><td>"+tr("SpO2:");
        html+="</td><td>"+a.sprintf("%.2f%%",oxi->summary_min(OXI_SPO2Min));
        html+="</td><td>"+a.sprintf("%.2f%%",oxi->summary_avg(OXI_SPO2Average));
        html+="</td><td>"+a.sprintf("%.2f%%",oxi->summary_max(OXI_SPO2Max))+"</td><tr>";

        //html+=wxT("<tr><td colspan=4>&nbsp;</td></tr>\n");

        PULSE->show();
        //SPO2->show();
    } else {
        PULSE->hide();
        //SPO2->hide();
    }
    if (!cpap && !oxi) {
        NoData->setText(tr("No CPAP Data for ")+date.toString(Qt::SystemLocaleLongDate));
        NoData->show();
    } else
        NoData->hide();

    if (cpap) {
        if (mode==MODE_BIPAP) {

        } else if (mode==MODE_APAP) {
            html+=("<tr><td colspan=4 align=center><i>")+tr("Time@Pressure")+("</i></td></tr>\n");
            TAP->setFixedSize(gwwidth,gwheight);

            QPixmap pixmap=TAP->renderPixmap(gwwidth,gwheight,false);
            QByteArray byteArray;
            QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
            buffer.open(QIODevice::WriteOnly);
            pixmap.save(&buffer, "PNG");
            html+="<tr><td colspan=4 align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\"></td></tr>\n";
        }
        html+="</table><hr height=2><table cellpadding=0 cellspacing=0 border=0 width=100%>";
        html+="<tr><td align=center>SessionID</td><td align=center>Date</td><td align=center>Start</td><td align=center>End</td></tr>";
        for (vector<Session *>::iterator s=cpap->begin();s!=cpap->end();s++) {

            tmp.sprintf(("<tr><td align=center>%08x</td><td align=center>"+(*s)->first().toString("yyyy-MM-dd")+"</td><td align=center>"+(*s)->first().toString("HH:mm ")+"</td><td align=center>"+(*s)->last().toString("HH:mm")+"</td></tr>").toLatin1(),(*s)->session());
            html+=tmp;
        }
        html+="</table>";
    }
    html+="</html>";

    ui->webView->setHtml(html);

    ui->JournalNotes->clear();
    Session *journal=GetJournalSession(date);
    if (journal) {
        ui->JournalNotes->setHtml(journal->summary[GEN_Notes].toString());
    }
    RedrawGraphs();

}
void Daily::Unload(QDate date)
{
    Session *journal=GetJournalSession(date);
    if (!ui->JournalNotes->toPlainText().isEmpty()) {
        QString jhtml=ui->JournalNotes->toHtml();
        if (journal) {
            if (journal->summary[GEN_Notes]!=jhtml) {
                journal->summary[GEN_Notes]=jhtml;
                journal->SetChanged(true);
            }

        } else {
            journal=CreateJournalSession(date);
            journal->summary[GEN_Notes]=jhtml;
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
    sess->set_first(dt);
    dt=dt.addSecs(3600);
    sess->set_last(dt);
    sess->SetChanged(true);
    m->AddSession(sess,profile);
    return sess;
}
Session * Daily::GetJournalSession(QDate date) // Get the first journal session
{
    Day *journal=profile->GetDay(date,MT_JOURNAL);
    if (!journal)
        return NULL; //CreateJournalSession(date);
    vector<Session *>::iterator s;
    s=journal->begin();
    if (s!=journal->end())
        return *s;
    return NULL;
}
void Daily::on_EnergySlider_sliderMoved(int position)
{
    Session *s=GetJournalSession(previous_date);
    if (!s)
        s=CreateJournalSession(previous_date);

    s->summary[GEN_Energy]=position;
    s->SetChanged(true);
}

void Daily::UpdateCPAPGraphs(Day *day)
{
    //if (!day) return;
    if (day) {
        day->OpenEvents();
        day->OpenWaveforms();
    }
    for (list<gPointData *>::iterator g=CPAPData.begin();g!=CPAPData.end();g++) {
        (*g)->Update(day);
    }
};

void Daily::UpdateOXIGraphs(Day *day)
{
    //if (!day) return;
    if (day) {
        day->OpenEvents();
        day->OpenWaveforms();
    }
    for (list<gPointData *>::iterator g=OXIData.begin();g!=OXIData.end();g++) {
        (*g)->Update(day);
    }
}

void Daily::RedrawGraphs()
{

    // could recall Min & Max stuff here to reset cache
    // instead of using the dodgy notify calls.
    for (list<gGraphWindow *>::iterator g=Graphs.begin();g!=Graphs.end();g++) {
        (*g)->updateGL();
    }
}

void Daily::on_treeWidget_itemSelectionChanged()
{
    if (ui->treeWidget->selectedItems().size()==0) return;
    QTreeWidgetItem *item=ui->treeWidget->selectedItems().at(0);
    if (!item) return;
    QDateTime d;
    if (!item->text(1).isEmpty()) {
        d=d.fromString(item->text(1),"yyyy-MM-dd HH:mm:ss");
        double st=(d.addSecs(-180)).toMSecsSinceEpoch()/86400000.0;
        double et=(d.addSecs(180)).toMSecsSinceEpoch()/86400000.0;
        FRW->SetXBounds(st,et);
        SF->SetXBounds(st,et);
        PRD->SetXBounds(st,et);
        LEAK->SetXBounds(st,et);
        SNORE->SetXBounds(st,et);
        MV->SetXBounds(st,et);
        TV->SetXBounds(st,et);
        RR->SetXBounds(st,et);
        FLG->SetXBounds(st,et);

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



AHIGraph::AHIGraph(QObject * parent)
{
}
AHIGraph::~AHIGraph()
{
}
QObject * AHIGraph::create(const QString & mimeType, const QUrl & url, const QStringList & argumentNames, const QStringList & argumentValues) const
{
    gGraphWindow * ahi;
    ahi=new gGraphWindow(NULL,"",(QGLWidget *)NULL);
    ahi->SetMargins(0,0,0,0);
    gPointData *g_ahi=new AHIData();
    //gCandleStick *l=new gCandleStick(g_ahi);
    gPieChart *l=new gPieChart(g_ahi);
    l->AddName(tr("H"));
    l->AddName(tr("OA"));
    l->AddName(tr("CA"));
    l->AddName(tr("RE"));
    l->AddName(tr("FL"));
    l->AddName(tr("CSR"));
    l->color.clear();
    l->color.push_back(QColor("blue"));
    l->color.push_back(QColor(0x40,0xaf,0xbf,0xff)); //#40afbf
    l->color.push_back(QColor(0xb2,0x54,0xcd,0xff)); //b254cd; //wxPURPLE);
    l->color.push_back(QColor("yellow"));
    l->color.push_back(QColor(0x40,0x40,0x40,255));
    l->color.push_back(QColor(0x60,0xff,0x60,0xff)); //80ff80

    return ahi;
}
QList<QWebPluginFactory::Plugin> AHIGraph::plugins() const
{
    QWebPluginFactory::MimeType mimeType;
    mimeType.name = "text/csv";
    mimeType.description = "Comma-separated values";
    mimeType.fileExtensions = QStringList() << "csv";

    QWebPluginFactory::Plugin plugin;
    plugin.name = "Pie Chart";
    plugin.description = "A Pie Chart Web plugin.";
    plugin.mimeTypes = QList<MimeType>() << mimeType;

    return QList<QWebPluginFactory::Plugin>() << plugin;
}
