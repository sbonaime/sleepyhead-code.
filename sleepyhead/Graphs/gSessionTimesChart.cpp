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

#include "SleepLib/profiles.h"
#include "gSessionTimesChart.h"

#include "gYAxis.h"

gSessionTimesChart::gSessionTimesChart(QString label, MachineType machtype)
    :Layer(NoChannel), m_label(label), m_machtype(machtype)
{
    m_layertype = LT_SessionTimes;
    QDateTime d1 = QDateTime::currentDateTime();
    QDateTime d2 = d1;
    d1.setTimeSpec(Qt::UTC);  // CHECK: Does this deal with DST?
    tz_offset = d2.secsTo(d1);
    tz_hours = tz_offset / 3600.0;
}

gSessionTimesChart::~gSessionTimesChart()
{
}

void gSessionTimesChart::SetDay(Day *unused_day)
{
    Q_UNUSED(unused_day)
    Layer::SetDay(nullptr);

    firstday = p_profile->FirstDay(m_machtype);
    lastday = p_profile->LastDay(m_machtype);

    QDate date = firstday;
    do {
        QMap<QDate, Day *>::iterator di = p_profile->daylist.find(date);
        Day * day = di.value();
        if (di == p_profile->daylist.end()) {
        }
    } while ((date = date.addDays(1)) < lastday);

    m_minx = QDateTime(firstday, QTime(0,0,0)).toMSecsSinceEpoch();
    m_maxx = QDateTime(lastday, QTime(23,59,59)).toMSecsSinceEpoch();
    m_miny = 0;
    m_maxy = 30;
    m_empty = false;

}

QColor brighten(QColor color, float mult = 2.0);


void gSessionTimesChart::paint(QPainter &painter, gGraph &graph, const QRegion &region)
{
    QRect rect = region.boundingRect();

    painter.setPen(QColor(Qt::black));
    painter.drawRect(rect);

    m_minx = graph.min_x;
    m_maxx = graph.max_x;

    EventDataType miny;
    EventDataType maxy;

    graph.roundY(miny, maxy);

    QDateTime date2 = QDateTime::fromMSecsSinceEpoch(m_minx);
    QDateTime enddate2 = QDateTime::fromMSecsSinceEpoch(m_maxx);

    QDate date = date2.date();
    QDate enddate = enddate2.date();

    QString text = QString("Work in progress, overview is slightly broken... I know about the bugs :P There is a very good reason for redoing these overview graphs..."); //.arg(date2.toString("yyyyMMdd hh:mm:ss")).arg(enddate2.toString("yyyyMMdd hh:mm:ss"));
    painter.setFont(*defaultfont);
    painter.drawText(rect.left(), rect.top()-4, text);

    int days = ceil(double(m_maxx - m_minx) / 86400000.0);

    float barw = float(rect.width()) / float(days);

    QTime split = p_profile->session->daySplitTime();
    QDateTime splittime;


    //float maxy = m_maxy;
    float ymult = float(rect.height()) / (maxy-miny);

    int dn = 0;
    float lasty1 = rect.bottom();
    float lastx1 = rect.left();

    do {
        QMap<QDate, Day *>::iterator di = p_profile->daylist.find(date);

        if (di == p_profile->daylist.end()) {
            dn++;
            lasty1 = rect.bottom();
            lastx1 += barw;
            continue;
        }
        Day * day = di.value();
//        if (day->first() > m_maxx) { //|| (day->last() < m_minx)) {
//            continue;
//        }
        splittime = QDateTime(date, split);

        float x1 = lastx1 + barw;
        QList<Session *>::iterator si;


        if ((lastx1 + barw) > (rect.left()+rect.width()+1))
            break;
        bool hl = false;

        QPoint mouse = graph.graphView()->currentMousePos();

        QRect rec2(lastx1, rect.top(), barw, rect.height());
        if (rec2.contains(mouse)) {
            QColor col2(255,0,0,64);
            painter.fillRect(rec2, QBrush(col2));
            hl = true;
        }

        bool haveoxi = day->hasMachine(MT_OXIMETER);

        QColor goodcolor = haveoxi ? QColor(128,196,255) : Qt::blue;

        for (si = day->begin(); si != day->end(); ++si) {
            Session *sess = (*si);
            if (!sess->enabled() || (sess->machine()->type() != m_machtype)) continue;

            int slize = sess->m_slices.size();
            if (slize > 0) {
                // segments
                for (int i=0; i<slize; ++i) {
                    const SessionSlice & slice = sess->m_slices.at(i);
                    float s1 = float(splittime.secsTo(QDateTime::fromMSecsSinceEpoch(slice.start))) / 3600.0;

                    float s2 = double(slice.end - slice.start) / 3600000.0;

                    float y1 = (s1 * ymult);
                    float y2 = (s2 * ymult);

                    QColor col = (slice.status == EquipmentOn) ? goodcolor : Qt::black;
                    QColor col2 = brighten(col,2.5);


                    QRect rec(lastx1, rect.bottom() - y1 - y2, barw, y2);
                    QLinearGradient gradient(lastx1, rect.bottom(), lastx1+barw, rect.bottom());

                    if (rec.contains(mouse)) {
//                    if (hl) {
                        col = Qt::yellow;
                    }

                    gradient.setColorAt(0,col);
                    gradient.setColorAt(1,col2);
                    painter.fillRect(rec, QBrush(gradient));
                    painter.setPen(QPen(Qt::black,1));
                    painter.drawRect(rec);

                }
            } else {
                qint64 sf = sess->first();
                QDateTime st = QDateTime::fromMSecsSinceEpoch(sf);
                float s1 = float(splittime.secsTo(st)) / 3600.0;

                float s2 = sess->hours();

                float y1 = (s1 * ymult);
                float y2 = (s2 * ymult);

                QColor col = goodcolor;

                QLinearGradient gradient(lastx1, rect.bottom(), lastx1+barw, rect.bottom());
                QRect rec(lastx1, rect.bottom() - y1 - y2, barw, y2);
                if (rec.contains(mouse)) {
                    QString text = QObject::tr("%1\nBedtime:%2\nLength:%3").arg(st.date().toString(Qt::SystemLocaleDate)).arg(st.time().toString("hh:mm:ss")).arg(s2,0,'f',2);
                    graph.ToolTip(text,mouse.x() - 15,mouse.y() + 15, TT_AlignRight);

                    col = QColor("gold");
                }

                QColor col2 = brighten(col,2.5);

                gradient.setColorAt(0,col);
                gradient.setColorAt(1,col2);

                painter.fillRect(rec, QBrush(gradient));
                painter.setPen(QPen(Qt::black,1));
                painter.drawRect(rec);


                // no segments
            }
        }

//        float y = double(day->total_time(m_machtype)) / 3600000.0;
//        float y1 = rect.bottom() - (y * ymult);
//        float x1 = lastx1 + barw;
//        painter.drawLine(lastx1, lasty1, lastx1,y1);
//        painter.drawLine(lastx1, y1, x1, y1);

        dn++;
//        lasty1 = y1;
        lastx1 = x1;

    } while ((date = date.addDays(1)) <= enddate);

}

bool gSessionTimesChart::keyPressEvent(QKeyEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return false;
}

bool gSessionTimesChart::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return false;
}

bool gSessionTimesChart::mousePressEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return false;
}

bool gSessionTimesChart::mouseReleaseEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return false;
}

