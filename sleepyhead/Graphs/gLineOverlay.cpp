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
    addVertexBuffer(points=new gVertexBuffer(2048,GL_POINTS));
    points->setSize(4);
    points->setColor(m_flag_color);
    addVertexBuffer(quads=new gVertexBuffer(2048,GL_QUADS));
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

    gVertexBuffer * lines=w.lines();
    int start_py=topp;

    double xx=w.max_x-w.min_x;
    double yy=w.max_y-w.min_y;
    if (xx<=0) return;

    float x1,x2;

    int x,y;

    float bottom=start_py+height-25*w.printScaleY(), top=start_py+25*w.printScaleY();

    double X;
    double Y;

    bool verts_exceeded=false;

    m_count=0;
    m_sum=0;
    m_flag_color=schema::channel[m_code].defaultColor();

    lines->setColor(m_flag_color);
    points->setColor(m_flag_color);
    if (m_flt==FT_Span) {
        m_flag_color.setAlpha(128);
    }
    EventStoreType raw;

    quint32 * tptr;
    EventStoreType * dptr, *eptr;
    qint64 stime;

    OverlayDisplayType odt=PROFILE.appearance->overlayType();
    QHash<ChannelID,QVector<EventList *> >::iterator cei;
    int count;

    qint64 clockdrift=qint64(PROFILE.cpap->clockDrift()) * 1000L;
    qint64 drift=0;

    // For each session, process it's eventlist
    for (QVector<Session *>::iterator s=m_day->begin();s!=m_day->end(); s++) {

        if (!(*s)->enabled()) continue;
        cei=(*s)->eventlist.find(m_code);
        if (cei==(*s)->eventlist.end()) continue;
        QVector<EventList *> & evlist=cei.value();
        if (evlist.size()==0) continue;
        drift=((*s)->machine()->GetType()==MT_CPAP) ? clockdrift : 0;

        // Could loop through here, but nowhere uses more than one yet..
        for (int k=0;k<evlist.size();k++) {
            EventList & el=*(evlist[k]);
            count=el.count();
            stime=el.first() + drift;
            dptr=el.rawData();
            eptr=dptr+count;
            tptr=el.rawTime();

            ////////////////////////////////////////////////////////////////////////////
            // Skip data previous to minx bounds
            ////////////////////////////////////////////////////////////////////////////
            for (; dptr < eptr; dptr++) {
                X=stime + *tptr;
                if (X >= w.min_x)
                    break;
                tptr++;
            }

            if (m_flt==FT_Span) {
                ////////////////////////////////////////////////////////////////////////////
                // FT_Span
                ////////////////////////////////////////////////////////////////////////////
                for (;dptr < eptr; dptr++) {
                    X=stime + *tptr++;
                    raw=*dptr;
                    Y=X-(qint64(raw)*1000.0L); // duration
                    if (Y > w.max_x)
                        break;
                    x1=double(width)/double(xx)*double(X-w.min_x)+left;
                    m_count++;
                    m_sum+=raw;
                    x2=double(width)/double(xx)*double(Y-w.min_x)+left;

                    if (int(x1)==int(x2))
                        x2+=1;
                    if (x2<left)
                        x2=left;
                    if (x1>width+left)
                        x1=width+left;

                    quads->add(x2,start_py, x1,start_py, x1,start_py+height, x2,start_py+height,m_flag_color.rgba());
                    if (quads->full()) {
                        verts_exceeded=true;
                        break;
                    }
                }
            } else if (m_flt==FT_Dot) {
                ////////////////////////////////////////////////////////////////////////////
                // FT_Dot
                ////////////////////////////////////////////////////////////////////////////
                for (; dptr < eptr; dptr++) {
                    X=stime + *tptr++; //el.time(i);
                    raw=*dptr; //el.data(i);
                    if (X > w.max_x)
                        break;
                    x1=double(width)/double(xx)*double(X-w.min_x)+left;
                    m_count++;
                    m_sum+=raw;
                    if ((odt==ODT_Bars) || (xx<3600000)) {
                        // show the fat dots in the middle
                        points->add(x1,double(height)/double(yy)*double(-20-w.min_y)+topp);
                        if (points->full()) {
                            verts_exceeded=true;
                            break;
                        }
                    } else {
                        // thin lines down the bottom
                        lines->add(x1,start_py+1,x1,start_py+1+12);
                        if (lines->full()) {
                            verts_exceeded=true;
                            break;
                        }
                    }

                }
            } else if (m_flt==FT_Bar) {
                ////////////////////////////////////////////////////////////////////////////
                // FT_Bar
                ////////////////////////////////////////////////////////////////////////////
                for (; dptr < eptr; dptr++) {
                    X=stime + *tptr++;
                    raw=*dptr;
                    if (X > w.max_x)
                        break;
                    x1=double(width)/double(xx)*double(X-w.min_x)+left;
                    m_count++;
                    m_sum+=raw;
                    int z=start_py+height;
                    if ((odt==ODT_Bars) || (xx<3600000)) {
                        z=top;

                        points->add(x1,top);
                        lines->add(x1,top,x1,bottom);
                        if (points->full()) {
                            verts_exceeded=true;
                            break;
                        }
                    } else {
                        lines->add(x1,z,x1,z-12);
                    }
                    if (lines->full()) {
                        verts_exceeded=true;
                        break;
                    }
                    if (xx<(1800000)) {
                        GetTextExtent(m_label,x,y);
                        w.renderText(m_label,x1-(x/2),top-y+(3*w.printScaleY()));
                    }
                }
            }

            if (verts_exceeded)
                break;
        }
        if (verts_exceeded)
            break;
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
    double sum=0;
    bool isSpan=false;
    for (int i=0;i<m_overlays.size();i++) {
        cnt+=m_overlays[i]->count();
        sum+=m_overlays[i]->sum();
        if (m_overlays[i]->flagtype()==FT_Span) isSpan=true;
    }

    double val,first,last;
    double time=0;

    // Calculate the session time.
    for (QVector<Session *>::iterator s=m_day->begin();s!=m_day->end(); s++) {
        if (!(*s)->enabled())  continue;
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



    QString a="Events="+QString::number(cnt)+" Duration="+QString().sprintf("%02i:%02i:%02i",h,m,s)+", "+m_text+"="+QString::number(val,'f',2);

    if (isSpan) {
        float sph;
        if (!time) sph=0; else {
            sph=(100.0/float(time))*(sum/3600.0);
            if (sph>100) sph=100;
        }
        a+=QString(" (\%%1 in events)").arg(sph,0,'f',2);
    }
    w.renderText(a,left+m_x,top+m_y);
}
