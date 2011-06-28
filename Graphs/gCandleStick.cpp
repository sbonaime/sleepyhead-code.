/********************************************************************
 gCandleStick Graph Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include "gCandleStick.h"

gCandleStick::gCandleStick(gPointData *d,Qt::Orientation o)
:gLayer(d)
{
    m_orientation=o;
}
gCandleStick::~gCandleStick()
{
}
void gCandleStick::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin())-1;
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin())-1;

    float sum=0;
    for (int i=0;i<data->np[0];i++)
        sum+=data->point[0][i].y();

    float pxr;
    float px;
    if (m_orientation==Qt::Vertical) {
        pxr=height/sum;
        px=start_py;
    } else {
        pxr=width/sum;
        px=start_px;
    }

    float x,y;

    float t1,t2;
    int barwidth;
    if (m_orientation==Qt::Vertical) {
        barwidth=width;
    } else {
        barwidth=height;
    }

    QColor col2=QColor("light grey");
    QColor c(0,0,0,255);
    QString str,st;
    QRect rect;

    //Qt::AlignmDirection dir;

    glLineWidth(1);

//    glDisable(GL_LINE_SMOOTH);
    for (int i=0;i<data->np[0];i++) {
        t1=floor(px);
        t2=data->point[0][i].y()*pxr;
        px+=t2;
        t2=ceil(t2);
        if (m_orientation==Qt::Vertical) {
            rect=QRect(start_px,t1,barwidth,t2);
            //dir=wxEAST;
        } else {
            rect=QRect(t1,start_py,t2,barwidth);
            //dir=wxSOUTH;
        }

        QColor col1=color[i % color.size()];

        glBegin(GL_QUADS);
        glColor4ub(col1.red(),col1.green(),col1.blue(),col1.alpha());
        glVertex2f(rect.x()+1, rect.y()+height);
        glVertex2f(rect.x()+rect.width(), rect.y()+height);

        glColor4ub(col2.red(),col2.green(),col2.blue(),col2.alpha());
        glVertex2f(rect.x()+rect.width(), rect.y()+1);
        glVertex2f(rect.x()+1, rect.y()+1);
        glEnd();

        glColor4ub(0,0,0,255);
        glBegin(GL_LINE_LOOP);
        glVertex2f(rect.x()+1, rect.y()+height);
        glVertex2f(rect.x()+rect.width(), rect.y()+height);

        glVertex2f(rect.x()+rect.width(), rect.y()+1);
        glVertex2f(rect.x()+1, rect.y()+1);
        glEnd();
        //LinedRoundedRectangle(rect.x,rect.y,rect.width,rect.height,0,1,c);

        str="";
        if ((int)m_names.size()>i) {
        //    str=m_names[i]+" ";
        }
        st.sprintf("%0.1f",data->point[0][i].x());
        str+=st;
        GetTextExtent(str, x, y);
        //x+=5;
        if (t2>x+5) {
            int j=t1+((t2/2.0)-(x/2.0));
            if (m_orientation==Qt::Vertical) {
                DrawText(w,str,start_px+barwidth+2+y,scry-j,270.0);
            } else {
                w.renderText(j,float(scry)-(float(start_py)+(barwidth/2.0)-(y/2.0)),str);
                //DrawText(w,str,j,scry-(start_py+(barwidth/2.0)-(y/2.0)));
            }
        }
    } // for (int i
    glFlush();
}

