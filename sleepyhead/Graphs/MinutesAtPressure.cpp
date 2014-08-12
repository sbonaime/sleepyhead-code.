/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * MinutesAtPressure Graph Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
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


MinutesAtPressure::MinutesAtPressure() :Layer(NoChannel)
{
    m_remap = nullptr;
    m_minpressure = 3;
    m_maxpressure = 30;
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
    if (day) {
        QList<Session *>::iterator sit;
        EventDataType minpressure = 40;
        EventDataType maxpressure = 0;

        Machine * mach = day->machine;
        QMap<QDate, Day *>::iterator it;
        QMap<QDate, Day *>::iterator day_end = mach->day.end();
        // look at overall pressure ranges and find the max

        for (it = mach->day.begin(); it != day_end; ++it) {
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

        m_minpressure = floor(minpressure);
        m_maxpressure = floor(maxpressure);

        const int minimum_cells = 12;
        int c = m_maxpressure - m_minpressure;

        if (c < minimum_cells) {
            int v = minimum_cells - c;
            m_minpressure -= v/2;
            m_minpressure = qMin((EventStoreType)4, m_minpressure);
            m_maxpressure = m_minpressure + minimum_cells;
        }
    }

    m_empty = false;
    m_recalculating = false;
    m_lastminx = 0;
    m_lastmaxx = 0;
    m_empty = !m_day || !(m_day->channelExists(CPAP_Pressure) || m_day->channelExists(CPAP_EPAP));
}


bool MinutesAtPressure::isEmpty()
{

    return m_empty;
}

void MinutesAtPressure::paint(QPainter &painter, gGraph &graph, const QRegion &region)
{
    QRect rect = region.boundingRect();

    float width = rect.width();

    float cells = m_maxpressure-m_minpressure+1;

    float pix = width / cells;

    float left = rect.left();

    m_minx = graph.min_x;
    m_maxx = graph.max_x;

    if ((m_lastminx != m_minx) || (m_lastmaxx != m_maxx)) {
        recalculate(&graph);
    }
    m_lastminx = m_minx;
    m_lastmaxx = m_maxx;

    QMap<EventStoreType, int>::iterator it;
    int top = rect.top();
    painter.setFont(*defaultfont);
    painter.setPen(Qt::black);

    // Lock the stuff we need to draw
    timelock.lock();

    QMap<EventStoreType, int>::iterator times_end = times.end();

    QString text = STR_TR_Pressure;
    QRect rec(left,top, pix * 3,0);
    rec = painter.boundingRect(rec, Qt::AlignTop | Qt::AlignRight, text);
    rec.moveRight(left-4);
    painter.drawText(rec, Qt::AlignRight | Qt::AlignVCenter, text);

    text = STR_UNIT_Minutes;
    QRect rec2(left, top + rec.height(),pix * 3, 0);
    rec2 = painter.boundingRect(rec2, Qt::AlignTop | Qt::AlignRight, text);
    rec2.moveRight(left-4);
    painter.drawText(rec2, Qt::AlignRight | Qt::AlignVCenter, text);

    int xpos = left;
    for (it = times.begin(); it != times_end; ++it) {
        QString text = QString::number(it.key());
        QString value = QString("%1").arg(float(it.value()) / 60.0, 5, 'f', 1);
        QRect rec(xpos, top, pix-1, 0);
        rec = painter.boundingRect(rec, Qt::AlignTop | Qt::AlignLeft, text);
        rec = painter.boundingRect(rec, Qt::AlignTop | Qt::AlignLeft, value);
        rec.setWidth(pix - 1);

        painter.fillRect(rec, QColor("orange"));
        painter.drawText(rec, Qt::AlignCenter, text);
        rec.moveTop(top + rec.height());
        painter.drawText(rec, Qt::AlignCenter, value);

        xpos += pix;
    }

    float hh = rec.height();

    int ypos = top + hh * 2;

    QHash<ChannelID, QMap<EventStoreType, EventDataType> >::iterator eit;
    QHash<ChannelID, QMap<EventStoreType, EventDataType> >::iterator ev_end = events.end();
    QMap<EventStoreType, EventDataType>::iterator vit;

    int row = 0;
    int numchans = chans.size();
    for (int i=0; i< numchans; ++i) {
        ChannelID code = chans.at(i);

        schema::Channel & chan = schema::channel[code];
        schema::ChanType type = chan.type();
        eit = events.find(code);

        xpos = left;

        QMap<EventStoreType, EventDataType>::iterator eit_end = eit.value().end();

        QString text = chan.label();
        QRect rec2(xpos, ypos, pix * 3, hh);
        rec2 = painter.boundingRect(rec2, Qt::AlignTop | Qt::AlignRight, text);
        rec2.moveRight(left-4);
        painter.drawText(rec2, Qt::AlignRight | Qt::AlignVCenter, text);

        for (it = times.begin(), vit = eit.value().begin(); vit != eit_end; ++vit, ++it) {
            float minutes = float(it.value()) / 60.0;
            float value = vit.value();

            if (type != schema::SPAN) {
                value = (minutes > 0.000001) ? (value * 60.0) / minutes : 0;
            } else {
                value = (minutes > 0.000001) ? (100/minutes) * (value / 60.0) : 0;
            }

            QRect rec(xpos, ypos, pix-1, hh);
            if ((row & 1) == 0) {
                painter.fillRect(rec, QColor(245,245,255,240));
            }
            painter.drawText(rec, Qt::AlignCenter, QString("%1").arg(value,5,'f',2));
            xpos += pix;

        }
        ypos += hh;
        row++;
    }

    timelock.unlock();

    if (m_recalculating) {
//        painter.setFont(*defaultfont);
//        painter.setPen(QColor(0,0,0,125));
//        painter.drawText(region.boundingRect(), Qt::AlignCenter, QObject::tr("Recalculating..."));
    }

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

    if (day->channelExists(CPAP_IPAP)) {
        prescode = CPAP_IPAP;
    } else if (day->channelExists(CPAP_EPAP)) {
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
            EventList *EL = evec[ei];
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
                    if (first) {
                        lasttime = time;
                        lastdata = data;
                        first = false;
                    }
                    goto skip;
                }

                if (first) {
                    lasttime = time;
                    lastdata = data;
                    first = false;
                }

                if ((lastdata != data) || (time > maxx)) {
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
                if (time > maxx) break;
skip:
                if (m_quit) {
                    m_done = true;
                    return;
                }
            }

        }
    }


    QMap<EventStoreType, int>::iterator it;
    QMap<EventStoreType, int>::iterator times_end = times.end();
    int maxtime = 0;

    QList<EventStoreType> trash;
    for (it = times.begin(); it != times_end; ++it) {
        EventStoreType key = it.key();
        int value = it.value();
//        if (value == 0) {
//            trash.append(key);
//        } else {
            maxtime = qMax(value, maxtime);
//        }
    }
    chans.push_front(CPAP_AHI);

    for (int i = map->m_minpressure; i <= map->m_maxpressure; i++) {
        events[CPAP_AHI].insert(i,events[CPAP_Obstructive][i] + events[CPAP_Hypopnea][i] + events[CPAP_Apnea][i] + events[CPAP_ClearAirway][i]);
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
    map->chans = chans;
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

    while(!tp->tryStart(m_remap));


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


bool MinutesAtPressure::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event);
    Q_UNUSED(graph);
    return true;
}

bool MinutesAtPressure::mousePressEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event);
    Q_UNUSED(graph);
    return true;
}

bool MinutesAtPressure::mouseReleaseEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event);
    Q_UNUSED(graph);
    return true;
}