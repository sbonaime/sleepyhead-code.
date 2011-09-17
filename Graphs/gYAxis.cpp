/*
 gYAxis Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <QDebug>
#include "gYAxis.h"

gYSpacer::gYSpacer(int spacer)
    :Layer("")
{
}

gXGrid::gXGrid(QColor col)
    :Layer("")
{
    m_major_color=QColor(180,180,180,128);
    m_minor_color=QColor(220,220,220,128);
    m_show_major_lines=true;
    m_show_minor_lines=true;
}
gXGrid::~gXGrid()
{
}
void gXGrid::paint(gGraph & w,int left,int top, int width, int height)
{
    int x,y;

    EventDataType miny=w.min_y;
    EventDataType maxy=w.max_y;

    if (miny<0) { // even it up if it's starts negative
        miny=-MAX(fabs(miny),fabs(maxy));
    }

    w.roundY(miny,maxy);
    EventDataType dy=maxy-miny;

    if (height<0) return;

    QString fd="0";
    GetTextExtent(fd,x,y);

    double max_yticks=round(height / (y+15.0)); // plus spacing between lines
    double yt=1/max_yticks;

    double mxy=MAX(fabs(maxy),fabs(miny));
    double mny=miny;
    if (miny<0) {
        mny=-mxy;
    }
    double rxy=mxy-mny;
    double ymult=height/rxy;

    double min_ytick=rxy*yt;

    float ty,h;

    if (min_ytick<=0) {
        qDebug() << "min_ytick error in gXGrid::paint()";
        return;
    }
    if (min_ytick>=1000000) {
        min_ytick=100;
    }


    lines=w.backlines();
    for (double i=miny; i<=maxy+min_ytick-0.00001; i+=min_ytick) {
        ty=(i - miny) * ymult;
        h=top+height-ty;
        if (m_show_major_lines && (i > miny)) {
            lines->add(left,h,left+width,h,m_major_color);
        }
        double z=(min_ytick/4)*ymult;
        double g=h;
        for (int i=0;i<3;i++) {
            g+=z;
            if (g>top+height) break;
            //if (vertcnt>=maxverts) {
              //  qWarning() << "vertarray bounds exceeded in gYAxis for " << w.title() << "graph" << "MinY =" <<miny << "MaxY =" << maxy << "min_ytick=" <<min_ytick;
//                break;
  //          }
            if (m_show_minor_lines) {// && (i > miny)) {
                lines->add(left,g,left+width,g,m_minor_color);
            }
            if (lines->full()) {
                break;
            }
        }
        if (lines->full()) {
            qWarning() << "vertarray bounds exceeded in gYAxis for " << w.title() << "graph" << "MinY =" <<miny << "MaxY =" << maxy << "min_ytick=" <<min_ytick;
            break;
        }
    }
}



gYAxis::gYAxis(ChannelID code,QColor col)
:Layer(code)
{
    m_line_color=col;
    m_text_color=col;

    m_yaxis_scale=1;
}
gYAxis::~gYAxis()
{
}
void gYAxis::paint(gGraph & w,int left,int top, int width, int height)
{
    int x,y;
    int labelW=0;

    EventDataType miny=w.min_y;
    EventDataType maxy=w.max_y;

    if (miny<0) { // even it up if it's starts negative
        miny=-MAX(fabs(miny),fabs(maxy));
    }

    w.roundY(miny,maxy);
    EventDataType dy=maxy-miny;

    //if ((w.max_x-w.min_x)==0)
    //    return;

    if (height<0) return;

    QString fd="0";
    GetTextExtent(fd,x,y);

    double max_yticks=round(height / (y+15.0)); // plus spacing between lines
    double yt=1/max_yticks;

    double mxy=MAX(fabs(maxy),fabs(miny));
    double mny=miny;
    if (miny<0) {
        mny=-mxy;
    }

    double rxy=mxy-mny;
    double ymult=height/rxy;

    double min_ytick=rxy*yt;

    float ty,h;

    if (min_ytick<=0) {
        qDebug() << "min_ytick error in gYAxis::Plot()";
        return;
    }
    if (min_ytick>=1000000) {
        min_ytick=100;
    }
    lines=w.backlines();

    for (double i=miny; i<=maxy+min_ytick-0.00001; i+=min_ytick) {
        ty=(i - miny) * ymult;
        if (dy<5) {
            fd=QString().sprintf("%.2f",i*m_yaxis_scale);
        } else {
            fd=QString().sprintf("%.1f",i*m_yaxis_scale);
        }

        GetTextExtent(fd,x,y);
        if (x>labelW) labelW=x;
        h=top+height-ty;
        w.renderText(fd,left+width-8-x,(h+(y/2.0)),0,m_text_color);

        lines->add(left+width-4,h,left+width,h,m_line_color);

        double z=(min_ytick/4)*ymult;
        double g=h;
        for (int i=0;i<3;i++) {
            g+=z;
            if (g>top+height) break;
            lines->add(left+width-3,g,left+width,g,m_line_color);
            if (lines->full()) {
                qWarning() << "vertarray bounds exceeded in gYAxis for " << w.title() << "graph" << "MinY =" <<miny << "MaxY =" << maxy << "min_ytick=" <<min_ytick;
                break;
            }
        }
        if (lines->full()) {
            qWarning() << "vertarray bounds exceeded in gYAxis for " << w.title() << "graph" << "MinY =" <<miny << "MaxY =" << maxy << "min_ytick=" <<min_ytick;
            break;
        }
    }
}

bool gYAxis::mouseMoveEvent(QMouseEvent * event)
{
    int x=event->x();
    int y=event->y();
    //qDebug() << "Hover at " << x << y;
}
