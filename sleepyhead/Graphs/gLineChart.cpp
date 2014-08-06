/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gLineChart Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "Graphs/gLineChart.h"

#include <QString>
#include <QDebug>

#include <math.h>

#include "Graphs/glcommon.h"
#include "Graphs/gGraph.h"
#include "Graphs/gGraphView.h"
#include "SleepLib/profiles.h"
#include "Graphs/gLineOverlay.h"

#define EXTRA_ASSERTS 1
gLineChart::gLineChart(ChannelID code, QColor col, bool square_plot, bool disable_accel)
    : Layer(code), m_square_plot(square_plot), m_disable_accel(disable_accel)
{
    addPlot(code, col, square_plot);
    m_line_color = col;
    m_report_empty = false;
    lines.reserve(50000);
    lasttime = 0;
}
gLineChart::~gLineChart()
{
    QHash<ChannelID, gLineOverlayBar *>::iterator fit;
    for (fit = flags.begin(); fit != flags.end(); ++fit) {
        // destroy any overlay bar from previous day
        delete fit.value();
    }

    flags.clear();

}

bool gLineChart::isEmpty()
{
    if (!m_day) { return true; }

    for (int j = 0; j < m_codes.size(); j++) {
        ChannelID code = m_codes[j];

        for (int i = 0; i < m_day->size(); i++) {
            Session *sess = m_day->sessions[i];

            if (sess->channelExists(code)) {
                return false;
            }
        }
    }

    return true;
}

void gLineChart::SetDay(Day *d)
{
    //    Layer::SetDay(d);
    m_day = d;

    m_minx = 0, m_maxx = 0;
    m_miny = 0, m_maxy = 0;
    m_physminy = 0, m_physmaxy = 0;

    if (!d) {
        return;
    }

    qint64 t64;
    EventDataType tmp;

    bool first = true;

    for (int j = 0; j < m_codes.size(); j++) {
        ChannelID code = m_codes[j];

        for (int i = 0; i < d->size(); i++) {
            Session *sess = d->sessions[i];

            if (code == CPAP_MaskPressure) {
                if (sess->channelExists(CPAP_MaskPressureHi)) {
                    code = m_codes[j] = CPAP_MaskPressureHi;
                    goto skipcheck; // why not :P
                }
            }

            if (!sess->channelExists(code)) {
                continue;
            }

skipcheck:
            if (first) {
                m_miny = sess->Min(code);
                m_maxy = sess->Max(code);
                m_physminy = sess->physMin(code);
                m_physmaxy = sess->physMax(code);
                m_minx = sess->first(code);
                m_maxx = sess->last(code);
                first = false;
            } else {
                tmp = sess->physMin(code);

                if (m_physminy > tmp) {
                    m_physminy = tmp;
                }

                tmp = sess->physMax(code);

                if (m_physmaxy < tmp) {
                    m_physmaxy = tmp;
                }

                tmp = sess->Min(code);

                if (m_miny > tmp) {
                    m_miny = tmp;
                }

                tmp = sess->Max(code);

                if (m_maxy < tmp) {
                    m_maxy = tmp;
                }

                t64 = sess->first(code);

                if (m_minx > t64) {
                    m_minx = t64;
                }

                t64 = sess->last(code);

                if (m_maxx < t64) {
                    m_maxx = t64;
                }
            }
        }

    }

    subtract_offset = 0;

    QHash<ChannelID, gLineOverlayBar *>::iterator fit;
    for (fit = flags.begin(); fit != flags.end(); ++fit) {
        // destroy any overlay bar from previous day
        delete fit.value();
    }

    flags.clear();

    for (int i=0; i< m_day->size(); ++i) {
        Session * sess = m_day->sessions.at(i);
        QHash<ChannelID, QVector<EventList *> >::iterator it;
        for (it = sess->eventlist.begin(); it != sess->eventlist.end(); ++it) {
            ChannelID code = it.key();

            if (flags.contains(code)) continue;

            schema::Channel * chan = &schema::channel[code];
            gLineOverlayBar * lob = nullptr;

            if (chan->type() == schema::FLAG) {
                lob = new gLineOverlayBar(code, chan->defaultColor(), chan->label(), FT_Bar);
            } else if (chan->type() == schema::MINOR_FLAG) {
                lob = new gLineOverlayBar(code, chan->defaultColor(), chan->label(), FT_Dot);
            } else if (chan->type() == schema::SPAN) {
                lob = new gLineOverlayBar(code, chan->defaultColor(), chan->label(), FT_Span);
            }
            if (lob != nullptr) {
                lob->setOverlayDisplayType((m_codes[0] == CPAP_FlowRate) ? (OverlayDisplayType)p_profile->appearance->overlayType() : ODT_TopAndBottom);
                lob->SetDay(m_day);
                flags[code] = lob;
            }
        }
    }
}
EventDataType gLineChart::Miny()
{
    int m = Layer::Miny();

    if (subtract_offset > 0) {
        m -= subtract_offset;

        if (m < 0) { m = 0; }
    }

    return m;
}
EventDataType gLineChart::Maxy()
{
    return Layer::Maxy() - subtract_offset;
}

bool gLineChart::mouseMoveEvent(QMouseEvent *event, gGraph *graph)
{
    Q_UNUSED(event)
    Q_UNUSED(graph)
    return true;
}

QString gLineChart::getMetaString(qint64 time)
{
    lasttext = QString();
    if (!m_day) return lasttext;


    EventDataType val;

    EventDataType ipap = 0, epap = 0;
    bool addPS = false;

    for (int i=0; i<m_codes.size(); ++i) {
        ChannelID code = m_codes[i];
        if (m_day->channelHasData(code)) {
            val = m_day->lookupValue(code, time, m_square_plot);
            lasttext += " "+QString("%1: %2 %3").arg(schema::channel[code].label()).arg(val,0,'f',2).arg(schema::channel[code].units());

            if (code == CPAP_IPAP) {
                ipap = val;
                addPS = true;
            }
            if (code == CPAP_EPAP) {
                epap = val;
            }
        }

    }
    if (addPS) {
        val = ipap - epap;
        lasttext += " "+QString("%1: %2 %3").arg(schema::channel[CPAP_PS].label()).arg(val,0,'f',2).arg(schema::channel[CPAP_PS].units());
    }

    lasttime = time;
    return lasttext;
}

// Time Domain Line Chart
void gLineChart::paint(QPainter &painter, gGraph &w, const QRegion &region)
{
    QRect rect = region.boundingRect();
    // TODO: Just use QRect directly.
    int left = rect.left();
    int top = rect.top();
    int width = rect.width();
    int height = rect.height();

    if (!m_visible) {
        return;
    }

    if (!m_day) {
        return;
    }

    //if (!m_day->channelExists(m_code)) return;

    if (width < 0) {
        return;
    }


    top++;

    double minx, maxx;


    if (w.blockZoom()) {
        minx = w.rmin_x, maxx = w.rmax_x;
    } else {
        maxx = w.max_x, minx = w.min_x;
    }

    // hmmm.. subtract_offset..

    EventDataType miny = m_physminy;
    EventDataType maxy = m_physmaxy;

    w.roundY(miny, maxy);


//#define DEBUG_AUTOSCALER
#ifdef DEBUG_AUTOSCALER
    QString a = QString().sprintf("%.2f - %.2f",miny, maxy);
    w.renderText(a,width/2,top-5);
#endif

    // the middle of minx and maxy does not have to be the center...


    double xx = maxx - minx;
    double xmult = double(width) / xx;

    EventDataType yy = maxy - miny;
    EventDataType ymult = EventDataType(height - 3) / yy; // time to pixel conversion multiplier

    // Return on screwy min/max conditions
    if (xx < 0) {
        return;
    }

    if (yy <= 0) {
        if (miny == 0) {
            return;
        }
    }

    bool mouseover = false;
    if (rect.contains(w.graphView()->currentMousePos())) {
        mouseover = true;

        painter.fillRect(rect, QBrush(QColor(255,255,245,128)));
    }


    // Display Line Cursor
    if (p_profile->appearance->lineCursorMode()) {
        double time = w.currentTime();

        if ((time > minx) && (time < maxx)) {
            double xpos = (time - double(minx)) * xmult;
            painter.setPen(QPen(QBrush(QColor(0,255,0,255)),1));
            painter.drawLine(left+xpos, top-w.marginTop()-3, left+xpos, top+height+w.bottom-1);
        }

        if ((time != lasttime) || lasttext.isEmpty()) {
            getMetaString(time);
        }

        QDateTime dt=QDateTime::fromMSecsSinceEpoch(time);
        QString text = dt.toString("MMM dd - HH:mm:ss:zzz") + lasttext;

        int wid, h;
        GetTextExtent(text, wid, h);
        w.renderText(text, left + width/2 - wid/2, top-h+5);

    }

    EventDataType lastpx, lastpy;
    EventDataType px, py;
    int idx;
    bool done;
    double x0, xL;
    double sr;
    int sam;
    int minz, maxz;

    // Draw bounding box
    painter.setPen(QColor(Qt::black));
    painter.drawLine(left, top, left, top + height);
    painter.drawLine(left, top + height, left + width, top + height);
    painter.drawLine(left + width, top + height, left + width, top);
    painter.drawLine(left + width, top, left, top);

    width--;
    height -= 2;

    int num_points = 0;
    int visible_points = 0;
    int total_points = 0;
    int total_visible = 0;
    bool square_plot, accel;
    qint64 clockdrift = qint64(p_profile->cpap->clockDrift()) * 1000L;
    qint64 drift = 0;

    QHash<ChannelID, QVector<EventList *> >::iterator ci;

    //m_line_color=schema::channel[m_code].defaultColor();
    int legendx = left + width;

    int codepoints;

    painter.setClipRect(left, top, width, height+1);
    painter.setClipping(true);
    painter.setRenderHint(QPainter::Antialiasing, p_profile->appearance->antiAliasing());


    for (int gi = 0; gi < m_codes.size(); gi++) {
        ChannelID code = m_codes[gi];
        schema::Channel &chan = schema::channel[code];

        if (chan.upperThreshold() > 0) {
            QColor color = chan.upperThresholdColor();
            color.setAlpha(100);
            painter.setPen(color);

            EventDataType y=top + height + 1 - ((chan.upperThreshold() - miny) * ymult);
            painter.drawLine(left + 1, y, left + 1 + width, y);
        }
        if (chan.lowerThreshold() > 0) {
            QColor color = chan.lowerThresholdColor();
            color.setAlpha(100);
            painter.setPen(color);

            EventDataType y=top + height + 1 - ((chan.lowerThreshold() - miny) * ymult);
            painter.drawLine(left+1, y, left + 1 + width, y);
        }

        lines.clear();

        codepoints = 0;

        int daysize = m_day->size();
        for (int svi = 0; svi < daysize; svi++) {
            Session *sess = (*m_day)[svi];

            if (!sess) {
                qWarning() << "gLineChart::Plot() nullptr Session Record.. This should not happen";
                continue;
            }

            drift = (sess->machine()->type() == MT_CPAP) ? clockdrift : 0;

            if (!sess->enabled()) { continue; }

            schema::Channel ch = schema::channel[code];
            bool fndbetter = false;

            QList<schema::Channel *>::iterator mlend=ch.m_links.end();
            for (QList<schema::Channel *>::iterator l = ch.m_links.begin(); l != mlend; l++) {
                schema::Channel &c = *(*l);
                ci = (*m_day)[svi]->eventlist.find(c.id());

                if (ci != (*m_day)[svi]->eventlist.end()) {
                    fndbetter = true;
                    break;
                }

            }

            if (!fndbetter) {
                ci = (*m_day)[svi]->eventlist.find(code);

                if (ci == (*m_day)[svi]->eventlist.end()) { continue; }
            }


            QVector<EventList *> &evec = ci.value();
            num_points = 0;
            int evecsize=evec.size();

            for (int i = 0; i < evecsize; i++) {
                num_points += evec[i]->count();
            }

            total_points += num_points;
            codepoints += num_points;

            // Max number of samples taken from samples per pixel for better min/max values
            const int num_averages = 20;

            for (int n = 0; n < evecsize; ++n) { // for each segment
                EventList &el = *evec[n];

                accel = (el.type() == EVL_Waveform); // Turn on acceleration if this is a waveform.

                if (accel) {
                    sr = el.rate();         // Time distance between samples

                    if (sr <= 0) {
                        qWarning() << "qLineChart::Plot() assert(sr>0)";
                        continue;
                    }
                }

                if (m_disable_accel) { accel = false; }


                square_plot = m_square_plot;

                if (accel || num_points > 20000) { // Don't square plot if too many points or waveform
                    square_plot = false;
                }

                int siz = evec[n]->count();

                if (siz <= 1) { continue; } // Don't bother drawing 1 point or less.

                x0 = el.time(0) + drift;
                xL = el.time(siz - 1) + drift;

                if (maxx < x0) { continue; }

                if (xL < minx) { continue; }

                if (x0 > xL) {
                    if (siz == 2) { // this happens on CPAP
                        quint32 t = el.getTime()[0];
                        el.getTime()[0] = el.getTime()[1];
                        el.getTime()[1] = t;
                        EventStoreType d = el.getData()[0];
                        el.getData()[0] = el.getData()[1];
                        el.getData()[1] = d;

                    } else {
                        qDebug() << "Reversed order sample fed to gLineChart - ignored.";
                        continue;
                        //assert(x1<x2);
                    }
                }

                if (accel) {
                    //x1=el.time(1);

                    double XR = xx / sr;
                    double Z1 = MAX(x0, minx);
                    double Z2 = MIN(xL, maxx);
                    double ZD = Z2 - Z1;
                    double ZR = ZD / sr;
                    double ZQ = ZR / XR;
                    double ZW = ZR / (width * ZQ);
                    visible_points += ZR * ZQ;

                    if (accel && n > 0) {
                        sam = 1;
                    }

                    if (ZW < num_averages) {
                        sam = 1;
                        accel = false;
                    } else {
                        sam = ZW / num_averages;

                        if (sam < 1) {
                            sam = 1;
                            accel = false;
                        }
                    }

                    // Prepare the min max y values if we still are accelerating this plot
                    if (accel) {
                        for (int i = 0; i < width; i++) {
                            m_drawlist[i].setX(height);
                            m_drawlist[i].setY(0);
                        }

                        minz = width;
                        maxz = 0;
                    }

                    total_visible += visible_points;
                } else {
                    sam = 1;
                }

                // these calculations over estimate
                // The Z? values are much more accurate

                idx = 0;

                if (el.type() == EVL_Waveform)  {
                    // We can skip data previous to minx if this is a waveform

                    if (minx > x0) {
                        double j = minx - x0; // == starting min of first sample in this segment
                        idx = (j / sr);
                        //idx/=(sam*num_averages);
                        //idx*=(sam*num_averages);
                        // Loose the precision
                        idx += sam - (idx % sam);

                    } // else just start from the beginning
                }

                int xst = left + 1;
                int yst = top + height + 1;

                double time;
                EventDataType data;
                EventDataType gain = el.gain();

                done = false;

                if (el.type() == EVL_Waveform) { // Waveform Plot
                    if (idx > sam) { idx -= sam; }

                    time = el.time(idx) + drift;
                    double rate = double(sr) * double(sam);
                    EventStoreType *ptr = el.rawData() + idx;

                    if (accel) {
                        //////////////////////////////////////////////////////////////////
                        // Accelerated Waveform Plot
                        //////////////////////////////////////////////////////////////////

                        for (int i = idx; i < siz; i += sam, ptr += sam) {
                            time += rate;
                            // This is much faster than QVector access.
                            data = *ptr + el.offset();
                            data *= gain;

                            // Scale the time scale X to pixel scale X
                            px = ((time - minx) * xmult);

                            // Same for Y scale, with gain factored in nmult
                            py = ((data - miny) * ymult);

                            // In accel mode, each pixel has a min/max Y value.
                            // m_drawlist's index is the pixel index for the X pixel axis.
                            int z = round(px); // Hmmm... round may screw this up.

                            if (z < minz) {
                                minz = z;    // minz=First pixel
                            }

                            if (z > maxz) {
                                maxz = z;    // maxz=Last pixel
                            }

                            if (minz < 0) {
                                qDebug() << "gLineChart::Plot() minz<0  should never happen!! minz =" << minz;
                                minz = 0;
                            }

                            if (maxz > max_drawlist_size) {
                                qDebug() << "gLineChart::Plot() maxz>max_drawlist_size!!!! maxz = " << maxz <<
                                         " max_drawlist_size =" << max_drawlist_size;
                                maxz = max_drawlist_size;
                            }

                            // Update the Y pixel bounds.
                            if (py < m_drawlist[z].x()) {
                                m_drawlist[z].setX(py);
                            }

                            if (py > m_drawlist[z].y()) {
                                m_drawlist[z].setY(py);
                            }

                            if (time > maxx) {
                                done = true;
                                break;
                            }

                        }

                        // Plot compressed accelerated vertex list
                        if (maxz > width) {
                            maxz = width;
                        }

                        float ax1, ay1;
                        QPoint *drl = m_drawlist + minz;
                        // Don't need to cap VertexBuffer here, as it's limited to max_drawlist_size anyway


                        // Cap within VertexBuffer capacity, one vertex per line point
//                        int np = (maxz - minz) * 2;

                        for (int i = minz; i < maxz; i++, drl++) {
                            ax1 = drl->x();
                            ay1 = drl->y();
                            lines.append(QLine(xst + i, yst - ax1, xst + i, yst - ay1));
                        }

                    } else { // Zoomed in Waveform
                        //////////////////////////////////////////////////////////////////
                        // Normal Waveform Plot
                        //////////////////////////////////////////////////////////////////

                        // Prime first point
                        data = (*ptr + el.offset()) * gain;
                        lastpx = xst + ((time - minx) * xmult);
                        lastpy = yst - ((data - miny) * ymult);

                        siz--;
                        for (int i = idx; i < siz; i += sam) {
                            ptr += sam;
                            time += rate;

                            data = (*ptr + el.offset()) * gain;

                            px = xst + ((time - minx) * xmult); // Scale the time scale X to pixel scale X
                            py = yst - ((data - miny) * ymult); // Same for Y scale, with precomputed gain
                            //py=yst-((data - ymin) * nmult);   // Same for Y scale, with precomputed gain

                            lines.append(QLine(lastpx, lastpy, px, py));

                            lastpx = px;
                            lastpy = py;

                            if (time > maxx) {
                                done = true;
                                break;
                            }
                        }
                    }

                    painter.setPen(QPen(chan.defaultColor(),p_profile->appearance->lineThickness()));
                    painter.drawLines(lines);
                    w.graphView()->lines_drawn_this_frame += lines.count();
                    lines.clear();


                } else  {
                    //////////////////////////////////////////////////////////////////
                    // Standard events/zoomed in Plot
                    //////////////////////////////////////////////////////////////////

                    double start = el.first() + drift;

                    quint32 *tptr = el.rawTime();

                    int idx = 0;

                    if (siz > 15) {

                        for (; idx < siz; ++idx) {
                            time = start + *tptr++;

                            if (time >= minx) {
                                break;
                            }
                        }

                        if (idx > 0) {
                            idx--;
                        }
                    }

                    // Step one backwards if possible (to draw through the left margin)
                    EventStoreType *dptr = el.rawData() + idx;
                    tptr = el.rawTime() + idx;

                    time = start + *tptr++;
                    data = (*dptr++ + el.offset()) * gain;

                    idx++;

                    lastpx = xst + ((time - minx) * xmult); // Scale the time scale X to pixel scale X
                    lastpy = yst - ((data - miny) * ymult); // Same for Y scale without precomputed gain

                    siz -= idx;

                    int gs = siz << 1;

                    if (square_plot) {
                        gs <<= 1;
                    }

                    // Unrolling square plot outside of loop to gain a minor speed improvement.
                    EventStoreType *eptr = dptr + siz;

                    if (square_plot) {
                        for (; dptr < eptr; dptr++) {
                            time = start + *tptr++;
                            data = gain * (*dptr + el.offset());

                            px = xst + ((time - minx) * xmult); // Scale the time scale X to pixel scale X
                            py = yst - ((data - miny) * ymult); // Same for Y scale without precomputed gain

                            // Horizontal lines are easy to cap
                            if (py == lastpy) {
                                // Cap px to left margin
                                if (lastpx < xst) { lastpx = xst; }

                                // Cap px to right margin
                                if (px > xst + width) { px = xst + width; }

//                                lines.append(QLine(lastpx, lastpy, px, lastpy));
//                                lines.append(QLine(px, lastpy, px, py));
                            } // else {
                                // Letting the scissor do the dirty work for non horizontal lines
                                // This really should be changed, as it might be cause that weird
                                // display glitch on Linux..

                                lines.append(QLine(lastpx, lastpy, px, lastpy));
                                lines.append(QLine(px, lastpy, px, py));
//                            }

                            lastpx = px;
                            lastpy = py;

                            if (time > maxx) {
                                done = true; // Let this iteration finish.. (This point will be in far clipping)
                                break;
                            }
                        }
                    } else {
                        for (; dptr < eptr; dptr++) {
                            //for (int i=0;i<siz;i++) {
                            time = start + *tptr++;
                            data = gain * (*dptr + el.offset());

                            px = xst + ((time - minx) * xmult); // Scale the time scale X to pixel scale X
                            py = yst - ((data - miny) * ymult); // Same for Y scale without precomputed gain

                            // Horizontal lines are easy to cap
                            if (py == lastpy) {
                                // Cap px to left margin
                                if (lastpx < xst) { lastpx = xst; }

                                // Cap px to right margin
                                if (px > xst + width) { px = xst + width; }

                              //  lines.append(QLine(lastpx, lastpy, px, py));
                            } //else {
                                // Letting the scissor do the dirty work for non horizontal lines
                                // This really should be changed, as it might be cause that weird
                                // display glitch on Linux..
                                lines.append(QLine(lastpx, lastpy, px, py));
                            //}

                            lastpx = px;
                            lastpy = py;

                            if (time > maxx) { // Past right edge, abort further drawing..
                                done = true;
                                break;
                            }
                        }
                    }
                    painter.setPen(QPen(chan.defaultColor(),p_profile->appearance->lineThickness()));
                    painter.drawLines(lines);
                    w.graphView()->lines_drawn_this_frame+=lines.count();
                    lines.clear();

                }

                if (done) { break; }
            }
        }


//        painter.setPen(QPen(m_colors[gi],p_profile->appearance->lineThickness()));
//        painter.drawLines(lines);
//        w.graphView()->lines_drawn_this_frame+=lines.count();
//        lines.clear();

        ////////////////////////////////////////////////////////////////////
        // Draw Legends on the top line
        ////////////////////////////////////////////////////////////////////

        QFontMetrics fm(*defaultfont);
        int bw = fm.width('X');
        int bh = fm.height() / 1.8;


        if ((codepoints > 0)) {
            QString text = schema::channel[code].label();
            int wid, hi;
            GetTextExtent(text, wid, hi);
            legendx -= wid;
            painter.setClipping(false);
            w.renderText(text, legendx, top - 4);
            legendx -= bw /2;
            painter.fillRect(legendx - bw, top - w.marginTop()-2, bh, w.marginTop()+1, QBrush(chan.defaultColor()));
            painter.setClipping(true);

            legendx -= bw*2;
        }
    }


    if (!total_points) { // No Data?

        if (m_report_empty) {
            QString msg = QObject::tr("No Waveform Available");
            int x, y;
            GetTextExtent(msg, x, y, bigfont);
            //DrawText(w,msg,left+(width/2.0)-(x/2.0),scry-w.GetBottomMargin()-height/2.0+y/2.0,0,Qt::gray,bigfont);
        }
    }
    painter.setClipping(false);
    if (m_day && (p_profile->appearance->lineCursorMode() || (m_codes[0]==CPAP_FlowRate))) {
        QHash<ChannelID, gLineOverlayBar *>::iterator fit;
        bool blockhover = false;

        for (fit = flags.begin(); fit != flags.end(); ++fit) {
            gLineOverlayBar * lob = fit.value();
            lob->setBlockHover(blockhover);
            lob->paint(painter, w, region);
            if (lob->hover()) blockhover = true; // did it render a hover over?
        }
    }
    painter.setRenderHint(QPainter::Antialiasing, false);
}
