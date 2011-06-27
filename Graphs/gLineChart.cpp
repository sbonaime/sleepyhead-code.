/********************************************************************
 gLineChart Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include <QString>
#include <SleepLib/profiles.h>
#include "gLineChart.h"

#define EXTRA_ASSERTS 1
gLineChart::gLineChart(gPointData *d,QColor col,int dlsize,bool _accelerate,bool _hide_axes,bool _square_plot)
:gLayer(d),m_accelerate(_accelerate),m_drawlist_size(dlsize),m_hide_axes(_hide_axes),m_square_plot(_square_plot)
{
    m_drawlist=new QPointD [dlsize];
    color.clear();
    color.push_back(col);
    m_report_empty=false;
}
gLineChart::~gLineChart()
{
    delete [] m_drawlist;
}

// Time Domain Line Chart
void gLineChart::Plot(gGraphWindow & w,float scrx,float scry)
{

    if (!m_visible)
        return;
    if (!data)
        return;
    if (!data->IsReady())
        return;

    int start_px=w.GetLeftMargin(), start_py=w.GetBottomMargin();

    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    double minx=w.min_x, miny=w.min_y, maxx=w.max_x, maxy=w.max_y;
    double xx=maxx-minx, yy=maxy-miny;
    float xmult=width/xx, ymult=height/yy;   // time to pixel conversion multiplier

    // Return on screwy min/max conditions
    if ((xx<0) || (yy<0))
        return;
    if ((yy==0) && (miny==0))
        return;

    int num_points=0;
    for (int z=0;z<data->VC();z++) num_points+=data->np[z]; // how many points all up?

    // Draw bounding box if something else will be drawn.
    if (!(!m_report_empty && !num_points)) {
        glColor3f (0.1F, 0.1F, 0.1F);
        glLineWidth (1);
        glBegin (GL_LINE_LOOP);
        glVertex2f (start_px, start_py);
        glVertex2f (start_px, start_py+height);
        glVertex2f (start_px+width,start_py+height);
        glVertex2f (start_px+width, start_py);
        glEnd ();
    }

    width--;
    if (!num_points) { // No Data?
        if (m_report_empty) {
            QString msg="No Waveform Available";
            float x,y;
            //TextMarkup...
            GetTextExtent(msg,x,y,bigfont);
            //w.renderText(start_px+(width/2.0)-(x/2.0),start_py+(height/2.0)-(y/2.0),msg,*bigfont);
            DrawText(w,msg,start_px+(width/2.0)-(x/2.0),scry-w.GetBottomMargin()-height/2.0+y/2.0,0,Qt::gray,bigfont);//-(y/2.0)
        }
        return;
    }

    bool accel=m_accelerate;
    double sfit,sr;
    int dp,sam;

    QColor & col=color[0];
    // Selected the plot line color

    const long maxverts=65536*4; // Resolution dependant..
    long vertcnt=0;
    static GLshort vertarray[maxverts+8];

    float lastpx,lastpy;
    float px,py;
    int idx,idxend,np;
    bool done,first;
    double x0,x1,xL;

    int visible_points=0;

    for (int n=0;n<data->VC();n++) { // for each segment

        int siz=data->np[n];
        if (siz<=1) continue; // Don't bother drawing 1 point or less.

        QPointD * point=data->point[n];

        x0=point[0].x();
        xL=point[siz-1].x();

        if (maxx<x0) continue;
        if (xL<minx) continue;

        if (x0>xL) {
            if (siz==2) { // this happens on CPAP
                QPointD t=point[0];
                point[0]=point[siz-1];
                point[siz-1]=t;
                x0=point[0].x();
            } else {
                qDebug("Reversed order sample fed to gLineChart - ignored.");
                continue;
                //assert(x1<x2);
            }
        }
        done=false;
        first=true;
        dp=0;

        x1=point[1].x();
//        if (accel) {
        sr=x1-x0;           // Time distance between samples
        assert(sr>0);
        double qx=xL-x0;    // Full time range of this segment
        double gx=xx/qx;    // ratio of how much of the whole data set this represents
        double segwidth=width*gx;
        double XR=xx/sr;
        double Z1=MAX(x0,minx);
        double Z2=MIN(xL,maxx);
        double ZD=Z2-Z1;
        double ZR=ZD/sr;
        double ZQ=ZR/XR;
        double ZW=ZR/(width*ZQ);
        const int num_averages=15;  // Max n umber of samples taken from samples per pixel for better min/max values
        visible_points+=ZR*ZQ;
        if (accel && n>0) {
            sam=1;
        }
        if (ZW<num_averages) {
            sam=1;
            accel=false;
        } else {
            sam=ZW/num_averages;
            if (sam<1) {
                sam=1;
                accel=false;
            }
        }

        // Prepare the min max y values if we still are accelerating this plot
        if (accel) {
            for (int i=0;i<width;i++) {
                m_drawlist[i].setX(height);
                m_drawlist[i].setY(0);
            }
        }
        int minz=width,maxz=0;

        // Technically shouldn't never ever get fed reverse data.


        // these calculations over estimate
        // The Z? values are much more accurate

        idx=0;
        idxend=0;
        np=0;
        if (m_accelerate)  {
            if (minx>x1) {
                double j=minx-x0;  // == starting min of first sample in this segment
                idx=floor(j/sr);
                // Loose the precision
                idx-=idx % sam;

            } // else just start from the beginning

            idxend=floor(xx/sr);
            idxend/=sam; // devide by number of samples skips

            np=(idxend-idx)+sam;
            np /= sam;
        } else {

            np=siz;
        }

        bool watch_verts_carefully=false;
        // better to do it here than in the main loop.
        np <<= 2;

        if (!accel && m_square_plot)
            np <<= 1; // double it again

        if (np>=maxverts) {
            watch_verts_carefully=true;
            //assert(np<maxverts);
        }

        bool firstpx=true;
        for (int i=idx;i<siz;i+=sam) {

                if (point[i].x() < minx) continue; // Skip stuff before the start of our data window

                if (first) {
                    first=false;
                    if (i>=sam)  i-=sam; // Start with the previous sample (which will be in clipping area)
                }

                if (point[i].x() > maxx) done=true; // Let this iteration finish.. (This point will be in far clipping)

            px=1+((point[i].x() - minx) * xmult);   // Scale the time scale X to pixel scale X


            if (!accel) {
                py=1+((point[i].y() - miny) * ymult);   // Same for Y scale
                if (firstpx) {
                    firstpx=false;
                } else {
                    if (m_square_plot) {
                        vertarray[vertcnt++]=lastpx;
                        vertarray[vertcnt++]=lastpy;
                        vertarray[vertcnt++]=start_px+px;
                        vertarray[vertcnt++]=lastpy;
                        vertarray[vertcnt++]=start_px+px;
                        vertarray[vertcnt++]=lastpy;
                    } else {
                        vertarray[vertcnt++]=lastpx;
                        vertarray[vertcnt++]=lastpy;
                    }

                    vertarray[vertcnt++]=start_px+px;
                    vertarray[vertcnt++]=start_py+py;
                    #if defined(EXTRA_ASSERTS)
                    assert(vertcnt<maxverts);
                    #endif
                }
                lastpx=start_px+px;
                lastpy=start_py+py;
            } else {
                // Just clip ugly in accel mode.. Too darn complicated otherwise
               /* if (px<0) {
                    px=0;
                }
                if (px>width) {
                    px=width;
                } */
                // In accel mode, each pixel has a min/max Y value.
                // m_drawlist's index is the pixel index for the X pixel axis.

                float zz=(maxy-miny)/2.0;  // centreline
                float jy=fabs(point[i].y());

                int y1=1+(jy-miny)*ymult;
                int y2=1+(-jy-miny)*ymult;
                //py=1+((point[i].m_y - miny) * ymult);   // Same for Y scale


                int z=round(px);
                if (z<minz) minz=z;  // minz=First pixel
                if (z>maxz) maxz=z;  // maxz=Last pixel

                // Update the Y pixel bounds.
                if (y2<m_drawlist[z].x()) m_drawlist[z].setX(y2);
                if (y1>m_drawlist[z].y()) m_drawlist[z].setY(y1);

            }

            if (done) break;
        }

        if (accel) {
            dp=0;
            // Plot compressed accelerated vertex list
            for (int i=minz;i<maxz;i++) {
                vertarray[vertcnt++]=start_px+i+1;
                vertarray[vertcnt++]=start_py+m_drawlist[i].x();
                vertarray[vertcnt++]=start_px+i+1;
                vertarray[vertcnt++]=start_py+m_drawlist[i].y();
                #if defined(EXTRA_ASSERTS)
                assert(vertcnt<maxverts);
                #endif
            }
        }
    }



    QString b;
    long j=vertcnt/2;
    if (accel) j/=2;
    //b.sprintf("%i %i %i %i",visible_points,sam,num_points,j);
    //float x,y;
    //GetTextExtent(b,x,y);
    //DrawText(w,b,scrx-w.GetRightMargin()-x-15,scry-w.GetTopMargin()-10);

    glColor4ub(col.red(),col.green(),col.blue(),255);

    // Crop to inside the margins.
    glScissor(w.GetLeftMargin(),w.GetBottomMargin(),width,height);
    glEnable(GL_SCISSOR_TEST);
    //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glLineWidth (1);
    bool antialias=pref["UseAntiAliasing"].toBool();
    if (antialias) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendFunc(GL_ONE, GL_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);

    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_SHORT, 0, vertarray);
    glDrawArrays(GL_LINES, 0, vertcnt>>1);
    glDisableClientState(GL_VERTEX_ARRAY);

    if (antialias) {
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_BLEND);
    }
    glDisable(GL_SCISSOR_TEST);
}

