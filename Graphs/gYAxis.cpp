/********************************************************************
 gYAxis Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include "gYAxis.h"

gYAxis::gYAxis(QColor col)
:gLayer(NULL)
{
    color.clear();
    color.push_back(col);

    m_show_major_lines=true;
    m_show_minor_lines=true;
}
gYAxis::~gYAxis()
{
}
void gYAxis::Plot(gGraphWindow &w,float scrx,float scry)
{
    static QColor DARK_GREY(0xb8,0xb8,0xb8,0xa0);
    static QColor LIGHT_GREY(0xd8,0xd8,0xd8,0xa0);
    float x,y;
    int labelW=0;

    double miny=w.min_y;
    double maxy=w.max_y;
    if (maxy==miny)
        return;
    if ((w.max_x-w.min_x)==0)
        return;

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
    assert(vertarray!=NULL);

    glColor4ub(linecol1.red(),linecol1.green(),linecol1.blue(),linecol1.alpha());
    glLineWidth(1);


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
    }

    for (double i=miny; i<=maxy+min_ytick-0.00001; i+=min_ytick) {
        ty=(i - miny) * ymult;
        fd=Format(i); // Override this as a function.
        GetTextExtent(fd,x,y);
        if (x>labelW) labelW=x;
        h=start_py+ty;
        DrawText(w,fd,start_px-12-x,scry-(h-(y/2.0)),0); //
        //glColor3ub(0,0,0);
        //w.renderText(start_px-15-x, scry-(h - (y / 2)),fd);

        vertarray[vertcnt++]=start_px-4;
        vertarray[vertcnt++]=h;
        vertarray[vertcnt++]=start_px;
        vertarray[vertcnt++]=h;

        if (m_show_major_lines && (i > miny)) {
            glColor4ub(linecol2.red(),linecol2.green(),linecol2.blue(),linecol2.alpha());
            glBegin(GL_LINES);
            glVertex2f(start_px+1, h);
            glVertex2f(start_px+width, h);
            glEnd();
        }
    }
    assert(vertcnt<maxverts);

    // Draw the little ticks.
    glLineWidth(1);
    glColor3f(0,0,0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_SHORT, 0, vertarray);
    glDrawArrays(GL_LINES, 0, vertcnt>>1);
    glDisableClientState(GL_VERTEX_ARRAY); // deactivate vertex arrays after drawing
}

