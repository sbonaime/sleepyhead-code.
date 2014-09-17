/* gFlagsLine Implementation
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

gLabelArea::gLabelArea(Layer * layer)
    : gSpacer(20)
{
    m_layertype = LT_Spacer;
    m_mainlayer = layer;
}
bool gLabelArea::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    if (m_mainlayer) {
        return m_mainlayer->mouseMoveEvent(event, graph);
    }

    return false;
}
int gLabelArea::minimumWidth()
{
    return gYAxis::Margin;
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
    int cnt = 0;

    if (!d) {
        m_empty = true;
        return;
    }


    quint32 z = schema::FLAG | schema::SPAN;
    if (p_profile->general->showUnknownFlags()) z |= schema::UNKNOWN;
    availableChans = d->getSortedMachineChannels(z);

    m_rebuild_cpap = (availableChans.size() == 0);

    if (m_rebuild_cpap) {
        QHash<ChannelID, schema::Channel *> chans;

        for (int i=0; i< m_day->size(); ++i) {
            Session * sess = m_day->sessions.at(i);
            QHash<ChannelID, QVector<EventList *> >::iterator it;
            for (it = sess->eventlist.begin(); it != sess->eventlist.end(); ++it) {
                ChannelID code = it.key();
                if (chans.contains(code)) continue;

                schema::Channel * chan = &schema::channel[code];

                if (chan->type() == schema::FLAG) {
                    availableChans.push_back(code);
                } else if (chan->type() == schema::MINOR_FLAG) {
                    availableChans.push_back(code);
                } else if (chan->type() == schema::SPAN) {
                    availableChans.push_back(code);
                } else if (chan->type() == schema::UNKNOWN) {
                    availableChans.push_back(code);
                }
            }
        }
        availableChans = chans.keys();
    }

    lvisible.clear();
    for (int i=0; i < availableChans.size(); ++i) {
        ChannelID code = availableChans.at(i);
//        const schema::Channel & chan = schema::channel[code];

        gFlagsLine * fl = new gFlagsLine(code);
        fl->SetDay(d);
        lvisible.push_back(fl);
    }


    cnt = lvisible.size();

//    for (int i = 0; i < layers.size(); i++) {
//        gFlagsLine *f = dynamic_cast<gFlagsLine *>(layers[i]);

//        if (!f) { continue; }

//        bool e = f->isEmpty();

//        if (!e || f->isAlwaysVisible()) {
//            lvisible.push_back(f);

//            if (!e) {
//                cnt++;
//            }
//        }
//    }

    m_empty = (cnt == 0);

    if (m_empty) {
        if (d) {
            m_empty = !d->hasEvents();
        }
    }

    m_barh = 0;
}
bool gFlagsGroup::isEmpty()
{
    if (m_day) {
        if (m_day->hasEnabledSessions() && m_day->hasEvents())
            return false;
    }
    return true;
}

void gFlagsGroup::paint(QPainter &painter, gGraph &g, const QRegion &region)
{
    int left = region.boundingRect().left();
    int top = region.boundingRect().top();
    int width = region.boundingRect().width();
    int height = region.boundingRect().height();

    if (!m_visible) { return; }

    if (!m_day) { return; }


    QVector<gFlagsLine *> visflags;

    for (int i = 0; i < lvisible.size(); i++) {
        if (schema::channel[lvisible.at(i)->code()].enabled())
            visflags.push_back(lvisible.at(i));
    }

    int vis = visflags.size();
    m_barh = float(height) / float(vis);
    float linetop = top;

    QColor barcol;

    for (int i = 0; i < visflags.size(); i++) {
        // Alternating box color
        if (i & 1) { barcol = COLOR_ALT_BG1; }
        else { barcol = COLOR_ALT_BG2; }

        painter.fillRect(left, linetop, width-1, m_barh, QBrush(barcol));

        // Paint the actual flags
        QRect rect(left, linetop, width, m_barh);
        visflags[i]->m_rect = rect;
        visflags[i]->paint(painter, g, QRegion(rect));
        linetop += m_barh;
    }

    painter.setPen(COLOR_Outline);
    painter.drawLine(left - 1, top, left - 1, top + height);
    painter.drawLine(left - 1, top + height, left + width, top + height);
    painter.drawLine(left + width, top + height, left + width, top);
    painter.drawLine(left + width, top, left - 1, top);

    if (m_rebuild_cpap) {

        QString txt = QObject::tr("Database Outdated\nPlease Rebuild CPAP Data");
        painter.setFont(*bigfont);
        painter.setPen(QPen(QColor(0,0,0,32)));
        painter.drawText(region.boundingRect(), Qt::AlignCenter | Qt::AlignVCenter, txt);
    }
}

bool gFlagsGroup::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{

    //if (p_profile->appearance->lineCursorMode()) {
        graph->timedRedraw(0);
   // }

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
                    graph->ToolTip(ttip, event->x()+15, event->y(), TT_AlignLeft);
                    graph->timedRedraw(0);
                }
            }

        }
    }
    return false;
}


gFlagsLine::gFlagsLine(ChannelID code)
    : Layer(code)
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

    schema::Channel & chan = schema::channel[m_code];

    GetTextExtent(chan.label(), m_lx, m_ly);

    // Draw text label
    w.renderText(chan.label(), left - m_lx - 10, top + (height / 2) + (m_ly / 2));

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

    bool hover = false;
    for (QList<Session *>::iterator s = m_day->begin(); s != m_day->end(); s++) {
        if (!(*s)->enabled()) {
            continue;
        }

        drift = ((*s)->type() == MT_CPAP) ? clockdrift : 0;

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

            if (chan.type() == schema::SPAN) {
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

                    brush = QBrush(color);
                    painter.fillRect(x2, bartop, x1-x2, bottom-bartop, brush);
                    if (!w.selectingArea() && !hover && QRect(x2, bartop, x1-x2, bottom-bartop).contains(w.graphView()->currentMousePos())) {
                        hover = true;
                        painter.setPen(QPen(Qt::red,1));

                        painter.drawRect(x2, bartop, x1-x2, bottom-bartop);
                        int x,y;
                        QString lab = QString("%1 (%2)").arg(schema::channel[m_code].fullname()).arg(*dptr);
                        GetTextExtent(lab, x, y);

                        w.ToolTip(lab, x2 - 10, bartop + (3 * w.printScaleY()), TT_AlignRight, p_profile->general->tooltipTimeout());

                    }
                }

            } else { //if (chan.type() == schema::FLAG) {
                ///////////////////////////////////////////////////////////////////////////
                // Draw Event Flag Bars
                ///////////////////////////////////////////////////////////////////////////

                for (int i = 0; i < np; i++) {
                    X = start + *tptr++;

                    if (X > maxx) {
                        break;
                    }

                    x1 = (X - minx) * xmult + left;

                    if (!w.selectingArea() && !hover && QRect(x1-3, bartop-2, 6, bottom-bartop+4).contains(w.graphView()->currentMousePos())) {
                        hover = true;
                        painter.setPen(QPen(Qt::red,1));

                        painter.drawRect(x1-2, bartop-2, 4, bottom-bartop+4);
                        int x,y;
                        QString lab = QString("%1 (%2)").arg(schema::channel[m_code].fullname()).arg(*dptr);
                        GetTextExtent(lab, x, y);

                        w.ToolTip(lab, x1 - 10, bartop + (3 * w.printScaleY()), TT_AlignRight, p_profile->general->tooltipTimeout());
                    }

                    vlines.append(QLine(x1, bartop, x1, bottom));
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
