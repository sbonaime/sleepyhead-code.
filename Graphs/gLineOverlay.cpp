/********************************************************************
 gLineOverlayBar Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include "SleepLib/profiles.h"
#include "gLineOverlay.h"

gLineOverlayBar::gLineOverlayBar(MachineCode code,QColor col,QString _label,LO_Type _lot)
:gLayer(code),label(_label),lo_type(_lot)
{
    color.clear();
    color.push_back(col);
}
gLineOverlayBar::~gLineOverlayBar()
{
}

void gLineOverlayBar::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!m_day) return;

    //int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double xx=w.max_x-w.min_x;
    if (xx<=0) return;

    float x1,x2;

    float x,y;

    // Crop to inside the margins.
    glScissor(w.GetLeftMargin(),w.GetBottomMargin(),width,height);
    glEnable(GL_SCISSOR_TEST);

    qint32 vertcnt=0;
    GLshort * vertarray=vertex_array[0];
    qint32 pointcnt=0;
    GLshort * pointarray=vertex_array[1];
    qint32 quadcnt=0;
    GLshort * quadarray=vertex_array[2];
    if (!vertarray || !quadarray || !pointarray) {
        qWarning() << "VertArray/quadarray/pointarray==NULL";
        return;
    }

    float bottom=start_py+25, top=start_py+height-25;
    QColor & col=color[0];

    qint64 X;
    qint64 Y;
    for (vector<Session *>::iterator s=m_day->begin();s!=m_day->end(); s++) {
        if ((*s)->eventlist.find(m_code)==(*s)->eventlist.end()) continue;

        EventList & el=*((*s)->eventlist[m_code][0]);

       // bool done=false;
        for (int i=0;i<el.count();i++) {
            X=el.time(i);
            if (lo_type==LOT_Span) {
                Y=X-(qint64(el.raw(i))*1000.0L); // duration

                if (X < w.min_x) continue;
                if (Y > w.max_x) break;
            } else {
                if (X < w.min_x) continue;
                if (X > w.max_x) break;
            }

            x1=w.x2p(X);
            if (lo_type==LOT_Span) {
                x2=w.x2p(Y);
                //double w1=x2-x1;
                quadarray[quadcnt++]=x1;
                quadarray[quadcnt++]=start_py;
                quadarray[quadcnt++]=x1;
                quadarray[quadcnt++]=start_py+height;
                quadarray[quadcnt++]=x2;
                quadarray[quadcnt++]=start_py+height;
                quadarray[quadcnt++]=x2;
                quadarray[quadcnt++]=start_py;
            } else if (lo_type==LOT_Dot) {
                //if (pref["AlwaysShowOverlayBars"].toBool()) {

                if (pref["AlwaysShowOverlayBars"].toBool() || (xx<3600000.0)) {
                    // show the fat dots in the middle
                    pointarray[pointcnt++]=x1;
                    pointarray[pointcnt++]=w.y2p(20);
                } else {
                    // thin lines down the bottom
                    vertarray[vertcnt++]=x1;
                    vertarray[vertcnt++]=start_py+1;
                    vertarray[vertcnt++]=x1;
                    vertarray[vertcnt++]=start_py+1+12;
                }
            } else if (lo_type==LOT_Bar) {
                int z=start_py+height;
                if (pref["AlwaysShowOverlayBars"].toBool() || (xx<3600000)) {
                    z=top;

                    pointarray[pointcnt++]=x1;
                    pointarray[pointcnt++]=top; //z+2;
                    vertarray[vertcnt++]=x1;
                    vertarray[vertcnt++]=top;
                    vertarray[vertcnt++]=x1;
                    vertarray[vertcnt++]=bottom;
               } else {
                    vertarray[vertcnt++]=x1;
                    vertarray[vertcnt++]=z;
                    vertarray[vertcnt++]=x1;
                    vertarray[vertcnt++]=z-12;
               }
               if (xx<(1800000)) {
                    GetTextExtent(label,x,y);
                    DrawText(w,label,x1-(x/2),scry-(start_py+height-30+y));
                    //w.renderText(x1-(x/2),scry-(start_py+height-30+y),label);
               }

           }
        }
        if ((vertcnt>=maxverts) || (quadcnt>=maxverts) || (pointcnt>=maxverts)) break;

    }

    glColor4ub(col.red(),col.green(),col.blue(),col.alpha());
    if (quadcnt>0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_SHORT, 0, quadarray);
        glDrawArrays(GL_QUADS, 0, quadcnt>>1);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    if (vertcnt>0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_SHORT, 0, vertarray);
        glDrawArrays(GL_LINES, 0, vertcnt>>1);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    if (pointcnt>0) {
        glPointSize(4);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_SHORT, 0, pointarray);
        glDrawArrays(GL_POINTS, 0, pointcnt>>1);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    glDisable(GL_SCISSOR_TEST);
}

