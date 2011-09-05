/*
 gLineOverlayBar Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include "SleepLib/profiles.h"
#include "gLineOverlay.h"

gLineOverlayBar::gLineOverlayBar(ChannelID code,QColor color,QString label,FlagType flt)
:Layer(code),m_flag_color(color),m_label(label),m_flt(flt)
{
    addGLBuf(points=new GLBuffer(color,1024,GL_POINTS));
    points->setSize(4);
    addGLBuf(quads=new GLBuffer(color,2048,GL_QUADS));
    //addGLBuf(lines=new GLBuffer(color,1024,GL_LINES));
    points->setAntiAlias(true);
    quads->setAntiAlias(true);
    //lines->setAntiAlias(true);
}
gLineOverlayBar::~gLineOverlayBar()
{
    //delete lines;
    delete quads;
    delete points;
}

void gLineOverlayBar::paint(gGraph & w, int left, int topp, int width, int height)
{
    if (!m_visible) return;
    if (!m_day) return;

    lines=w.lines();
    int start_py=topp;

    double xx=w.max_x-w.min_x;
    double yy=w.max_y-w.min_y;
    if (xx<=0) return;

    float x1,x2;

    int x,y;

    float bottom=start_py+height-25, top=start_py+25;

    double X;
    double Y;

    bool verts_exceeded=false;
    QHash<ChannelID,QVector<EventList *> >::iterator cei;

    for (QVector<Session *>::iterator s=m_day->begin();s!=m_day->end(); s++) {
        cei=(*s)->eventlist.find(m_code);
        if (cei==(*s)->eventlist.end()) continue;
        if (cei.value().size()==0) continue;

        EventList & el=*cei.value()[0];

        for (int i=0;i<el.count();i++) {
            X=el.time(i);
            if (m_flt==FT_Span) {
                Y=X-(qint64(el.raw(i))*1000.0L); // duration

                if (X < w.min_x) continue;
                if (Y > w.max_x) break;
            } else {
                if (X < w.min_x) continue;
                if (X > w.max_x) break;
            }

            //x1=w.x2p(X);
            x1=double(width)/double(xx)*double(X-w.min_x)+left;
            if (m_flt==FT_Span) {
                //x2=w.x2p(Y);
                x2=double(width)/double(xx)*double(Y-w.min_x)+left;
                if (x2<left) x2=left;
                if (x1>width+left) x1=width+left;
                //double w1=x2-x1;
                quads->add(x1,start_py,x2,start_py);
                quads->add(x2,start_py+height,x1,start_py+height);
                if (quads->full()) { verts_exceeded=true; break; }
            } else if (m_flt==FT_Dot) {
                if (pref["AlwaysShowOverlayBars"].toBool() || (xx<3600000)) {
                    // show the fat dots in the middle
                    points->add(x1,double(height)/double(yy)*double(-20-w.min_y)+topp);
                    if (points->full()) { verts_exceeded=true; break; }
                } else {
                    // thin lines down the bottom
                    lines->add(x1,start_py+1,x1,start_py+1+12,m_flag_color);
                    if (lines->full()) { verts_exceeded=true; break; }

                }
            } else if (m_flt==FT_Bar) {
                int z=start_py+height;
                if (pref["AlwaysShowOverlayBars"].toBool() || (xx<3600000)) {
                    z=top;

                    points->add(x1,top);
                    lines->add(x1,top,x1,bottom,m_flag_color);
                    if (points->full()) { verts_exceeded=true; break; }
               } else {
                    lines->add(x1,z,x1,z-12,m_flag_color);
               }
               if (lines->full()) { verts_exceeded=true; break; }
               if (xx<(1800000)) {
                    GetTextExtent(m_label,x,y);
                    w.renderText(m_label,x1-(x/2),top-y+3);
               }

           }
        }
        if (verts_exceeded) break;
    }
    if (verts_exceeded) {
        qWarning() << "exceeded maxverts in gLineOverlay::Plot()";
    }
}

