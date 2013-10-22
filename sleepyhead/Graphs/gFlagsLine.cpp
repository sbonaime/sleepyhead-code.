/*
 gFlagsLine Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <cmath>
#include <QVector>
#include "SleepLib/profiles.h"
#include "gFlagsLine.h"
#include "gYAxis.h"

gFlagsGroup::gFlagsGroup()
{
    addVertexBuffer(quads=new gVertexBuffer(512,GL_QUADS));
    addVertexBuffer(lines=new gVertexBuffer(20,GL_LINE_LOOP));
    quads->setAntiAlias(true);
    lines->setAntiAlias(false);
    m_barh=0;
    m_empty=true;
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
void gFlagsGroup::SetDay(Day * d)
{
    LayerGroup::SetDay(d);
    lvisible.clear();
    int cnt=0;
    for (int i=0;i<layers.size();i++) {
        gFlagsLine *f=dynamic_cast<gFlagsLine *>(layers[i]);
        if (!f) continue;

        bool e=f->isEmpty();
        if (!e || f->isAlwaysVisible()) {
            lvisible.push_back(f);
            if (!e)
               cnt++;
        }
    }
    m_empty=(cnt==0);
    if (m_empty) {
        if (d) {
            m_empty=!d->channelExists(CPAP_Pressure);
        }
    }
    m_barh=0;
}

void gFlagsGroup::paint(gGraph &w, int left, int top, int width, int height)
{
    if (!m_visible) return;
    if (!m_day) return;

    int vis=lvisible.size();
    m_barh=float(height)/float(vis);
    float linetop=top;

    QColor barcol;
    for (int i=0;i<lvisible.size();i++) {
        // Alternating box color
        if (i & 1) barcol=COLOR_ALT_BG1; else barcol=COLOR_ALT_BG2;
        quads->add(left, linetop, left, linetop+m_barh,   left+width-1, linetop+m_barh, left+width-1, linetop, barcol.rgba());

        // Paint the actual flags
        lvisible[i]->paint(w,left,linetop,width,m_barh);
        linetop+=m_barh;
    }

    gVertexBuffer *outlines=w.lines();
    outlines->add(left-1, top, left-1, top+height, COLOR_Outline.rgba());
    outlines->add(left-1, top+height, left+width,top+height, COLOR_Outline.rgba());
    outlines->add(left+width,top+height, left+width, top,COLOR_Outline.rgba());
    outlines->add(left+width, top, left-1, top, COLOR_Outline.rgba());

    //lines->add(left-1, top, left-1, top+height);
    //lines->add(left+width, top+height, left+width, top);
}

gFlagsLine::gFlagsLine(ChannelID code,QColor flag_color,QString label,bool always_visible,FlagType flt)
:Layer(code),m_label(label),m_always_visible(always_visible),m_flt(flt),m_flag_color(flag_color)
{
    addVertexBuffer(quads=new gVertexBuffer(2048,GL_QUADS));
    //addGLBuf(lines=new GLBuffer(flag_color,1024,GL_LINES));
    quads->setAntiAlias(true);
    //lines->setAntiAlias(true);
    //GetTextExtent(m_label,m_lx,m_ly);
    //m_static.setText(m_label);;
}
gFlagsLine::~gFlagsLine()
{
    //delete lines;
    //delete quads;
}
void gFlagsLine::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible) return;
    if (!m_day) return;
    lines=w.lines();
    double minx;
    double maxx;

    if (w.blockZoom()) {
        minx=w.rmin_x;
        maxx=w.rmax_x;
    } else {
        minx=w.min_x;
        maxx=w.max_x;
    }

    double xx=maxx-minx;
    if (xx<=0) return;

    double xmult=width/xx;

    GetTextExtent(m_label,m_lx,m_ly);

    // Draw text label
    w.renderText(m_label,left-m_lx-10,top+(height/2)+(m_ly/2));

    float x1,x2;

    float bartop=top+2;
    float bottom=top+height-2;
    bool verts_exceeded=false;
    qint64 X,X2,L;
    lines->setColor(schema::channel[m_code].defaultColor());

    qint64 start;
    quint32 * tptr;
    EventStoreType *dptr, * eptr;
    int idx;
    QHash<ChannelID,QVector<EventList *> >::iterator cei;

    qint64 clockdrift=qint64(PROFILE.cpap->clockDrift()) * 1000L;
    qint64 drift=0;

    for (QList<Session *>::iterator s=m_day->begin();s!=m_day->end(); s++) {
        if (!(*s)->enabled())
            continue;
        drift=((*s)->machine()->GetType()==MT_CPAP) ? clockdrift : 0;

        cei=(*s)->eventlist.find(m_code);
        if (cei==(*s)->eventlist.end())
            continue;

        QVector<EventList *> & evlist=cei.value();
        for (int k=0;k<evlist.size();k++) {
            EventList & el=*(evlist[k]);
            start=el.first() + drift;
            tptr=el.rawTime();
            dptr=el.rawData();
            int np=el.count();
            eptr=dptr+np;

            for (idx=0;dptr < eptr; dptr++, tptr++, idx++) {
                X=start + *tptr;
                L=*dptr * 1000;
                if (X >= minx)
                    break;
                X2=X-L;
                if (X2 >= minx)
                    break;

            }
            np-=idx;

            if (m_flt==FT_Bar) {
                ///////////////////////////////////////////////////////////////////////////
                // Draw Event Flag Bars
                ///////////////////////////////////////////////////////////////////////////

                // Check bounds outside of loop is faster..
                // This will have to be reverted if multithreaded drawing is ever brought back

                int rem=lines->Max() - lines->cnt();
                if ((np<<1) > rem) {
                    qDebug() << "gFlagsLine would overfill lines for" << schema::channel[m_code].label();
                    np=rem >> 1;
                    verts_exceeded=true;
                }

                for (int i=0;i<np;i++) {
                    X=start + *tptr++;

                    if (X > maxx)
                        break;

                    x1=(X - minx) * xmult + left;
                    lines->add(x1,bartop,x1,bottom);

                    //if (lines->full()) { verts_exceeded=true; break; }
                }
            } else if (m_flt==FT_Span) {
                ///////////////////////////////////////////////////////////////////////////
                // Draw Event Flag Spans
                ///////////////////////////////////////////////////////////////////////////
                quads->setColor(m_flag_color);
                int rem=quads->Max() - quads->cnt();

                if ((np<<2) > rem) {
                    qDebug() << "gFlagsLine would overfill quads for" << schema::channel[m_code].label();
                    np=rem >> 2;
                    verts_exceeded=true;
                }

                for (; dptr < eptr; dptr++) {
                    X=start + * tptr++;

                    if (X > maxx)
                        break;

                    L=*dptr * 1000L;
                    X2=X-L;

                    x1=double(X - minx) * xmult + left;
                    x2=double(X2 - minx) * xmult + left;

                    quads->add(x2,bartop,x1,bartop, x1,bottom,x2,bottom);
                    //if (quads->full()) { verts_exceeded=true; break; }

                }
            }
            if (verts_exceeded) break;
        }
        if (verts_exceeded) break;
    }
    if (verts_exceeded) {
        qWarning() << "maxverts exceeded in gFlagsLine::plot()";
    }
}
