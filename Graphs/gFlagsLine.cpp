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

void gFlagsGroup::paint(gGraph &w, int left, int top, int width, int height)
{
    if (!m_visible) return;
    //if (!m_day) return;

    QVector<gFlagsLine *> lvisible;
    for (int i=0;i<layers.size();i++) {
        gFlagsLine *f=dynamic_cast<gFlagsLine *>(layers[i]);
        if (!f) continue;

        if (!f->isEmpty() || f->isAlwaysVisible()) {
            lvisible.push_back(f);
        }
    }
    int vis=lvisible.size();
    float barh=float(height)/float(vis);
    float linetop=top;

    static QColor col1=QColor(0xd0,0xff,0xd0,0xff);
    static QColor col2=QColor(0xff,0xff,0xff,0xff);

    for (int i=0;i<lvisible.size();i++) {
        // Alternating box color
        QColor * barcol=&col2;
        if (i & 1)
            barcol=&col1;

        //int qo=0;
        //if (evil_intel_graphics_card) qo=1;

        // Draw the bars with filled quads
        glBegin(GL_QUADS);
        w.qglColor(*barcol);
        glVertex2f(left, linetop);
        glVertex2f(left, linetop+barh);
        glVertex2f(left+width-1, linetop+barh);
        glVertex2f(left+width-1, linetop);
        glEnd();

        // Paint the actual flags
        lvisible[i]->paint(w,left,linetop,width,barh);
        linetop+=barh;
    }

    // Draw the outer rectangle outline
    glBegin(GL_LINE_LOOP);
    glLineWidth(1);
    w.qglColor(Qt::black);
    glVertex2f(left-1, top);
    glVertex2f(left-1, top+height);
    glVertex2f(left+width, top+height);
    glVertex2f(left+width, top);
    glEnd ();

}


gFlagsLine::gFlagsLine(ChannelID code,QColor flag_color,QString label,bool always_visible,FlagType flt)
:Layer(code),m_label(label),m_always_visible(always_visible),m_flt(flt),m_flag_color(flag_color)
{
    addGLBuf(quads=new GLBuffer(flag_color,2048,GL_QUADS));
    addGLBuf(lines=new GLBuffer(flag_color,2048,GL_LINES));
    quads->setAntiAlias(true);
    lines->setAntiAlias(true);
}
gFlagsLine::~gFlagsLine()
{
    delete lines;
    delete quads;
}
void gFlagsLine::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible) return;
    if (!m_day) return;

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
    int x,y;
    GetTextExtent(m_label,x,y);
    w.renderText(m_label,left-x-10,top+(height/2)+(y/2));
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
                lines->add(x1,bartop,x1,bottom);
                if (lines->full()) { verts_exceeded=true; break; }
            } else if (m_flt==FT_Span) {
                x2=(Y-minx)*xmult+left;
                //w1=x2-x1;
                quads->add(x1,bartop,x1,bottom);
                quads->add(x2,bottom,x2,bartop);
                if (quads->full()) { verts_exceeded=true; break; }
            }
        }
    }
    if (verts_exceeded) {
        qWarning() << "maxverts exceeded in gFlagsLine::plot()";
    }
   // glScissor(left,top,width,height);
    //glEnable(GL_SCISSOR_TEST);


    //quads->draw();
    //lines->draw();
    /*glEnableClientState(GL_VERTEX_ARRAY);

    bool antialias=pref["UseAntiAliasing"].toBool();
    if (antialias) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);
        glLineWidth (1.5);
    } else glLineWidth (1);

    w.qglColor(m_flag_color);
    if (quadcnt>0) {
        glVertexPointer(2, GL_SHORT, 0, quadarray);
        glDrawArrays(GL_QUADS, 0, quadcnt>>1);
    }
    if (vertcnt>0) {
        glVertexPointer(2, GL_SHORT, 0, vertarray);
        glDrawArrays(GL_LINES, 0, vertcnt>>1);
    }
    if (antialias) {
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_BLEND);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
*/
    //glDisable(GL_SCISSOR_TEST);
}
