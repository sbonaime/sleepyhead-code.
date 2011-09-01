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
    static QColor col1=QColor(0xd0,0xff,0xd0,0xff);
    static QColor col2=QColor(0xff,0xff,0xff,0xff);
    static QColor col=Qt::black;

    addGLBuf(quad1=new GLBuffer(col1,512,GL_QUADS));
    addGLBuf(quad2=new GLBuffer(col2,512,GL_QUADS));
    addGLBuf(lines=new GLBuffer(col,20,GL_LINE_LOOP));
    quad1->setAntiAlias(true);
    quad2->setAntiAlias(true);
    lines->setAntiAlias(false);
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
    for (int i=0;i<layers.size();i++) {
        gFlagsLine *f=dynamic_cast<gFlagsLine *>(layers[i]);
        if (!f) continue;

        if (!f->isEmpty() || f->isAlwaysVisible()) {
            lvisible.push_back(f);
        }
    }

}

void gFlagsGroup::paint(gGraph &w, int left, int top, int width, int height)
{
    if (!m_visible) return;
    if (!m_day) return;

    int vis=lvisible.size();
    float barh=float(height)/float(vis);
    float linetop=top;

    for (int i=0;i<lvisible.size();i++) {
        // Alternating box color
        //QColor * barcol=&col2;
        if (i & 1) {
            quad1->add(left,linetop,left,linetop+barh);
            quad1->add(left+width-1,linetop+barh,left+width-1,linetop);
        } else {
            quad2->add(left,linetop,left,linetop+barh);
            quad2->add(left+width-1,linetop+barh,left+width-1,linetop);
        }

        // Paint the actual flags
        lvisible[i]->paint(w,left,linetop,width,barh);
        linetop+=barh;
    }

    GLBuffer *outlines=w.lines();
    QColor blk=Qt::black;
    outlines->add(left-1, top, left-1, top+height, blk);
    outlines->add(left-1, top+height, left+width,top+height, blk);
    outlines->add(left+width,top+height, left+width, top, blk);
    outlines->add(left+width, top, left-1, top, blk);

    //lines->add(left-1, top, left-1, top+height);
    //lines->add(left+width, top+height, left+width, top);
}

gFlagsLine::gFlagsLine(ChannelID code,QColor flag_color,QString label,bool always_visible,FlagType flt)
:Layer(code),m_label(label),m_always_visible(always_visible),m_flt(flt),m_flag_color(flag_color)
{
    addGLBuf(quads=new GLBuffer(flag_color,2048,GL_QUADS));
    //addGLBuf(lines=new GLBuffer(flag_color,1024,GL_LINES));
    quads->setAntiAlias(true);
    //lines->setAntiAlias(true);
    GetTextExtent(m_label,m_lx,m_ly);
    //m_static.setText(m_label);;
}
gFlagsLine::~gFlagsLine()
{
    //delete lines;
    delete quads;
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


    // Draw text label
    w.renderText(m_label,left-m_lx-10,top+(height/2)+(m_ly/2));

    float x1,x2;

    float bartop=top+2;
    float bottom=top+height-2;
    bool verts_exceeded=false;
    qint64 X,Y;
    for (QVector<Session *>::iterator s=m_day->begin();s!=m_day->end(); s++) {
        if ((*s)->eventlist.find(m_code)==(*s)->eventlist.end()) continue;

        EventList & el=*((*s)->eventlist[m_code][0]);

        for (int i=0;i<el.count();i++) {
            X=el.time(i);
            Y=X-(el.data(i)*1000);
            if (Y < minx) continue;
            if (X > maxx) break;
            x1=(X - minx) * xmult + left;
            if (m_flt==FT_Bar) {
                lines->add(x1,bartop,x1,bottom,m_flag_color);
                if (lines->full()) { verts_exceeded=true; break; }
            } else if (m_flt==FT_Span) {
                x2=(Y-minx)*xmult+left;
                //w1=x2-x1;
                quads->add(x1,bartop,x1,bottom);
                quads->add(x2,bottom,x2,bartop);
                if (quads->full()) { verts_exceeded=true; break; }
            }
        }
        if (verts_exceeded) break;
    }
    if (verts_exceeded) {
        qWarning() << "maxverts exceeded in gFlagsLine::plot()";
    }
}
