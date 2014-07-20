/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gFlagsLine Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <cmath>
#include <QVector>
#include "SleepLib/profiles.h"
#include "gFlagsLine.h"
#include "gYAxis.h"

gFlagsLabelArea::gFlagsLabelArea(gFlagsGroup *group)
    : gSpacer(20)
{
    m_group = group;
}
bool gFlagsLabelArea::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    if (m_group) {
        return m_group->mouseMoveEvent(event, graph);
    }

    return false;
}


gFlagsGroup::gFlagsGroup()
{
    m_barh = 0;
    m_empty = true;
}
gFlagsGroup::~gFlagsGroup()
{
}
qint64 gFlagsGroup::Minx()
{
    if (m_day) {
        return m_day->first();
    }

    return 0;
}
qint64 gFlagsGroup::Maxx()
{
    if (m_day) {
        return m_day->last();
    }

    return 0;
}
void gFlagsGroup::SetDay(Day *d)
{
    LayerGroup::SetDay(d);
    lvisible.clear();
    int cnt = 0;

    for (int i = 0; i < layers.size(); i++) {
        gFlagsLine *f = dynamic_cast<gFlagsLine *>(layers[i]);

        if (!f) { continue; }

        bool e = f->isEmpty();

        if (!e || f->isAlwaysVisible()) {
            lvisible.push_back(f);

            if (!e) {
                cnt++;
            }
        }
    }

    m_empty = (cnt == 0);

    if (m_empty) {
        if (d) {
            m_empty = !d->channelExists(CPAP_Pressure);
        }
    }

    m_barh = 0;
}

void gFlagsGroup::paint(QPainter &painter, gGraph &g, const QRegion &region)
{
    int left = region.boundingRect().left();
    int top = region.boundingRect().top();
    int width = region.boundingRect().width();
    int height = region.boundingRect().height();

    if (!m_visible) { return; }

    if (!m_day) { return; }

    int vis = lvisible.size();
    m_barh = float(height) / float(vis);
    float linetop = top;

    QColor barcol;

    for (int i = 0; i < lvisible.size(); i++) {
        // Alternating box color
        if (i & 1) { barcol = COLOR_ALT_BG1; }
        else { barcol = COLOR_ALT_BG2; }

        painter.fillRect(left, linetop, width-1, m_barh, QBrush(barcol));

        // Paint the actual flags
        QRect rect(left, linetop, width, m_barh);
        lvisible[i]->m_rect = rect;
        lvisible[i]->paint(painter, g, QRegion(rect));
        linetop += m_barh;
    }

    painter.setPen(COLOR_Outline);
    painter.drawLine(left - 1, top, left - 1, top + height);
    painter.drawLine(left - 1, top + height, left + width, top + height);
    painter.drawLine(left + width, top + height, left + width, top);
    painter.drawLine(left + width, top, left - 1, top);
}

bool gFlagsGroup::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{

    if (graph->graphView()->metaSelect())
        graph->timedRedraw(30);

    if (!p_profile->appearance->graphTooltips()) {
        return false;
    }

    for (int i = 0; i < lvisible.size(); i++) {
        gFlagsLine *fl = lvisible[i];

        if (fl->m_rect.contains(event->x(), event->y())) {
            if (fl->mouseMoveEvent(event, graph)) { return true; }
        } else {
            // Inside main graph area?
            if ((event->y() > fl->m_rect.y()) && (event->y()) < (fl->m_rect.y() + fl->m_rect.height())) {
                if (event->x() < lvisible[i]->m_rect.x()) {
                    // Display tooltip
                    QString ttip = schema::channel[fl->code()].fullname() + "\n" +
                                   schema::channel[fl->code()].description();
                    graph->ToolTip(ttip, event->x(), event->y() - 15);
                    graph->timedRedraw(30);
                }
            }

        }
    }

    return false;
}


gFlagsLine::gFlagsLine(ChannelID code, QColor flag_color, QString label, bool always_visible,
                       FlagType flt)
    : Layer(code), m_label(label), m_always_visible(always_visible), m_flt(flt),
      m_flag_color(flag_color)
{
}
gFlagsLine::~gFlagsLine()
{
}
void gFlagsLine::paint(QPainter &painter, gGraph &w, const QRegion &region)
{
    int left = region.boundingRect().left();
    int top = region.boundingRect().top();
    int width = region.boundingRect().width();
    int height = region.boundingRect().height();

    if (!m_visible) { return; }

    if (!m_day) { return; }

    double minx;
    double maxx;

    if (w.blockZoom()) {
        minx = w.rmin_x;
        maxx = w.rmax_x;
    } else {
        minx = w.min_x;
        maxx = w.max_x;
    }

    double xx = maxx - minx;

    if (xx <= 0) { return; }

    double xmult = width / xx;

    GetTextExtent(m_label, m_lx, m_ly);

    // Draw text label
    w.renderText(m_label, left - m_lx - 10, top + (height / 2) + (m_ly / 2));

    float x1, x2;

    float bartop = top + 2;
    float bottom = top + height - 2;
    qint64 X, X2, L;

    qint64 start;
    quint32 *tptr;
    EventStoreType *dptr, * eptr;
    int idx;
    QHash<ChannelID, QVector<EventList *> >::iterator cei;

    qint64 clockdrift = qint64(p_profile->cpap->clockDrift()) * 1000L;
    qint64 drift = 0;

    QVector<QLine> vlines;

    QColor color=schema::channel[m_code].defaultColor();
    QBrush brush(color);

    for (QList<Session *>::iterator s = m_day->begin(); s != m_day->end(); s++) {
        if (!(*s)->enabled()) {
            continue;
        }

        drift = ((*s)->machine()->GetType() == MT_CPAP) ? clockdrift : 0;

        cei = (*s)->eventlist.find(m_code);

        if (cei == (*s)->eventlist.end()) {
            continue;
        }

        QVector<EventList *> &evlist = cei.value();

        for (int k = 0; k < evlist.size(); k++) {
            EventList &el = *(evlist[k]);
            start = el.first() + drift;
            tptr = el.rawTime();
            dptr = el.rawData();
            int np = el.count();
            eptr = dptr + np;

            for (idx = 0; dptr < eptr; dptr++, tptr++, idx++) {
                X = start + *tptr;
                L = *dptr * 1000;

                if (X >= minx) {
                    break;
                }

                X2 = X - L;

                if (X2 >= minx) {
                    break;
                }

            }

            np -= idx;

            if (m_flt == FT_Bar) {
                ///////////////////////////////////////////////////////////////////////////
                // Draw Event Flag Bars
                ///////////////////////////////////////////////////////////////////////////

                for (int i = 0; i < np; i++) {
                    X = start + *tptr++;

                    if (X > maxx) {
                        break;
                    }

                    x1 = (X - minx) * xmult + left;
                    vlines.append(QLine(x1, bartop, x1, bottom));
                }
            } else if (m_flt == FT_Span) {
                ///////////////////////////////////////////////////////////////////////////
                // Draw Event Flag Spans
                ///////////////////////////////////////////////////////////////////////////

                for (; dptr < eptr; dptr++) {
                    X = start + * tptr++;

                    if (X > maxx) {
                        break;
                    }

                    L = *dptr * 1000L;
                    X2 = X - L;

                    x1 = double(X - minx) * xmult + left;
                    x2 = double(X2 - minx) * xmult + left;

                    painter.fillRect(x2, bartop, x1-x2, bottom-bartop, brush);
                }
            }
        }
    }

    painter.setPen(color);
    painter.drawLines(vlines);
}

bool gFlagsLine::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    //  qDebug() << code() << event->x() << event->y() << graph->rect();

    return false;
}
