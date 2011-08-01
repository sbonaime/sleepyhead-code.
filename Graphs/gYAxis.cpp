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
    color.clear();
    color.push_back(col);

    m_show_major_lines=true;
    m_show_minor_lines=true;
    m_yaxis_scale=1;
}
gYAxis::~gYAxis()
{
}
void gYAxis::Plot(gGraphWindow &w,float scrx,float scry)
{
    static QColor DARK_GREY(0xc0,0xc0,0xc0,0x80);
    static QColor LIGHT_GREY(0xd8,0xd8,0xd8,0x80);
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

    const QColor & linecol1=LIGHT_GREY;
    const QColor & linecol2=DARK_GREY;

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
    GLshort * vertarray=vertex_array[0];
    if (vertarray==NULL) {
        qWarning() << "VertArray==NULL";
        return;
    }

    glColor4ub(linecol1.red(),linecol1.green(),linecol1.blue(),linecol1.alpha());
    glLineWidth(1);
    if (min_ytick<=0) {
        qDebug() << "min_ytick error in gYAxis::Plot()";
        return;
    }
    if (min_ytick>=1000000) {
        min_ytick=100;
    }


    for (double i=miny+(min_ytick/2.0); i<maxy; i+=min_ytick) {
        ty=(i - miny) * ymult;
        h=(start_py+height)-ty;
        vertarray[vertcnt++]=start_px-4;
        vertarray[vertcnt++]=h;
        vertarray[vertcnt++]=start_px;
        vertarray[vertcnt++]=h;
        if (m_show_minor_lines && (i > miny)) {
            glBegin(GL_LINES);
            glVertex2f(start_px+1, h);
            glVertex2f(start_px+width, h);
            glEnd();
        }
        if (vertcnt>maxverts) {
            qWarning() << "vertarray bounds exceeded in gYAxis for " << w.Title() << "graph" << "MinY =" <<miny << "MaxY =" << maxy << "min_ytick=" <<min_ytick;
            return;
        }
    }

    for (double i=miny; i<=maxy+min_ytick-0.00001; i+=min_ytick) {
        ty=(i - miny) * ymult;
        fd=Format(i*m_yaxis_scale); // Override this as a function.
        GetTextExtent(fd,x,y);
        if (x>labelW) labelW=x;
        h=start_py+ty;
        DrawText(w,fd,start_px-12-x,scry-(h-(y/2.0)),0);

        vertarray[vertcnt++]=start_px-4;
        vertarray[vertcnt++]=h;
        vertarray[vertcnt++]=start_px;
        vertarray[vertcnt++]=h;
        if (vertcnt>maxverts) {
            qWarning() << "vertarray bounds exceeded in gYAxis for " << w.Title() << "graph" << "MinY =" <<miny << "MaxY =" << maxy << "min_ytick=" <<min_ytick;
            return;
        }

        if (m_show_major_lines && (i > miny)) {
            glColor4ub(linecol2.red(),linecol2.green(),linecol2.blue(),linecol2.alpha());
            glBegin(GL_LINES);
            glVertex2f(start_px+1, h);
            glVertex2f(start_px+width, h);
            glEnd();
        }
    }
    if (vertcnt>=maxverts) {
        qWarning() << "vertarray bounds exceeded in gYAxis for " << w.Title() << "graph";
        return;
    }

    // Draw the little ticks.
    glLineWidth(1);
    glColor3f(0,0,0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_SHORT, 0, vertarray);
    glDrawArrays(GL_LINES, 0, vertcnt>>1);
    glDisableClientState(GL_VERTEX_ARRAY); // deactivate vertex arrays after drawing
}

