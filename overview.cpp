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
#include "Graphs/graphdata_custom.h"
#include "Graphs/gXAxis.h"
#include "Graphs/gBarChart.h"
#include "Graphs/gLineChart.h"
#include "Graphs/gYAxis.h"
#include "Graphs/gFooBar.h"
#include "Graphs/gSessionTime.h"

Overview::Overview(QWidget *parent,QGLWidget * shared) :
    QWidget(parent),
    ui(new Ui::Overview)
{
    ui->setupUi(this);
    profile=Profiles::Get(pref["Profile"].toString());
    AddData(ahidata=new HistoryData(profile));
    AddData(pressure=new HistoryCodeData(profile,CPAP_PressureAverage));
    AddData(pressure_min=new HistoryCodeData(profile,CPAP_PressureMin));
    AddData(pressure_max=new HistoryCodeData(profile,CPAP_PressureMax));
    AddData(pressure_eap=new HistoryCodeData(profile,BIPAP_EAPAverage));
    AddData(pressure_iap=new HistoryCodeData(profile,BIPAP_IAPAverage));
    AddData(leak=new HistoryCodeData(profile,CPAP_LeakMedian));
    AddData(usage=new UsageHistoryData(profile,UHD_Hours));
    AddData(waketime=new UsageHistoryData(profile,UHD_Waketime));
    AddData(bedtime=new UsageHistoryData(profile,UHD_Bedtime));
    AddData(session_times=new SessionTimes(profile));

    // pressure->ForceMinY(3);
    // pressure->ForceMaxY(12);

    gSplitter=new QSplitter(Qt::Vertical,ui->SummaryGraphWindow);
    gSplitter->setStyleSheet("QSplitter::handle { background-color: 'dark grey'; }");
    gSplitter->setChildrenCollapsible(true);
    gSplitter->setHandleWidth(3);
    ui->graphLayout->addWidget(gSplitter);

    AddGraph(AHI=new gGraphWindow(ui->SummaryGraphWindow,tr("AHI"),shared)); //(QGLContext *)NULL));
    AHI->SetTopMargin(10);
    AHI->SetBottomMargin(AHI->GetBottomMargin()+gXAxis::Margin+25);
    //AHI->AddLayer(new gFooBar(7));
    AHI->AddLayer(new gYAxis());
    AHI->AddLayer(new gBarChart(ahidata,QColor("red")));
    AHI->setMinimumHeight(170);

    AddGraph(PRESSURE=new gGraphWindow(ui->SummaryGraphWindow,tr("Pressure"),AHI));
    //PRESSURE->SetMargins(10,15,65,80);
    PRESSURE->AddLayer(new gYAxis());
    PRESSURE->AddLayer(new gXAxis());
    //PRESSURE->AddLayer(new gFooBar(7));
    PRESSURE->AddLayer(prmax=new gLineChart(pressure_max,QColor("blue"),6192,false,true,true));
    PRESSURE->AddLayer(prmin=new gLineChart(pressure_min,QColor("red"),6192,false,true,true));
    PRESSURE->AddLayer(eap=new gLineChart(pressure_eap,QColor("blue"),6192,false,true,true));
    PRESSURE->AddLayer(iap=new gLineChart(pressure_iap,QColor("red"),6192,false,true,true));
    PRESSURE->AddLayer(pr=new gLineChart(pressure,QColor("dark green"),6192,false,true,true));
    PRESSURE->SetBottomMargin(PRESSURE->GetBottomMargin()+25);
    PRESSURE->setMinimumHeight(170);

    AddGraph(LEAK=new gGraphWindow(ui->SummaryGraphWindow,tr("Leak"),AHI));
    LEAK->AddLayer(new gXAxis());
    LEAK->AddLayer(new gYAxis());
    //LEAK->AddLayer(new gFooBar(7));
    LEAK->AddLayer(new gLineChart(leak,QColor("purple"),6192,false,false,true));
    LEAK->SetBottomMargin(LEAK->GetBottomMargin()+25);
    LEAK->setMinimumHeight(170);

    AddGraph(USAGE=new gGraphWindow(ui->SummaryGraphWindow,tr("Usage (Hours)"),AHI));
    //USAGE->AddLayer(new gFooBar(7));
    USAGE->AddLayer(new gYAxis());
    USAGE->AddLayer(new gBarChart(usage,QColor("green")));
    USAGE->SetBottomMargin(USAGE->GetBottomMargin()+gXAxis::Margin+25);
    //USAGE->AddLayer(new gXAxis());
    //USAGE->AddLayer(new gLineChart(usage,QColor("green")));
    USAGE->setMinimumHeight(170);

    AddGraph(SESSTIMES=new gGraphWindow(ui->SummaryGraphWindow,tr("Session Times"),AHI));
    //SESSTIMES->SetMargins(10,15,65,80);
    //SESSTIMES->AddLayer(new gFooBar(7));
    SESSTIMES->AddLayer(new gTimeYAxis());
    SESSTIMES->AddLayer(new gSessionTime(session_times,QColor("green")));
    SESSTIMES->SetBottomMargin(SESSTIMES->GetBottomMargin()+gXAxis::Margin+25);
    //SESSTIMES->AddLayer(new gXAxis());
    SESSTIMES->setMinimumHeight(270);

    NoData=new QLabel(tr("No data"),gSplitter);
    NoData->setAlignment(Qt::AlignCenter);
    QFont font("FreeSans",20); //NoData->font();
    //font.setBold(true);
    NoData->setFont(font);
    NoData->hide();


    gGraphWindow * graphs[]={AHI,PRESSURE,LEAK,USAGE,SESSTIMES};
    int ss=sizeof(graphs)/sizeof(gGraphWindow *);

    for (int i=0;i<ss;i++) {
        AddGraph(graphs[i]);
        graphs[i]->AddLayer(new gFooBar(7));
        for (int j=0;j<ss;j++) {
            if (graphs[i]!=graphs[j])
                graphs[i]->LinkZoom(graphs[j]);
        }
        gSplitter->addWidget(graphs[i]);
        graphs[i]->SetSplitter(gSplitter);
    }

    dummyday=new Day(NULL);

    ReloadGraphs();
}

Overview::~Overview()
{
    delete dummyday;
    delete ui;
}
void Overview::RedrawGraphs()
{
    for (list<gGraphWindow *>::iterator g=Graphs.begin();g!=Graphs.end();g++) {
        (*g)->updateGL();
    }
    //SESSTIMES->updateGL();
}
void Overview::ReloadGraphs()
{
    for (list<HistoryData *>::iterator h=Data.begin();h!=Data.end();h++) {
        if (HistoryData *hd=dynamic_cast<HistoryData *>(*h)){
            hd->SetProfile(profile);
            hd->ResetDateRange();
            hd->Reload(NULL);
        }

    }
//    session_times->SetProfile(profile);
 //   session_times->ResetDateRange();
  //  session_times->Reload(NULL);
    on_rbLastWeek_clicked();
}
void Overview::UpdateHTML()
{
    QString html="<html><body><div align=center>";
    html+="<table width='100%' cellpadding=2 cellspacing=0 border=0>";
    html+="<tr align=center><td colspan=4><b><i>Statistics</i></b></td></tr>";
    html+="<tr><td><b>Details</b></td><td><b>Min</b></td><td><b>Avg</b></td><td><b>Max</b></td></tr>";
    html+=QString("<tr><td>AHI</td><td>%1</td><td>%2</td><td>%3</td></tr>\n")
            .arg(ahidata->CalcMinY(),2,'f',2,'0')
            .arg(ahidata->CalcAverage(),2,'f',2,'0')
            .arg(ahidata->CalcMaxY(),2,'f',2,'0');
    html+=QString("<tr><td>Leak</td><td>%1</td><td>%2</td><td>%3</td></tr>")
            .arg(leak->CalcMinY(),2,'f',2,'0')
            .arg(leak->CalcAverage(),2,'f',2,'0')
            .arg(leak->CalcMaxY(),2,'f',2,'0');
    html+=QString("<tr><td>Pressure</td><td>%1</td><td>%2</td><td>%3</td></tr>")
            .arg(pressure->CalcMinY(),2,'f',2,'0')
            .arg(pressure->CalcAverage(),2,'f',2,'0')
            .arg(pressure->CalcMaxY(),2,'f',2,'0');
    html+=QString("<tr><td>Usage</td><td>%1</td><td>%2</td><td>%3</td></tr>")
            .arg(usage->CalcMinY(),2,'f',2,'0')
            .arg(usage->CalcAverage(),2,'f',2,'0')
            .arg(usage->CalcMaxY(),2,'f',2,'0');

    html+="</table>"
    "</div></body></html>";
    ui->webView->setHtml(html);
}
void Overview::UpdateGraphs()
{
    QDate first=ui->drStart->date();
    QDate last=ui->drEnd->date();
    for (list<HistoryData *>::iterator h=Data.begin();h!=Data.end();h++) {
          //(*h)->Update(dummyday);
          (*h)->SetDateRange(first,last);
    }
    session_times->SetDateRange(first,last);
    RedrawGraphs();
    UpdateHTML();
}


void UpdateCal(QCalendarWidget *cal)
{
    QDate d1=cal->minimumDate();
    d1.setYMD(d1.year(),d1.month(),1);
    QTextCharFormat fmt=cal->weekdayTextFormat(Qt::Monday);
    fmt.setForeground(QBrush(Qt::gray));
    for (QDate d=d1;d < cal->minimumDate();d=d.addDays(1)) {
        cal->setDateTextFormat(d,fmt);
    }
    d1=cal->maximumDate();
    d1.setYMD(d1.year(),d1.month(),d1.daysInMonth());
    for (QDate d=cal->maximumDate().addDays(1);d <= d1;d=d.addDays(1)) {
        cal->setDateTextFormat(d,fmt);
    }

}
void Overview::on_drStart_dateChanged(const QDate &date)
{
    ui->drEnd->setMinimumDate(date);
    QCalendarWidget *cal=ui->drEnd->calendarWidget();
    cal->setDateRange(date,profile->LastDay());
    UpdateCal(cal);
    UpdateGraphs();
}

void Overview::on_drEnd_dateChanged(const QDate &date)
{
    ui->drStart->setMaximumDate(date);
    QCalendarWidget *cal=ui->drStart->calendarWidget();
    cal->setDateRange(profile->FirstDay(),date);
    UpdateCal(cal);
    UpdateGraphs();
}

void Overview::on_rbDateRange_toggled(bool checked)
{
    ui->drStart->setEnabled(checked);
    ui->drEnd->setEnabled(checked);
    ui->drStartLabel->setEnabled(checked);
    ui->drEndLabel->setEnabled(checked);
}

void Overview::on_rbLastWeek_clicked()
{
    ui->drStart->setDateRange(profile->FirstDay(),profile->LastDay());
    ui->drEnd->setDateRange(profile->FirstDay(),profile->LastDay());

    QDate d=profile->LastDay();
    ui->drEnd->setDate(d);
    d=d.addDays(-7);
    if (d<profile->FirstDay()) d=profile->FirstDay();
    ui->drStart->setDate(d);
    UpdateGraphs();
}

void Overview::on_rbLastMonth_clicked()
{
    ui->drStart->setDateRange(profile->FirstDay(),profile->LastDay());
    ui->drEnd->setDateRange(profile->FirstDay(),profile->LastDay());

    QDate d=profile->LastDay();
    ui->drEnd->setDate(d);
    d=d.addDays(-30);
    if (d<profile->FirstDay()) d=profile->FirstDay();
    ui->drStart->setDate(d);
    UpdateGraphs();
}

void Overview::on_rbEverything_clicked()
{
    ui->drStart->setDateRange(profile->FirstDay(),profile->LastDay());
    ui->drEnd->setDateRange(profile->FirstDay(),profile->LastDay());

    ui->drEnd->setDate(profile->LastDay());
    ui->drStart->setDate(profile->FirstDay());
    UpdateGraphs();
}

void Overview::on_rbDateRange_clicked()
{
    UpdateGraphs();
}
