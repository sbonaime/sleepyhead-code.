/* gSessionTimesChart Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <math.h>
#include <QLabel>
#include <QDateTime>

#include "mainwindow.h"
#include "SleepLib/profiles.h"
#include "gSessionTimesChart.h"

#include "gYAxis.h"

extern MainWindow * mainwin;
QColor brighten(QColor color, float mult = 2.0);

gSummaryChart::gSummaryChart(QString label, MachineType machtype)
    :Layer(NoChannel), m_label(label), m_machtype(machtype)
{
    m_layertype = LT_Overview;
    QDateTime d1 = QDateTime::currentDateTime();
    QDateTime d2 = d1;
    d1.setTimeSpec(Qt::UTC);  // CHECK: Does this deal with DST?
    tz_offset = d2.secsTo(d1);
    tz_hours = tz_offset / 3600.0;
    expected_slices = 5;
}

gSummaryChart::gSummaryChart(ChannelID code, MachineType machtype)
    :Layer(code), m_machtype(machtype)
{
    m_layertype = LT_Overview;
    QDateTime d1 = QDateTime::currentDateTime();
    QDateTime d2 = d1;
    d1.setTimeSpec(Qt::UTC);  // CHECK: Does this deal with DST?
    tz_offset = d2.secsTo(d1);
    tz_hours = tz_offset / 3600.0;
    expected_slices = 5;

    addCalc(code, ST_MIN);
    addCalc(code, ST_MID);
    addCalc(code, ST_90P);
    addCalc(code, ST_MAX);
}

gSummaryChart::~gSummaryChart()
{
}

void gSummaryChart::SetDay(Day *unused_day)
{
    cache.clear();

    Q_UNUSED(unused_day)
    Layer::SetDay(nullptr);

    firstday = p_profile->FirstDay(m_machtype);
    lastday = p_profile->LastDay(m_machtype);


    dayindex.clear();
    daylist.clear();
    if (!firstday.isValid() || !lastday.isValid()) return;
   // daylist.reserve(firstday.daysTo(lastday)+1);
    QDate date = firstday;
    int idx = 0;
    do {
        QMap<QDate, Day *>::iterator di = p_profile->daylist.find(date);
        Day * day = nullptr;
        if (di != p_profile->daylist.end()) {
            day = di.value();
            if (!day->hasMachine(m_machtype)) day = nullptr;
        }
        daylist.append(day);
        dayindex[date] = idx;
        idx++;
        date = date.addDays(1);
    } while (date <= lastday);

    m_minx = QDateTime(firstday, QTime(0,0,0)).toMSecsSinceEpoch();
    m_maxx = QDateTime(lastday, QTime(23,59,59)).toMSecsSinceEpoch();
    m_miny = 0;
    m_maxy = 20;

    m_empty = false;

}

bool gSummaryChart::keyPressEvent(QKeyEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return false;
}

bool gSummaryChart::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return false;
}

bool gSummaryChart::mousePressEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return false;
}

bool gSummaryChart::mouseReleaseEvent(QMouseEvent *event, gGraph *graph)
{
    if (!(event->modifiers() & Qt::ShiftModifier)) return false;

    float x = event->x() - m_rect.left();
    float y = event->y() - m_rect.top();
    qDebug() << x << y;

    EventDataType miny;
    EventDataType maxy;

    graph->roundY(miny, maxy);

    QDate date = QDateTime::fromMSecsSinceEpoch(m_minx).date();

    int days = ceil(double(m_maxx - m_minx) / 86400000.0);

    float barw = float(m_rect.width()) / float(days);

    float idx = x/barw;

    date = date.addDays(idx);

    QMap<QDate, int>::iterator it = dayindex.find(date);
    if (it != dayindex.end()) {
        Day * day = daylist.at(it.value());
        if (day) {
            mainwin->getDaily()->LoadDate(date);
            mainwin->JumpDaily();
        }
    }

    return true;
}

QMap<QDate, int> gSummaryChart::dayindex;
QList<Day *> gSummaryChart::daylist;

QString gSummaryChart::tooltipData(Day *, int idx)
{
    QList<SummaryChartSlice> & slices = cache[idx];
    QString txt;
    for (int i=0; i< slices.size(); ++i) {
        SummaryChartSlice & slice = slices[i];
        txt += QString("\n%1: %2").arg(slice.name).arg(float(slice.value), 0, 'f', 2);

    }
    return txt;
}

void gSummaryChart::populate(Day * day, int idx)
{
    int size = calcitems.size();
    bool good = false;
    for (int i=0; i < size; ++i) {
        const SummaryCalcItem & item = calcitems.at(i);
        if (day->hasData(item.code, item.type)) {
            good = true;
            break;
        }
    }
    if (!good) return;

    QList<SummaryChartSlice> & slices = cache[idx];

    float hours = day->hours(m_machtype);
    float base = 0;
    for (int i=0; i < size; ++i) {
        const SummaryCalcItem & item = calcitems.at(i);
        ChannelID code = item.code;
        schema::Channel & chan = schema::channel[code];
        float value = 0;
        QString name;
        QColor color;
        switch (item.type) {
        case ST_CPH:
            value = day->count(code) / hours;
            name = chan.label();
            color = chan.defaultColor();
            slices.append(SummaryChartSlice(code, value, value, name, color));
            break;
        case ST_SPH:
            value = (100.0 / hours) * (day->sum(code) / 3600.0);
            name = QObject::tr("% in %1").arg(chan.label());
            color = chan.defaultColor();
            slices.append(SummaryChartSlice(code, value, value, name, color));
            break;
        case ST_HOURS:
            value = hours;
            name = QObject::tr("Hours");
            color = COLOR_LightBlue;
            slices.append(SummaryChartSlice(code, hours, hours, name, color));
            break;
        case ST_MIN:
            value = day->Min(code);
            name = QObject::tr("Min %1").arg(chan.label());
            color = brighten(chan.defaultColor(),0.60);
            slices.append(SummaryChartSlice(code, value, value - base, name, color));
            base = value;
            break;
        case ST_MID:
            value = day->calcMiddle(code);
            name = day->calcMiddleLabel(code);
            color = brighten(chan.defaultColor(),1.25);
            slices.append(SummaryChartSlice(code, value, value - base, name, color));
            base = value;
            break;
        case ST_90P:
            value = day->calcPercentile(code);
            name = day->calcPercentileLabel(code);
            color = brighten(chan.defaultColor(),1.50);
            slices.append(SummaryChartSlice(code, value, value - base, name, color));
            base = value;
            break;
        case ST_MAX:
            value = day->calcMax(code);
            name = day->calcMaxLabel(code);
            color = brighten(chan.defaultColor(),2);
            slices.append(SummaryChartSlice(code, value, value - base, name, color));
            base = value;
            break;
        default:
            break;
        }
    }
}

void gSummaryChart::paint(QPainter &painter, gGraph &graph, const QRegion &region)
{
    QRect rect = region.boundingRect();

    painter.setPen(QColor(Qt::black));
    painter.drawRect(rect);

    rect.moveBottom(rect.bottom()+1);

    m_minx = graph.min_x;
    m_maxx = graph.max_x;


    QDateTime date2 = QDateTime::fromMSecsSinceEpoch(m_minx);
    QDateTime enddate2 = QDateTime::fromMSecsSinceEpoch(m_maxx);

    QDate date = date2.date();
    QDate enddate = enddate2.date();

    int days = ceil(double(m_maxx - m_minx) / 86400000.0);

    float lasty1 = rect.bottom();
    float lastx1 = rect.left();

    QMap<QDate, int>::iterator it = dayindex.find(date);
    int idx=0;
    if (it != dayindex.end()) {
        idx = it.value();
    }

    QMap<QDate, int>::iterator ite = dayindex.find(enddate);
    int idx_end = daylist.size()-1;
    if (ite != dayindex.end()) {
        idx_end = ite.value();
    }

    QPoint mouse = graph.graphView()->currentMousePos();

    nousedays = 0;
    totaldays = 0;

    QRectF hl_rect, hl2_rect;
    QDate hl_date;
    Day * hl_day = nullptr;
    int hl_idx = -1;
    bool hl = false;

    if ((daylist.size() == 0) || (it == dayindex.end())) return;

    Day * lastday = nullptr;

    //    int dc = 0;
//    for (int i=idx; i<=idx_end; ++i) {
//        Day * day = daylist.at(i);
//        if (day || lastday) {
//            dc++;
//        }
//        lastday = day;
//    }
//    days = dc;
//    lastday = nullptr;
    float barw = float(rect.width()) / float(days);

    QString hl2_text = "";

    QVector<QRectF> outlines;
    int size = idx_end - idx;
    outlines.reserve(size * expected_slices);

    // Virtual call to setup any custom graph stuff
    preCalc();

    /////////////////////////////////////////////////////////////////////
    /// Calculate Graph Peaks
    /////////////////////////////////////////////////////////////////////
    peak_value = 0;
    for (int i=idx; i < idx_end; ++i) {
        Day * day = daylist.at(i);

        if (!day)
            continue;

        day->OpenSummary();

        QHash<int, QList<SummaryChartSlice> >::iterator cit = cache.find(i);

        if (cit == cache.end()) {
            populate(day, i);
            cit = cache.find(i);
        }

        if (cit != cache.end()) {
            QList<SummaryChartSlice> & list = cit.value();
            float base = 0, val;
            int listsize = list.size();
            for (int j=0; j < listsize; ++j) {
                SummaryChartSlice & slice = list[j];
                val = slice.height;
                base += val;
            }
            peak_value = qMax(peak_value, base);
        }

    }
    m_miny = 0;
    m_maxy = ceil(peak_value);

    /////////////////////////////////////////////////////////////////////
    /// Y-Axis scaling
    /////////////////////////////////////////////////////////////////////

    EventDataType miny;
    EventDataType maxy;

    graph.roundY(miny, maxy);
    float ymult = float(rect.height()) / (maxy-miny);

    /////////////////////////////////////////////////////////////////////
    /// Main drawing loop
    /////////////////////////////////////////////////////////////////////
    do {
        Day * day = daylist.at(idx);

        if ((lastx1 + barw) > (rect.left()+rect.width()+1))
            break;

        totaldays++;

        if (!day) {
            lasty1 = rect.bottom();
            lastx1 += barw;
            it++;
            nousedays++;
            lastday = day;
            continue;
        }

        lastday = day;

        float x1 = lastx1 + barw;

        day->OpenSummary();
        QRectF hl2_rect;

        bool hlday = false;
        QRectF rec2(lastx1, rect.top(), barw, rect.height());
        if (rec2.contains(mouse)) {
            hl_rect = rec2;
            hl_day = day;
            hl_date = it.key();
            hl_idx = idx;

            hl = true;
            hlday = true;
        }

        QHash<int, QList<SummaryChartSlice> >::iterator cit = cache.find(idx);

        if (cit == cache.end()) {
            populate(day, idx);
            cit = cache.find(idx);
        }

        float lastval = 0, val, y1,y2;
        if (cit != cache.end()) {
            /////////////////////////////////////////////////////////////////////////////////////
            /// Draw pressure settings
            /////////////////////////////////////////////////////////////////////////////////////
            QList<SummaryChartSlice> & list = cit.value();
            customCalc(day, list);

            int listsize = list.size();

            for (int i=0; i < listsize; ++i) {
                SummaryChartSlice & slice = list[i];

                val = slice.height;
                y1 = ((lastval-miny) * ymult);
                y2 = (val * ymult);
                QColor color = slice.color;
                QRectF rec(lastx1, rect.bottom() - y1, barw, -y2);

                rec = rec.intersected(rect);

                if (hlday) {
                    if (rec.contains(mouse.x(), mouse.y())) {
                        color = Qt::yellow;
                        hl2_rect = rec;
                    }
                }

                QColor col2 = brighten(color,2.5);

                if (barw > 8) {
                    QLinearGradient gradient(lastx1, rect.bottom(), lastx1+barw, rect.bottom());
                    gradient.setColorAt(0,color);
                    gradient.setColorAt(1,col2);
                    painter.fillRect(rec, QBrush(gradient));
                    outlines.append(rec);
                } else if (barw > 3) {
                    painter.fillRect(rec, QBrush(brighten(color,1.25)));
                    outlines.append(rec);
                } else {
                    painter.fillRect(rec, QBrush(color));
                }

                lastval += val;

            }

        }

        lastx1 = x1;
        it++;

    } while (++idx <= idx_end);
    painter.setPen(QPen(Qt::black,1));
    painter.drawRects(outlines);

    if (hl) {
        QColor col2(255,0,0,64);
        painter.fillRect(hl_rect, QBrush(col2));

        QString txt = hl_date.toString(Qt::SystemLocaleDate)+" ";
        if (hl_day) {
            // grab extra tooltip data
            txt += tooltipData(hl_day, hl_idx);
            if (!hl2_text.isEmpty()) {
                QColor col = Qt::yellow;
                col.setAlpha(255);
           //     painter.fillRect(hl2_rect, QBrush(col));
                txt += hl2_text;
            }
        }

        graph.ToolTip(txt, mouse.x()-15, mouse.y()+5, TT_AlignRight);
    }
    afterDraw(painter, graph, rect);


    // This could be turning off graphs prematurely..
    if (cache.size() == 0) {

        m_empty = true;
        graph.graphView()->updateScale();
    }

}

QString gUsageChart::tooltipData(Day * day, int)
{
    return QObject::tr("\nHours: %1").arg(day->hours(m_machtype), 0, 'f', 2);
}

void gUsageChart::populate(Day *day, int idx)
{
    QList<SummaryChartSlice> & slices = cache[idx];

    float hours = day->hours();

    QColor cpapcolor = day->summaryOnly() ? QColor(128,128,128) : QColor(64,128,255);
    bool haveoxi = day->hasMachine(MT_OXIMETER);

    QColor goodcolor = haveoxi ? QColor(128,255,196) : cpapcolor;

    QColor color = (hours < compliance_threshold) ? QColor(255,64,64) : goodcolor;
    slices.append(SummaryChartSlice(NoChannel, hours, hours, QObject::tr("Hours"), color));
}

void gUsageChart::preCalc()
{
    compliance_threshold = p_profile->cpap->complianceHours();
    incompdays = 0;
    totalhours = 0;
    totaldays = 0;
}

void gUsageChart::customCalc(Day *, QList<SummaryChartSlice> &list)
{
    SummaryChartSlice & slice = list[0];
    if (slice.value < compliance_threshold) incompdays++;
    totalhours += slice.value;
    totaldays++;
}

void gUsageChart::afterDraw(QPainter &, gGraph &graph, QRect rect)
{
    if (totaldays > 1) {
        float comp = 100.0 - ((float(incompdays + nousedays) / float(totaldays)) * 100.0);
        double avg = totalhours / double(totaldays);
        QString txt = QObject::tr("%1 low usage, %2 no usage, out of %3 days (%4% compliant.) Average %5 hours").arg(incompdays).arg(nousedays).arg(totaldays).arg(comp,0,'f',1).arg(avg, 0, 'f', 2);
        graph.renderText(txt, rect.left(), rect.top()-5*graph.printScaleY(), 0);
    }
}



void gSessionTimesChart::afterDraw(QPainter & /*painter */, gGraph &graph, QRect rect)
{
    float avgsess = float(num_slices) / float(num_days);
    double avglength = total_length / double(num_slices);

    QString txt = QObject::tr("Avg Sessions: %1 Avg Length: %2").arg(avgsess, 0, 'f', 1).arg(avglength, 0, 'f', 2);
    graph.renderText(txt, rect.left(), rect.top()-5*graph.printScaleY(), 0);
}

void gSessionTimesChart::paint(QPainter &painter, gGraph &graph, const QRegion &region)
{
    QRect rect = region.boundingRect();

    painter.setPen(QColor(Qt::black));
    painter.drawRect(rect);

    m_minx = graph.min_x;
    m_maxx = graph.max_x;

    QDateTime date2 = QDateTime::fromMSecsSinceEpoch(m_minx);
    QDateTime enddate2 = QDateTime::fromMSecsSinceEpoch(m_maxx);

    QDate date = date2.date();
    QDate enddate = enddate2.date();

    int days = ceil(double(m_maxx - m_minx) / 86400000.0);

    float barw = float(rect.width()) / float(days);

    QDateTime splittime;

    float lasty1 = rect.bottom();
    float lastx1 = rect.left();

    QMap<QDate, int>::iterator it = dayindex.find(date);
    int idx=0;
    if (it != dayindex.end()) {
        idx = it.value();
    }

    QMap<QDate, int>::iterator ite = dayindex.find(enddate);
    int idx_end = daylist.size()-1;
    if (ite != dayindex.end()) {
        idx_end = ite.value();
    }

    QPoint mouse = graph.graphView()->currentMousePos();

    if (daylist.size() == 0) return;

    QVector<QRectF> outlines;
    int size = idx_end - idx;
    outlines.reserve(size * 5);

    QMap<QDate, int>::iterator it2 = it;

    /////////////////////////////////////////////////////////////////////
    /// Calculate Graph Peaks
    /////////////////////////////////////////////////////////////////////
    peak_value = 0;
    min_value = 999;
    for (int i=idx; i <= idx_end; ++i, ++it2) {
        Day * day = daylist.at(i);

        if (!day)
            continue;

        QHash<int, QList<SummaryChartSlice> >::iterator cit = cache.find(i);

        if (cit == cache.end()) {
            day->OpenSummary();
            date = it2.key();
            splittime = QDateTime(date, split);
            QList<Session *>::iterator si;

            QList<SummaryChartSlice> & slices = cache[i];

            bool haveoxi = day->hasMachine(MT_OXIMETER);

            QColor goodcolor = haveoxi ? QColor(128,255,196) : QColor(64,128,255);

            for (si = day->begin(); si != day->end(); ++si) {
                Session *sess = (*si);
                if (!sess->enabled() || (sess->machine()->type() != m_machtype)) continue;

                // Look at mask on/off slices...
                int slize = sess->m_slices.size();
                if (slize > 0) {
                    // segments
                    for (int j=0; j<slize; ++j) {
                        const SessionSlice & slice = sess->m_slices.at(j);
                        float s1 = float(splittime.secsTo(QDateTime::fromMSecsSinceEpoch(slice.start))) / 3600.0;

                        float s2 = double(slice.end - slice.start) / 3600000.0;

                        QColor col = (slice.status == EquipmentOn) ? goodcolor : Qt::black;
                        slices.append(SummaryChartSlice(NoChannel, s1, s2, (slice.status == EquipmentOn) ? QObject::tr("Mask On") : QObject::tr("Mask Off"), col));
                    }
                } else {
                    // otherwise just show session duration
                    qint64 sf = sess->first();
                    QDateTime st = QDateTime::fromMSecsSinceEpoch(sf);
                    float s1 = float(splittime.secsTo(st)) / 3600.0;

                    float s2 = sess->hours();

                    QString txt = QObject::tr("%1\nStart:%2\nLength:%3").arg(it.key().toString(Qt::SystemLocaleDate)).arg(st.time().toString("hh:mm:ss")).arg(s2,0,'f',2);

                    slices.append(SummaryChartSlice(NoChannel, s1, s2, txt, goodcolor));
                }
            }

            cit = cache.find(i);
        }

        if (cit != cache.end()) {
            QList<SummaryChartSlice> & list = cit.value();
            int listsize = list.size();

            float peak = 0, base = 999;

            for (int j=0; j < listsize; ++j) {
                SummaryChartSlice & slice = list[j];
                float s1 = slice.value;
                float s2 = slice.height;

                peak = qMax(peak, s1+s2);
                base = qMin(base, s1);
            }
            peak_value = qMax(peak_value, peak);
            min_value = qMin(min_value, base);

        }

    }
    m_miny = (min_value < 999) ? floor(min_value) : 0;
    m_maxy = ceil(peak_value);

    /////////////////////////////////////////////////////////////////////
    /// Y-Axis scaling
    /////////////////////////////////////////////////////////////////////

    EventDataType miny;
    EventDataType maxy;

    graph.roundY(miny, maxy);
    float ymult = float(rect.height()) / (maxy-miny);


    preCalc();

    /////////////////////////////////////////////////////////////////////
    /// Main Loop scaling
    /////////////////////////////////////////////////////////////////////
    do {
        Day * day = daylist.at(idx);

        if ((lastx1 + barw) > (rect.left()+rect.width()+1))
            break;


        if (!day) {
            lasty1 = rect.bottom();
            lastx1 += barw;
         //   it++;
            continue;
        }

        QHash<int, QList<SummaryChartSlice> >::iterator cit = cache.find(idx);

        float x1 = lastx1 + barw;

        bool hl = false;

        QRect rec2(lastx1, rect.top(), barw, rect.height());
        if (rec2.contains(mouse)) {
            QColor col2(255,0,0,64);
            painter.fillRect(rec2, QBrush(col2));
            hl = true;
        }

        if (cit != cache.end()) {
            QList<SummaryChartSlice> & slices = cit.value();

            customCalc(day, slices);
            int size = slices.size();

            for (int i=0; i < size; ++i) {
                const SummaryChartSlice & slice = slices.at(i);
                float s1 = slice.value - miny;
                float s2 = slice.height;

                float y1 = (s1 * ymult);
                float y2 = (s2 * ymult);

                QColor col = slice.color;

                QRect rec(lastx1, rect.bottom() - y1 - y2, barw, y2);
                rec = rec.intersected(rect);

                if (rec.contains(mouse)) {
                    col = Qt::yellow;
                    graph.ToolTip(slice.name, mouse.x() - 15,mouse.y() + 15, TT_AlignRight);

                }
                QColor col2 = brighten(col,2.5);

                if (barw > 8) {
                    QLinearGradient gradient(lastx1, rect.bottom(), lastx1+barw, rect.bottom());
                    gradient.setColorAt(0,col);
                    gradient.setColorAt(1,col2);
                    painter.fillRect(rec, QBrush(gradient));
                    outlines.append(rec);
                } else if (barw > 3) {
                    painter.fillRect(rec, QBrush(brighten(col,1.25)));
                    outlines.append(rec);
                } else {
                    painter.fillRect(rec, QBrush(col));
                }

            }
        }


   //     it++;
        lastx1 = x1;
    } while (++idx <= idx_end);

    painter.setPen(QPen(Qt::black,1));
    painter.drawRects(outlines);
    afterDraw(painter, graph, rect);
}

void gAHIChart::preCalc()
{
    indices.clear();
    ahi_total = 0;
    calc_cnt = 0;
    total_hours = 0;
}
void gAHIChart::customCalc(Day *day, QList<SummaryChartSlice> &list)
{
    int size = list.size();
    float hours = day->hours(m_machtype);
    for (int i=0; i < size; ++i) {
        const SummaryChartSlice & slice = list.at(i);
        EventDataType value = slice.value;
        indices[slice.code] += value;
        ahi_total += value;
    }
    total_hours += hours;
    calc_cnt++;
}
void gAHIChart::afterDraw(QPainter & /*painter */, gGraph &graph, QRect rect)
{
    QStringList txtlist;
    txtlist.append(QString("%1: %2").arg(STR_TR_AHI).arg(ahi_total / total_hours, 0, 'f', 2));

    QHash<ChannelID, double>::iterator it;
    QHash<ChannelID, double>::iterator it_end = indices.end();

    for (it = indices.begin(); it != it_end; ++it) {
        ChannelID code = it.key();
        schema::Channel & chan = schema::channel[code];
        double indice = it.value() / total_hours;
        txtlist.append(QString("%1: %2").arg(chan.label()).arg(indice, 0, 'f', 2));
    }
    QString txt = txtlist.join(" ");
    graph.renderText(txt, rect.left(), rect.top()-5*graph.printScaleY(), 0);
}

void gAHIChart::populate(Day *day, int idx)
{
    QList<SummaryChartSlice> & slices = cache[idx];

    float hours = day->hours();
    for (int i=0; i < num_channels; ++i) {
        ChannelID code = channels.at(i);
        if (!day->hasData(code, ST_CNT)) continue;
        schema::Channel *chan = schema::channel.channels.find(code).value();
        float c = day->count(code);
        slices.append(SummaryChartSlice(code, c, c  / hours, chan->label(), chan->defaultColor()));
    }
}
QString gAHIChart::tooltipData(Day *day, int idx)
{
    QList<SummaryChartSlice> & slices = cache[idx];
    float total = 0;
    float hour = day->hours(m_machtype);
    QString txt;
    for (int i=0; i< slices.size(); ++i) {
        SummaryChartSlice & slice = slices[i];
        total += slice.value;
        txt += QString("\n%1: %2").arg(slice.name).arg(float(slice.value) / hour, 0, 'f', 2);

    }
    return QString("\n%1: %2").arg(STR_TR_AHI).arg(float(total) / hour,0,'f',2)+txt;
}

void gPressureChart::populate(Day * day, int idx)
{
    float tmp;
    CPAPMode mode =  (CPAPMode)(int)qRound(day->settings_wavg(CPAP_Mode));
    if (mode == MODE_CPAP) {
        float pr = day->settings_max(CPAP_Pressure);
        cache[idx].append(SummaryChartSlice(CPAP_Pressure, pr, pr, schema::channel[CPAP_Pressure].label(), schema::channel[CPAP_Pressure].defaultColor()));
    } else if (mode == MODE_APAP) {
        float min = day->settings_min(CPAP_PressureMin);
        float max = day->settings_max(CPAP_PressureMax);
        QList<SummaryChartSlice> & slices = cache[idx];

        tmp = min;

        slices.append(SummaryChartSlice(CPAP_PressureMin, min, min, schema::channel[CPAP_PressureMin].label(), schema::channel[CPAP_PressureMin].defaultColor()));
        if (!day->summaryOnly()) {
            float med = day->calcMiddle(CPAP_Pressure);
            slices.append(SummaryChartSlice(CPAP_Pressure, med, med - tmp, day->calcMiddleLabel(CPAP_Pressure), schema::channel[CPAP_Pressure].defaultColor()));
            tmp += med - tmp;

            float p90 = day->calcPercentile(CPAP_Pressure);
            slices.append(SummaryChartSlice(CPAP_Pressure, p90, p90 - tmp, day->calcPercentileLabel(CPAP_Pressure), brighten(schema::channel[CPAP_Pressure].defaultColor(), 1.33)));
            tmp += p90 - tmp;
        }
        slices.append(SummaryChartSlice(CPAP_PressureMax, max, max - tmp, schema::channel[CPAP_PressureMax].label(), schema::channel[CPAP_PressureMax].defaultColor()));

    } else if (mode == MODE_BILEVEL_FIXED) {
        float epap = day->settings_max(CPAP_EPAP);
        float ipap = day->settings_max(CPAP_IPAP);
        QList<SummaryChartSlice> & slices = cache[idx];

        slices.append(SummaryChartSlice(CPAP_EPAP, epap, epap, schema::channel[CPAP_EPAP].label(), schema::channel[CPAP_EPAP].defaultColor()));
        slices.append(SummaryChartSlice(CPAP_IPAP, ipap, ipap - epap, schema::channel[CPAP_IPAP].label(), schema::channel[CPAP_IPAP].defaultColor()));

    } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
        float epap = day->settings_max(CPAP_EPAPLo);
        tmp = epap;
        float ipap = day->settings_max(CPAP_IPAPHi);
        QList<SummaryChartSlice> & slices = cache[idx];

        slices.append(SummaryChartSlice(CPAP_EPAP, epap, epap, schema::channel[CPAP_EPAPLo].label(), schema::channel[CPAP_EPAPLo].defaultColor()));
        if (!day->summaryOnly()) {

            float e50 = day->calcMiddle(CPAP_EPAP);
            slices.append(SummaryChartSlice(CPAP_EPAP, e50, e50 - tmp, day->calcMiddleLabel(CPAP_EPAP), schema::channel[CPAP_EPAP].defaultColor()));
            tmp += e50 - tmp;

            float e90 = day->calcPercentile(CPAP_EPAP);
            slices.append(SummaryChartSlice(CPAP_EPAP, e90, e90 - tmp, day->calcPercentileLabel(CPAP_EPAP), brighten(schema::channel[CPAP_EPAP].defaultColor(),1.33)));
            tmp += e90 - tmp;

            float i50 = day->calcMiddle(CPAP_IPAP);
            slices.append(SummaryChartSlice(CPAP_IPAP, i50, i50 - tmp, day->calcMiddleLabel(CPAP_IPAP), schema::channel[CPAP_IPAP].defaultColor()));
            tmp += i50 - tmp;

            float i90 = day->calcPercentile(CPAP_IPAP);
            slices.append(SummaryChartSlice(CPAP_IPAP, i90, i90 - tmp, day->calcPercentileLabel(CPAP_IPAP), brighten(schema::channel[CPAP_IPAP].defaultColor(),1.33)));
            tmp += i90 - tmp;
        }
        slices.append(SummaryChartSlice(CPAP_EPAP, ipap, ipap - tmp, schema::channel[CPAP_IPAPHi].label(), schema::channel[CPAP_IPAPHi].defaultColor()));
    } else if ((mode == MODE_BILEVEL_AUTO_VARIABLE_PS) || (mode == MODE_ASV_VARIABLE_EPAP)) {
        float epap = day->settings_max(CPAP_EPAPLo);
        tmp = epap;
        QList<SummaryChartSlice> & slices = cache[idx];

        slices.append(SummaryChartSlice(CPAP_EPAPLo, epap, epap, schema::channel[CPAP_EPAPLo].label(), schema::channel[CPAP_EPAPLo].defaultColor()));
        if (!day->summaryOnly()) {
            float e50 = day->calcMiddle(CPAP_EPAP);
            slices.append(SummaryChartSlice(CPAP_EPAP, e50, e50 - tmp, day->calcMiddleLabel(CPAP_EPAP), schema::channel[CPAP_EPAP].defaultColor()));
            tmp += e50 - tmp;

            float e90 = day->calcPercentile(CPAP_EPAP);
            slices.append(SummaryChartSlice(CPAP_EPAP, e90, e90 - tmp, day->calcPercentileLabel(CPAP_EPAP), brighten(schema::channel[CPAP_EPAP].defaultColor(),1.33)));
            tmp += e90 - tmp;

            float i50 = day->calcMiddle(CPAP_IPAP);
            slices.append(SummaryChartSlice(CPAP_IPAP, i50, i50 - tmp, day->calcMiddleLabel(CPAP_IPAP), schema::channel[CPAP_IPAP].defaultColor()));
            tmp += i50 - tmp;

            float i90 = day->calcPercentile(CPAP_IPAP);
            slices.append(SummaryChartSlice(CPAP_IPAP, i90, i90 - tmp, day->calcPercentileLabel(CPAP_IPAP), brighten(schema::channel[CPAP_IPAP].defaultColor(),1.33)));
            tmp += i90 - tmp;
        }
        float ipap = day->settings_max(CPAP_IPAPHi);
        slices.append(SummaryChartSlice(CPAP_IPAPHi, ipap, ipap - tmp, schema::channel[CPAP_IPAPHi].label(), schema::channel[CPAP_IPAPHi].defaultColor()));
    } else if (mode == MODE_ASV) {
        float epap = day->settings_max(CPAP_EPAP);
        tmp = epap;
        QList<SummaryChartSlice> & slices = cache[idx];

        slices.append(SummaryChartSlice(CPAP_EPAP, epap, epap, schema::channel[CPAP_EPAP].label(), schema::channel[CPAP_EPAP].defaultColor()));
        if (!day->summaryOnly()) {
            float i50 = day->calcMiddle(CPAP_IPAP);
            slices.append(SummaryChartSlice(CPAP_IPAP, i50, i50 - tmp, day->calcMiddleLabel(CPAP_IPAP), schema::channel[CPAP_IPAP].defaultColor()));
            tmp += i50 - tmp;

            float i90 = day->calcPercentile(CPAP_IPAP);
            slices.append(SummaryChartSlice(CPAP_IPAP, i90, i90 - tmp, day->calcPercentileLabel(CPAP_IPAP), brighten(schema::channel[CPAP_IPAP].defaultColor(),1.33)));
            tmp += i90 - tmp;
        }
        float ipap = day->settings_max(CPAP_IPAPHi);
        slices.append(SummaryChartSlice(CPAP_IPAPHi, ipap, ipap - tmp, schema::channel[CPAP_IPAPHi].label(), schema::channel[CPAP_IPAPHi].defaultColor()));
    }

}
