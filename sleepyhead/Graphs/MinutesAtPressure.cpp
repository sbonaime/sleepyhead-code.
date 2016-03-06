/* MinutesAtPressure Graph Implementation
 *
 * Copyright (c) 2011-2016 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <cmath>
#include <QApplication>
#include <QThreadPool>
#include <QMutexLocker>

#include "MinutesAtPressure.h"
#include "Graphs/gGraph.h"
#include "Graphs/gGraphView.h"
#include "SleepLib/profiles.h"

#include "Graphs/gXAxis.h"
#include "Graphs/gYAxis.h"



MinutesAtPressure::MinutesAtPressure() :Layer(NoChannel)
{
    m_remap = nullptr;
    m_minpressure = 3;
    m_maxpressure = 30;
    m_minimum_height = 0;
}
MinutesAtPressure::~MinutesAtPressure()
{
    while (recalculating()) {};
}

RecalcMAP::~RecalcMAP()
{
}
void RecalcMAP::quit() {
    m_quit = true;
    map->mutex.lock();
    map->mutex.unlock();
}


void MinutesAtPressure::SetDay(Day *day)
{
    Layer::SetDay(day);

    // look at session summaryValues.
    Machine * cpap = nullptr;
    if (day) cpap = day->machine(MT_CPAP);
    if (cpap) {
        QList<Session *>::iterator sit;
        EventDataType minpressure = 40;
        EventDataType maxpressure = 0;

        QMap<QDate, Day *>::iterator it;
        QMap<QDate, Day *>::iterator day_end = cpap->day.end();
        // look at overall pressure ranges and find the max

        for (it = cpap->day.begin(); it != day_end; ++it) {
            Day * d = it.value();
            QList<Session *>::iterator sess_end = d->end();
            for (sit = d->begin(); sit != sess_end; ++sit) {
                Session * sess = (*sit);
                if (sess->channelExists(CPAP_Pressure)) {
                    minpressure = qMin(sess->Min(CPAP_Pressure), minpressure);
                    maxpressure = qMax(sess->Max(CPAP_Pressure), maxpressure);
                }
                if (sess->channelExists(CPAP_EPAP)) {
                    minpressure = qMin(sess->Min(CPAP_EPAP), minpressure);
                    maxpressure = qMax(sess->Max(CPAP_EPAP), maxpressure);
                }
                if (sess->channelExists(CPAP_IPAP)) {
                    minpressure = qMin(sess->Min(CPAP_IPAP), minpressure);
                    maxpressure = qMax(sess->Max(CPAP_IPAP), maxpressure);
                }
            }
        }

        m_minpressure = qMax(float(4), floor(minpressure));
        m_maxpressure = ceil(maxpressure);

        const int minimum_cells = 12;
        int c = m_maxpressure - m_minpressure;


        if (c < minimum_cells) {
            int v = minimum_cells - c;
            m_minpressure -= v/2.0;
            m_minpressure = qMax((EventStoreType)4, m_minpressure);

            m_maxpressure = m_minpressure + minimum_cells;
        }
        QFontMetrics FM(*defaultfont);
        QList<ChannelID> chans = day->getSortedMachineChannels(schema::SPAN | schema::FLAG | schema::MINOR_FLAG);
        m_minimum_height = (chans.size()+3) * FM.height() - 5;
    }


    m_empty = false;
    m_recalculating = false;
    m_lastminx = 0;
    m_lastmaxx = 0;
    m_empty = !m_day || !(m_day->channelExists(CPAP_Pressure) || m_day->channelExists(CPAP_EPAP));
}

 int MinutesAtPressure::minimumHeight()
{
    return m_minimum_height;
}



bool MinutesAtPressure::isEmpty()
{

    return m_empty;
}

// Calculate Catmull-Rom Spline of given 4 samples, with t between 0-1;
float CatmullRomSpline(float p0, float p1, float p2, float p3, float t = 0.5)
{
    float t2 = t*t;
    float t3 = t2 * t;

    return (float)0.5 * ((2 * p1) +
    (-p0 + p2) * t +
    (2*p0 - 5*p1 + 4*p2 - p3) * t2 +
    (-p0 + 3*p1- 3*p2 + p3) * t3);
}

void MinutesAtPressure::paint(QPainter &painter, gGraph &graph, const QRegion &region)
{
    QRect rect = region.boundingRect();


    int cells = m_maxpressure-m_minpressure+1;


    int top = rect.top()-10;
    float width = rect.width();
    float height = rect.height();
    float left = rect.left();
    float pix = width / float(cells);


    int numchans = chans.size();

    int cells_high = numchans + 2;

    //height += 10;
    float hix = height / cells_high;

    m_minx = graph.min_x;
    m_maxx = graph.max_x;

    if (graph.printing() || ((m_lastminx != m_minx) || (m_lastmaxx != m_maxx))) {
        recalculate(&graph);
    }


    m_lastminx = m_minx;
    m_lastmaxx = m_maxx;

    QMap<EventStoreType, int>::iterator it;
    if (graph.printing()) {
        // lock the other mutex...
//        while (recalculating()) {};
//        recalculate(&graph);
        while (recalculating()) {};

    }

    if (!painter.isActive()) return;

    // Lock the stuff we need to draw
    timelock.lock();
    painter.setFont(*defaultfont);
    painter.setPen(Qt::black);

    QMap<EventStoreType, int>::iterator times_end = times.end();
    QPoint mouse = graph.graphView()->currentMousePos();

    float ypos = top;

    int titleWidth = graph.graphView()->titleWidth;
    int marginWidth = gYAxis::Margin;

    QString text = schema::channel[m_presChannel].label();
    QRectF rec(titleWidth-4, ypos, marginWidth, hix);
    rec.moveRight(left - 4);
//    graph.renderText(text, rec, Qt::AlignRight | Qt::AlignVCenter);

    if (rec.contains(mouse)) {
        QString text = schema::channel[m_presChannel].description();
        graph.ToolTip(text, mouse.x() + 10, mouse.y(), TT_AlignLeft);
    }
    int w,h;
    GetTextExtent(text, w,h);
    graph.renderText(text, (left-4) - w, ypos + hix/2.0 + float(h)/2.0);

    text = STR_UNIT_Minutes;
    rec = QRectF(titleWidth-4, ypos+hix, marginWidth, hix);
    rec.moveRight(left - 4);

    GetTextExtent(text, w,h);
    graph.renderText(text, (left-4) - w, ypos + hix + hix/2.0 + float(h)/2.0);
//    graph.renderText(text, rec, Qt::AlignRight | Qt::AlignVCenter);

    float xpos = left;
    for (it = times.begin(); it != times_end; ++it) {
        QString text = QString::number(it.key());
        QString value = QString("%1").arg(float(it.value()) / 60.0, 5, 'f', 1);
        QRectF rec(xpos, top, pix-1, hix);

        GetTextExtent(text, w,h);

        painter.fillRect(rec, QColor("orange"));
        graph.renderText(text, xpos + pix/2 - w/2, top + hix /2 + h/2);

        GetTextExtent(value, w,h);

//        rec.moveTop(top + hix);
        graph.renderText(value, xpos + pix/2 - w/2, top + hix+ hix /2+ h/2);

        xpos += pix;
    }

    ypos += hix * 2;
  //  left = rect.left();

    QHash<ChannelID, QMap<EventStoreType, EventDataType> >::iterator eit;
    //QHash<ChannelID, QMap<EventStoreType, EventDataType> >::iterator ev_end = events.end();
    QMap<EventStoreType, EventDataType>::iterator vit;


    int row = 0;
    for (int i=0; i< numchans; ++i) {
        ChannelID code = chans.at(i);

        schema::Channel & chan = schema::channel[code];
        if (!chan.enabled())
            continue;
        schema::ChanType type = chan.type();
        eit = events.find(code);

        xpos = left;

        QMap<EventStoreType, EventDataType>::iterator eit_end = eit.value().end();

        QString text = chan.label();
        rec = QRectF(titleWidth, ypos, marginWidth, hix);
        rec.moveRight(xpos - 4);

        if (rec.contains(mouse)) {
                QString text = chan.fullname();
                if (type == schema::SPAN) {
                    text += "\n"+QObject::tr("(% of time)");
                }
                graph.ToolTip(text, mouse.x() + 10, mouse.y(), TT_AlignLeft);
        }

        GetTextExtent(text, w,h);

        graph.renderText(text, (left-4) - w, ypos + hix/2.0 + float(h)/2.0);

        for (it = times.begin(), vit = eit.value().begin(); vit != eit_end; ++vit, ++it) {
            float minutes = float(it.value()) / 60.0;
            float value = vit.value();

            QString fmt = "%1";
            if (type != schema::SPAN) {
                //fmt = "%1";
                value = (minutes > 0.000001) ? (value * 60.0) / minutes : 0;
            } else {
                //fmt = "%1%";
                value = (minutes > 0.000001) ? (100/minutes) * (value / 60.0) : 0;
            }

            QRectF rec(xpos, ypos, pix-1, hix);
            if ((row & 1) == 0) {
                painter.fillRect(rec, QColor(245,245,255,240));
            }

            text = QString(fmt).arg(value,5,'f',2);

            GetTextExtent(text, w,h);

            graph.renderText(text, xpos + pix/2 - w/2, ypos + hix /2 + h/2);
          //  painter.drawText(rec, Qt::AlignCenter, QString(fmt).arg(value,5,'f',2));
            xpos += pix;

        }
        ypos += hix;
        row++;
    }

    float maxmins = float(maxtime) / 60.0;
    float ymult = height / maxmins;


    row = 0;

    xpos = left ;//+ pix / 2;

    float y1, y2;
    it = times.begin();
    float bottom = top+height;

    if (it != times_end) {
        QVector<float> P;
        QVector<float> tap;
        P.resize(26);
        tap.reserve(260);

        for (;  it != times_end; ++it) {
            int p = it.key();
            Q_ASSERT(p < 255);
            float v = float(it.value()) / 60.0;
            P[p] = v;
        }
        tap.append(P[0]);
        for (int i=1; i<24; ++i) {

            float p0 = P[i-1];
            float p1 = P[i];
            float p2 = P[i+1];
            float p3 = P[i+2];

            // Calculate Catmull-Rom Splines in between samples
            tap.append(P[i]);

            tap.append(qMax(0.0f,CatmullRomSpline(p0,p1,p2,p3,0.1)));
            tap.append(qMax(0.0f,CatmullRomSpline(p0,p1,p2,p3,0.2)));
            tap.append(qMax(0.0f,CatmullRomSpline(p0,p1,p2,p3,0.3)));
            tap.append(qMax(0.0f,CatmullRomSpline(p0,p1,p2,p3,0.4)));
            tap.append(qMax(0.0f,CatmullRomSpline(p0,p1,p2,p3,0.5)));
            tap.append(qMax(0.0f,CatmullRomSpline(p0,p1,p2,p3,0.6)));
            tap.append(qMax(0.0f,CatmullRomSpline(p0,p1,p2,p3,0.7)));
            tap.append(qMax(0.0f,CatmullRomSpline(p0,p1,p2,p3,0.8)));
            tap.append(qMax(0.0f,CatmullRomSpline(p0,p1,p2,p3,0.9)));
        }
        tap.append(P[24]);
        tap.append(P[25]);

        painter.setPen(QPen(QColor(Qt::gray), 2));

        float minutes = tap[0];
        y1 = minutes * ymult;

        int tapsize = tap.size();
        for (int i=0; i<tapsize; ++i) {
            minutes = tap[i];
            y2 = minutes * ymult;
            painter.drawLine(xpos, bottom-y1, xpos+pix/10.0, bottom-y2);
            y1 = y2;
            xpos += pix / 10.0;
        }

      /*  float minutes = float(it.value()) / 60.0;
        y1 = minutes * ymult;

        it=times.begin();
        it++;
        for (;  it != times_end; ++it) {
            float minutes = float(it.value()) / 60.0;
            y2 = minutes * ymult;

            painter.drawLine(xpos, bottom-y1, xpos+pix, bottom-y2);
            y1 = y2;
            xpos += pix;
        }*/


        float maxev = 0;
        for (int i=0; i< numchans; ++i) {
            ChannelID code = chans.at(i);
            if (code == CPAP_AHI) continue;


            schema::Channel & chan = schema::channel[code];
            if (!chan.enabled())
                continue;
            schema::ChanType type = chan.type();
            if (type == schema::SPAN)
                continue;
            eit = events.find(code);
            QMap<EventStoreType, EventDataType>::iterator eit_end = eit.value().end();
            for (it = times.begin(), vit = eit.value().begin(); vit != eit_end; ++vit, ++it) {
                //float minutes = float(it.value()) / 60.0;
                float value = vit.value();
                maxev = qMax(value, maxev);
            }
        }
        float emult = height / float(maxev);
        if (maxev < 0.00001) emult = 0;


        for (int i=0; i< numchans; ++i) {
            ChannelID code = chans.at(i);
            if (code == CPAP_AHI) continue;


            schema::Channel & chan = schema::channel[code];
            if (!chan.enabled())
                continue;
            schema::ChanType type = chan.type();
            if (type == schema::SPAN)
                continue;
            painter.setPen(QPen(QColor(chan.defaultColor()), 2));
            eit = events.find(code);
            xpos = left;//+pix/2;

            y1 = 0;
            QMap<EventStoreType, EventDataType>::iterator eit_end = eit.value().end();
            for (it = times.begin(), vit = eit.value().begin(); vit != eit_end; ++vit, ++it) {
                //float minutes = float(it.value()) / 60.0;
                float value = vit.value();

                y2 = value * emult;
                //painter.drawPoint(xpos, bottom-y1);

               // painter.drawLine(xpos, bottom-y1, xpos+pix, bottom-y2);

                xpos += pix;
                y1 = y2;

            }
        }
    }

//    QString txt=QString("%1 %2").arg(maxmins).arg(float(maxevents * 60.0) / maxmins);
//    graph.renderText(txt, rect.left(), rect.top()-10);

    timelock.unlock();

    if (m_recalculating) {
//        painter.setFont(*defaultfont);
//        painter.setPen(QColor(0,0,0,125));
//        painter.drawText(region.boundingRect(), Qt::AlignCenter, QObject::tr("Recalculating..."));
    }


//    painter.setPen(QPen(Qt::black,1));
//    painter.drawRect(rect);


    // Draw the goodies...
}


void RecalcMAP::run()
{
    QMutexLocker locker(&map->mutex);
    map->m_recalculating = true;
    Day * day = map->m_day;
    if (!day) return;

    QList<Session *>::iterator sit;
    QList<Session *>::iterator sess_end = day->end();


    QMap<EventStoreType, int> times;

    QHash<ChannelID, QMap<EventStoreType, EventDataType> > events;

    QList<ChannelID> chans = day->getSortedMachineChannels(schema::SPAN | schema::FLAG | schema::MINOR_FLAG);

    ChannelID code;

    QList<ChannelID> badchans;
    for (int i=0 ; i < chans.size(); ++i) {
        code = chans.at(i);
      //  if (!day->channelExists(code)) badchans.push_back(code);
    }

    for (int i=0; i < badchans.size(); ++i) {
        code = badchans.at(i);
        chans.removeAll(code);
    }


    int numchans = chans.size();
    // Zero the pressure counts
    for (int i=map->m_minpressure; i <= map->m_maxpressure; i++) {
        times[i] = 0;

        for (int c = 0; c < numchans; ++c) {
            code = chans.at(c);
            events[code].insert(i, 0);
        }
    }

    ChannelID prescode = CPAP_Pressure;

//    if (day->channelExists(CPAP_IPAP)) {
//        prescode = CPAP_IPAP;
//    } else
    if (day->channelExists(CPAP_EPAP)) {
        prescode = CPAP_EPAP;
    }

    qint64 minx, maxx;
    map->m_graph->graphView()->GetXBounds(minx, maxx);

    for (sit = day->begin(); sit != sess_end; ++sit) {
        Session * sess = (*sit);
        QHash<ChannelID, QVector<EventList *> >::iterator ei = sess->eventlist.find(prescode);
        if (ei == sess->eventlist.end())
            continue;

        const QVector<EventList *> & evec = ei.value();
        int esize = evec.size();
        for (int ei = 0; ei < esize; ++ei) {
            const EventList *EL = evec.at(ei);
            EventDataType gain = EL->gain();
            quint32 ELsize = EL->count();
            if (ELsize < 1)  return;
            qint64 lasttime = 0; //EL->time(0);
            EventStoreType lastdata = 0; // EL->raw(0);

            bool first = true;
            if ((EL->first() > maxx) || (EL->last() < minx)) {
                continue;
            }

            for (quint32 e = 0; e < ELsize; ++e) {
                qint64 time = EL->time(e);
                EventStoreType data = EL->raw(e);

                if ((time < minx)) {
                    lasttime = time;
                    lastdata = data;
                    first = false;
                    goto skip;
                }

                if (first) {
                    lasttime = time;
                    lastdata = data;
                    first = false;
                }

                if (lastdata != data) {
                    qint64 d1 = qMax(minx, lasttime);
                    qint64 d2 = qMin(maxx, time);


                    int duration = (d2 - d1) / 1000L;
                    EventStoreType key = floor(lastdata * gain);
                    if (key <= 30) {
                        times[key] += duration;
                        for (int c = 0; c < chans.size(); ++c) {
                            ChannelID code = chans.at(c);
                            schema::Channel & chan = schema::channel[code];
                            if (chan.type() == schema::SPAN) {
                                events[code][key] += sess->rangeSum(code, d1, d2);
                            } else {
                                events[code][key] += sess->rangeCount(code, d1, d2);
                            }
                        }
                    }
                    lasttime = time;
                    lastdata = data;
                }
                if (time > maxx) {

                    break;
                }
skip:
                if (m_quit) {
                    m_done = true;
                    return;
                }
            }
            if (lasttime < maxx) {
                qint64 d1 = qMax(lasttime, minx);
                qint64 d2 = qMin(maxx, EL->last());

                int duration = (d2 - d1) / 1000L;
                EventStoreType key = floor(lastdata * gain);
                if (key <= 30) {
                    times[key] += duration;
                    for (int c = 0; c < chans.size(); ++c) {
                        ChannelID code = chans.at(c);
                        schema::Channel & chan = schema::channel[code];
                        if (chan.type() == schema::SPAN) {
                            events[code][key] += sess->rangeSum(code, d1, d2);
                        } else {
                            events[code][key] += sess->rangeCount(code, d1, d2);
                        }
                    }
                }

            }


        }
    }


    QMap<EventStoreType, int>::iterator it;
    QMap<EventStoreType, int>::iterator times_end = times.end();
    int maxtime = 0;

    QList<EventStoreType> trash;
    for (it = times.begin(); it != times_end; ++it) {
        //EventStoreType key = it.key();
        int value = it.value();
//        if (value == 0) {
//            trash.append(key);
//        } else {
            maxtime = qMax(value, maxtime);
//        }
    }
    chans.push_front(CPAP_AHI);

    int maxevents = 0, val;

    for (int i = map->m_minpressure; i <= map->m_maxpressure; i++) {
        val = events[CPAP_Obstructive][i] + events[CPAP_Hypopnea][i] + events[CPAP_Apnea][i] + events[CPAP_ClearAirway][i];
        events[CPAP_AHI].insert(i, val);
     //   maxevents = qMax(val, maxevents);
    }

    for (int i = map->m_minpressure; i <= map->m_maxpressure; i++) {
        for (int j=0 ; j < chans.size(); ++j) {
            code = chans.at(j);
            if ((code == CPAP_AHI) || (schema::channel[code].type() == schema::SPAN)) continue;
            val = events[code][i];
            maxevents = qMax(val, maxevents);
        }
    }
    QHash<ChannelID, QMap<EventStoreType, EventDataType> >::iterator eit;

//    for (int i=0; i< trash.size(); ++i) {
//        EventStoreType key = trash.at(i);

//        times.remove(key);
//        for (eit = events.begin(); eit != events.end(); ++eit) {
//            eit.value().remove(key);
//        }
//    }


    QMutexLocker timelock(&map->timelock);
    map->times = times;
    map->events = events;
    map->maxtime = maxtime;
    map->maxevents = maxevents;
    map->chans = chans;
    map->m_presChannel = prescode;
    timelock.unlock();

    map->recalcFinished();
    m_done = true;
}

void MinutesAtPressure::recalculate(gGraph * graph)
{

    while (recalculating())
        m_remap->quit();

    m_remap = new RecalcMAP(this);
    m_remap->setAutoDelete(true);

    m_graph = graph;

    QThreadPool * tp = QThreadPool::globalInstance();
//    tp->reserveThread();

    if (graph->printing()) {
        m_remap->run();
    } else {
        while(!tp->tryStart(m_remap));
    }


    // Start recalculating in another thread, organize a callback to redraw when done..


}

void MinutesAtPressure::recalcFinished()
{
    if (m_graph) {
        m_graph->timedRedraw(0);
    }
    m_recalculating = false;
    m_remap = nullptr;
//    QThreadPool * tp = QThreadPool::globalInstance();
//    tp->releaseThread();

}


bool MinutesAtPressure::mouseMoveEvent(QMouseEvent *, gGraph *graph)
{
//    int y = event->y() - m_rect.top();
//    int x = event->x() - graph->graphView()->titleWidth;

//    double w = m_rect.width() - gYAxis::Margin;

//    double xmult = (graph->blockZoom() ? double(graph->rmax_x - graph->rmin_x) : double(graph->max_x - graph->min_x)) / w;

//    double a = x - gYAxis::Margin;
//    if (a < 0) a = 0;
//    if (a > w) a = w;

//    double b = a * xmult;
//    double c= b + (graph->blockZoom() ? graph->rmin_x : graph->min_x);

//    graph->graphView()->setCurrentTime(c);

    graph->timedRedraw(0);
    return false;
}

bool MinutesAtPressure::mousePressEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event);
    Q_UNUSED(graph);
    return false;
}

bool MinutesAtPressure::mouseReleaseEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event);
    Q_UNUSED(graph);
    return false;
}
