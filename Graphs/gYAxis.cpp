/********************************************************************
 gYAxis Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include <QDebug>
#include "gYAxis.h"

gYAxis::gYAxis(QColor col)
:gLayer(EmptyChannel)
{
    m_line_color=col;
    m_text_color=col;
    m_major_color=Qt::darkGray;
    m_minor_color=Qt::lightGray;

    m_show_major_lines=true;
    m_show_minor_lines=true;
    m_yaxis_scale=1;
}
gYAxis::~gYAxis()
{
}
void gYAxis::Plot(gGraphWindow &w,float scrx,float scry)
{
    float x,y;
    int labelW=0;

    double miny=w.min_y;
    double maxy=w.max_y;

    double dy=maxy-miny;
    if (dy<=0)
        return;

    int m;
    if (maxy>500) {
        m=ceil(maxy/100.0);
        maxy=m*100;
        m=floor(miny/100.0);
        miny=m*100;
    } else if (maxy>150) {
        m=ceil(maxy/50.0);
        maxy=m*50;
        m=floor(miny/50.0);
        miny=m*50;
    } else if (maxy>80) {
        m=ceil(maxy/20.0);
        maxy=m*20;
        m=floor(miny/20.0);
        miny=m*20;
    } else if (maxy>30) {
        m=ceil(maxy/10.0);
        maxy=m*10;
        m=floor(miny/10.0);
        miny=m*10;
    } else if (maxy>5) {
        m=ceil(maxy/5.0);
        maxy=m*5;
        m=floor(miny/5.0);
        miny=m*5;
    } else {
        maxy=ceil(maxy);
        if (maxy<1) maxy=1;
        miny=floor(miny);
        //if (miny<1) miny=0;

    }

    //if ((w.max_x-w.min_x)==0)
    //    return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetRightMargin()+start_px);
    int topm=w.GetTopMargin();
    int height=scry-(topm+start_py);
    if (height<0) return;

    QString fd="0";
    GetTextExtent(fd,x,y);

    double max_yticks=round(height / (y+15.0)); // plus spacing between lines
    double yt=1/max_yticks;


    double mxy=MAX(maxy,fabs(miny));
    double mny=MIN(maxy,fabs(miny));
    if (miny<0) mny=-mny;
    if (maxy<0) mxy=-mxy;

    double rxy=mxy-mny;
    double ymult=height/rxy;

    double min_ytick=rxy*yt;

    float ty,h;

    qint32 vertcnt=0;
    GLshort * vertarray=(GLshort *)vertex_array[0];
    qint32 minorvertcnt=0;
    GLshort * minorvertarray=(GLshort *)vertex_array[1];
    qint32 majorvertcnt=0;
    GLshort * majorvertarray=(GLshort *)vertex_array[2];

    if ((vertarray==NULL) || (minorvertarray==NULL) || (majorvertarray==NULL)) {
        qWarning() << "gYAxis::Plot()  VertArray==NULL";
        return;
    }

    if (min_ytick<=0) {
        qDebug() << "min_ytick error in gYAxis::Plot()";
        return;
    }
    if (min_ytick>=1000000) {
        min_ytick=100;
    }

    double q=((maxy-(miny+(min_ytick/2.0)))/min_ytick)*4;
    if (q>=maxverts) {
        qDebug() << "Would exeed maxverts. Should be another two bounds exceeded messages after this. (I can do a minor optimisation by disabling the other checks if this turns out to be consistent)" << q << maxverts;
    }


    for (double i=miny+(min_ytick/2.0); i<maxy; i+=min_ytick) {
        ty=(i - miny) * ymult;
        h=(start_py+height)-ty;
        vertarray[vertcnt++]=start_px-4;
        vertarray[vertcnt++]=h;
        vertarray[vertcnt++]=start_px;
        vertarray[vertcnt++]=h;
        if (m_show_minor_lines && (i > miny)) {
            minorvertarray[minorvertcnt++]=start_px+1;
            minorvertarray[minorvertcnt++]=h;
            minorvertarray[minorvertcnt++]=start_px+width;
            minorvertarray[minorvertcnt++]=h;
        }
        if (vertcnt>=maxverts) { // Should only need to check one.. The above check should be enough.
            qWarning() << "vertarray bounds exceeded in gYAxis for " << w.Title() << "graph" << "MinY =" <<miny << "MaxY =" << maxy << "min_ytick=" <<min_ytick;
            break;
        }
    }

    for (double i=miny; i<=maxy+min_ytick-0.00001; i+=min_ytick) {
        ty=(i - miny) * ymult;
        fd=Format(i*m_yaxis_scale); // Override this as a function.
        GetTextExtent(fd,x,y);
        if (x>labelW) labelW=x;
        h=start_py+ty;
        DrawText(w,fd,start_px-12-x,scry-(h-(y/2.0)),0,m_text_color);

        vertarray[vertcnt++]=start_px-4;
        vertarray[vertcnt++]=h;
        vertarray[vertcnt++]=start_px;
        vertarray[vertcnt++]=h;
        if (vertcnt>=maxverts) {
            qWarning() << "vertarray bounds exceeded in gYAxis for " << w.Title() << "graph" << "MinY =" <<miny << "MaxY =" << maxy << "min_ytick=" <<min_ytick;
            break;
        }

        if (m_show_major_lines && (i > miny)) {
            majorvertarray[majorvertcnt++]=start_px+1;
            majorvertarray[majorvertcnt++]=h;
            majorvertarray[majorvertcnt++]=start_px+width;
            majorvertarray[majorvertcnt++]=h;
        }
    }
    if (vertcnt>=maxverts) {
        qWarning() << "yAxis tickers and display should be corrupted, because something dumb is happening in " << w.Title() << "graph";
        return;
    }

    // Draw the lines & ticks
    // Turn on blending??
    glLineWidth(1);
    w.qglColor(m_line_color);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_SHORT, 0, vertarray);
    glDrawArrays(GL_LINES, 0, vertcnt>>1);
    w.qglColor(m_minor_color);
    glVertexPointer(2, GL_SHORT, 0, minorvertarray);
    glDrawArrays(GL_LINES, 0, minorvertcnt>>1);
    w.qglColor(m_major_color);
    glVertexPointer(2, GL_SHORT, 0, majorvertarray);
    glDrawArrays(GL_LINES, 0, majorvertcnt>>1);
    glDisableClientState(GL_VERTEX_ARRAY); // deactivate vertex arrays after drawing
}

