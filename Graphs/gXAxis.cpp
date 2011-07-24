/********************************************************************
 gXAxis Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include <QDebug>
#include "gXAxis.h"

gXAxis::gXAxis(QColor col)
:gLayer(NULL)
{
    color.clear();
    color.push_back(col);
    m_show_major_lines=false;
    m_show_minor_lines=false;
    m_show_minor_ticks=true;
    m_show_major_ticks=true;

}
gXAxis::~gXAxis()
{
}
void gXAxis::Plot(gGraphWindow & w,float scrx,float scry)
{
    float px,py;

    //int start_px=w.GetLeftMargin();
    //int start_py=w.GetTopMargin();
    float width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
//    float height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double minx;
    double maxx;
    if (w.BlockZoom()) {
        minx=w.rmin_x;
        maxx=w.rmax_x;
    } else {
        minx=w.min_x;
        maxx=w.max_x;
    }
    double xx=maxx-minx;

    if (xx<=0) return;

    double xmult=width/xx;

    QString fd;
    if (xx<1.5) {
        fd="00:00:00:0000";
    } else {
        fd="XXX";
    }
    float x,y;
    GetTextExtent(fd,x,y);
    if (x<=0) {
        qWarning() << "gXAxis::Plot() x<=0";
        return;
    }

    double max_ticks=(x+25.0)/width; // y+50 for rotated text
    double jj=1/max_ticks;
    double minor_tick=max_ticks*xx;
    double st2=minx; //double(int(frac*1440.0))/1440.0;
    double st,q;

    bool show_seconds=false;
    bool show_milliseconds=false;
    bool show_time=true;

    double min_tick;
    if (xx<1.5) {
        int rounding[16]={12,24,48,72,96,144,288,720,1440,2880,5760,8640,17280,86400,172800,345600}; // time rounding

        int ri;
        for (ri=0;ri<16;ri++) {
            st=round(st2*rounding[ri])/rounding[ri];
            min_tick=round(minor_tick*rounding[ri])/rounding[ri];
            q=xx/min_tick;  // number of ticks that fits in range
            if (q<=jj) break; // compared to number of ticks that fit on screen.
        }
        if (ri>8) show_seconds=true;
        if (ri>=14) show_milliseconds=true;

        if (min_tick<=0.25/86400.0)
            min_tick=0.25/86400;
    } else { // Day ticks..
        show_time=false;
        st=st2;
        min_tick=1.0;
        double mtiks=(x+20.0)/width;
        double mt=mtiks*xx;
        min_tick=mt;
        //if (min_tick<1.0) min_tick=1.0;
        //if (min_tick>10) min_tick=10;
    }
    if (min_tick<=0) {
        qWarning() << "gXAxis::Plot() min_tick<=0 :(";
        return;
    }

    double st3=st;
    while (st3>minx) {
        st3-=min_tick/10.0;
    }
    st3+=min_tick/10.0;

    py=w.GetBottomMargin();

    qint32 vertcnt=0;
    GLshort * vertarray=vertex_array[0];
    if (vertarray==NULL) {
        qWarning() << "VertArray==NULL";
        return;
    }

    if (m_show_minor_ticks) {
        for (double i=st3; i<=maxx; i+=min_tick/10.0) {
            if (i<minx) continue;
            px=(i-minx)*xmult+w.GetLeftMargin();
            vertarray[vertcnt++]=px;
            vertarray[vertcnt++]=py;
            vertarray[vertcnt++]=px;
            vertarray[vertcnt++]=py-4;
            if (vertcnt>maxverts) {
                qWarning() << "gXAxis::Plot() maxverts exceeded trying to draw minor ticks";
                return;
            }
        }
    }


    while (st<minx) {
        st+=min_tick; //10.0;  // mucking with this changes the scrollyness of the ticker.
    }

    int hour,minute,second,millisecond;
    QDateTime d;
    // Need to use Qpainter clipping..
    // messy. :(
    glScissor(w.GetLeftMargin()-20,0,width+20,w.GetBottomMargin());
    glEnable(GL_SCISSOR_TEST);

    //const double extra=min_tick/10.0;
    const double extra=0;
    for (double i=st; i<=maxx+extra; i+=min_tick) {
        d=QDateTime::fromMSecsSinceEpoch(i*86400000.0);

        if (show_time) {
            minute=d.time().minute();
            hour=d.time().hour();
            second=d.time().second();
            millisecond=d.time().msec();

            if (show_milliseconds) {
                fd.sprintf("%02i:%02i:%02i:%04i",hour,minute,second,millisecond);
            } else if (show_seconds) {
                fd.sprintf("%02i:%02i:%02i",hour,minute,second);
            } else {
                fd.sprintf("%02i:%02i",hour,minute);
            }
        } else {
            fd=d.toString("MMM dd");
        }

        px=(i-minx)*xmult+w.GetLeftMargin();
        if ((i>=st && i<=maxx) && (m_show_major_ticks)) {
            vertarray[vertcnt++]=px;
            vertarray[vertcnt++]=py;
            vertarray[vertcnt++]=px;
            vertarray[vertcnt++]=py-6;
            if (vertcnt>maxverts) {
                qWarning() << "gXAxis::Plot() maxverts exceeded trying to draw Major ticks";
                return;
            }
        }

        GetTextExtent(fd,x,y);
        //glColor3ub(0,0,0);
        if (!show_time) {
            DrawText(fd, px+y, scry-(py-(x/2)-8), 90.0);
            //w.renderText(px-(y/2)+2, scry-(py-(x/2)-20), 90.0,fd);
        } else {
            DrawText(fd, px-(x/2), scry-(py-8-y));
            //w.renderText(px-(x/2), scry-(py-(y/2)-20), fd);
        }

    }
// Draw the little ticks.
    if (vertcnt>=maxverts) {
        qWarning() << "maxverts exceeded in gYAxis::Plot()";
        return;
    }

    glLineWidth(1);
    glColor3f(0,0,0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_SHORT, 0, vertarray);
    glDrawArrays(GL_LINES, 0, vertcnt>>1);
    glDisableClientState(GL_VERTEX_ARRAY); // deactivate vertex arrays after drawing

    glDisable(GL_SCISSOR_TEST);


}

