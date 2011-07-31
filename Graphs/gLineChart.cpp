/********************************************************************
 gLineChart Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <math.h>
#include <QString>
#include <QDebug>
#include <SleepLib/profiles.h>
#include "gLineChart.h"

#define EXTRA_ASSERTS 1
gLineChart::gLineChart(ChannelID code,QColor col,bool square_plot, bool disable_accel)
:gLayer(code),m_square_plot(square_plot),m_disable_accel(disable_accel)
{
    color.clear();
    color.push_back(col);
    m_report_empty=false;
}
gLineChart::~gLineChart()
{
}


// Time Domain Line Chart
void gLineChart::Plot(gGraphWindow & w,float scrx,float scry)
{
    const int max_drawlist_size=4096;
    QPoint m_drawlist[max_drawlist_size];

    if (!m_visible)
        return;
    if (!m_day)
        return;
    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
    int height=scry-(w.GetTopMargin()+w.GetBottomMargin())-2;
    EventDataType miny,maxy;
    double minx,maxx;
    miny=w.min_y, maxy=w.max_y, maxx=w.max_x, minx=w.min_x;

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

    double xx=maxx-minx;
    double xmult=double(width)/xx;

    EventDataType yy=maxy-miny;
    EventDataType ymult=EventDataType(height)/yy;   // time to pixel conversion multiplier

    // Return on screwy min/max conditions
    if ((xx<0) || (yy<0))
        return;
    if ((yy==0) && (miny==0))
        return;

    EventDataType lastpx,lastpy;
    EventDataType px,py;
    int idx;
    bool done,first;
    double x0,xL;
    double sr;
    int sam;
    int minz,maxz;

    // Draw bounding box
    {
        glColor3f (0.0, 0.0, 0.0);
        glLineWidth (1);
        glBegin (GL_LINE_LOOP);
        glVertex2f (start_px, start_py);
        glVertex2f (start_px, start_py+height+2);
        glVertex2f (start_px+width,start_py+height+2);
        glVertex2f (start_px+width, start_py);
        glEnd ();
    }
    width--;

    qint32 vertcnt=0;
    GLshort * vertarray=vertex_array[0];
    if (vertarray==NULL){
        qWarning() << "VertArray==NULL";
        return;
    }

    // Selected the plot line color
    QColor & col=color[0];

    int num_points=0;
    int visible_points=0;
    int total_points=0;
    int total_visible=0;
    bool square_plot,accel;

    for (QVector<Session *>::iterator s=m_day->begin(); s!=m_day->end(); s++) {
        QHash<ChannelID,QVector<EventList *> >::iterator ci=(*s)->eventlist.find(m_code);
        if (ci==(*s)->eventlist.end()) continue;

        QVector<EventList *> & evec=ci.value();
        num_points=0;
        for (int i=0;i<evec.size();i++)
            num_points+=evec[i]->count();
        total_points+=num_points;

        for (int n=0;n<evec.size();n++) { // for each segment
            EventList & el=*evec[n];

            accel=(el.type()==EVL_Waveform); // Turn on acceleration if this is a waveform.
            if (m_disable_accel) accel=false;


            square_plot=m_square_plot;
            if (accel || num_points>1000) { // Don't square plot if too many points or waveform
                square_plot=false;
            }

            int siz=evec[n]->count();
            if (siz<=1) continue; // Don't bother drawing 1 point or less.

            x0=el.time(0);
            xL=el.time(siz-1);

            if (maxx<x0) continue;
            if (xL<minx) continue;

            if (x0>xL) {
                if (siz==2) { // this happens on CPAP
                    quint32 t=el.getTime()[0];
                    el.getTime()[0]=el.getTime()[1];
                    el.getTime()[1]=t;
                    EventStoreType d=el.getData()[0];
                    el.getData()[0]=el.getData()[1];
                    el.getData()[1]=d;

                } else {
                    qDebug() << "Reversed order sample fed to gLineChart - ignored.";
                    continue;
                    //assert(x1<x2);
                }
            }
            if (accel) {
                //x1=el.time(1);
                sr=el.rate();           // Time distance between samples
                if (sr<=0) {
                    qWarning() << "qLineChart::Plot() assert(sr>0)";
                    continue;
                }

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
                    minz=width;
                    maxz=0;
                }
                total_visible+=visible_points;
            } else {
                sam=1;
            }

            // these calculations over estimate
            // The Z? values are much more accurate

            idx=0;

            if (el.type()==EVL_Waveform)  {
                // We can skip data previous to minx if this is a waveform

                if (minx>x0) {
                    double j=minx-x0;  // == starting min of first sample in this segment
                    idx=(j/sr);
                    // Loose the precision
                    //idx-=idx % sam;

                } // else just start from the beginning
            }

            int xst=start_px+1;
            int yst=start_py+1;

            double time;
            EventDataType data;
            EventDataType gain=el.gain();
            EventDataType nmult=ymult*gain;
            EventDataType ymin=EventDataType(miny)/gain;

            const QVector<EventStoreType> & dat=el.getData();
            const QVector<quint32> & tim=el.getTime();

            done=false;
            first=true;

            bool firstpx=true;
            if (el.type()==EVL_Waveform) {  // Waveform Plot
                if (idx>sam) idx-=sam;
                time=el.time(idx);
                double rate=double(sr)*double(sam);

                if (accel) {
//////////////////////////////////////////////////////////////////
// Accelerated Waveform Plot
//////////////////////////////////////////////////////////////////
                    for (int i=idx;i<siz;i+=sam) {
                        time+=rate;
                        //time=el.time(i);
                        //if (time < minx)
                        //    continue; // Skip stuff before the start of our data window

                        //data=el.data(i);
                        data=dat[i];//*gain;
                        px=((time - minx) * xmult);   // Scale the time scale X to pixel scale X
                        py=((data - ymin) * nmult);   // Same for Y scale

                        // In accel mode, each pixel has a min/max Y value.
                        // m_drawlist's index is the pixel index for the X pixel axis.

                        int z=floor(px); // Hmmm... round may screw this up.
                        if (z<minz) minz=z;  // minz=First pixel
                        if (z>maxz) maxz=z;  // maxz=Last pixel

                        // Update the Y pixel bounds.
                        if (py<m_drawlist[z].x()) m_drawlist[z].setX(py);
                        if (py>m_drawlist[z].y()) m_drawlist[z].setY(py);

                        if (time > maxx) {
                            done=true; // Let this iteration finish.. (This point will be in far clipping)
                            break;
                        }
                    }
                    // Plot compressed accelerated vertex list
                    for (int i=minz;i<maxz;i++) {
                        vertarray[vertcnt++]=xst+i;
                        vertarray[vertcnt++]=yst+m_drawlist[i].x();
                        vertarray[vertcnt++]=xst+i;
                        vertarray[vertcnt++]=yst+m_drawlist[i].y();

                        if (vertcnt>=maxverts) break;
                    }

                } else { // Zoomed in Waveform
//////////////////////////////////////////////////////////////////
// Normal Waveform Plot
//////////////////////////////////////////////////////////////////
                    for (int i=idx;i<siz;i+=sam) {
                        time+=rate;
                        //if (time < minx)
                        //    continue; // Skip stuff before the start of our data window
                        data=dat[i];//el.data(i);

                        px=xst+((time - minx) * xmult);   // Scale the time scale X to pixel scale X
                        py=yst+((data - ymin) * nmult);   // Same for Y scale, with precomputed gain

                        if (firstpx) {
                            lastpx=px;
                            lastpy=py;
                            firstpx=false;
                            continue;
                        }
                        vertarray[vertcnt++]=lastpx;
                        vertarray[vertcnt++]=lastpy;
                        vertarray[vertcnt++]=px;
                        vertarray[vertcnt++]=py;

                        if (vertcnt>=maxverts) {
                            done=true;
                            break;
                        }
                        if (time > maxx) {
                            //done=true; // Let this iteration finish.. (This point will be in far clipping)
                            break;
                        }
                        lastpx=px;
                        lastpy=py;
                    }
                }

            } else {
//////////////////////////////////////////////////////////////////
// Standard events/zoomed in Plot
//////////////////////////////////////////////////////////////////
                first=true;
                double start=el.first();
                for (int i=idx;i<siz;i+=sam) {

                    time=start+tim[i];
                    if (first) {
                        if (num_points>5 && (time < minx)) continue; // Skip stuff before the start of our data window
                        first=false;
                        if (i>=sam)  i-=sam; // Start with the previous sample (which will be in clipping area)
                        time=start+tim[i];
                    }
                    data=dat[i]*gain; //
                    //data=el.data(i); // raw access is faster

                    px=xst+((time - minx) * xmult);   // Scale the time scale X to pixel scale X
                    //py=yst+((data - ymin) * nmult);   // Same for Y scale with precomputed gain
                    py=yst+((data - miny) * ymult);   // Same for Y scale with precomputed gain

                    if (firstpx) {
                        firstpx=false;
                    } else {
                        if (square_plot) {
                            vertarray[vertcnt++]=lastpx;
                            vertarray[vertcnt++]=lastpy;
                            vertarray[vertcnt++]=px;
                            vertarray[vertcnt++]=lastpy;
                            vertarray[vertcnt++]=px;
                            vertarray[vertcnt++]=lastpy;
                        } else {
                            vertarray[vertcnt++]=lastpx;
                            vertarray[vertcnt++]=lastpy;
                        }

                        vertarray[vertcnt++]=px;
                        vertarray[vertcnt++]=py;

                        if (vertcnt>=maxverts) {
                            done=true;
                            break;
                        }
                    }
                    lastpx=px;
                    lastpy=py;
                    //if (lastpx>start_px+width) done=true;
                    if (time > maxx) {
                        //done=true; // Let this iteration finish.. (This point will be in far clipping)
                        break;
                    }
                }
            }

            if (done) break;
        }

    }

    if (!total_points) { // No Data?

        if (m_report_empty) {
            QString msg="No Waveform Available";
            float x,y;
            GetTextExtent(msg,x,y,bigfont);
            DrawText(w,msg,start_px+(width/2.0)-(x/2.0),scry-w.GetBottomMargin()-height/2.0+y/2.0,0,Qt::gray,bigfont);
        }
    } else {

        /*QString b;
        long j=vertcnt/2;
        if (accel) j/=2;
        b.sprintf("%i %i %i %li",visible_points,sam,num_points,j);
        float x,y;
        GetTextExtent(b,x,y);
        DrawText(b,scrx-w.GetRightMargin()-x-15,scry-w.GetBottomMargin()-10); */

        glColor4ub(col.red(),col.green(),col.blue(),255);

        // Crop to inside the margins.
        glScissor(w.GetLeftMargin(),w.GetBottomMargin(),width,height+2);
        glEnable(GL_SCISSOR_TEST);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        bool antialias=pref["UseAntiAliasing"].toBool();
        if (antialias) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);
            glLineWidth (1.5);

        } else glLineWidth(1);

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
}

