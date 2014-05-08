/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gSummaryChart Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <math.h>
#include <QLabel>
#include <QDateTime>
#include "gYAxis.h"
#include "gSummaryChart.h"

extern QLabel *qstatus2;
SummaryChart::SummaryChart(QString label, GraphType type)
    : Layer(NoChannel), m_label(label), m_graphtype(type)
{
    m_empty = true;
    hl_day = -1;
    m_machinetype = MT_CPAP;

    QDateTime d1 = QDateTime::currentDateTime();
    QDateTime d2 = d1;
    d1.setTimeSpec(Qt::UTC);
    tz_offset = d2.secsTo(d1);
    tz_hours = tz_offset / 3600.0;

}
SummaryChart::~SummaryChart()
{
}
void SummaryChart::SetDay(Day *nullday)
{
    Day *day = nullday;
    Layer::SetDay(day);

    m_values.clear();
    m_times.clear();
    m_days.clear();
    m_hours.clear();
    m_goodcodes.clear();
    m_miny = 999999999.0F;
    m_maxy = -999999999.0F;
    m_physmaxy = 0;
    m_physminy = 0;
    m_minx = 0;
    m_maxx = 0;


    int dn;
    EventDataType tmp, tmp2, total;
    ChannelID code;
    CPAPMode cpapmode = (CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode, MT_CPAP,
                        PROFILE.FirstDay(MT_CPAP), PROFILE.LastDay(MT_CPAP));

    if (m_label == STR_TR_Pressure) {
        m_codes.clear();
        m_colors.clear();
        m_type.clear();
        m_typeval.clear();

        float perc = PROFILE.general->prefCalcPercentile() / 100.0;
        int mididx = PROFILE.general->prefCalcMiddle();
        SummaryType mid;

        if (mididx == 0) { mid = ST_PERC; }

        if (mididx == 1) { mid = ST_WAVG; }

        if (mididx == 2) { mid = ST_AVG; }

        if (cpapmode >= MODE_ASV) {
            addSlice(CPAP_EPAP, QColor("green"), ST_SETMIN);

            addSlice(CPAP_IPAPLo, QColor("light blue"), ST_SETMIN);
            addSlice(CPAP_IPAP, QColor("cyan"), mid, 0.5);
            addSlice(CPAP_IPAP, QColor("dark cyan"), ST_PERC, perc);
            //addSlice(CPAP_IPAP,QColor("light blue"),ST_PERC,0.95);
            addSlice(CPAP_IPAPHi, QColor("blue"), ST_SETMAX);
        } else if (cpapmode >= MODE_BIPAP) {
            addSlice(CPAP_EPAP, QColor("green"), ST_SETMIN);
            addSlice(CPAP_EPAP, QColor("light green"), ST_PERC, perc);
            addSlice(CPAP_IPAP, QColor("light cyan"), mid, 0.5);
            addSlice(CPAP_IPAP, QColor("light blue"), ST_PERC, perc);
            addSlice(CPAP_IPAP, QColor("blue"), ST_SETMAX);
        } else if (cpapmode >= MODE_APAP) {
            addSlice(CPAP_PressureMin, QColor("orange"), ST_SETMIN);
            addSlice(CPAP_Pressure, QColor("dark green"), mid, 0.5);
            addSlice(CPAP_Pressure, QColor("grey"), ST_PERC, perc);
            addSlice(CPAP_PressureMax, QColor("red"), ST_SETMAX);
        } else {
            addSlice(CPAP_Pressure, QColor("dark green"), ST_SETWAVG);
        }
    }

    m_goodcodes.resize(m_codes.size());

    for (int i = 0; i < m_codes.size(); i++) {
        m_goodcodes[i] = false;
    }

    m_fday = 0;
    qint64 tt, zt;
    m_empty = true;

    if (m_graphtype == GT_SESSIONS) {
        if (PROFILE.countDays(MT_CPAP, PROFILE.FirstDay(MT_CPAP), PROFILE.LastDay(MT_CPAP)) == 0) {
            return;
        }
    }

    int suboffset;
    SummaryType type;
    bool first = true;

    for (QMap<QDate, QList<Day *> >::iterator d = PROFILE.daylist.begin(); d != PROFILE.daylist.end();
            d++) {
        tt = QDateTime(d.key(), QTime(0, 0, 0), Qt::UTC).toTime_t();
        dn = tt / 86400;
        tt *= 1000L;

        if (!m_minx || tt < m_minx) { m_minx = tt; }

        if (!m_maxx || tt > m_maxx) { m_maxx = tt; }

        total = 0;
        bool fnd = false;

        if (m_graphtype == GT_SESSIONS) {
            for (int i = 0; i < m_codes.size(); i++) {
                m_goodcodes[i] = true;
            }

            for (int i = 0; i < d.value().size(); i++) { // for each day
                day = d.value().at(i);

                if (!day) { continue; }

                if (day->machine_type() != m_machinetype) { continue; }

                int ft = qint64(day->first()) / 1000L;
                ft += tz_offset; // convert to local time

                int dz2 = ft / 86400;
                dz2 *= 86400; // ft = first sessions time, rounded back to midnight..

                for (int s = 0; s < day->size(); s++) {
                    Session *sess = (*day)[s];

                    if (!sess->enabled()) { continue; }

                    tmp = sess->hours();
                    m_values[dn][s] = tmp;
                    total += tmp;
                    zt = qint64(sess->first()) / 1000L;
                    zt += tz_offset;
                    tmp2 = zt - dn * 86400;
                    tmp2 /= 3600.0;

                    m_times[dn][s] = tmp2;

                    if (first) {
                        m_miny = tmp2;
                        m_maxy = tmp2 + tmp;
                        first = false;
                    } else {
                        if (tmp2 < m_miny) {
                            m_miny = tmp2;
                        }

                        if (tmp2 + tmp > m_maxy) {
                            m_maxy = tmp2 + tmp;
                        }
                    }
                }

                if (total > 0) {
                    m_days[dn] = day;
                    m_hours[dn] = total;
                    m_empty = false;
                }
            }
        } else {
            for (int j = 0; j < m_codes.size(); j++) { // for each code slice
                code = m_codes[j];
                suboffset = 0;
                type = m_type[j];
                EventDataType typeval = m_typeval[j];

                for (int i = 0; i < d.value().size(); i++) { // for each machine object for this day
                    day = d.value()[i];
                    CPAPMode mode = (CPAPMode)(int)day->settings_max(CPAP_Mode);

                    if (day->machine_type() != m_machinetype) { continue; }

                    bool hascode = //day->channelHasData(code) ||
                        type == ST_HOURS ||
                        type == ST_SESSIONS ||
                        day->settingExists(code) ||
                        day->hasData(code, type);

                    if (code == CPAP_Pressure) {
                        if ((cpapmode > MODE_CPAP) && (mode == MODE_CPAP)) {
                            hascode = false;

                            if ((type == ST_WAVG) || (type == ST_AVG) || ((type == ST_PERC) && (typeval == 0.5))) {
                                type = ST_SETWAVG;
                                hascode = true;
                            }
                        } else {
                            type = m_type[j];
                        }
                    }

                    //if (code==CPAP_Hypopnea) { // Make sure at least one of the CPAP data gets through with 0
                    //    hascode=true;
                    //}
                    if (hascode) {
                        m_days[dn] = day;

                        switch (type) {
                        case ST_AVG:
                            tmp = day->avg(code);
                            break;

                        case ST_SUM:
                            tmp = day->sum(code);
                            break;

                        case ST_WAVG:
                            tmp = day->wavg(code);
                            break;

                        case ST_90P:
                            tmp = day->p90(code);
                            break;

                        case ST_PERC:
                            tmp = day->percentile(code, typeval);
                            break;

                        case ST_MIN:
                            tmp = day->Min(code);
                            break;

                        case ST_MAX:
                            tmp = day->Max(code);
                            break;

                        case ST_CNT:
                            tmp = day->count(code);
                            break;

                        case ST_CPH:
                            tmp = day->cph(code);
                            break;

                        case ST_SPH:
                            tmp = day->sph(code);
                            break;

                        case ST_HOURS:
                            tmp = day->hours();
                            break;

                        case ST_SESSIONS:
                            tmp = day->size();
                            break;

                        case ST_SETMIN:
                            tmp = day->settings_min(code);
                            break;

                        case ST_SETMAX:
                            tmp = day->settings_max(code);
                            break;

                        case ST_SETAVG:
                            tmp = day->settings_avg(code);
                            break;

                        case ST_SETWAVG:
                            tmp = day->settings_wavg(code);
                            break;

                        case ST_SETSUM:
                            tmp = day->settings_sum(code);
                            break;

                        default:
                            tmp = 0;
                            break;
                        }

                        if (suboffset > 0) {
                            tmp -= suboffset;

                            if (tmp < 0) { tmp = 0; }
                        }

                        total += tmp;
                        m_values[dn][j + 1] = tmp;

                        if (tmp < m_miny) { m_miny = tmp; }

                        if (tmp > m_maxy) { m_maxy = tmp; }

                        m_goodcodes[j] = true;
                        fnd = true;
                        break;
                    }
                }
            }

            if (fnd) {
                if (!m_fday) { m_fday = dn; }

                m_values[dn][0] = total;
                m_hours[dn] = day->hours();

                if (m_graphtype == GT_BAR) {
                    if (total < m_miny) { m_miny = total; }

                    if (total > m_maxy) { m_maxy = total; }
                }
            }
        }
    }

    m_empty = true;

    for (int i = 0; i < m_goodcodes.size(); i++) {
        if (m_goodcodes[i]) {
            m_empty = false;
            break;
        }
    }

    if (m_graphtype == GT_BAR) {
        m_miny = 0;
    }

    // m_minx=qint64(QDateTime(PROFILE.FirstDay(),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    m_maxx = qint64(QDateTime(PROFILE.LastDay(), QTime(23, 59, 0), Qt::UTC).toTime_t()) * 1000L;
    m_physmaxy = m_maxy;
    m_physminy = m_miny;
}

QColor brighten(QColor color)
{
    int cr, cg, cb;

    cr = color.red();
    cg = color.green();
    cb = color.blue();

    if (cr < 64) { cr = 64; }

    if (cg < 64) { cg = 64; }

    if (cb < 64) { cb = 64; }

    cr *= 2;
    cg *= 2;
    cb *= 2;

    if (cr > 255) { cr = 255; }

    if (cg > 255) { cg = 255; }

    if (cb > 255) { cb = 255; }

    return QColor(cr, cg, cb, 255);

}

void SummaryChart::paint(QPainter &painter, gGraph &w, int left, int top, int width, int height)
{
    if (!m_visible) { return; }

    GraphType graphtype = m_graphtype;

    if (graphtype == GT_LINE || graphtype == GT_POINTS) {
        bool pts = PROFILE.appearance->overviewLinechartMode() == OLC_Lines;
        graphtype = pts ? GT_POINTS : GT_LINE;
    }

    rtop = top;

    painter.setPen(QColor(Qt::black));
    painter.drawLine(left, top, left, top+height);
    painter.drawLine(left, top+height, left+width, top+height);
    painter.drawLine(left+width, top+height, left+width, top);
    painter.drawLine( left+width, top, left, top);

    qint64 minx = w.min_x, maxx = w.max_x;
    qint64 xx = maxx - minx;
    float days = double(xx) / 86400000.0;

    EventDataType maxy;
    EventDataType miny;

    if (w.zoomY() == 0 && PROFILE.appearance->allowYAxisScaling()) {
        maxy = m_physmaxy;
        miny = m_physminy;
        w.roundY(miny, maxy);
    } else {
        maxy = m_maxy;
        miny = m_miny;
        w.roundY(miny, maxy);
    }

    EventDataType yy = maxy - miny;
    EventDataType ymult = float(height - 2) / yy;

    barw = (float(width) / float(days));

    graph = &w;
    float px = left;
    l_left = w.marginLeft() + gYAxis::Margin;
    l_top = w.marginTop();
    l_width = width;
    l_height = height;
    float py;
    EventDataType total;

    int daynum = 0;
    EventDataType h, tmp, tmp2;


    l_offset = (minx) % 86400000L;
    offset = float(l_offset) / 86400000.0;

    offset *= barw;
    px = left - offset;
    l_minx = minx;
    l_maxx = maxx + 86400000L;

    int total_days = 0;
    double total_val = 0;
    double total_hours = 0;
    bool lastdaygood = false;
    QVector<double> totalcounts;
    QVector<double> totalvalues;
    QVector<float> lastX;
    QVector<short> lastY;
    int numcodes = m_codes.size();
    totalcounts.resize(numcodes);
    totalvalues.resize(numcodes);
    lastX.resize(numcodes);
    lastY.resize(numcodes);
    int zd = minx / 86400000L;
    zd--;
    QHash<int, QHash<short, EventDataType> >::iterator d = m_values.find(zd);

    QVector<bool> goodcodes;
    goodcodes.resize(m_goodcodes.size());

    lastdaygood = true;

    for (int i = 0; i < numcodes; i++) {
        totalcounts[i] = 0;

        // Set min and max to the opposite largest starting value
        if ((m_type[i] == ST_MIN) || (m_type[i] == ST_SETMIN)) {
            totalvalues[i] = maxy;
        } else if ((m_type[i] == ST_MAX) || (m_type[i] == ST_SETMAX)) {
            totalvalues[i] = miny;
        } else {
            totalvalues[i] = 0;
        }

        // Turn off legend display.. It will only display if it's turned back on during draw.
        goodcodes[i] = false;

        if (!m_goodcodes[i]) { continue; }

        lastX[i] = px;

        if (d != m_values.end() && d.value().contains(i + 1)) {

            tmp = d.value()[i + 1];
            h = tmp * ymult;
        } else {
            lastdaygood = false;
            h = 0;
        }

        lastY[i] = top + height - 1 - h;
    }

    float compliance_hours = 0;

    if (PROFILE.cpap->showComplianceInfo()) {
        compliance_hours = PROFILE.cpap->complianceHours();
    }

    int incompliant = 0;
    Day *day;
    EventDataType hours;

    //quint32 * tptr;
    //EventStoreType * dptr;

    short px2, py2;
    const qint64 ms_per_day = 86400000L;


    painter.setClipRect(left, top, width, height);
    painter.setClipping(true);

    for (qint64 Q = minx; Q <= maxx + ms_per_day; Q += ms_per_day) {
        zd = Q / ms_per_day;
        d = m_values.find(zd);

        if (Q < minx) {
            goto jumpnext;
        }

        if (d != m_values.end()) {
            day = m_days[zd];

            if (!m_hours.contains(zd)) {
                goto jumpnext;
            }

            hours = m_hours[zd];

            int x1 = px;
            //x1-=(barw/2.0);
            int x2 = px + barw;

            //if (x1 < left) { x1 = left; }

            if (x2 > left + width) { x2 = left + width; }

            //  if (x2<x1)
            //      goto jumpnext;


            if (zd == hl_day) {
                QColor col = QColor("red");
                col.setAlpha(64);

                if (graphtype != GT_POINTS) {
                    painter.fillRect(x1-1, top, barw, height, QBrush(col));
//                    quads->add(x1 - 1, top, x1 - 1, top + height, x2, top + height, x2, top, col.rgba());
                } else {
                    painter.fillRect((x1+barw/2)-5, top, barw, height, QBrush(col));
//                    quads->add((x1 + barw / 2) - 5, top, (x1 + barw / 2) - 5, top + height, (x2 - barw / 2) + 5,
//                               top + height, (x2 - barw / 2) + 5, top, col.rgba());
                }
            }

            if (graphtype == GT_SESSIONS) {
                int j;
                QHash<int, QHash<short, EventDataType> >::iterator times = m_times.find(zd);
                QColor col = m_colors[0];
                //if (hours<compliance_hours) col=QColor("#f03030");

                if (zd == hl_day) {
                    col = QColor("gold");
                }

                QColor col1 = col;
                QColor col2 = brighten(col);
                //outlines->setColor(Qt::black);

                int np = d.value().size();

                if (np > 0) {
                    for (int i = 0; i < goodcodes.size(); i++) {
                        goodcodes[i] = true;
                    }
                }

                for (j = 0; j < np; j++) {
                    tmp2 = times.value()[j] - miny;
                    py = top + height - (tmp2 * ymult);

                    tmp = d.value()[j]; // length

                    //tmp-=miny;
                    h = tmp * ymult;

                    QLinearGradient gradient(QPoint(0,0),QPoint(barw,0));
                    gradient.setColorAt(0,col1);
                    gradient.setColorAt(1,col2);
                    painter.fillRect(x1, py-h, barw, h, QBrush(gradient));
//                    quads->add(x1, py, x1, py - h, x2, py - h, x2, py, col1, col2);

                    if ((h > 0) && (barw > 2)) {
                        painter.setPen(QColor(Qt::black));
                        painter.drawLine(x1, py, x1, py - h);
                        painter.drawLine(x1, py - h, x2, py - h);
                        painter.drawLine(x1, py, x2, py);
                        painter.drawLine(x2, py, x2, py - h);
                    }

                    totalvalues[0] += hours * tmp;
                }

                totalcounts[0] += hours;
                totalvalues[1] += j;
                totalcounts[1]++;
                total_val += hours;
                total_hours += hours;
                total_days++;
            } else {
                if (!d.value().contains(0)) {
                    goto jumpnext;
                }

                total = d.value()[0];

                //if (total>0) {
                if (day) {
                    EventDataType hours = m_hours[zd];
                    total_val += total * hours;
                    total_hours += hours;
                    total_days++;
                }

                py = top + height;

                //}
                bool good;
                SummaryType type;

                for (QHash<short, EventDataType>::iterator g = d.value().begin(); g != d.value().end(); g++) {
                    short j = g.key();

                    if (!j) { continue; }

                    j--;
                    good = m_goodcodes[j];

                    if (!good) {
                        continue;
                    }

                    type = m_type[j];
                    // code was actually used (to signal the display of the legend summary)
                    goodcodes[j] = good;

                    tmp = g.value();

                    QColor col = m_colors[j];

                    if (type == ST_HOURS) {
                        if (tmp < compliance_hours) {
                            col = QColor("#f04040");
                            incompliant++;
                        }
                    }

                    if (zd == hl_day) {
                        col = QColor("gold");
                    }

                    //if (!tmp) continue;
                    if ((type == ST_MAX) || (type == ST_SETMAX)) {
                        if (totalvalues[j] < tmp) {
                            totalvalues[j] = tmp;
                        }
                    } else if ((type == ST_MIN) || (type == ST_SETMIN)) {
                        if (totalvalues[j] > tmp) {
                            totalvalues[j] = tmp;
                        }
                    } else {
                        totalvalues[j] += tmp * hours;
                    }

                    //if (tmp) {
                    totalcounts[j] += hours;
                    //}
                    tmp -= miny;
                    h = tmp * ymult; // height in pixels

                    if (graphtype == GT_BAR) {
                        QColor col1 = col;
                        QColor col2 = brighten(col);

                        QLinearGradient gradient(0,0,barw,h);
                        gradient.setColorAt(0,col1);
                        gradient.setColorAt(1,col2);
                        painter.fillRect(x1, py-h, barw, h, QBrush(gradient));

//                        quads->add(x1, py, x1, py - h, col1);
//                        quads->add(x2, py - h, x2, py, col2);

                        if (h > 0 && barw > 2) {
                            painter.setPen(QColor(Qt::black));
                            painter.drawLine(x1, py, x1, py - h);
                            painter.drawLine(x1, py - h, x2, py - h);
                            painter.drawLine(x1, py, x2, py);
                            painter.drawLine(x2, py, x2, py - h);
                        } // if (bar

                        py -= h;
                    } else if (graphtype == GT_LINE) { // if (m_graphtype==GT_BAR
                        GLuint col1 = col.rgba();
                        GLuint col2 = m_colors[j].rgba();
                        px2 = px + barw;
                        py2 = (top + height - 1) - h;

                        // If more than 1 day between records, skip the vertical crud.
                        if ((px2 - lastX[j]) > barw + 1) {
                            lastdaygood = false;
                        }

                        if (lastdaygood) {
                            if (lastY[j] != py2) { // vertical line
                                painter.setPen(col2);
                                painter.drawLine(lastX[j], lastY[j], px, py2);
                            }

                            painter.setPen(col1);
                            painter.drawLine(px - 1, py2, px2 + 1, py2);
                        } else {
                            painter.setPen(col1);
                            painter.drawLine(x1 - 1, py2, x2 + 1, py2);
                        }

                        lastX[j] = px2;
                        lastY[j] = py2;
                    } else if (graphtype == GT_POINTS) {
                        QColor col1 = col;
                        QColor  col2 = m_colors[j];
                        px2 = px + barw;
                        py2 = (top + height - 2) - h;

                        // If more than 1 day between records, skip the vertical crud.
                        if ((px2 - lastX[j]) > barw + 1) {
                            lastdaygood = false;
                        }

                        if (zd == hl_day) {
                            painter.setPen(QPen(col2,10));
                            painter.drawPoint(px2 - barw / 2, py2);
                        }

                        if (lastdaygood) {
                            painter.setPen(QPen(col2,1));
                            painter.drawLine(lastX[j] - barw / 2, lastY[j], px2 - barw / 2, py2);
                        } else {
                            painter.setPen(QPen(col1,1));
                            painter.drawLine(px + barw / 2 - 1, py2, px + barw / 2 + 1, py2);
                        }

                        lastX[j] = px2;
                        lastY[j] = py2;
                    }
                }  // for(QHash<short
            }

            lastdaygood = true;
            //    if (Q>maxx+extra) break;
        } else {
            if (Q < maxx) {
                incompliant++;
            }

            lastdaygood = false;
        }

jumpnext:

        if (px >= left + width + barw) {
            break;
        }

        px += barw;

        daynum++;
        //lastQ=Q;
    }
    painter.setClipping(false);

//#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
//    quads->scissor(left * dpr, w.flipY(top + height + 2)*dpr, (width)*dpr, (height + 1)*dpr);
//    lines->scissor(left * dpr, w.flipY(top + height + 2)*dpr, (width + 1)*dpr, (height + 1)*dpr);
//    outlines->scissor(left * dpr, w.flipY(top + height + 2)*dpr, (width)*dpr, (height + 1)*dpr);
//#else
//    lines->scissor(left, w.flipY(top + height + 2), width + 1, height + 2);
//    outlines->scissor(left, w.flipY(top + height + 2), width, height + 2);
//    quads->scissor(left, w.flipY(top + height + 2), width, height + 2);
//#endif
    // Draw Ledgend
    px = left + width - 3;
    py = top - 5;
    int legendx = px;
    QString a, b;
    int x, y;

    QFontMetrics fm(*defaultfont);
    int bw = fm.width('X');
    int bh = fm.height() / 1.8;

    bool ishours = false;
    int good = 0;

    for (int j = 0; j < m_codes.size(); j++) {
        if (!goodcodes[j]) { continue; }

        good++;
        SummaryType type = m_type[j];
        ChannelID code = m_codes[j];
        EventDataType tval = m_typeval[j];

        switch (type) {
        case ST_WAVG:
            b = "Avg";
            break;

        case ST_AVG:
            b = "Avg";
            break;

        case ST_90P:
            b = "90%";
            break;

        case ST_PERC:
            if (tval >= 0.99) { b = "Max"; }
            else if (tval == 0.5) { b = "Med"; }
            else { b = QString("%1%").arg(tval * 100.0, 0, 'f', 0); }

            break;

        //b=QString("%1%").arg(tval*100.0,0,'f',0); break;
        case ST_MIN:
            b = "Min";
            break;

        case ST_MAX:
            b = "Max";
            break;

        case ST_SETMIN:
            b = "Min";
            break;

        case ST_SETMAX:
            b = "Max";
            break;

        case ST_CPH:
            b = "";
            break;

        case ST_SPH:
            b = "%";
            break;

        case ST_HOURS:
            b = STR_UNIT_Hours;
            break;

        case ST_SESSIONS:
            b = "Sessions";
            break;

        default:
            b = "";
            break;
        }

        a = schema::channel[code].label();

        if (a == w.title() && !b.isEmpty()) { a = b; }
        else { a += " " + b; }

        QString val;
        float f = 0;

        if (totalcounts[j] > 0) {
            if ((type == ST_MIN) || (type == ST_MAX) || (type == ST_SETMIN) || (type == ST_SETMAX)) {
                f = totalvalues[j];
            } else {
                f = totalvalues[j] / totalcounts[j];
            }
        }

        if (type == ST_HOURS) {
            int h = f;
            int m = int(f * 60) % 60;
            val.sprintf("%02i:%02i", h, m);
            ishours = true;
        } else {
            val = QString::number(f, 'f', 2);
        }

        a += "=" + val;
        //GetTextExtent(a,x,y);
        //float wt=20*w.printScaleX();
        //px-=wt+x;
        //w.renderText(a,px+wt,py+1);
        //quads->add(px+wt-y/4-y,py-y,px+wt-y/4,py-y,px+wt-y/4,py+1,px+wt-y/4-y,py+1,m_colors[j].rgba());


        //QString text=schema::channel[code].label();

        int wid, hi;
        GetTextExtent(a, wid, hi);
        legendx -= wid;
        w.renderText(a, legendx, top - 4);
        //  legendx-=bw/2;

        painter.fillRect(legendx - bw, top-w.marginTop()-1, bh, w.marginTop(), QBrush(m_colors[j]));
        legendx -= bw * 2;


        //lines->add(px,py,px+20,py,m_colors[j]);
        //lines->add(px,py+1,px+20,py+1,m_colors[j]);
    }

    if ((m_graphtype == GT_BAR) && (good > 0)) {

        if (m_type.size() > 1) {
            float val = total_val / float(total_hours);
            a = m_label + "=" + QString::number(val, 'f', 2) + " ";
            GetTextExtent(a, x, y);
            legendx -= x;
            w.renderText(a, legendx, py + 1);
        }
    }

    a = "";
    /*if (m_graphtype==GT_BAR) {
        if (m_type.size()>1) {
            float val=total_val/float(total_days);
            a+=m_label+"="+QString::number(val,'f',2)+" ";
            //
        }
    }*/
    a += "Days=" + QString::number(total_days, 'f', 0);

    if (PROFILE.cpap->showComplianceInfo()) {
        if (ishours && incompliant > 0) {
            a += " Low Usage Days=" + QString::number(incompliant, 'f',
                    0) + " (%" + QString::number((1.0 / daynum) * (total_days - incompliant) * 100.0, 'f',
                                                 2) + " compliant, defined as >" + QString::number(compliance_hours, 'f', 1) + " hours)";
        }
    }


    //GetTextExtent(a,x,y);
    //legendx-=30+x;
    //w.renderText(a,px+24,py+5);
    w.renderText(a, left, py + 1);
}

QString formatTime(EventDataType v, bool show_seconds = false, bool duration = false,
                   bool show_12hr = false)
{
    int h = int(v);

    if (!duration) {
        h %= 24;
    } else { show_12hr = false; }

    int m = int(v * 60) % 60;
    int s = int(v * 3600) % 60;

    char pm[3] = {"am"};

    if (show_12hr) {
        h >= 12 ? pm[0] = 'p' : pm[0] = 'a';
        h %= 12;

        if (h == 0) { h = 12; }

    } else {
        pm[0] = 0;
    }

    if (show_seconds) {
        return QString().sprintf("%i:%02i:%02i%s", h, m, s, pm);
    } else {
        return QString().sprintf("%i:%02i%s", h, m, pm);
    }
}

bool SummaryChart::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    int x = event->x();
    int y = event->y();

    if (!m_rect.contains(x, y)) {
        //    if ((x<0 || y<0 || x>l_width || y>l_height)) {
        hl_day = -1;
        //graph->timedRedraw(2000);
        return false;
    }

    x -= m_rect.left();
    y -= m_rect.top();

    double xx = l_maxx - l_minx;

    double xmult = xx / double(l_width + barw);

    qint64 mx = ceil(xmult * double(x - offset));
    mx += l_minx;
    mx = mx + l_offset; //-86400000L;
    int zd = mx / 86400000L;

    Day *day;
    //if (hl_day!=zd)   // This line is an optimization

    {
        hl_day = zd;
        graph->Trigger(2000);

        QHash<int, QHash<short, EventDataType> >::iterator d = m_values.find(hl_day);

        QHash<short, EventDataType> &valhash = d.value();

        x += m_rect.left(); //gYAxis::Margin+gGraphView::titleWidth; //graph->m_marginleft+
        int y = event->y() - m_rect.top() + rtop - 15;
        //QDateTime dt1=QDateTime::fromTime_t(hl_day*86400).toLocalTime();
        QDateTime dt2 = QDateTime::fromTime_t(hl_day * 86400).toUTC();

        //QTime t1=dt1.time();
        //QTime t2=dt2.time();

        QDate dt = dt2.date();

        if (d != m_values.end()) {

            day = m_days[zd];

            QString z = dt.toString(Qt::SystemLocaleShortDate);

            // Day * day=m_days[hl_day];
            //EventDataType val;
            QString val;

            if (m_graphtype == GT_SESSIONS) {
                if (m_type[0] == ST_HOURS) {

                    int t = m_hours[zd] * 3600.0;
                    int h = t / 3600;
                    int m = (t / 60) % 60;
                    //int s=t % 60;
                    val.sprintf("%02i:%02i", h, m);
                } else {
                    val = QString::number(d.value()[0], 'f', 2);
                }

                z += "\r\n" + m_label + "=" + val;

                if (m_type[1] == ST_SESSIONS) {
                    z += " (Sess=" + QString::number(day->size(), 'f', 0) + ")";
                }

                EventDataType v = m_times[zd][0];
                int lastt = m_times[zd].size() - 1;

                if (lastt < 0) { lastt = 0; }

                z += "\r\nBedtime=" + formatTime(v, false, false, true);
                v = m_times[zd][lastt] + m_values[zd][lastt];
                z += "\r\nWaketime=" + formatTime(v, false, false, true);

            } else if (m_graphtype == GT_BAR) {
                if (m_type[0] == ST_HOURS) {
                    int t = d.value()[0] * 3600.0;
                    int h = t / 3600;
                    int m = (t / 60) % 60;
                    //int s=t % 60;
                    val.sprintf("%02i:%02i", h, m);
                } else {
                    val = QString::number(d.value()[0], 'f', 2);
                }

                z += "\r\n" + m_label + "=" + val;
                //z+="\r\nMode="+QString::number(day->settings_min("FlexSet"),'f',0);

            } else {
                QString a;

                for (int i = 0; i < m_type.size(); i++) {
                    if (!m_goodcodes[i]) {
                        continue;
                    }

                    if (!valhash.contains(i + 1)) {
                        continue;
                    }

                    EventDataType tval = m_typeval[i];

                    switch (m_type[i]) {
                    case ST_WAVG:
                        a = "W-avg";
                        break;

                    case ST_AVG:
                        a = "Avg";
                        break;

                    case ST_90P:
                        a = "90%";
                        break;

                    case ST_PERC:
                        if (tval >= 0.99) { a = "Max"; }
                        else if (tval == 0.5) { a = "Med"; }
                        else { a = QString("%1%").arg(tval * 100.0, 0, 'f', 0); }

                        break;

                    case ST_MIN:
                        a = "Min";
                        break;

                    case ST_MAX:
                        a = "Max";
                        break;

                    case ST_CPH:
                        a = "";
                        break;

                    case ST_SPH:
                        a = "%";
                        break;

                    case ST_HOURS:
                        a = STR_UNIT_Hours;
                        break;

                    case ST_SESSIONS:
                        a = "Sessions";
                        break;

                    case ST_SETMIN:
                        a = "Min";
                        break;

                    case ST_SETMAX:
                        a = "Max";
                        break;

                    default:
                        a = "";
                        break;
                    }

                    if (m_type[i] == ST_SESSIONS) {
                        val = QString::number(d.value()[i + 1], 'f', 0);
                        z += "\r\n" + a + "=" + val;
                    } else {
                        //if (day && (day->channelExists(m_codes[i]) || day->settingExists(m_codes[i]))) {
                        schema::Channel &chan = schema::channel[m_codes[i]];
                        EventDataType v;

                        if (valhash.contains(i + 1)) {
                            v = valhash[i + 1];
                        } else { v = 0; }

                        if (m_codes[i] == Journal_Weight) {
                            val = weightString(v, PROFILE.general->unitSystem());
                        } else {
                            val = QString::number(v, 'f', 2);
                        }

                        z += "\r\n" + chan.label() + " " + a + "=" + val;
                        //}
                    }
                }

            }

            graph->ToolTip(z, x, y - 15, p_profile->general->tooltipTimeout());
            return true;
        } else {
            QString z = dt.toString(Qt::SystemLocaleShortDate) + "\r\nNo Data";
            graph->ToolTip(z, x, y - 15, p_profile->general->tooltipTimeout());
            return true;
        }
    }


    return false;
}

bool SummaryChart::mousePressEvent(QMouseEvent *event, gGraph *graph)
{
    if (event->modifiers() & Qt::ShiftModifier) {
        //qDebug() << "Jump to daily view?";
        return true;
    }

    Q_UNUSED(graph)
    return false;
}

bool SummaryChart::keyPressEvent(QKeyEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    //qDebug() << "Summarychart Keypress";
    return false;
}

#include "mainwindow.h"
extern MainWindow *mainwin;
bool SummaryChart::mouseReleaseEvent(QMouseEvent *event, gGraph *graph)
{
    if (event->modifiers() & Qt::ShiftModifier) {
        if (hl_day < 0) {
            mouseMoveEvent(event, graph);
        }

        if (hl_day > 0) {
            QDateTime d = QDateTime::fromTime_t(hl_day * 86400).toUTC();
            mainwin->getDaily()->LoadDate(d.date());
            mainwin->JumpDaily();
            //qDebug() << "Jump to daily view?" << d;
            return true;
        }
    }

    Q_UNUSED(event)
    hl_day = -1;
    graph->timedRedraw(2000);
    return false;
}

