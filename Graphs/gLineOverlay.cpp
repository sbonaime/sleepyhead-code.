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
    addGLBuf(points=new GLShortBuffer(1024,GL_POINTS));
    points->setSize(4);
    points->setColor(m_flag_color);
    addGLBuf(quads=new GLShortBuffer(2048,GL_QUADS));
    //addGLBuf(lines=new GLBuffer(color,1024,GL_LINES));
    points->setAntiAlias(true);
    quads->setAntiAlias(true);
    quads->setColor(m_flag_color);
    //lines->setAntiAlias(true);
}
gLineOverlayBar::~gLineOverlayBar()
{
    //delete lines;
    //delete quads;
    //delete points;
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

    m_count=0;
    m_flag_color=schema::channel[m_code].defaultColor();

    for (QVector<Session *>::iterator s=m_day->begin();s!=m_day->end(); s++) {
        cei=(*s)->eventlist.find(m_code);
        if (cei==(*s)->eventlist.end()) continue;
        if (cei.value().size()==0) continue;

        EventList & el=*cei.value()[0];

        for (quint32 i=0;i<el.count();i++) {
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
            m_count++;
            if (m_flt==FT_Span) {
                //x2=w.x2p(Y);
                x2=double(width)/double(xx)*double(Y-w.min_x)+left;
                if (x2<left) x2=left;
                if (x1>width+left) x1=width+left;
                //double w1=x2-x1;
                quads->add(x1,start_py,x2,start_py,x2,start_py+height,x1,start_py+height);
                if (quads->full()) { verts_exceeded=true; break; }
            } else if (m_flt==FT_Dot) {
                if ((PROFILE["AlwaysShowOverlayBars"].toInt()==0) || (xx<3600000)) {
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
                if ((PROFILE["AlwaysShowOverlayBars"].toInt()==0) || (xx<3600000)) {
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

gLineOverlaySummary::gLineOverlaySummary(QString text, int x, int y)
:Layer(CPAP_Obstructive),m_text(text),m_x(x),m_y(y) // The Layer code is a dummy here.
{
}

gLineOverlaySummary::~gLineOverlaySummary()
{
}

void gLineOverlaySummary::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible) return;
    if (!m_day) return;


    Q_UNUSED(width);
    Q_UNUSED(height);
    float cnt=0;
    for (int i=0;i<m_overlays.size();i++) {
        cnt+=m_overlays[i]->count();
    }

    double val,first,last;
    double time=0;

    // Calculate the session time.
    for (QVector<Session *>::iterator s=m_day->begin();s!=m_day->end(); s++) {
        first=(*s)->first();
        last=(*s)->last();
        if (last < w.min_x) continue;
        if (first > w.max_x) continue;

        if (first < w.min_x)
            first=w.min_x;
        if (last > w.max_x)
            last=w.max_x;

        time+=last-first;
    }

    val=0;

    time/=1000;
    int h=time/3600;
    int m=int(time/60) % 60;
    int s=int(time) % 60;


    time/=3600;

    //if (time<1) time=1;

    if (time>0) val=cnt/time;


    QString a="Event Count="+QString::number(cnt)+" Selection Time="+QString().sprintf("%02i:%02i:%02i",h,m,s)+" "+m_text+"="+QString::number(val,'f',2);
    w.renderText(a,left+m_x,top+m_y);
}
