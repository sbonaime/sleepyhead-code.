/*
 Summary Module
 Copyright (c)2013 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL3
*/

#include <QApplication>
#include <cmath>

#include "mainwindow.h"
#include "summary.h"
#include "SleepLib/schema.h"


extern MainWindow *mainwin;


Summary::Summary(QObject *parent) :
    QObject(parent)
{
}


QString htmlHeader()
{

    //   "a:link,a:visited { color: '#000020'; text-decoration: none; font-weight: bold;}"
//   "a:hover { background-color: inherit; color: red; text-decoration:none; font-weight: bold; }"
    return QString("<html><head>"
"</head>"
"<style type='text/css'>"
"<!--h1,p,a,td,body { font-family: 'FreeSans', 'Sans Serif' } --/>"
"p,a,td,body { font-size: 14px }"
"</style>"
"<link rel='stylesheet' type='text/css' href='qrc:/docs/tooltips.css' />"
"<script type='text/javascript'>"
"function ChangeColor(tableRow, highLight)"
"{ tableRow.style.backgroundColor = highLight; }"
"function Go(url) { throw(url); }"
"</script>"
"</head>"
"<body leftmargin=0 topmargin=0 rightmargin=0>"
"<div align=center><table cellpadding=3 cellspacing=0 border=0>"
"<tr><td><img src='qrc:/icons/bob-v3.0.png' width=140px height=140px><td valign=center align=center><h1>"+
QObject::tr("SleepyHead")+" v"+VersionString+"</h1><i>"+
QObject::tr("This is a beta software and some functionality may not work as intended yet.")+"<br/>"+
QObject::tr("Please report any bugs you find to SleepyHead's SourceForge page.")+
"</i></td></tr></table>"
"</div>"
"<hr/>");
}
QString htmlFooter()
{
    return QString("</body></html>");
}

QString formatTime(float time)
{
    int hours=time;
    int seconds=time*3600.0;
    int minutes=(seconds / 60) % 60;
    seconds %= 60;
    return QString().sprintf("%02i:%02i",hours,minutes); //,seconds);
}


EventDataType calcAHI(QDate start, QDate end)
{
    EventDataType val=(p_profile->calcCount(CPAP_Obstructive,MT_CPAP,start,end)
              +p_profile->calcCount(CPAP_Hypopnea,MT_CPAP,start,end)
              +p_profile->calcCount(CPAP_ClearAirway,MT_CPAP,start,end)
              +p_profile->calcCount(CPAP_Apnea,MT_CPAP,start,end));
    if (PROFILE.general->calculateRDI())
        val+=p_profile->calcCount(CPAP_RERA,MT_CPAP,start,end);
    EventDataType hours=p_profile->calcHours(MT_CPAP,start,end);

    if (hours>0)
        val/=hours;
    else
        val=0;

    return val;
}

EventDataType calcFL(QDate start, QDate end)
{
    EventDataType val=(p_profile->calcCount(CPAP_FlowLimit,MT_CPAP,start,end));
    EventDataType hours=p_profile->calcHours(MT_CPAP,start,end);

    if (hours>0)
        val/=hours;
    else
        val=0;

    return val;
}


struct RXChange
{
    RXChange() { highlight=0; machine=NULL; }
    RXChange(const RXChange & copy) {
        first=copy.first;
        last=copy.last;
        days=copy.days;
        ahi=copy.ahi;
    fl=copy.fl;
        mode=copy.mode;
        min=copy.min;
        max=copy.max;
        maxhi=copy.maxhi;
        machine=copy.machine;
        per1=copy.per1;
        per2=copy.per2;
        highlight=copy.highlight;
        weighted=copy.weighted;
        prelief=copy.prelief;
        prelset=copy.prelset;
    }
    QDate first;
    QDate last;
    int days;
    EventDataType ahi;
    EventDataType fl;
    CPAPMode mode;
    EventDataType min;
    EventDataType max;
    EventDataType maxhi;
    EventDataType per1;
    EventDataType per2;
    EventDataType weighted;
    PRTypes prelief;
    Machine * machine;
    short prelset;
    short highlight;
};

enum RXSortMode { RX_first, RX_last, RX_days, RX_ahi, RX_mode, RX_min, RX_max, RX_maxhi, RX_per1, RX_per2, RX_weighted };
RXSortMode RXsort=RX_first;
bool RXorder=false;

bool operator<(const RXChange & c1, const RXChange & c2) {
    const RXChange * comp1=&c1;
    const RXChange * comp2=&c2;
    if (RXorder) {
        switch (RXsort) {
        case RX_ahi: return comp1->ahi < comp2->ahi;
        case RX_days: return comp1->days < comp2->days;
        case RX_first: return comp1->first < comp2->first;
        case RX_last: return comp1->last < comp2->last;
        case RX_mode: return comp1->mode < comp2->mode;
        case RX_min:  return comp1->min < comp2->min;
        case RX_max:  return comp1->max < comp2->max;
        case RX_maxhi: return comp1->maxhi < comp2->maxhi;
        case RX_per1:  return comp1->per1 < comp2->per1;
        case RX_per2:  return comp1->per2 < comp2->per2;
        case RX_weighted:  return comp1->weighted < comp2->weighted;
        };
    } else {
        switch (RXsort) {
        case RX_ahi: return comp1->ahi > comp2->ahi;
        case RX_days: return comp1->days > comp2->days;
        case RX_first: return comp1->first > comp2->first;
        case RX_last: return comp1->last > comp2->last;
        case RX_mode: return comp1->mode > comp2->mode;
        case RX_min:  return comp1->min > comp2->min;
        case RX_max:  return comp1->max > comp2->max;
        case RX_maxhi: return comp1->maxhi > comp2->maxhi;
        case RX_per1:  return comp1->per1 > comp2->per1;
        case RX_per2:  return comp1->per2 > comp2->per2;
        case RX_weighted:  return comp1->weighted > comp2->weighted;
        };
    }
    return true;
}

bool RXSort(const RXChange * comp1, const RXChange * comp2) {
    if (RXorder) {
        switch (RXsort) {
        case RX_ahi: return comp1->ahi < comp2->ahi;
        case RX_days: return comp1->days < comp2->days;
        case RX_first: return comp1->first < comp2->first;
        case RX_last: return comp1->last < comp2->last;
        case RX_mode: return comp1->mode < comp2->mode;
        case RX_min:  return comp1->min < comp2->min;
        case RX_max:  return comp1->max < comp2->max;
        case RX_maxhi: return comp1->maxhi < comp2->maxhi;
        case RX_per1:  return comp1->per1 < comp2->per1;
        case RX_per2:  return comp1->per2 < comp2->per2;
        case RX_weighted:  return comp1->weighted < comp2->weighted;
        };
    } else {
        switch (RXsort) {
        case RX_ahi: return comp1->ahi > comp2->ahi;
        case RX_days: return comp1->days > comp2->days;
        case RX_first: return comp1->first > comp2->first;
        case RX_last: return comp1->last > comp2->last;
        case RX_mode: return comp1->mode > comp2->mode;
        case RX_min:  return comp1->min > comp2->min;
        case RX_max:  return comp1->max > comp2->max;
        case RX_maxhi: return comp1->maxhi > comp2->maxhi;
        case RX_per1:  return comp1->per1 > comp2->per1;
        case RX_per2:  return comp1->per2 > comp2->per2;
        case RX_weighted:  return comp1->weighted > comp2->weighted;
        };
    }
    return true;
}
struct UsageData {
    UsageData() { ahi=0; hours=0; }
    UsageData(QDate d, EventDataType v, EventDataType h) { date=d; ahi=v; hours=h; }
    UsageData(const UsageData & copy) { date=copy.date; ahi=copy.ahi; hours=copy.hours; }
    QDate date;
    EventDataType ahi;
    EventDataType hours;
};
bool operator <(const UsageData & c1, const UsageData & c2)
{
    if (c1.ahi < c2.ahi)
        return true;
    if ((c1.ahi == c2.ahi) && (c1.date > c2.date)) return true;
    return false;
    //return c1.value < c2.value;
}

QString Summary::GenerateHTML()
{
    QString html=htmlHeader();

    QDate lastcpap=p_profile->LastGoodDay(MT_CPAP);
    QDate firstcpap=p_profile->FirstGoodDay(MT_CPAP);
    QDate cpapweek=lastcpap.addDays(-7);
    QDate cpapmonth=lastcpap.addDays(-30);
    QDate cpap6month=lastcpap.addMonths(-6);
    QDate cpapyear=lastcpap.addYears(-12);
    if (cpapweek<firstcpap) cpapweek=firstcpap;
    if (cpapmonth<firstcpap) cpapmonth=firstcpap;
    if (cpap6month<firstcpap) cpap6month=firstcpap;
    if (cpapyear<firstcpap) cpapyear=firstcpap;


    QList<Machine *> cpap_machines=PROFILE.GetMachines(MT_CPAP);
    QList<Machine *> oximeters=PROFILE.GetMachines(MT_OXIMETER);
    QList<Machine *> mach;
    mach.append(cpap_machines);
    mach.append(oximeters);


    if (mach.size()==0) {
        html+="<table cellpadding=2 cellspacing=0 border=0 width=100% height=60%>";
        QString datacard;
        html+="<tr><td align=center><h1>"+tr("Please Import Some Data")+"</h1><i>"+tr("SleepyHead is pretty much useless without it.")+"</i><br/><p>"+tr("It might be a good idea to check preferences first,</br>as there are some options that affect import.")+"</p><p>"+tr("First import can take a few minutes.")+"</p></td></tr></table>";
        html+=htmlFooter();
        return html;
    }
    int cpapdays=PROFILE.countDays(MT_CPAP,firstcpap,lastcpap);
    int cpapweekdays=PROFILE.countDays(MT_CPAP,cpapweek,lastcpap);
    int cpapmonthdays=PROFILE.countDays(MT_CPAP,cpapmonth,lastcpap);
    int cpapyeardays=PROFILE.countDays(MT_CPAP,cpapyear,lastcpap);
    int cpap6monthdays=PROFILE.countDays(MT_CPAP,cpap6month,lastcpap);

    CPAPMode cpapmode=(CPAPMode)(int)p_profile->calcSettingsMax(CPAP_Mode,MT_CPAP,firstcpap,lastcpap);

    float percentile=PROFILE.general->prefCalcPercentile()/100.0;

//    int mididx=PROFILE.general->prefCalcMiddle();
//    SummaryType ST_mid;
//    if (mididx==0) ST_mid=ST_PERC;
//    if (mididx==1) ST_mid=ST_WAVG;
//    if (mididx==2) ST_mid=ST_AVG;

    QString ahitxt;
    if (PROFILE.general->calculateRDI()) {
        ahitxt=STR_TR_RDI;
    } else {
        ahitxt=STR_TR_AHI;
    }

    int decimals=2;
    html+="<div align=center>";
    html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");
    if (cpapdays==0)  {
        html+="<tr><td colspan=6 align=center>"+tr("No CPAP Machine Data Imported")+"</td></tr>";
    } else {
        html+=QString("<tr><td colspan=6 align=center><b>")+tr("CPAP Statistics as of")+QString(" %1</b></td></tr>").arg(lastcpap.toString(Qt::SystemLocaleLongDate));

        if (cpap_machines.size()>0) {
           // html+=QString("<tr><td colspan=6 align=center><b>%1</b></td></tr>").arg(tr("CPAP Summary"));

            if (!cpapdays) {
                html+=QString("<tr><td colspan=6 align=center><b>%1</b></td></tr>").arg(tr("No CPAP data available."));
            } else if (cpapdays==1) {
                html+=QString("<tr><td colspan=6 align=center>%1</td></tr>").arg(QString(tr("%1 day of CPAP Data, on %2.")).arg(cpapdays).arg(firstcpap.toString(Qt::SystemLocaleShortDate)));
            } else {
                html+=QString("<tr><td colspan=6 align=center>%1</td></tr>").arg(QString(tr("%1 days of CPAP Data, between %2 and %3")).arg(cpapdays).arg(firstcpap.toString(Qt::SystemLocaleShortDate)).arg(lastcpap.toString(Qt::SystemLocaleShortDate)));
            }

            html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</td></tr>")
                .arg(tr("Details")).arg(tr("Most Recent")).arg(tr("Last 7 Days")).arg(tr("Last 30 Days")).arg(tr("Last 6 months")).arg(tr("Last Year"));
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(ahitxt)
            .arg(calcAHI(lastcpap,lastcpap),0,'f',decimals)
            .arg(calcAHI(cpapweek,lastcpap),0,'f',decimals)
            .arg(calcAHI(cpapmonth,lastcpap),0,'f',decimals)
            .arg(calcAHI(cpap6month,lastcpap),0,'f',decimals)
            .arg(calcAHI(cpapyear,lastcpap),0,'f',decimals);

            if (PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpapyear,lastcpap)) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("RERA Index"))
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,lastcpap,lastcpap)/PROFILE.calcHours(MT_CPAP,lastcpap,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpapweek,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpapmonth,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpap6month,lastcpap)/PROFILE.calcHours(MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_RERA,MT_CPAP,cpapyear,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            }

            if (PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpapyear,lastcpap)) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Flow Limit Index"))
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,lastcpap,lastcpap)/PROFILE.calcHours(MT_CPAP,lastcpap,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpapweek,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpapmonth,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpap6month,lastcpap)/PROFILE.calcHours(MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(PROFILE.calcCount(CPAP_FlowLimit,MT_CPAP,cpapyear,lastcpap)/PROFILE.calcHours(MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            }



            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(tr("Hours per Night"))
            .arg(formatTime(p_profile->calcHours(MT_CPAP)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpapweek,lastcpap)/float(cpapweekdays)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpapmonth,lastcpap)/float(cpapmonthdays)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpap6month,lastcpap)/float(cpap6monthdays)))
            .arg(formatTime(p_profile->calcHours(MT_CPAP,cpapyear,lastcpap)/float(cpapyeardays)));

            if (cpapmode>=MODE_BIPAP) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Min EPAP"))
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMin(CPAP_EPAP,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(QString("%1% "+STR_TR_EPAP).arg(percentile*100.0,0,'f',0))
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Max IPAP"))
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcMax(CPAP_IPAP,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(QString("%1% "+STR_TR_IPAP).arg(percentile*100.0,0,'f',0))
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            } else if (cpapmode>=MODE_APAP) {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Average Pressure"))
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcWavg(CPAP_Pressure,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("%1% Pressure").arg(percentile*100.0,0,'f',0))
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            } else {
                html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Pressure"))
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP),0,'f',decimals)
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
                .arg(p_profile->calcSettingsMin(CPAP_Pressure,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            }
            //html+="<tr><td colspan=6>TODO: 90% pressure.. Any point showing if this is all CPAP?</td></tr>";


            ChannelID leak;
            if (p_profile->calcCount(CPAP_LeakTotal,MT_CPAP,cpapyear,lastcpap)>0) {
                leak=CPAP_LeakTotal;
            } else leak=CPAP_Leak;

            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(tr("Average %1").arg(schema::channel[leak].label()))
            .arg(p_profile->calcWavg(leak,MT_CPAP),0,'f',decimals)
            .arg(p_profile->calcWavg(leak,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
            .arg(p_profile->calcWavg(leak,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
            .arg(p_profile->calcWavg(leak,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
            .arg(p_profile->calcWavg(leak,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
            .arg(tr("%1% %2").arg(percentile*100.0f,0,'f',0).arg(schema::channel[leak].label()))
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP),0,'f',decimals)
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP,cpapweek,lastcpap),0,'f',decimals)
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP,cpapmonth,lastcpap),0,'f',decimals)
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP,cpap6month,lastcpap),0,'f',decimals)
            .arg(p_profile->calcPercentile(leak,percentile,MT_CPAP,cpapyear,lastcpap),0,'f',decimals);
        }
    }
    int oxisize=oximeters.size();
    if (oxisize>0) {
        QDate lastoxi=p_profile->LastGoodDay(MT_OXIMETER);
        QDate firstoxi=p_profile->FirstGoodDay(MT_OXIMETER);
        int days=PROFILE.countDays(MT_OXIMETER,firstoxi,lastoxi);
        if (days>0) {
            html+=QString("<tr><td colspan=6 align=center><b>%1</b></td></tr>").arg(tr("Oximetry Summary"));
            if (days==1) {
                html+=QString("<tr><td colspan=6 align=center>%1</td></tr>").arg(QString(tr("%1 day of Oximetry Data, on %2.")).arg(days).arg(firstoxi.toString(Qt::SystemLocaleShortDate)));
            } else {
                html+=QString("<tr><td colspan=6 align=center>%1</td></tr>").arg(QString(tr("%1 days of Oximetry Data, between %2 and %3")).arg(days).arg(firstoxi.toString(Qt::SystemLocaleShortDate)).arg(lastoxi.toString(Qt::SystemLocaleShortDate)));
            }

            html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</td></tr>")
                    .arg(tr("Details")).arg(tr("Most Recent")).arg(tr("Last 7 Days")).arg(tr("Last 30 Days")).arg(tr("Last 6 months")).arg(tr("Last Year"));
            QDate oxiweek=lastoxi.addDays(-7);
            QDate oximonth=lastoxi.addDays(-30);
            QDate oxi6month=lastoxi.addMonths(-6);
            QDate oxiyear=lastoxi.addYears(-12);
            if (oxiweek<firstoxi) oxiweek=firstoxi;
            if (oximonth<firstoxi) oximonth=firstoxi;
            if (oxi6month<firstoxi) oxi6month=firstoxi;
            if (oxiyear<firstoxi) oxiyear=firstoxi;
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Average SpO2"))
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_SPO2,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Minimum SpO2"))
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_SPO2,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("SpO2 Events / Hour"))
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER)/p_profile->calcHours(MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oxiweek,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oximonth,lastoxi)/p_profile->calcHours(MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oxi6month,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_SPO2Drop,MT_OXIMETER,oxiyear,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2\%</td><td>%3\%</td><td>%4\%</td><td>%5\%</td><td>%6\%</td></tr>")
                .arg(tr("% of time in SpO2 Events"))
                .arg(100.0/p_profile->calcHours(MT_OXIMETER)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER)/3600.0,0,'f',decimals)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oxiweek,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oxiweek,lastoxi)/3600.0,0,'f',decimals)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oximonth,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oximonth,lastoxi)/3600.0,0,'f',decimals)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oxi6month,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oxi6month,lastoxi)/3600.0,0,'f',decimals)
                .arg(100.0/p_profile->calcHours(MT_OXIMETER,oxiyear,lastoxi)*p_profile->calcSum(OXI_SPO2Drop,MT_OXIMETER,oxiyear,lastoxi)/3600.0,0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Average Pulse Rate"))
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcWavg(OXI_Pulse,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Minimum Pulse Rate"))
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMin(OXI_Pulse,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Maximum Pulse Rate"))
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcMax(OXI_Pulse,MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>")
                .arg(tr("Pulse Change Events / Hour"))
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER)/p_profile->calcHours(MT_OXIMETER),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER,oxiweek,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiweek,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER,oximonth,lastoxi)/p_profile->calcHours(MT_OXIMETER,oximonth,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER,oxi6month,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxi6month,lastoxi),0,'f',decimals)
                .arg(p_profile->calcCount(OXI_PulseChange,MT_OXIMETER,oxiyear,lastoxi)/p_profile->calcHours(MT_OXIMETER,oxiyear,lastoxi),0,'f',decimals);
        }
    }

    html+="</table>";
    html+="</div>";

    QList<UsageData> AHI;

    //QDate bestAHIdate, worstAHIdate;
    //EventDataType bestAHI=999.0, worstAHI=0;
    if (cpapdays>0) {
        QDate first,last=lastcpap;
        CPAPMode mode=MODE_UNKNOWN,cmode=MODE_UNKNOWN;
        EventDataType cmin=0,cmax=0,cmaxhi=0, min=0,max=0,maxhi=0;
        Machine *mach=NULL,*lastmach=NULL;
        PRTypes lastpr=PR_UNKNOWN, prelief=PR_UNKNOWN;
        short prelset=0, lastprelset=-1;
        QDate date=lastcpap;
        Day * day;
        bool lastchanged=false;
        QVector<RXChange> rxchange;
        EventDataType hours;

        int compliant=0;
        do {
            day=PROFILE.GetGoodDay(date,MT_CPAP);

            if (day) {
                lastchanged=false;

                hours=day->hours();

                if (hours > PROFILE.cpap->complianceHours())
                    compliant++;

                EventDataType ahi=day->count(CPAP_Obstructive)+day->count(CPAP_Hypopnea)+day->count(CPAP_Apnea)+day->count(CPAP_ClearAirway);
                if (PROFILE.general->calculateRDI()) ahi+=day->count(CPAP_RERA);
                ahi/=hours;
                AHI.push_back(UsageData(date,ahi,hours));

                prelief=(PRTypes)(int)round(day->settings_wavg(CPAP_PresReliefType));
                prelset=round(day->settings_wavg(CPAP_PresReliefSet));
                mode=(CPAPMode)(int)round(day->settings_wavg(CPAP_Mode));
                mach=day->machine;
                if (mode>=MODE_ASV) {
                    min=day->settings_min(CPAP_EPAP);
                    max=day->settings_max(CPAP_IPAPLo);
                    maxhi=day->settings_max(CPAP_IPAPHi);
                } else if (mode>=MODE_BIPAP) {
                    min=day->settings_min(CPAP_EPAP);
                    max=day->settings_max(CPAP_IPAP);
                } else if (mode>=MODE_APAP) {
                    min=day->settings_min(CPAP_PressureMin);
                    max=day->settings_max(CPAP_PressureMax);
                } else {
                    min=day->settings_min(CPAP_Pressure);
                }
                if ((mode!=cmode) || (min!=cmin) || (max!=cmax) || (mach!=lastmach) || (prelset!=lastprelset))  {
                    if ((cmode!=MODE_UNKNOWN) && (lastmach!=NULL)) {
                        first=date.addDays(1);
                        int days=PROFILE.countDays(MT_CPAP,first,last);
                        RXChange rx;
                        rx.first=first;
                        rx.last=last;
                        rx.days=days;
                        rx.ahi=calcAHI(first,last);
                        rx.fl=calcFL(first,last);
                        rx.mode=cmode;
                        rx.min=cmin;
                        rx.max=cmax;
                        rx.maxhi=cmaxhi;
                        rx.prelief=lastpr;
                        rx.prelset=lastprelset;
                        rx.machine=lastmach;

                        if (mode<MODE_BIPAP) {
                            rx.per1=p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,first,last);
                            rx.per2=0;
                        } else if (mode<MODE_ASV) {
                            rx.per1=p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,first,last);
                            rx.per2=p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,first,last);
                        } else {
                            rx.per1=p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,first,last);
                            rx.per2=p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,first,last);
                        }
                        rx.weighted=float(rx.days)/float(cpapdays)*rx.ahi;
                        rxchange.push_back(rx);
                    }
                    cmode=mode;
                    cmin=min;
                    cmax=max;
                    cmaxhi=maxhi;
                    lastpr=prelief;
                    lastprelset=prelset;
                    last=date;
                    lastmach=mach;
                    lastchanged=true;
                }

            }
            date=date.addDays(-1);
        } while (date>=firstcpap);

        // Sort list by AHI
        qSort(AHI);

        lastchanged=false;
        if (!lastchanged && (mach!=NULL)) {
           // last=date.addDays(1);
            first=firstcpap;
            int days=PROFILE.countDays(MT_CPAP,first,last);
            RXChange rx;
            rx.first=first;
            rx.last=last;
            rx.days=days;
            rx.ahi=calcAHI(first,last);
            rx.fl=calcFL(first,last);
            rx.mode=mode;
            rx.min=min;
            rx.max=max;
            rx.maxhi=maxhi;
            rx.prelief=prelief;
            rx.prelset=prelset;
            rx.machine=mach;
            if (mode<MODE_BIPAP) {
                rx.per1=p_profile->calcPercentile(CPAP_Pressure,percentile,MT_CPAP,first,last);
                rx.per2=0;
            } else if (mode<MODE_ASV) {
                rx.per1=p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,first,last);
                rx.per2=p_profile->calcPercentile(CPAP_IPAP,percentile,MT_CPAP,first,last);
            } else {
                rx.per1=p_profile->calcPercentile(CPAP_EPAP,percentile,MT_CPAP,first,last);
                rx.per2=p_profile->calcPercentile(CPAP_IPAPHi,percentile,MT_CPAP,first,last);
            }
            rx.weighted=float(rx.days)/float(cpapdays);
            //rx.weighted=float(days)*rx.ahi;
            rxchange.push_back(rx);
        }

        int rxthresh=5;
        QVector<RXChange *> tmpRX;
        for (int i=0;i<rxchange.size();i++) {
            RXChange & rx=rxchange[i];
            if (rx.days>rxthresh)
                tmpRX.push_back(&rx);
        }

        QString recbox="<html><head><style type='text/css'>"
            "p,a,td,body { font-family: '"+QApplication::font().family()+"'; }"
            "p,a,td,body { font-size: "+QString::number(QApplication::font().pointSize() + 2)+"px; }"
            "a:link,a:visited { color: inherit; text-decoration: none; }" //font-weight: normal;
            "a:hover { background-color: inherit; color: white; text-decoration:none; font-weight: bold; }"
            "</style></head><body>";
        recbox+="<table width=100% cellpadding=1 cellspacing=0>";
        int numdays=AHI.size();
        if (numdays>1) {
            int z=numdays/2;
            if (z>4) z=4;

            recbox+=QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(tr("Usage Information"));
            recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Total Days")).arg(numdays);
            if (PROFILE.cpap->showComplianceInfo()) {
                recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Compliant Days")).arg(compliant);
            }
            int highahi=0;
            for (int i=0;i<numdays;i++) {
                if (AHI.at(i).ahi > 5.0)
                    highahi++;
            }
            recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Days AHI &gt;5.0")).arg(highahi);


            recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");
            recbox+=QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(tr("Best&nbsp;%1").arg(ahitxt));
            for (int i=0;i<z;i++) {
                const UsageData & a=AHI.at(i);
                recbox+=QString("<tr><td><a href='daily=%1'>%2</a></td><td  align=right>%3</td></tr>")
                    .arg(a.date.toString(Qt::ISODate))
                    .arg(a.date.toString(Qt::SystemLocaleShortDate))
                    .arg(a.ahi,0,'f',decimals);
            }
            recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");
            recbox+=QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(tr("Worst&nbsp;%1").arg(ahitxt));
            for (int i=0;i<z;i++) {
                const UsageData & a=AHI.at((numdays-1)-i);
                recbox+=QString("<tr><td><a href='daily=%1'>%2</a></td><td align=right>%3</td></tr>")
                    .arg(a.date.toString(Qt::ISODate))
                    .arg(a.date.toString(Qt::SystemLocaleShortDate))
                    .arg(a.ahi,0,'f',decimals);
            }
            recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");
        }


        if (tmpRX.size()>0) {
            RXsort=RX_ahi;
            QString minstr,maxstr,modestr,maxhistr;
            qSort(tmpRX.begin(),tmpRX.end(),RXSort);
            tmpRX[0]->highlight=4; // worst
            int ls=tmpRX.size()-1;
            tmpRX[ls]->highlight=1; //best
            CPAPMode mode=(CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode,MT_CPAP,tmpRX[ls]->first,tmpRX[ls]->last);

            if (mode<MODE_APAP) { // is CPAP?
                minstr=STR_TR_Pressure;
                maxstr="";
                modestr=STR_TR_CPAP;
            } else if (mode<MODE_BIPAP) { // is AUTO?
                minstr=STR_TR_Min;
                maxstr=STR_TR_Max;
                modestr=STR_TR_APAP;
            } else if (mode<MODE_ASV) { // BIPAP
                minstr=STR_TR_EPAP;
                maxstr=STR_TR_IPAP;
                modestr=STR_TR_BiLevel;
            } else {
                minstr=STR_TR_EPAP;
                maxstr=STR_TR_IPAPLo;
                maxhistr=STR_TR_IPAPHi;
                modestr=STR_TR_STASV;

            }

            recbox+=QString("<tr><td colspan=2><table width=100% border=0 cellpadding=1 cellspacing=0><tr><td colspan=2 align=center><b>%3</b></td></tr>")
                    .arg(tr("Best RX Setting"));
            recbox+=QString("<tr><td valign=top>")+STR_TR_Start+"<br/>"+STR_TR_End+QString("</td><td align=right><a href='overview=%1,%2'>%3<br/>%4</a></td></tr>")
                    .arg(tmpRX[ls]->first.toString(Qt::ISODate))
                    .arg(tmpRX[ls]->last.toString(Qt::ISODate))
                    .arg(tmpRX[ls]->first.toString(Qt::SystemLocaleShortDate))
                    .arg(tmpRX[ls]->last.toString(Qt::SystemLocaleShortDate));
            recbox+=QString("<tr><td><b>%1</b></td><td align=right><b>%2</b></td></tr>").arg(ahitxt).arg(tmpRX[ls]->ahi,0,'f',decimals);
            recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(STR_TR_Mode).arg(modestr);
            recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(minstr).arg(tmpRX[ls]->min,0,'f',1).arg(STR_UNIT_CMH2O);
            if (!maxstr.isEmpty()) recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxstr).arg(tmpRX[ls]->max,0,'f',1).arg(STR_UNIT_CMH2O);
            if (!maxhistr.isEmpty()) recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxhistr).arg(tmpRX[ls]->maxhi,0,'f',1).arg(STR_UNIT_CMH2O);
            recbox+="</table></td></tr>";

            recbox+=QString("<tr><td colspan=2>&nbsp;</td></tr>");

            mode=(CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode,MT_CPAP,tmpRX[0]->first,tmpRX[0]->last);
            if (mode<MODE_APAP) { // is CPAP?
                minstr=STR_TR_Pressure;
                maxstr="";
                modestr=STR_TR_CPAP;
            } else if (mode<MODE_BIPAP) { // is AUTO?
                minstr=STR_TR_Min;
                maxstr=STR_TR_Max;
                modestr=STR_TR_APAP;
            } else if (mode<MODE_ASV) { // BIPAP or greater
                minstr=STR_TR_EPAP;
                maxstr=STR_TR_IPAP;
                modestr=STR_TR_BiLevel;
            } else {
                minstr=STR_TR_EPAP;
                maxstr=STR_TR_IPAPLo;
                maxhistr=STR_TR_IPAPHi;
                modestr=STR_TR_STASV;
            }

            recbox+=QString("<tr><td colspan=2><table width=100% border=0 cellpadding=1 cellspacing=0><tr><td colspan=2 align=center><b>%3</b></td></tr>")
                    .arg(tr("Worst RX Setting"));
            recbox+=QString("<tr><td valign=top>")+STR_TR_Start+"<br/>"+STR_TR_End+QString("</td><td align=right><a href='overview=%1,%2'>%3<br/>%4</a></td></tr>")
                    .arg(tmpRX[0]->first.toString(Qt::ISODate))
                    .arg(tmpRX[0]->last.toString(Qt::ISODate))
                    .arg(tmpRX[0]->first.toString(Qt::SystemLocaleShortDate))
                    .arg(tmpRX[0]->last.toString(Qt::SystemLocaleShortDate));
            recbox+=QString("<tr><td><b>%1</b></td><td align=right><b>%2</b></td></tr>").arg(ahitxt).arg(tmpRX[0]->ahi,0,'f',decimals);
            recbox+=QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(STR_TR_Mode).arg(modestr);
            recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(minstr).arg(tmpRX[0]->min,0,'f',1).arg(STR_UNIT_CMH2O);
            if (!maxstr.isEmpty()) recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxstr).arg(tmpRX[0]->max,0,'f',1).arg(STR_UNIT_CMH2O);
            if (!maxhistr.isEmpty()) recbox+=QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxhistr).arg(tmpRX[0]->maxhi,0,'f',1).arg(STR_UNIT_CMH2O);
            recbox+="</table></td></tr>";

        }
        recbox+="</table>";
        recbox+="</body></html>";
        mainwin->setRecBoxHTML(recbox);

        /*RXsort=RX_min;
        RXorder=true;
        qSort(rxchange.begin(),rxchange.end());*/
        html+="<div align=center>";
        html+=QString("<br/><b>")+tr("Changes to Prescription Settings")+"</b>";
        html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");
        QString extratxt;

        if (cpapmode>=MODE_ASV) {
            extratxt=QString("<td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td>")
                .arg(STR_TR_EPAP).arg(STR_TR_IPAPLo).arg(STR_TR_IPAPHi).arg(tr("PS Min")).arg(tr("PS Max"));
        } else if (cpapmode>=MODE_BIPAP) {
            extratxt=QString("<td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td>")
                .arg(STR_TR_EPAP).arg(STR_TR_IPAP).arg(STR_TR_PS);
        } else if (cpapmode>MODE_CPAP) {
            extratxt=QString("<td><b>%1</b></td><td><b>%2</b></td>")
                .arg(tr("Min Pres.")).arg(tr("Max Pres."));
        } else {
            extratxt=QString("<td><b>%1</b></td>")
                .arg(STR_TR_Pressure);
        }
        QString tooltip;
        html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</b></td><td><b>%7</td><td><b>%8</td>%9</tr>")
                  .arg(STR_TR_First)
                  .arg(STR_TR_Last)
                  .arg(tr("Days"))
                  .arg(ahitxt)
          .arg(tr("FL"))
                  .arg(STR_TR_Machine)
                  .arg(STR_TR_Mode)
                  .arg(tr("Pr. Rel."))
                  .arg(extratxt);

        for (int i=0;i<rxchange.size();i++) {
            RXChange rx=rxchange.at(i);
            QString color;
            if (rx.highlight==1) {
                color="#c0ffc0";
            } else if (rx.highlight==2) {
                color="#e0ffe0";
            } else if (rx.highlight==3) {
                color="#ffe0e0";
            } else if (rx.highlight==4) {
                color="#ffc0c0";
            } else color="";
            QString machstr;
            if (rx.machine->properties.contains(STR_PROP_Brand))
                machstr+=rx.machine->properties[STR_PROP_Brand];
            if (rx.machine->properties.contains(STR_PROP_Model)) {
                 machstr+=" "+rx.machine->properties[STR_PROP_Model];
            }
            if (rx.machine->properties.contains(STR_PROP_Serial)) {
                 machstr+=" ("+rx.machine->properties[STR_PROP_Serial]+")<br/>";
            }

            mode=rx.mode;
            if(mode>=MODE_ASV) {
                extratxt=QString("<td>%1</td><td>%2</td><td>%3</td><td>%4</td>")
                        .arg(rx.max,0,'f',decimals).arg(rx.maxhi,0,'f',decimals).arg(rx.max-rx.min,0,'f',decimals).arg(rx.maxhi-rx.min,0,'f',decimals);

                tooltip=QString("%1 %2% ").arg(machstr).arg(percentile*100.0)+STR_TR_EPAP+
                        QString("=%1<br/>%2% ").arg(rx.per1,0,'f',decimals).arg(percentile*100.0)+
                        STR_TR_IPAP+QString("=%1").arg(rx.per2,0,'f',decimals);
            } else if (mode>=MODE_BIPAP) {
                extratxt=QString("<td>%1</td><td>%2</td>")
                       .arg(rx.max,0,'f',decimals).arg(rx.max-rx.min,0,'f',decimals);
                tooltip=QString("%1 %2% ").arg(machstr).arg(percentile*100.0)+
                        STR_TR_EPAP+
                        QString("=%1<br/>%2% ").arg(rx.per1,0,'f',decimals)
                        .arg(percentile*100.0)
                        +STR_TR_IPAP+QString("=%1").arg(rx.per2,0,'f',decimals);
            } else if (mode>MODE_CPAP) {
                extratxt=QString("<td>%1</td>").arg(rx.max,0,'f',decimals);
                tooltip=QString("%1 %2% ").arg(machstr).arg(percentile*100.0)+STR_TR_Pressure+
                        QString("=%2").arg(rx.per1,0,'f',decimals);
            } else {
                if (cpapmode>MODE_CPAP) {
                    extratxt="<td>&nbsp;</td>";
                    tooltip=QString("%1").arg(machstr);
                } else {
                    extratxt="";
                    tooltip="";
                }
            }
            QString presrel;
            if (rx.prelset>0) {
                presrel=schema::channel[CPAP_PresReliefType].option(int(rx.prelief));
                presrel+=QString(" x%1").arg(rx.prelset);
            } else presrel=STR_TR_None;
            QString tooltipshow,tooltiphide;
            if (!tooltip.isEmpty()) {
                tooltipshow=QString("tooltip.show(\"%1\");").arg(tooltip);
                tooltiphide="tooltip.hide();";
            }
            html+=QString("<tr bgcolor='"+color+"' onmouseover='ChangeColor(this, \"#eeeeee\"); %13' onmouseout='ChangeColor(this, \""+color+"\"); %14' onclick='alert(\"overview=%1,%2\");'><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8</td><td>%9</td><td>%10</td><td>%11</td>%12</tr>")
                    .arg(rx.first.toString(Qt::ISODate))
                    .arg(rx.last.toString(Qt::ISODate))
                    .arg(rx.first.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.last.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.days)
                    .arg(rx.ahi,0,'f',decimals)
            .arg(rx.fl,0,'f',decimals)
                    .arg(rx.machine->GetClass())
                    .arg(schema::channel[CPAP_Mode].option(int(rx.mode)-1))
                    .arg(presrel)
                    .arg(rx.min,0,'f',decimals)
                    .arg(extratxt)
                    .arg(tooltipshow)
                    .arg(tooltiphide);
        }
        html+="</table>";
        html+=QString("<i>")+tr("The above has a threshold which excludes day counts less than %1 from the best/worst highlighting").arg(rxthresh)+QString("</i><br/>");
        html+="</div>";

    }
    if (mach.size()>0) {
        html+="<div align=center>";

        html+=QString("<br/><b>")+tr("Machine Information")+"</b>";
        html+=QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");
        html+=QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td></tr>")
                      .arg(STR_TR_Brand)
                      .arg(STR_TR_Model)
                      .arg(STR_TR_Serial)
                      .arg(tr("First Use"))
                      .arg(tr("Last Use"));
        Machine *m;
        for (int i=0;i<mach.size();i++) {
            m=mach.at(i);
            if (m->GetType()==MT_JOURNAL) continue;
            QString mn=m->properties[STR_PROP_ModelNumber];
            //if (mn.isEmpty())
            html+=QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                    .arg(m->properties[STR_PROP_Brand])
                    .arg(m->properties[STR_PROP_Model]+" "+m->properties[STR_PROP_SubModel]+ (mn.isEmpty() ? "" : QString(" (")+mn+QString(")")))
                    .arg(m->properties[STR_PROP_Serial])
                    .arg(m->FirstDay().toString(Qt::SystemLocaleShortDate))
                    .arg(m->LastDay().toString(Qt::SystemLocaleShortDate));
        }
        html+="</table>";
        html+="</div>";
    }
    html+="<script type='text/javascript' language='javascript' src='qrc:/docs/script.js'></script>";
    //updateFavourites();
    html+=htmlFooter();
    return html;
}
