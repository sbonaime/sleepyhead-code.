/*
 gSessionTime Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <SleepLib/profiles.h>
#include "gSessionTime.h"

gTimeYAxis::gTimeYAxis(QColor col)
   :gYAxis("",col)
{
}
gTimeYAxis::~gTimeYAxis()
{
}
const QString gTimeYAxis::Format(double v)
{
    static QString t;
    int i=v;
    if (i<0) i=24+i;


    t.sprintf("%02i:00",i);
    return t;
};


gSessionTime::gSessionTime(ChannelID code,QColor col,Qt::Orientation o)
:Layer(code),m_orientation(o)
{
    color.clear();
    color.push_back(col);

    Xaxis=new gXAxis();
}
gSessionTime::~gSessionTime()
{
    delete Xaxis;
}

void gSessionTime::paint(gGraph & w,int left, int top, int width, int height)
{
    Q_UNUSED(w);
    Q_UNUSED(left);
    Q_UNUSED(top);
    Q_UNUSED(width);
    Q_UNUSED(height);

    if (!m_visible) return;
    /*if (!data) return;
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
    double px1,py1;//,py2,px2;
    QColor & col1=color[0];
    QColor col2("light grey");
    QString str;
    bool draw_xticks_instead=false;
    bool antialias=(*profile)["UseAntiAliasing"].toBool();

    QDateTime d;
    QTime t;
    double start,total;//end,
    float textX,textY;
    map<int,bool> datedrawn;

    int idx=-1;
    for (int i=0;i<data->np[0];i++) {
        QPointD & rp=data->point[0][i];
        if (int(rp.x()) < int(minx)) continue;
        if (int(rp.x()) > int(maxx+.5)) break;
        if (idx<0) idx=i;
        d=QDateTime::fromTime_t(rp.x()*86400.0);
        t=d.time();
        start=t.hour()+(t.minute()/60.0)+(t.second()/3600.0);
        //d=QDateTime::fromTime_t(rp.y()*86400.0);
        //t=d.time();
        //end=t.hour()+(t.minute()/60.0)+(t.second()/3600.0);

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

        //double sd=floor(rp.x());
        px1=dy*(barwidth+1);            // x position for this day
        //px2=px1+barwidth;               // plus bar width
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
        glVertex2f(rect.x(), rect.y()+rect.height()+.5);
        glVertex2f(rect.x(), rect.y());
        glVertex2f(rect.x()+rect.width(),rect.y());
        glVertex2f(rect.x()+rect.width(), rect.y()+rect.height()+.5);
        glEnd();
        if (!draw_xticks_instead) {
            if (datedrawn.find(dy)==datedrawn.end()) {
                datedrawn[dy]=true;
                str=FormatX(rp.y());

                GetTextExtent(str, textX, textY);
                if (!draw_xticks_instead && (textY+6<barwidth)) {
                    glBegin(GL_LINE);
                    glVertex2f(start_px+px1+barwidth/2,start_py);
                    glVertex2f(start_px+px1+barwidth/2,start_py-6);
                    glEnd();
                    DrawText(str,start_px+px1+barwidth/2+textY,scry-(start_py-8-textX/2),90);
                } else draw_xticks_instead=true;
            }
        }

    }
    if (draw_xticks_instead) {
        // turn off the minor ticks..
        Xaxis->SetShowMinorTicks(false);
        Xaxis->Plot(w,scrx,scry);
    }

    glColor3f (0.0F, 0.0F, 0.0F);
    glLineWidth(1);
    glBegin (GL_LINES);
    glVertex2f (start_px, start_py);
    glVertex2f (start_px, start_py+height+1);
    glVertex2f (start_px,start_py);
    glVertex2f (start_px+width, start_py);
    glEnd (); */
}
