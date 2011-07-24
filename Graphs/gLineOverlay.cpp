/********************************************************************
 gLineOverlayBar Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include "SleepLib/profiles.h"
#include "gLineOverlay.h"

gLineOverlayBar::gLineOverlayBar(gPointData *d,QColor col,QString _label,LO_Type _lot)
:gLayer(d),label(_label),lo_type(_lot)
{
    color.clear();
    color.push_back(col);
}
gLineOverlayBar::~gLineOverlayBar()
{
}
//nov = number of vertex
//r = radius
void Dot(int nov, float r)
{
    if (nov < 4) nov = 4;
    if (nov > 360) nov = 360;
    float angle = (360/nov)*(3.142159/180);
    float x[360] = {0};
    float y[360] = {0};
    for (int i = 0; i < nov; i++){
        x[i] = cosf(r*cosf(i*angle));
        y[i] = sinf(r*sinf(i*angle));
        glBegin(GL_POLYGON);
        for (int i = 0; i < nov; i++){
            glVertex2f(x[i],y[i]);
        }
        glEnd();
    }
    //render to texture and map to GL_POINT_SPRITE
}
void gLineOverlayBar::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    if (!data) return;
    if (!data->IsReady()) return;

    //int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double xx=w.max_x-w.min_x;
    if (xx<=0) return;

    float x1,x2;

    float x,y;//,descent,leading;

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
    for (int n=0;n<data->VC();n++) {

       // bool done=false;
        for (int i=0;i<data->np[n];i++) {
            QPointD & rp=data->point[n][i];
            //float X=rp.x();
            //float Y=rp.y();
            if (rp.y() < w.min_x) continue;
            if (rp.x() > w.max_x) break;

            x1=w.x2p(rp.x());
            if (rp.x()!=rp.y()) {
                x2=w.x2p(rp.y());
                //double w1=x2-x1;
                quadarray[quadcnt++]=x1;
                quadarray[quadcnt++]=start_py;
                quadarray[quadcnt++]=x1;
                quadarray[quadcnt++]=start_py+height;
                quadarray[quadcnt++]=x2;
                quadarray[quadcnt++]=start_py+height;
                quadarray[quadcnt++]=x2;
                quadarray[quadcnt++]=start_py;
            } else {
                if (lo_type==LOT_Dot) {
                    //if (pref["AlwaysShowOverlayBars"].toBool()) {

                    if (pref["AlwaysShowOverlayBars"].toBool() || (xx<(3600.0/86400.0))) {
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
                    if (pref["AlwaysShowOverlayBars"].toBool() || (xx<(3600.0/86400.0))) {
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
                    if (xx<(1800.0/86400.0)) {
                        GetTextExtent(label,x,y);
                        DrawText(label,x1-(x/2),scry-(start_py+height-30+y));
                        //w.renderText(x1-(x/2),scry-(start_py+height-30+y),label);
                    }

                }
            }
            if ((vertcnt>=maxverts) || (quadcnt>=maxverts) || (pointcnt>=maxverts)) break;
        }
    }

    //assert (vertcnt<maxverts);
    //assert (quadcnt<maxverts);
    //assert (pointcnt<maxverts);
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

