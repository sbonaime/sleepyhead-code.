/*
 gSessionTime Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <SleepLib/profiles.h>
#include "gSessionTime.h"

gSessionTime::gSessionTime(gPointData *d,QColor col,Qt::Orientation o)
:gLayer(d),m_orientation(o)
{
    color.clear();
    color.push_back(col);

    Xaxis=new gXAxis();
}
gSessionTime::~gSessionTime()
{
    delete Xaxis;
}

void gSessionTime::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double maxx=w.max_x;
    double minx=w.min_x;
    double xx=maxx - minx;

    int days=ceil(xx);

    float barwidth=float(width-days)/float(days);
    qint64 dy;
    //double sd;
    double px1,px2,py1,py2;
    QColor & col1=color[0];
    QColor col2("light grey");

    QDateTime d;
    QTime t;
    double start,end,total;
    for (int i=0;i<data->np[0];i++) {
        QPointD & rp=data->point[0][i];
        if (int(rp.x()) < int(minx)) continue;
        if (int(rp.x()) > int(maxx+.5)) break;
        d=QDateTime::fromTime_t(rp.x()*86400.0);
        t=d.time();
        start=t.hour()+(t.minute()/60.0)+(t.second()/3600.0);
        d=QDateTime::fromTime_t(rp.y()*86400.0);
        t=d.time();
        end=t.hour()+(t.minute()/60.0)+(t.second()/3600.0);

        total=(rp.y()-rp.x())*24;

        dy=int(rp.x())-int(minx);    // day number.
        if (dy>=days) continue;
        if (start>=12) {
         //   dy++;
            start-=24;
        }
        start+=12;

        if (start+total>24) {
//            total=24-start;
        }
        if (total<0.25) continue; // Hide sessions less than 15 minutes

        double sd=floor(rp.x());
        px1=dy*(barwidth+1);            // x position for this day
        px2=px1+barwidth;               // plus bar width
        py1=height/24.0*start;
        double h=height/24.0*(total);

        QRect rect(start_px+px1,start_py+py1,barwidth,h);

        glBegin(GL_QUADS);
        glColor4ub(col1.red(),col1.green(),col1.blue(),col1.alpha());
        glVertex2f(rect.x(), rect.y()+rect.height());
        glVertex2f(rect.x(), rect.y());

        glColor4ub(col2.red(),col2.green(),col2.blue(),col2.alpha());
        glVertex2f(rect.x()+rect.width(),rect.y());
        glVertex2f(rect.x()+rect.width(), rect.y()+rect.height());
        glEnd();

        glColor4ub(0,0,0,255);
        glLineWidth (1);
        glBegin(GL_LINE_LOOP);
        glVertex2f(rect.x(), rect.y()+rect.height());
        glVertex2f(rect.x(), rect.y());
        glVertex2f(rect.x()+rect.width(),rect.y());
        glVertex2f(rect.x()+rect.width(), rect.y()+rect.height());
        glEnd();
    }
    glColor3f (0.0F, 0.0F, 0.0F);
    glLineWidth(1);
    glBegin (GL_LINES);
    glVertex2f (start_px, start_py);
    glVertex2f (start_px, start_py+height+1);
    glVertex2f (start_px,start_py);
    glVertex2f (start_px+width, start_py);
    glEnd ();

    Xaxis->Plot(w,scrx,scry);
}
