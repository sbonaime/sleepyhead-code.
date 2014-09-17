/* Welcome Page Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */


#include <QString>
#include <QApplication>
#include <QFont>
#include "SleepLib/profiles.h"


QString GenerateWelcomeHTML()
{
    QList<Machine *> cpap_machines = p_profile->GetMachines(MT_CPAP);
    QList<Machine *> oximeters = p_profile->GetMachines(MT_OXIMETER);
    QList<Machine *> mach;

    mach.append(cpap_machines);
    mach.append(oximeters);

    bool havecpapdata = false;
    bool haveoximeterdata = false;
    for (int i=0; i < cpap_machines.size(); ++i) {
        int daysize = cpap_machines[i]->day.size();
        if (daysize > 0) {
            havecpapdata = true;
            break;
        }
    }
    for (int i=0; i < oximeters.size(); ++i) {
        int daysize = oximeters[i]->day.size();
        if (daysize > 0) {
            haveoximeterdata = true;
            break;
        }
    }


    QString html = QString("<html><head>")+
    "</head>"
    "<style type='text/css'>"
    "p,a,td,body { font-family: '"+QApplication::font().family()+"'; }"
    "p,a,td,body { font-size: "+QString::number(QApplication::font().pointSize() + 2)+"px; }"

    "table.curved {"
        "border: 1px solid gray;"
        "border-radius:10px;"
        "-moz-border-radius:10px;"
        "-webkit-border-radius:10px;"
        "page-break-after:auto;"
        "-fs-table-paginate: paginate;"
    "}"
            "table.curved2 {"
                "border: 1px solid gray;"
                "border-radius:10px;"
                "background:#ffffc0;"
                "-moz-border-radius:10px;"
                "-webkit-border-radius:10px;"
                "page-break-after:auto;"
                "-fs-table-paginate: paginate;"
            "}"
    "tr.datarow:nth-child(even) {"
        "background-color: #f8f8f8;"
    "}"
    "table { page-break-after:auto; -fs-table-paginate: paginate; }"
    "tr    { page-break-inside:avoid; page-break-after:auto }"
    "td    { page-break-inside:avoid; page-break-after:auto }"
    "thead { display:table-header-group; }"
    "tfoot { display:table-footer-group; }"


    "</style>"
    "<link rel='stylesheet' type='text/css' href='qrc:/docs/tooltips.css' />"
    "<script type='text/javascript'>"
    "function ChangeColor(tableRow, highLight)"
    "{ tableRow.style.backgroundColor = highLight; }"
    "function Go(url) { throw(url); }"
    "</script>"
    "</head>"
    "<body leftmargin=0 topmargin=5 rightmargin=0>";

    html += "<div align=center><table border=0 height=100% width=99%>";
    html += QString("<tr><td colspan=2 align=center>") +
    "<img src='qrc:/icons/bob-v3.0.png' height=100px>"

    "<h1>" + QObject::tr("Welcome to SleepyHead") + "</h1>" +

    "<table cellpadding=4 border=0>";

    int cols=2;
    if (havecpapdata || haveoximeterdata) cols=4;


     html+=QString("<tr><td colspan=%1 align=center>").arg(cols)+
     "<font size=+1>"+((havecpapdata || haveoximeterdata) ? QObject::tr("What would you like to do?") : QObject::tr("Please Import Some Data")) +"</font></td>"
     "</tr>"
     "<tr>"
      "<td align=center>"
        "<table class=curved cellpadding=4>"
         "<tr><td align=center onmouseover='ChangeColor(this, \"#eeeeee\");' onmouseout='ChangeColor(this, \"#ffffff\");' onclick='alert(\"import=cpap\");'><font size=+1><img src='qrc:/icons/sdcard.png' width=128px><br/>" + QObject::tr("CPAP<br/>Importer")+"</font></td></tr>"
        "</table>"
      "</td>"
      "<td align=center>"
       "<table class=curved cellpadding=4>"
         "<tr><td align=center onmouseover='ChangeColor(this, \"#eeeeee\");' onmouseout='ChangeColor(this, \"#ffffff\");' onclick='alert(\"import=oximeter\");'><font size=+1><img src='qrc:/icons/cms50f.png' width=128px><br/>" + QObject::tr("Oximetery<br/>Wizard")+"</font></td></tr>"
       "</table>"
      "</td>";
      if (havecpapdata || haveoximeterdata) {
          html += "<td align=center><font size=+2>"+QObject::tr("or")+"</font></td>"
          "<td align=center>"
          "<table class=curved cellpadding=4>"
          "<tr><td align=center onmouseover='ChangeColor(this, \"#eeeeee\");' onmouseout='ChangeColor(this, \"#ffffff\");' onclick='alert(\"statistics=1\");'><font size=+1><img src='qrc:/icons/statistics.png' width=128px><br/>" + QObject::tr("View<br/>Statistics")+"</font></td></tr>"
          "</table>"
          "</td>";
      }
     html += "</tr></table>";
    if (!havecpapdata && !haveoximeterdata) {
        html += "<p>" + QObject::tr("It might be a good idea to check preferences first,</br>as there are some options that affect import.")+"</p>"
        "<p>" + QObject::tr("First import can take a few minutes.") + "</p>";
    } else {
        QDate date = p_profile->LastDay(MT_CPAP);
        Day *day = p_profile->GetDay(date, MT_CPAP);

        if (havecpapdata && day) {
            QString cpapimage = "";

            Machine * cpap = day->machine(MT_CPAP);
            if (cpap) {
                cpapimage = "qrc"+cpap->getPixmapPath();
            }
            html += "<table cellpadding=4><tr><td><img src='"+cpapimage+"' width=160px><br/>";

            html+="</td><td align=center><table cellpadding=4 class=curved2 title=\""+QObject::tr("Click this box to see this in daily view.")+"\"><tr>"+
                    QString("<td align=center  onmouseover='ChangeColor(this, \"#efefa0\");' onmouseout='ChangeColor(this, \"#ffffc0\");' onclick='alert(\"daily=%1\");'>").arg(date.toString(Qt::ISODate))+"<b>"+
                    QObject::tr("The last time you used your %1...").arg(cpap->brand()+" "+cpap->model())+"</b><br/>";

            int daysto = date.daysTo(QDate::currentDate());
            QString daystring;
            if (daysto == 1) daystring += QObject::tr("last night");
            else if (daysto == 2) daystring += QObject::tr("yesterday");
            else daystring += QObject::tr("%2 days ago").arg(date.daysTo(QDate::currentDate()));

            html += QObject::tr("was %1 (on %2)").arg(daystring).arg(date.toString(Qt::SystemLocaleLongDate)) + "<br/>";


            EventDataType hours = day->hours();

            EventDataType ahi=(day->count(CPAP_Obstructive) + day->count(CPAP_Hypopnea) + day->count(CPAP_ClearAirway) + day->count(CPAP_Apnea)) / hours;

            QString ahitext;
            if (ahi < 0.000001) ahitext =  QString("<font color=green>")+QObject::tr("perfect :)")+"</font>";
            else if (ahi < 1) ahitext = QObject::tr("pretty darn good");
            else if (ahi < 3) ahitext = QObject::tr("reasonably good");
            else if (ahi < 5) ahitext = QObject::tr("technically \"treated\"");
            else if (ahi < 10) ahitext = QString("<font color=red>")+QObject::tr("not very good")+"</font>";
            else ahitext = QString("<font color=red>")+QObject::tr("horrible, please consult your doctor")+"</font>";

            html += QObject::tr("You had an AHI of %1, which is considered %2").arg(ahi,0,'f',2).arg(ahitext)+"<br/>";

            int seconds = int(hours * 3600.0) % 60;
            int minutes = int(hours * 60) % 60;
            int hour = hours;
            QString timestr = QObject::tr("%1 hours, %2 minutes and %3 seconds").arg(hour).arg(minutes).arg(seconds);

            if (hours > 4) html += QObject::tr("You machine was on for %1.").arg(timestr)+"<br/>";
            else html += QObject::tr("<font color = red>You only had the mask on for %1.</font>").arg(timestr)+"<br/>";


            EventDataType lat = day->timeAboveThreshold(CPAP_Leak, p_profile->cpap->leakRedline())/ 60.0;
            EventDataType leaks = 1.0/hours * lat;
            EventDataType leakmax = day->Max(CPAP_Leak);
            QString leaktext;
            if (leaks < 0.000001) leaktext=QObject::tr("You had no <i>major</i> mask leaks (maximum was %1 %2).").arg(leakmax,0,'f',2).arg(schema::channel[CPAP_Leak].units());
            else if (leaks < 0.01) leaktext=QObject::tr("You had a small but acceptable amount of <i>major</i> mask leakage.");
            else if (leaks < 0.10) leaktext=QObject::tr("You had significant periods of <i>major</i> mask leakage.");
            else leaktext=QObject::tr("Your mask is leaking way too much.. Talk to your CPAP advisor.");
            html += leaktext + "<br/>";

            CPAPMode cpapmode = (CPAPMode)(int)day->settings_max(CPAP_Mode);
            double perc= p_profile->general->prefCalcPercentile();

            if (cpapmode == MODE_CPAP) {
                EventDataType pressure = day->settings_max(CPAP_Pressure);
                html += QObject::tr("Your CPAP machine blasted you with a constant %1%2 of air").arg(pressure).arg(schema::channel[CPAP_Pressure].units());
            } else if (cpapmode == MODE_APAP) {
                EventDataType pressure = day->percentile(CPAP_Pressure, perc/100.0);
                html += QObject::tr("Your pressure was under %1%2 for %3% of the time.").arg(pressure).arg(schema::channel[CPAP_Pressure].units()).arg(perc);
            } else if (cpapmode == MODE_BILEVEL_FIXED) {
                EventDataType ipap = day->settings_max(CPAP_IPAP);
                EventDataType epap = day->settings_min(CPAP_EPAP);
                html += QObject::tr("Your machine blasted you with a constant %1-%2 %3 of air.").arg(epap).arg(ipap).arg(schema::channel[CPAP_Pressure].units());
            } else if (cpapmode == MODE_BILEVEL_AUTO_FIXED_PS) {
                EventDataType ipap = day->percentile(CPAP_IPAP, perc/100.0);
                EventDataType epap = day->percentile(CPAP_EPAP, perc/100.0);
                html += QObject::tr("Your machine was under %1-%2 %3 for %4% of the time.").arg(epap).arg(ipap).arg(schema::channel[CPAP_Pressure].units()).arg(perc);
            } else if (cpapmode == MODE_ASV){
                EventDataType ipap = day->percentile(CPAP_IPAP, perc/100.0);
                EventDataType epap = qRound(day->settings_wavg(CPAP_EPAP));

                html += QObject::tr("Your EPAP pressure fixed at %1%2.").arg(epap).arg(schema::channel[CPAP_EPAP].units())+"<br/>";
                html += QObject::tr("Your IPAP pressure was under %1%2 for %3% of the time.").arg(ipap).arg(schema::channel[CPAP_IPAP].units()).arg(perc);
            } else if (cpapmode == MODE_ASV_VARIABLE_EPAP){
                EventDataType ipap = day->percentile(CPAP_IPAP, perc/100.0);
                EventDataType epap = day->percentile(CPAP_EPAP, perc/100.0);

                html += QObject::tr("Your EPAP pressure was under %1%2 for %3% of the time.").arg(epap).arg(schema::channel[CPAP_EPAP].units()).arg(perc)+"<br/>";
                html += QObject::tr("Your IPAP pressure was under %1%2 for %3% of the time.").arg(ipap).arg(schema::channel[CPAP_IPAP].units()).arg(perc);
            }


            html+="</td></tr></table></td></tr></table>";

        } else {
            html += "<p>"+QObject::tr("No CPAP data has been imported yet.")+"</p>";
        }
        if (haveoximeterdata) {
            QDate oxidate=p_profile->LastDay(MT_OXIMETER);
            int daysto = oxidate.daysTo(QDate::currentDate());

            html += "<p>"+QObject::tr("Most recent Oximetery data: <a onclick='alert(\"daily=%2\");'>%1</a> ").arg(oxidate.toString(Qt::SystemLocaleLongDate)).arg(oxidate.toString(Qt::ISODate));
            if (daysto == 1) html += QObject::tr("(last night)");
            else if (daysto == 2) html += QObject::tr("(yesterday)");
            else html += QObject::tr("(%2 day ago)").arg(oxidate.daysTo(QDate::currentDate()));
            html+="</p>";
        } else {
            html += "<p>"+QObject::tr("No oximetery data has been imported yet.")+"</p>";
        }

    }
    html += QString("<div align=center><table class=curved cellpadding=3 width=45%>")+
    "<tr>"
    "<td align=center colspan=2><font size=+1><b>"+QObject::tr("Very Important Warning")+"</b></font></td></tr>"
    "<tr><td align=left>"+
    QObject::tr("<p>ALWAYS <font size=+1 color=red><b>write protect</b></font> CPAP SDCards before inserting them into your computer.")+"</p>"+
    QObject::tr("<p><span title=\"Mac OSX and Win8.1\"  onmouseover='ChangeColor(this, \"#eeeeee\");' onmouseout='ChangeColor(this, \"#ffffff\");'><font color=blue>Certain operating systems</font></span> write index files to the card without asking, which can render your card unreadable by your cpap machine.")+"</p>"+
    QObject::tr("<p>As a second line of protection, ALWAYS UNMOUNT the data card properly before removing it!</p>")+
    "</td>"
    "<td><img src=\"qrc:/icons/sdcard-lock.png\" width=128px></td>"
    "</tr>"
    "</table>"
    "</td></tr></table></div>"
    "<script type='text/javascript' language='javascript' src='qrc:/docs/script.js'></script>"
    "</body></html>";
    return html;
}

