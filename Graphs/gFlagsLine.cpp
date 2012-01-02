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
    //static QColor col=Qt::black;

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
            if (!e) cnt++;
        }
    }
    if (cnt==0)
        m_empty=true;
    else m_empty=false;
    //if (lvisible.size()==0) m_empty=false;
    m_barh=0;
}

void gFlagsGroup::paint(gGraph &w, int left, int top, int width, int height)
{
    if (!m_visible) return;
    if (!m_day) return;

    int vis=lvisible.size();
    m_barh=float(height)/float(vis);
    float linetop=top;

    static QColor col1=QColor(0xd0,0xff,0xd0,0xff);
    static QColor col2=QColor(0xff,0xff,0xff,0xff);
    QColor * barcol;
    for (int i=0;i<lvisible.size();i++) {
        // Alternating box color
        if (i & 1) barcol=&col1; else barcol=&col2;
        quads->add(left, linetop, left, linetop+m_barh,   left+width-1, linetop+m_barh, left+width-1, linetop, barcol->rgba());

        // Paint the actual flags
        lvisible[i]->paint(w,left,linetop,width,m_barh);
        linetop+=m_barh;
    }

    gVertexBuffer *outlines=w.lines();
    QColor blk=Qt::black;
    outlines->add(left-1, top, left-1, top+height, blk.rgba());
    outlines->add(left-1, top+height, left+width,top+height, blk.rgba());
    outlines->add(left+width,top+height, left+width, top,blk.rgba());
    outlines->add(left+width, top, left-1, top, blk.rgba());

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
    lines->setColor(schema::channel[m_code].defaultColor().rgba());
    for (QVector<Session *>::iterator s=m_day->begin();s!=m_day->end(); s++) {
        if (!(*s)->enabled()) continue;

        if ((*s)->eventlist.find(m_code)==(*s)->eventlist.end()) continue;

        EventList & el=*((*s)->eventlist[m_code][0]);

        for (quint32 i=0;i<el.count();i++) {
            X=el.time(i);
            L=el.data(i)*1000;
            X2=X-L;
            if (X2 < minx) continue;
            if (X > maxx) break;
            x1=(X - minx) * xmult + left;
            if (m_flt==FT_Bar) {
                lines->add(x1,bartop,x1,bottom);
                if (lines->full()) { verts_exceeded=true; break; }
            } else if (m_flt==FT_Span) {
                x2=(X2-minx)*xmult+left;
                //w1=x2-x1;
                /*if (qAbs(x1-x2)<=1) {
                    x1-=1;
                    x2+=1;
                }*/
                quads->add(x2,bartop,x1,bartop, x1,bottom,x2,bottom,m_flag_color.rgba());
                if (quads->full()) { verts_exceeded=true; break; }
            }
        }
        if (verts_exceeded) break;
    }
    if (verts_exceeded) {
        qWarning() << "maxverts exceeded in gFlagsLine::plot()";
    }
}
