/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Statistics Report Generator
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QApplication>
#include <cmath>

#include "mainwindow.h"
#include "statistics.h"


extern MainWindow *mainwin;

QString formatTime(float time)
{
    int hours = time;
    int seconds = time * 3600.0;
    int minutes = (seconds / 60) % 60;
    seconds %= 60;
    return QString().sprintf("%02i:%02i", hours, minutes); //,seconds);
}


Statistics::Statistics(QObject *parent) :
    QObject(parent)
{
    rows = {
                { tr("CPAP Statistics"), SC_HEADING, MT_CPAP },
                { "", SC_DAYS, MT_CPAP },
                { "", SC_COLUMNHEADERS, MT_CPAP },
                { tr("CPAP Usage"),  SC_SUBHEADING, MT_CPAP },
                { tr("Average Hours per Night"),      SC_HOURS,     MT_CPAP },
                { tr("Compliancy"),  SC_COMPLIANCE,  MT_CPAP },

                { tr("Therapy Efficiacy"),  SC_SUBHEADING, MT_CPAP },
                { "AHI",        SC_AHI,     MT_CPAP },
                { "Obstructive",   SC_CPH,     MT_CPAP },
                { "Hypopnea",   SC_CPH,     MT_CPAP },
                { "ClearAirway",   SC_CPH,     MT_CPAP },
                { "FlowLimit",  SC_CPH,     MT_CPAP },
                { "RERA",       SC_CPH,     MT_CPAP },

                { tr("Leak Statistics"),  SC_SUBHEADING, MT_CPAP },
                { "Leak",       SC_WAVG,    MT_CPAP },
                { "Leak",       SC_90P,     MT_CPAP },

                { tr("Pressure Statistics"),  SC_SUBHEADING, MT_CPAP },
                { "Pressure",   SC_WAVG,    MT_CPAP },
                { "Pressure",   SC_MIN,     MT_CPAP },
                { "Pressure",   SC_MAX,     MT_CPAP },
                { "Pressure",   SC_90P,     MT_CPAP },
                { "EPAP",       SC_WAVG,    MT_CPAP },
                { "EPAP",       SC_MIN,     MT_CPAP },
                { "EPAP",       SC_MAX,     MT_CPAP },
                { "IPAP",       SC_WAVG,    MT_CPAP },
                { "IPAP",       SC_90P,     MT_CPAP },
                { "IPAP",       SC_MIN,     MT_CPAP },
                { "IPAP",       SC_MAX,     MT_CPAP },

                { tr("Oximeter Statistics"), SC_HEADING, MT_OXIMETER },
                { "",           SC_DAYS,    MT_OXIMETER },
                { "",           SC_COLUMNHEADERS, MT_OXIMETER },

                { tr("Blood Oxygen Saturation"),  SC_SUBHEADING, MT_CPAP },
                { "SPO2",       SC_WAVG,     MT_OXIMETER },
                { "SPO2",       SC_MIN,     MT_OXIMETER },
                { "SPO2Drop",   SC_CPH,     MT_OXIMETER },
                { "SPO2Drop",   SC_SPH,     MT_OXIMETER },
                { tr("Pulse Rate"),  SC_SUBHEADING, MT_CPAP },
                { "Pulse",      SC_WAVG,     MT_OXIMETER },
                { "Pulse",      SC_MIN,     MT_OXIMETER },
                { "Pulse",      SC_MAX,     MT_OXIMETER },
                { "PulseChange",   SC_CPH,     MT_OXIMETER },
    };

    // These are for formatting the headers for the first column
    calcnames = {
        { SC_UNDEFINED, "" },
        { SC_MEDIAN, tr("%1 Median") },
        { SC_AVG, tr("Average %1") },
        { SC_WAVG, tr("Average %1") },
        { SC_90P, tr("90% %1") }, // this gets converted to whatever the upper percentile is set to
        { SC_MIN, tr("Min %1") },
        { SC_MAX, tr("Max %1") },
        { SC_CPH, tr("%1 Index") },
        { SC_SPH, tr("% of night in %1") },
    };
    machinenames = {
        { MT_UNKNOWN, STR_TR_Unknown },
        { MT_CPAP, STR_TR_CPAP },
        { MT_OXIMETER, STR_TR_Oximetry },
        { MT_SLEEPSTAGE, STR_TR_SleepStage },
//        { MT_JOURNAL, STR_TR_Journal },
//        { MT_POSITION, STR_TR_Position },
    };

}


QString htmlHeader()
{

    QString address = PROFILE.user->address();
    address.replace("\n", "<br/>");
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
                   "<div align=center><table cellpadding=3 cellspacing=0 border=0 width=100%>"
                   "<td>" +
                   QString(QObject::tr("Name: %1, %2")).arg(PROFILE.user->lastName()).arg(
                       PROFILE.user->firstName()) + "<br/>" +
                   QString(QObject::tr("DOB: %1")).arg(PROFILE.user->DOB().toString()) + "<br/>" +
                   QString(QObject::tr("Phone: %1")).arg(PROFILE.user->phone()) + "<br/>" +
                   QString(QObject::tr("Email: %1")).arg(PROFILE.user->email()) + "<br/><br/>" +
                   QObject::tr("Address:") + "<br/>" +
                   address +

                   "</td>"
                   "<td align='right'>"
                   "<h1>" + STR_TR_SleepyHead + "</h1><br/>"
                   "<font size='+3'>" + QObject::tr("Usage Statistics") + "</font>"
                   "</td>"
                   "<td align='right' width=170px><img src='qrc:/icons/bob-v3.0.png' height=140px><br/>"
                   "</td></tr></table>"
                   "</div>"
                   "<hr/>");
}
QString htmlFooter()
{
    return "<hr/><div align=center><font size='-1'><i>" +
           QString(QObject::tr("This report was generated by a pre-release version of SleepyHead (%1), <b>and has not been approved in any way for compliance or medical diagnostic purposes</b>.")).arg(
               FullVersionString) + "<br/><br/>" +
           QObject::tr("SleepyHead is free open-source software available from http://sourceforge.net/projects/SleepyHead")
           +
           "</i></i></div>"
           "</body></html>";
}



EventDataType calcAHI(QDate start, QDate end)
{
    EventDataType val = (p_profile->calcCount(CPAP_Obstructive, MT_CPAP, start, end)
                         + p_profile->calcCount(CPAP_Hypopnea, MT_CPAP, start, end)
                         + p_profile->calcCount(CPAP_ClearAirway, MT_CPAP, start, end)
                         + p_profile->calcCount(CPAP_Apnea, MT_CPAP, start, end));

    if (PROFILE.general->calculateRDI()) {
        val += p_profile->calcCount(CPAP_RERA, MT_CPAP, start, end);
    }

    EventDataType hours = p_profile->calcHours(MT_CPAP, start, end);

    if (hours > 0) {
        val /= hours;
    } else {
        val = 0;
    }

    return val;
}

EventDataType calcFL(QDate start, QDate end)
{
    EventDataType val = (p_profile->calcCount(CPAP_FlowLimit, MT_CPAP, start, end));
    EventDataType hours = p_profile->calcHours(MT_CPAP, start, end);

    if (hours > 0) {
        val /= hours;
    } else {
        val = 0;
    }

    return val;
}


struct RXChange {
    RXChange() { highlight = 0; machine = nullptr; }
    RXChange(const RXChange &copy) {
        first = copy.first;
        last = copy.last;
        days = copy.days;
        ahi = copy.ahi;
        fl = copy.fl;
        mode = copy.mode;
        min = copy.min;
        max = copy.max;
        ps = copy.ps;
        pshi = copy.pshi;
        maxipap = copy.maxipap;
        machine = copy.machine;
        per1 = copy.per1;
        per2 = copy.per2;
        highlight = copy.highlight;
        weighted = copy.weighted;
        prelief = copy.prelief;
        prelset = copy.prelset;
    }
    QDate first;
    QDate last;
    int days;
    EventDataType ahi;
    EventDataType fl;
    CPAPMode mode;
    EventDataType min;
    EventDataType max;
    EventDataType ps;
    EventDataType pshi;
    EventDataType maxipap;
    EventDataType per1;
    EventDataType per2;
    EventDataType weighted;
    PRTypes prelief;
    Machine *machine;
    short prelset;
    short highlight;
};

enum RXSortMode { RX_first, RX_last, RX_days, RX_ahi, RX_mode, RX_min, RX_max, RX_ps, RX_pshi, RX_maxipap, RX_per1, RX_per2, RX_weighted };
RXSortMode RXsort = RX_first;
bool RXorder = false;

bool operator<(const RXChange &c1, const RXChange &c2)
{
    const RXChange *comp1 = &c1;
    const RXChange *comp2 = &c2;

    if (RXorder) {
        switch (RXsort) {
        case RX_ahi:
            return comp1->ahi < comp2->ahi;

        case RX_days:
            return comp1->days < comp2->days;

        case RX_first:
            return comp1->first < comp2->first;

        case RX_last:
            return comp1->last < comp2->last;

        case RX_mode:
            return comp1->mode < comp2->mode;

        case RX_min:
            return comp1->min < comp2->min;

        case RX_max:
            return comp1->max < comp2->max;

        case RX_ps:
            return comp1->ps < comp2->ps;

        case RX_pshi:
            return comp1->pshi < comp2->pshi;

        case RX_maxipap:
            return comp1->maxipap < comp2->maxipap;

        case RX_per1:
            return comp1->per1 < comp2->per1;

        case RX_per2:
            return comp1->per2 < comp2->per2;

        case RX_weighted:
            return comp1->weighted < comp2->weighted;
        };
    } else {
        switch (RXsort) {
        case RX_ahi:
            return comp1->ahi > comp2->ahi;

        case RX_days:
            return comp1->days > comp2->days;

        case RX_first:
            return comp1->first > comp2->first;

        case RX_last:
            return comp1->last > comp2->last;

        case RX_mode:
            return comp1->mode > comp2->mode;

        case RX_min:
            return comp1->min > comp2->min;

        case RX_max:
            return comp1->max > comp2->max;

        case RX_ps:
            return comp1->ps > comp2->ps;

        case RX_pshi:
            return comp1->pshi > comp2->pshi;

        case RX_maxipap:
            return comp1->maxipap > comp2->maxipap;

        case RX_per1:
            return comp1->per1 > comp2->per1;

        case RX_per2:
            return comp1->per2 > comp2->per2;

        case RX_weighted:
            return comp1->weighted > comp2->weighted;
        };
    }

    return true;
}

bool RXSort(const RXChange *comp1, const RXChange *comp2)
{
    if (RXorder) {
        switch (RXsort) {
        case RX_ahi:
            return comp1->ahi < comp2->ahi;

        case RX_days:
            return comp1->days < comp2->days;

        case RX_first:
            return comp1->first < comp2->first;

        case RX_last:
            return comp1->last < comp2->last;

        case RX_mode:
            return comp1->mode < comp2->mode;

        case RX_min:
            return comp1->min < comp2->min;

        case RX_max:
            return comp1->max < comp2->max;

        case RX_ps:
            return comp1->ps < comp2->ps;

        case RX_pshi:
            return comp1->pshi < comp2->pshi;

        case RX_maxipap:
            return comp1->maxipap < comp2->maxipap;

        case RX_per1:
            return comp1->per1 < comp2->per1;

        case RX_per2:
            return comp1->per2 < comp2->per2;

        case RX_weighted:
            return comp1->weighted < comp2->weighted;
        };
    } else {
        switch (RXsort) {
        case RX_ahi:
            return comp1->ahi > comp2->ahi;

        case RX_days:
            return comp1->days > comp2->days;

        case RX_first:
            return comp1->first > comp2->first;

        case RX_last:
            return comp1->last > comp2->last;

        case RX_mode:
            return comp1->mode > comp2->mode;

        case RX_min:
            return comp1->min > comp2->min;

        case RX_max:
            return comp1->max > comp2->max;

        case RX_ps:
            return comp1->ps > comp2->ps;

        case RX_pshi:
            return comp1->pshi > comp2->pshi;

        case RX_maxipap:
            return comp1->maxipap > comp2->maxipap;

        case RX_per1:
            return comp1->per1 > comp2->per1;

        case RX_per2:
            return comp1->per2 > comp2->per2;

        case RX_weighted:
            return comp1->weighted > comp2->weighted;
        };
    }

    return true;
}
struct UsageData {
    UsageData() { ahi = 0; hours = 0; }
    UsageData(QDate d, EventDataType v, EventDataType h) { date = d; ahi = v; hours = h; }
    UsageData(const UsageData &copy) { date = copy.date; ahi = copy.ahi; hours = copy.hours; }
    QDate date;
    EventDataType ahi;
    EventDataType hours;
};
bool operator <(const UsageData &c1, const UsageData &c2)
{
    if (c1.ahi < c2.ahi) {
        return true;
    }

    if ((c1.ahi == c2.ahi) && (c1.date > c2.date)) { return true; }

    return false;
    //return c1.value < c2.value;
}

QString Statistics::GenerateHTML()
{

    QString heading_color="#f8f0ff";
    QString subheading_color="#efefef";

    QString html = htmlHeader();

    // Find first and last days with valid CPAP data
    QDate lastcpap = p_profile->LastGoodDay(MT_CPAP);
    QDate firstcpap = p_profile->FirstGoodDay(MT_CPAP);


    QDate cpapweek = lastcpap.addDays(-6);
    QDate cpapmonth = lastcpap.addDays(-29);
    QDate cpap6month = lastcpap.addMonths(-6);
    QDate cpapyear = lastcpap.addMonths(-12);

    if (cpapweek < firstcpap) { cpapweek = firstcpap; }
    if (cpapmonth < firstcpap) { cpapmonth = firstcpap; }
    if (cpap6month < firstcpap) { cpap6month = firstcpap; }
    if (cpapyear < firstcpap) { cpapyear = firstcpap; }

    QList<Machine *> cpap_machines = PROFILE.GetMachines(MT_CPAP);
    QList<Machine *> oximeters = PROFILE.GetMachines(MT_OXIMETER);
    QList<Machine *> mach;
    mach.append(cpap_machines);
    mach.append(oximeters);


    if (mach.size() == 0) {
        html += "<table cellpadding=2 cellspacing=0 border=0 width=100% height=60%>";
        html += "<tr><td align=center><h1>" + tr("Please Import Some Data") + "</h1><i>" +
                tr("SleepyHead is pretty much useless without it.") + "</i><br/><p>" +
                tr("It might be a good idea to check preferences first,</br>as there are some options that affect import.")
                + "</p><p>" + tr("First import can take a few minutes.") + "</p></td></tr></table>";
        html += htmlFooter();
        return html;
    }

    int cpapdays = PROFILE.countDays(MT_CPAP, firstcpap, lastcpap);

    CPAPMode cpapmode = (CPAPMode)(int)p_profile->calcSettingsMax(CPAP_Mode, MT_CPAP, firstcpap,
                        lastcpap);

    float percentile = PROFILE.general->prefCalcPercentile() / 100.0;

    //    int mididx=PROFILE.general->prefCalcMiddle();
    //    SummaryType ST_mid;
    //    if (mididx==0) ST_mid=ST_PERC;
    //    if (mididx==1) ST_mid=ST_WAVG;
    //    if (mididx==2) ST_mid=ST_AVG;

    QString ahitxt;

    if (PROFILE.general->calculateRDI()) {
        ahitxt = STR_TR_RDI;
    } else {
        ahitxt = STR_TR_AHI;
    }

    int decimals = 2;
    html += "<div align=center>";
    html += QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");


    QDate last = lastcpap, first = lastcpap, week = lastcpap, month = lastcpap, sixmonth = lastcpap, year = lastcpap;

    bool skipsection = false;;
    for (auto i = rows.begin(); i != rows.end(); ++i) {
        StatisticsRow &row = (*i);
        QString name;

        if (row.calc == SC_HEADING) {  // All sections begin with a heading
            last = p_profile->LastGoodDay(row.type);
            first  = p_profile->FirstGoodDay(row.type);

            week = last.addDays(-6);
            month = last.addDays(-29);
            sixmonth = last.addMonths(-6);
            year = last.addMonths(-12);

            if (week < first) { week = first; }
            if (month < first) { month = first; }
            if (sixmonth < first) { sixmonth = first; }
            if (year < first) { year = first; }

            int days = PROFILE.countDays(row.type, first, last);
            skipsection = (days == 0);
            if (days > 0) {
                html+=QString("<tr bgcolor='"+heading_color+"'><td colspan=6 align=center><font size=+3>%1</font></td></tr>\n").arg(row.src);
            }
            continue;
        }

        // Bypass this entire section if no data is present
        if (skipsection) continue;

        if (row.calc == SC_AHI) {
            name = ahitxt;
        } else if ((row.calc == SC_HOURS) || (row.calc == SC_COMPLIANCE)) {
            name = row.src;
        } else if (row.calc == SC_COLUMNHEADERS) {
            html += QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</td></tr>")
                    .arg(tr("Details")).arg(tr("Most Recent")).arg(tr("Last 7 Days")).arg(tr("Last 30 Days")).arg(
                        tr("Last 6 months")).arg(tr("Last Year"));
            continue;
        } else if (row.calc == SC_DAYS) {
            QDate first=p_profile->FirstGoodDay(row.type);
            QDate last=p_profile->LastGoodDay(row.type);
            QString & machine = machinenames[row.type];
            int value=p_profile->countDays(row.type, first, last);

            if (value == 0) {
                html+="<tr><td colspan=6 align=center>"+
                        QString(tr("No %1 data available.")).arg(machine)
                        +"</td></tr>";
            } else if (value == 1) {
                html+="<tr><td colspan=6 align=center>"+
                        QString("%1 day of %2 Data on %3")
                            .arg(value)
                            .arg(machine)
                            .arg(last.toString())
                        +"</td></tr>\n";
            } else {
                html+="<tr><td colspan=6 align=center>"+
                        QString("%1 days of %2 Data, between %3 and %4")
                            .arg(value)
                            .arg(machine)
                            .arg(first.toString())
                            .arg(last.toString())
                        +"</td></tr>\n";
            }
            continue;
        } else if (row.calc == SC_SUBHEADING) {  // subheading..
            html+=QString("<tr bgcolor='"+subheading_color+"'><td colspan=6 align=center><b>%1</b></td></tr>\n").arg(row.src);
            continue;
        } else if (row.calc == SC_UNDEFINED) {
            continue;
        } else {
            ChannelID id = schema::channel[row.src].id();
            if ((id == NoChannel) || (!PROFILE.hasChannel(id))) {
                continue;
            }
            name = calcnames[row.calc].arg(row.src);
        }

        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td></tr>\n")
        .arg(name)
        .arg(row.value(last,last))
        .arg(row.value(week,last))
        .arg(row.value(month,last))
        .arg(row.value(sixmonth,last))
        .arg(row.value(year,last));
    }

    html += "</table>";
    html += "</div>";

    QList<UsageData> AHI;

    if (cpapdays > 0) {
        QDate first, last = lastcpap;
        CPAPMode mode = MODE_UNKNOWN, cmode = MODE_UNKNOWN;
        EventDataType cmin = 0, cmax = 0, cps = 0, cpshi = 0, cmaxipap = 0, min = 0, max = 0, maxipap = 0,
                      ps = 0, pshi = 0;
        Machine *mach = nullptr, *lastmach = nullptr;
        PRTypes lastpr = PR_UNKNOWN, prelief = PR_UNKNOWN;
        short prelset = 0, lastprelset = -1;
        QDate date = lastcpap;
        Day *day;
        bool lastchanged = false;
        QVector<RXChange> rxchange;
        EventDataType hours;

        int compliant = 0;

        do {
            day = PROFILE.GetGoodDay(date, MT_CPAP);

            if (day) {
                lastchanged = false;

                hours = day->hours();

                if (hours > PROFILE.cpap->complianceHours()) {
                    compliant++;
                }

                EventDataType ahi = day->count(CPAP_Obstructive) + day->count(CPAP_Hypopnea) + day->count(
                                        CPAP_Apnea) + day->count(CPAP_ClearAirway);

                if (PROFILE.general->calculateRDI()) { ahi += day->count(CPAP_RERA); }

                ahi /= hours;
                AHI.push_back(UsageData(date, ahi, hours));

                prelief = (PRTypes)(int)round(day->settings_wavg(CPAP_PresReliefType));
                prelset = round(day->settings_wavg(CPAP_PresReliefSet));
                mode = (CPAPMode)(int)round(day->settings_wavg(CPAP_Mode));
                mach = day->machine;

                min = max = ps = pshi = maxipap = 0;

                if (mode == MODE_CPAP) {
                    min = day->settings_min(CPAP_Pressure);
                } else if (mode < MODE_BIPAP) {
                    min = day->settings_min(CPAP_PressureMin);
                    max = day->settings_max(CPAP_PressureMax);
                } else {
                    // BIPAP or ASV machines
                    // min & max hold EPAP
                    if (day->settingExists(CPAP_EPAPLo)) {
                        min = day->settings_min(CPAP_EPAPLo);
                    } else if (day->settingExists(CPAP_EPAP)) {
                        max = min = day->settings_min(CPAP_EPAP);
                    }

                    if (day->settingExists(CPAP_EPAPHi)) {
                        max = day->settings_min(CPAP_EPAPHi);
                    }

                    if (day->settingExists(CPAP_PSMin)) {
                        ps = day->settings_min(CPAP_PSMin);
                    } else if (day->settingExists(CPAP_PS)) {
                        pshi = ps = day->settings_min(CPAP_PS);
                    }

                    if (day->settingExists(CPAP_PSMax)) {
                        pshi = day->settings_max(CPAP_PSMax);
                    }

                    if (day->settingExists(CPAP_IPAPHi)) {
                        maxipap = day->settings_max(CPAP_IPAPHi);
                    }

                }

                if ((mode != cmode) || (min != cmin) || (max != cmax) || (ps != cps) || (pshi != cpshi)
                        || (maxipap != cmaxipap) || (mach != lastmach) || (prelset != lastprelset))  {
                    if ((cmode != MODE_UNKNOWN) && (lastmach != nullptr)) {
                        first = date.addDays(1);
                        int days = PROFILE.countDays(MT_CPAP, first, last);
                        RXChange rx;
                        rx.first = first;
                        rx.last = last;
                        rx.days = days;
                        rx.ahi = calcAHI(first, last);
                        rx.fl = calcFL(first, last);
                        rx.mode = cmode;
                        rx.min = cmin;
                        rx.max = cmax;
                        rx.ps = cps;
                        rx.pshi = cpshi;
                        rx.maxipap = cmaxipap;
                        rx.prelief = lastpr;
                        rx.prelset = lastprelset;
                        rx.machine = lastmach;

                        if (mode < MODE_BIPAP) {
                            rx.per1 = p_profile->calcPercentile(CPAP_Pressure, percentile, MT_CPAP, first, last);
                            rx.per2 = 0;
                        } else if (mode < MODE_ASV) {
                            rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                            rx.per2 = p_profile->calcPercentile(CPAP_IPAP, percentile, MT_CPAP, first, last);
                        } else {
                            rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                            rx.per2 = p_profile->calcPercentile(CPAP_IPAP, percentile, MT_CPAP, first, last);
                        }

                        rx.weighted = float(rx.days) / float(cpapdays) * rx.ahi;
                        rxchange.push_back(rx);
                    }

                    cmode = mode;
                    cmin = min;
                    cmax = max;
                    cps = ps;
                    cpshi = pshi;
                    cmaxipap = maxipap;
                    lastpr = prelief;
                    lastprelset = prelset;
                    last = date;
                    lastmach = mach;
                    lastchanged = true;
                }

            }

            date = date.addDays(-1);
        } while (date >= firstcpap);

        // Sort list by AHI
        qSort(AHI);

        lastchanged = false;

        if (!lastchanged && (mach != nullptr)) {
            // last=date.addDays(1);
            first = firstcpap;
            int days = PROFILE.countDays(MT_CPAP, first, last);
            RXChange rx;
            rx.first = first;
            rx.last = last;
            rx.days = days;
            rx.ahi = calcAHI(first, last);
            rx.fl = calcFL(first, last);
            rx.mode = mode;
            rx.min = min;
            rx.max = max;
            rx.ps = ps;
            rx.pshi = pshi;
            rx.maxipap = maxipap;
            rx.prelief = prelief;
            rx.prelset = prelset;
            rx.machine = mach;

            if (mode < MODE_BIPAP) {
                rx.per1 = p_profile->calcPercentile(CPAP_Pressure, percentile, MT_CPAP, first, last);
                rx.per2 = 0;
            } else if (mode < MODE_ASV) {
                rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                rx.per2 = p_profile->calcPercentile(CPAP_IPAP, percentile, MT_CPAP, first, last);
            } else {
                rx.per1 = p_profile->calcPercentile(CPAP_EPAP, percentile, MT_CPAP, first, last);
                rx.per2 = p_profile->calcPercentile(CPAP_IPAPHi, percentile, MT_CPAP, first, last);
            }

            rx.weighted = float(rx.days) / float(cpapdays);
            //rx.weighted=float(days)*rx.ahi;
            rxchange.push_back(rx);
        }

        int rxthresh = 5;
        QVector<RXChange *> tmpRX;

        for (int i = 0; i < rxchange.size(); i++) {
            RXChange &rx = rxchange[i];

            if (rx.days > rxthresh) {
                tmpRX.push_back(&rx);
            }
        }

        QString recbox = "<html><head><style type='text/css'>"
                         "p,a,td,body { font-family: '" + QApplication::font().family() + "'; }"
                         "p,a,td,body { font-size: " + QString::number(QApplication::font().pointSize() + 2) + "px; }"
                         "a:link,a:visited { color: inherit; text-decoration: none; }" //font-weight: normal;
                         "a:hover { background-color: inherit; color: white; text-decoration:none; font-weight: bold; }"
                         "</style></head><body>";
        recbox += "<table width=100% cellpadding=1 cellspacing=0>";
        int numdays = AHI.size();

        if (numdays > 1) {
            int z = numdays / 2;

            if (z > 4) { z = 4; }

            recbox += QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(
                          tr("Usage Information"));
            recbox += QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Total Days")).arg(
                          numdays);

            if (PROFILE.cpap->showComplianceInfo()) {
                recbox += QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Compliant Days")).arg(
                              compliant);
            }

            int highahi = 0;

            for (int i = 0; i < numdays; i++) {
                if (AHI.at(i).ahi > 5.0) {
                    highahi++;
                }
            }

            recbox += QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(tr("Days AHI &gt;5.0")).arg(
                          highahi);


            recbox += QString("<tr><td colspan=2>&nbsp;</td></tr>");
            recbox += QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(tr("Best&nbsp;%1").arg(
                          ahitxt));

            for (int i = 0; i < z; i++) {
                const UsageData &a = AHI.at(i);
                recbox += QString("<tr><td><a href='daily=%1'>%2</a></td><td  align=right>%3</td></tr>")
                          .arg(a.date.toString(Qt::ISODate))
                          .arg(a.date.toString(Qt::SystemLocaleShortDate))
                          .arg(a.ahi, 0, 'f', decimals);
            }

            recbox += QString("<tr><td colspan=2>&nbsp;</td></tr>");
            recbox += QString("<tr><td colspan=2 align=center><b>%1</b></td></tr>").arg(
                          tr("Worst&nbsp;%1").arg(ahitxt));

            for (int i = 0; i < z; i++) {
                const UsageData &a = AHI.at((numdays - 1) - i);
                recbox += QString("<tr><td><a href='daily=%1'>%2</a></td><td align=right>%3</td></tr>")
                          .arg(a.date.toString(Qt::ISODate))
                          .arg(a.date.toString(Qt::SystemLocaleShortDate))
                          .arg(a.ahi, 0, 'f', decimals);
            }

            recbox += QString("<tr><td colspan=2>&nbsp;</td></tr>");
        }


        if (tmpRX.size() > 0) {
            RXsort = RX_ahi;
            QString minstr, maxstr, modestr, maxhistr;
            qSort(tmpRX.begin(), tmpRX.end(), RXSort);
            tmpRX[0]->highlight = 4; // worst
            int ls = tmpRX.size() - 1;
            tmpRX[ls]->highlight = 1; //best
            CPAPMode mode = (CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode, MT_CPAP, tmpRX[ls]->first,
                            tmpRX[ls]->last);


            if (mode < MODE_APAP) { // is CPAP?
                minstr = STR_TR_Pressure;
                maxstr = "";
                modestr = STR_TR_CPAP;
            } else if (mode < MODE_BIPAP) { // is AUTO?
                minstr = STR_TR_Min;
                maxstr = STR_TR_Max;
                modestr = STR_TR_APAP;
            } else {

                if (mode < MODE_ASV) { // BIPAP
                    minstr = STR_TR_EPAP;
                    maxstr = STR_TR_IPAP;
                    modestr = STR_TR_BiLevel;
                } else {
                    minstr = STR_TR_EPAP;
                    maxstr = STR_TR_IPAPLo;
                    maxhistr = STR_TR_IPAPHi;
                    modestr = STR_TR_STASV;

                }
            }

            recbox += QString("<tr><td colspan=2><table width=100% border=0 cellpadding=1 cellspacing=0><tr><td colspan=2 align=center><b>%3</b></td></tr>")
                      .arg(tr("Best RX Setting"));
            recbox += QString("<tr><td valign=top>") + STR_TR_Start + "<br/>" + STR_TR_End +
                      QString("</td><td align=right><a href='overview=%1,%2'>%3<br/>%4</a></td></tr>")
                      .arg(tmpRX[ls]->first.toString(Qt::ISODate))
                      .arg(tmpRX[ls]->last.toString(Qt::ISODate))
                      .arg(tmpRX[ls]->first.toString(Qt::SystemLocaleShortDate))
                      .arg(tmpRX[ls]->last.toString(Qt::SystemLocaleShortDate));
            recbox += QString("<tr><td><b>%1</b></td><td align=right><b>%2</b></td></tr>").arg(ahitxt).arg(
                          tmpRX[ls]->ahi, 0, 'f', decimals);
            recbox += QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(STR_TR_Mode).arg(modestr);
            recbox += QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(minstr).arg(tmpRX[ls]->min,
                      0, 'f', 1).arg(STR_UNIT_CMH2O);

            if (!maxstr.isEmpty()) { recbox += QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxstr).arg(tmpRX[ls]->max, 0, 'f', 1).arg(STR_UNIT_CMH2O); }

            if (!maxhistr.isEmpty()) { recbox += QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxhistr).arg(tmpRX[ls]->maxipap, 0, 'f', 1).arg(STR_UNIT_CMH2O); }

            recbox += "</table></td></tr>";

            recbox += QString("<tr><td colspan=2>&nbsp;</td></tr>");

            mode = (CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode, MT_CPAP, tmpRX[0]->first, tmpRX[0]->last);

            if (mode < MODE_APAP) { // is CPAP?
                minstr = STR_TR_Pressure;
                maxstr = "";
                modestr = STR_TR_CPAP;
            } else if (mode < MODE_BIPAP) { // is AUTO?
                minstr = STR_TR_Min;
                maxstr = STR_TR_Max;
                modestr = STR_TR_APAP;
            } else if (mode < MODE_ASV) { // BIPAP or greater
                minstr = STR_TR_EPAP;
                maxstr = STR_TR_IPAP;
                modestr = STR_TR_BiLevel;
            } else {
                minstr = STR_TR_EPAP;
                maxstr = STR_TR_IPAPLo;
                maxhistr = STR_TR_IPAPHi;
                modestr = STR_TR_STASV;
            }

            recbox += QString("<tr><td colspan=2><table width=100% border=0 cellpadding=1 cellspacing=0><tr><td colspan=2 align=center><b>%3</b></td></tr>")
                      .arg(tr("Worst RX Setting"));
            recbox += QString("<tr><td valign=top>") + STR_TR_Start + "<br/>" + STR_TR_End +
                      QString("</td><td align=right><a href='overview=%1,%2'>%3<br/>%4</a></td></tr>")
                      .arg(tmpRX[0]->first.toString(Qt::ISODate))
                      .arg(tmpRX[0]->last.toString(Qt::ISODate))
                      .arg(tmpRX[0]->first.toString(Qt::SystemLocaleShortDate))
                      .arg(tmpRX[0]->last.toString(Qt::SystemLocaleShortDate));
            recbox += QString("<tr><td><b>%1</b></td><td align=right><b>%2</b></td></tr>").arg(ahitxt).arg(
                          tmpRX[0]->ahi, 0, 'f', decimals);
            recbox += QString("<tr><td>%1</td><td align=right>%2</td></tr>").arg(STR_TR_Mode).arg(modestr);
            recbox += QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(minstr).arg(tmpRX[0]->min,
                      0, 'f', 1).arg(STR_UNIT_CMH2O);

            if (!maxstr.isEmpty()) { recbox += QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxstr).arg(tmpRX[0]->max, 0, 'f', 1).arg(STR_UNIT_CMH2O); }

            if (!maxhistr.isEmpty()) { recbox += QString("<tr><td>%1</td><td align=right>%2%3</td></tr>").arg(maxhistr).arg(tmpRX[0]->maxipap, 0, 'f', 1).arg(STR_UNIT_CMH2O); }

            recbox += "</table></td></tr>";

        }

        recbox += "</table>";
        recbox += "</body></html>";
        mainwin->setRecBoxHTML(recbox);

        /*RXsort=RX_min;
        RXorder=true;
        qSort(rxchange.begin(),rxchange.end());*/
        html += "<div align=center><br/>";
        html += QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");
        html += "<tr bgcolor='"+heading_color+"'><td colspan=9 align=center><font size=+3>" + tr("Changes to Prescription Settings") + "</font></td></tr>";
        QString extratxt;

        QString tooltip;
        html += QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td><td><b>%6</b></td><td><b>%7</b></td><td><b>%8</b></td><td><b>%9</b></td></tr>")
                .arg(STR_TR_First)
                .arg(STR_TR_Last)
                .arg(tr("Days"))
                .arg(ahitxt)
                .arg(tr("FL"))
                .arg(STR_TR_Machine)
                .arg(tr("Pr. Rel."))
                .arg(STR_TR_Mode)
                .arg(tr("Pressure Settings"));

        for (int i = 0; i < rxchange.size(); i++) {
            RXChange rx = rxchange.at(i);
            QString color;

            if (rx.highlight == 1) {
                color = "#c0ffc0";
            } else if (rx.highlight == 2) {
                color = "#e0ffe0";
            } else if (rx.highlight == 3) {
                color = "#ffe0e0";
            } else if (rx.highlight == 4) {
                color = "#ffc0c0";
            } else { color = ""; }

            QString machstr;

            if (rx.machine->properties.contains(STR_PROP_Brand)) {
                machstr += rx.machine->properties[STR_PROP_Brand];
            }

            if (rx.machine->properties.contains(STR_PROP_Model)) {
                machstr += " " + rx.machine->properties[STR_PROP_Model];
            }

            if (rx.machine->properties.contains(STR_PROP_Serial)) {
                machstr += " (" + rx.machine->properties[STR_PROP_Serial] + ")<br/>";
            }

            mode = rx.mode;
            extratxt = "<table cellpadding=0 cellspacing=0 border=0 width=100%><tr>";

            //            tooltip=QString("%1 %2% ").arg(machstr).arg(percentile*100.0)+STR_TR_EPAP+
            //                    QString("=%1<br/>%2% ").arg(rx.per1,0,'f',decimals).arg(percentile*100.0)+
            //                    STR_TR_IPAP+QString("=%1").arg(rx.per2,0,'f',decimals);
            if (mode >= MODE_BIPAP) {
                if (rx.min > 0) {
                    extratxt += "<td>"+QString(tr("EPAP %1"))
                                .arg(rx.min, 4, 'f', 1);
                }

                if ((rx.max > 0) && (rx.min != rx.max)) {
                    extratxt += QString(" - %2")
                                .arg(rx.max, 4, 'f', 1);
                }

                extratxt += "</td><td>";

                if (rx.ps > 0) {
                    extratxt += "<td>"+QString(tr("PS %1")).arg(rx.ps, 4, 'f', 1);
                }

                if ((rx.pshi > 0) && (rx.ps != rx.pshi)) {
                    extratxt += QString(" - %2").arg(rx.pshi, 4, 'f', 1);
                }

                extratxt += "</td>";

                if (rx.maxipap > 0) {
                    extratxt += "<td>"+QString(tr("IPAP %1")+"</td>")
                                .arg(rx.maxipap, 4, 'f', 1);
                }

                tooltip = QString("%1 %2% ").arg(machstr).arg(percentile * 100.0) +
                          STR_TR_EPAP +
                          QString("=%1<br/>%2% ").arg(rx.per1, 0, 'f', decimals)
                          .arg(percentile * 100.0)
                          + STR_TR_IPAP + QString("=%1").arg(rx.per2, 0, 'f', decimals);
            } else if (mode > MODE_CPAP) {
                extratxt += "<td align=left>"+QString(tr("APAP %1 - %2")+"</td><td align=left></td>")
                            .arg(rx.min, 4, 'f', 1)
                            .arg(rx.max, 4, 'f', 1);
                tooltip = QString("%1 %2% ").arg(machstr).arg(percentile * 100.0) + STR_TR_Pressure +
                          QString("=%2").arg(rx.per1, 0, 'f', decimals);
            } else {

                if (cpapmode > MODE_CPAP) {
                    extratxt += "<td colspan=2>"+QString(tr("CPAP %1")+"</td>").arg(rx.min, 4, 'f', 1);
                    tooltip = QString("%1").arg(machstr);
                } else {
                    extratxt += "";
                    tooltip = "";
                }
            }

            extratxt += "</tr></table>";
            QString presrel;

            if (rx.prelset > 0) {
                presrel = schema::channel[CPAP_PresReliefType].option(int(rx.prelief));
                presrel += QString(" x%1").arg(rx.prelset);
            } else { presrel = STR_TR_None; }

            QString tooltipshow, tooltiphide;

            if (!tooltip.isEmpty()) {
                tooltipshow = QString("tooltip.show(\"%1\");").arg(tooltip);
                tooltiphide = "tooltip.hide();";
            }

            html += QString("<tr bgcolor='" + color +
                            "' onmouseover='ChangeColor(this, \"#eeeeee\"); %12' onmouseout='ChangeColor(this, \"" + color +
                            "\"); %13' onclick='alert(\"overview=%1,%2\");'><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8</td><td>%9</td><td>%10</td><td>%11</td></tr>")
                    .arg(rx.first.toString(Qt::ISODate))
                    .arg(rx.last.toString(Qt::ISODate))
                    .arg(rx.first.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.last.toString(Qt::SystemLocaleShortDate))
                    .arg(rx.days)
                    .arg(rx.ahi, 0, 'f', decimals)
                    .arg(rx.fl, 0, 'f', decimals) // Not the best way to do this.. Todo: Add an extra field for data..
                    .arg(rx.machine->GetClass())
                    .arg(presrel)
                    .arg(schema::channel[CPAP_Mode].option(int(rx.mode) - 1))
                    .arg(extratxt)
                    .arg(tooltipshow)
                    .arg(tooltiphide);
        }

        html += "</table>";
        html += QString("<i>") +
                tr("Efficacy highlighting ignores prescription settings with less than %1 days of recorded data.").arg(
                    rxthresh) + QString("</i><br/>");
        html += "</div>";

    }

    if (mach.size() > 0) {
        html += "<div align=center><br/>";

        html += QString("<table cellpadding=2 cellspacing=0 border=1 width=90%>");
        html += "<tr bgcolor='"+heading_color+"'><td colspan=5 align=center><font size=+3>" + tr("Machine Information") + "</font></td></tr>";

        html += QString("<tr><td><b>%1</b></td><td><b>%2</b></td><td><b>%3</b></td><td><b>%4</b></td><td><b>%5</b></td></tr>")
                .arg(STR_TR_Brand)
                .arg(STR_TR_Model)
                .arg(STR_TR_Serial)
                .arg(tr("First Use"))
                .arg(tr("Last Use"));
        Machine *m;

        for (int i = 0; i < mach.size(); i++) {
            m = mach.at(i);

            if (m->GetType() == MT_JOURNAL) { continue; }

            QString mn = m->properties[STR_PROP_ModelNumber];
            //if (mn.isEmpty())
            html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                    .arg(m->properties[STR_PROP_Brand])
                    .arg(m->properties[STR_PROP_Model] + " " + m->properties[STR_PROP_SubModel] +
                         (mn.isEmpty() ? "" : QString(" (") + mn + QString(")")))
                    .arg(m->properties[STR_PROP_Serial])
                    .arg(m->FirstDay().toString(Qt::SystemLocaleShortDate))
                    .arg(m->LastDay().toString(Qt::SystemLocaleShortDate));
        }

        html += "</table>";
        html += "</div>";
    }

    html += "<script type='text/javascript' language='javascript' src='qrc:/docs/script.js'></script>";
    //updateFavourites();
    html += htmlFooter();
    return html;
}


QString StatisticsRow::value(QDate start, QDate end)
{
    const int decimals=2;
    QString value = "???";
    qDebug() << "Calculating " << src << calc << "for" << start << end;
    float days = PROFILE.countDays(type, start, end);

    // Handle special data sources first
    if (calc == SC_AHI) {
        value = QString("%1").arg(calcAHI(start, end), 0, 'f', decimals);
    } else if (calc == SC_HOURS) {
        value = QString("%1").arg(formatTime(p_profile->calcHours(type, start, end) / days));
    } else if (calc == SC_COMPLIANCE) {
        float c = p_profile->countCompliantDays(type, start, end);
        float p = (100.0 / days) * c;
        value = QString("%1%").arg(p, 0, 'f', 0);
    } else if (calc == SC_DAYS) {
        value = QString("%1").arg(p_profile->countDays(type, start, end));
    } else if ((calc == SC_COLUMNHEADERS) || (calc == SC_SUBHEADING) || (calc == SC_UNDEFINED))  {
    } else {
        //
        ChannelID code=channel();
        if (code != NoChannel) {
            switch(calc) {
            case SC_AVG:
                value = QString("%1").arg(p_profile->calcAvg(code, type, start, end), 0, 'f', decimals);
                break;
            case SC_WAVG:
                value = QString("%1").arg(p_profile->calcWavg(code, type, start, end), 0, 'f', decimals);
                    break;
            case SC_MEDIAN:
                value = QString("%1").arg(p_profile->calcPercentile(code, 0.5, type, start, end), 0, 'f', decimals);
                break;
            case SC_90P:
                value = QString("%1").arg(p_profile->calcPercentile(code, 0.9, type, start, end), 0, 'f', decimals);
                break;
            case SC_MIN:
                value = QString("%1").arg(p_profile->calcMin(code, type, start, end), 0, 'f', decimals);
                break;
            case SC_MAX:
                value = QString("%1").arg(p_profile->calcMax(code, type, start, end), 0, 'f', decimals);
                break;
            case SC_CPH:
                value = QString("%1").arg(PROFILE.calcCount(code, type, start, end)
                     / PROFILE.calcHours(type, start, end), 0, 'f', decimals);
                break;
            case SC_SPH:
                value = QString("%1%").arg(100.0 / p_profile->calcHours(type, start, end)
                     * p_profile->calcSum(code, type, start, end)
                     / 3600.0, 0, 'f', decimals);

            default:
                break;
            };
        }
    }

    return value;
}
