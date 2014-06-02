/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gSessionTimesChart Implementation
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

    QDate firstday = PROFILE.FirstDay(m_machtype);
    QDate lastday = PROFILE.LastDay(m_machtype);

    m_minx = QDateTime(firstday, QTime(0,0,0)).toMSecsSinceEpoch();
    m_maxx = QDateTime(lastday, QTime(23,59,59)).toMSecsSinceEpoch();

    // Get list of valid day records in supplied date range
    QList<Day *> daylist = PROFILE.getDays(m_machtype, firstday, lastday);

    if (daylist.size() == 0) {
        m_miny = m_maxy = 0;
        return;
    }

    int cnt = 0;
    bool first = true;
    QList<Day *>::iterator end = daylist.end();

    quint32 dn; // day number

    // For each day
    for (QList<Day *>::iterator it = daylist.begin(); it != end; ++it) {
        Day * day = (*it);

        if (day->size() == 0) continue;
        dn = day->first() / 86400000L;

        // For each session
        for (int i=0; i < day->size(); i++) {
            Session * session = (*day)[i];
            if (!session->enabled()) continue;

            // calculate start and end hours
            float start = ((session->first() / 1000L) % 86400) / 3600.0;
            float end = ((session->last() / 1000L) % 86400) / 3600.0;

            // apply tzoffset??

            // update min & max Y values
            if (first) {
                first = false;
                m_miny = start;
                m_maxy = end;
            } else {
                if (start < m_miny) m_miny = start;
                if (end > m_maxy) m_maxy = end;
            }

            sessiontimes[cnt].push_back(TimeSpan(start,end));
        }
    }
}

void gSessionTimesChart::paint(QPainter &painter, gGraph &w, const QRegion &region)
{
    Q_UNUSED(painter)
    Q_UNUSED(w)
    Q_UNUSED(region)

    QMap<quint32, QList<TimeSpan> >::iterator st_end = sessiontimes.end();
    QMap<quint32, QList<TimeSpan> >::iterator it;


    for (it = sessiontimes.begin(); it != st_end; ++it) {
//        int dn = it.key();
        QList<TimeSpan> & st = it.value();
        int stsize = st.size();

        // Skip if empty
        if (stsize == 0) continue;


    }
}

bool gSessionTimesChart::keyPressEvent(QKeyEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return true;
}

bool gSessionTimesChart::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return true;
}

bool gSessionTimesChart::mousePressEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return true;
}

bool gSessionTimesChart::mouseReleaseEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return true;
}

