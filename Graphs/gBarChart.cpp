/*
 gBarChart Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <SleepLib/profiles.h>
#include "gBarChart.h"

gBarChart::gBarChart(MachineCode code,QColor col,Qt::Orientation o)
:gLayer(code),m_orientation(o)
{
    color.clear();
    color.push_back(col);

    Xaxis=new gXAxis();
}
gBarChart::~gBarChart()
{
    delete Xaxis;
}

void gBarChart::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    /*if (!data) return;
    if (!data->IsReady()) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double xx=w.max_x - w.min_x;
    double days=int(xx);
    //days=data->np[0];

    days=0;
    for (int i=0;i<data->np[0];i++) {
       if ((data->point[0][i].x() >= w.min_x) && (data->point[0][i].x()<w.max_x)) days+=1;
    }
    if (days==0) return;

    float barwidth,pxr;
    float px,zpx;//,py;

    if (m_orientation==Qt::Vertical) {
        barwidth=(height-days)/float(days);
        pxr=width/w.max_y;
        px=start_py;
    } else {
        barwidth=(width-days)/float(days);
        pxr=height/w.max_y;
        px=start_px;
    }
    px+=1;
    int t1,t2;
    int u1,u2;
    float textX, textY;

    QString str;
    bool draw_xticks_instead=false;
    bool antialias=pref["UseAntiAliasing"].toBool();

    if (antialias) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);
    }
    zpx=px;
    int i,idx=-1;

    for (i=0;i<data->np[0];i++) {
        if (data->point[0][i].x() < w.min_x) continue;
        if (data->point[0][i].x() >= w.max_x) break;
        if (idx<0) idx=i;
        t1=px;
        px+=barwidth+1;
        t2=px-t1-1;

        QRect rect;
        //Qt:wxDirection dir;

        u2=data->point[0][i].y()*pxr;
        u1=start_py;
        if (antialias) {
            u1++;
            u2++;
        }

        if (m_orientation==Qt::Vertical) {
            rect=QRect(start_px,t1,u2,t2);
        } else {
            rect=QRect(t1,u1,t2,u2);
        }
        //dir=wxEAST;
        //RoundedRectangle(rect.x,rect.y,rect.width,rect.height,1,color[0]); //,*wxLIGHT_GREY,dir);

        // TODO: Put this in a function..
        QColor & col1=color[0];
        QColor col2("light grey");

        glBegin(GL_QUADS);
        //red color
        glColor4ub(col1.red(),col1.green(),col1.blue(),col1.alpha());
        glVertex2f(rect.x(), rect.y()+rect.height());
        glVertex2f(rect.x(), rect.y());
        //blue color
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
        //glVertex2f(rect.x(), rect.y()+rect.height());
        glEnd();

        if (!draw_xticks_instead) {
            str=FormatX(data->point[0][i].x());

            GetTextExtent(str, textX, textY);
            if (t2<textY+6)
                draw_xticks_instead=true;
        }
    }
    if (antialias) {
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_BLEND);
    }

    if (draw_xticks_instead) {
        // turn off the minor ticks..
        Xaxis->SetShowMinorTicks(false);
        Xaxis->Plot(w,scrx,scry);
    } else {
        px=zpx;
        for (i=idx;i<data->np[0];i++) {
            if (data->point[0][i].x() < w.min_x) continue;
            if (data->point[0][i].x() >= w.max_x) break;
            t1=px;
            px+=barwidth+1;
            t2=px-t1-1;
            str=FormatX(data->point[0][i].x());
            GetTextExtent(str, textX, textY);
            float j=t1+((t2/2.0)+(textY/2.0)+5);
            if (m_orientation==Qt::Vertical) {
                DrawText(str,start_px-textX-8,scry-j);
            } else {
                DrawText(str,j,scry-(start_py-3-(textX/2)),90);
            }
        }
    }

    glColor3f (0.1F, 0.1F, 0.1F);
    glLineWidth(1);
    glBegin (GL_LINES);
    glVertex2f (start_px, start_py);
    glVertex2f (start_px, start_py+height+1);
    //glVertex2f (start_px,start_py);
    //glVertex2f (start_px+width, start_py);
    glEnd ();
*/
}

