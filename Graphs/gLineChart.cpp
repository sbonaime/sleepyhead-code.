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
:Layer(code),m_square_plot(square_plot),m_disable_accel(disable_accel)
{
    m_line_color=col;
    m_report_empty=false;
    addGLBuf(lines=new GLBuffer(col,100000,GL_LINES));

    lines->setAntiAlias(true);
}
gLineChart::~gLineChart()
{
    delete lines;
    //delete outlines;
}


// Time Domain Line Chart
void gLineChart::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible)
        return;

    if (!m_day)
        return;

    if (!m_day->channelExists(m_code)) return;

    if (width<0)
        return;

   // lines=w.lines();
    EventDataType miny,maxy;
    double minx,maxx;
    miny=w.min_y, maxy=w.max_y;

    if (w.blockZoom()) {
        minx=w.rmin_x, maxx=w.rmax_x;
    } else {
        maxx=w.max_x, minx=w.min_x;
    }


    if (miny<0) {
        miny=-MAX(fabs(miny),fabs(maxy));
    }

    w.roundY(miny,maxy);

    double xx=maxx-minx;
    double xmult=double(width)/xx;

    EventDataType yy=maxy-miny;
    EventDataType ymult=EventDataType(height-3)/yy;   // time to pixel conversion multiplier

    // Return on screwy min/max conditions
    if (xx<0)
        return;
    if (yy<=0) {
        if (miny==0)
            return;
     }

    EventDataType lastpx,lastpy;
    EventDataType px,py;
    int idx;
    bool done,first;
    double x0,xL;
    double sr;
    int sam;
    int minz,maxz;

    // Draw bounding box
    GLBuffer *outlines=w.lines();
    QColor blk=Qt::black;
    outlines->add(left, top, left, top+height, blk);
    outlines->add(left, top+height, left+width,top+height, blk);
    outlines->add(left+width,top+height, left+width, top, blk);
    outlines->add(left+width, top, left,top, blk);
    width--;
    height-=2;

    int num_points=0;
    int visible_points=0;
    int total_points=0;
    int total_visible=0;
    bool square_plot,accel;

    QHash<ChannelID,QVector<EventList *> >::iterator ci;

    for (int svi=0;svi<m_day->size();svi++) {
        if (!(*m_day)[svi]) {
            qWarning() << "gLineChart::Plot() NULL Session Record.. This should not happen";
            continue;
        }
        ci=(*m_day)[svi]->eventlist.find(m_code);
        if (ci==(*m_day)[svi]->eventlist.end()) continue;

        QVector<EventList *> & evec=ci.value();
        num_points=0;
        for (int i=0;i<evec.size();i++)
            num_points+=evec[i]->count();
        total_points+=num_points;

        const int num_averages=20;  // Max n umber of samples taken from samples per pixel for better min/max values
        for (int n=0;n<evec.size();n++) { // for each segment
            EventList & el=*evec[n];

            accel=(el.type()==EVL_Waveform); // Turn on acceleration if this is a waveform.
            if (accel) {
                sr=el.rate();           // Time distance between samples
                if (sr<=0) {
                    qWarning() << "qLineChart::Plot() assert(sr>0)";
                    continue;
                }
            }
            if (m_disable_accel) accel=false;


            square_plot=m_square_plot;
            if (accel || num_points>5000) { // Don't square plot if too many points or waveform
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

                double XR=xx/sr;
                double Z1=MAX(x0,minx);
                double Z2=MIN(xL,maxx);
                double ZD=Z2-Z1;
                double ZR=ZD/sr;
                double ZQ=ZR/XR;
                double ZW=ZR/(width*ZQ);
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
                    //idx/=(sam*num_averages);
                    //idx*=(sam*num_averages);
                    // Loose the precision
                    idx+=sam-(idx % sam);

                } // else just start from the beginning
            }

            int xst=left+1;
            int yst=top+height+1;

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

                        int z=round(px); // Hmmm... round may screw this up.
                        if (z<minz) minz=z;  // minz=First pixel
                        if (z>maxz) maxz=z;  // maxz=Last pixel
                        if (minz<0) {
                            qDebug() << "gLineChart::Plot() minz<0  should never happen!! minz =" << minz;
                            minz=0;
                        }
                        if (maxz>max_drawlist_size) {
                            qDebug() << "gLineChart::Plot() maxz>max_drawlist_size!!!! maxz = " << maxz << " max_drawlist_size =" << max_drawlist_size;
                            maxz=max_drawlist_size;
                        }

                        // Update the Y pixel bounds.
                        if (py<m_drawlist[z].x()) m_drawlist[z].setX(py);
                        if (py>m_drawlist[z].y()) m_drawlist[z].setY(py);

                        if (time > maxx) {
                            done=true; // Let this iteration finish.. (This point will be in far clipping)
                            break;
                        }
                    }
                    // Plot compressed accelerated vertex list
                    if (maxz>width) {
                        //qDebug() << "gLineChart::Plot() maxz exceeded graph width" << "maxz = " << maxz << "width =" << width;
                        maxz=width;
                    }
                    float ax1,ay1;
                    for (int i=minz;i<maxz;i++) {
                       // ax1=(m_drawlist[i-1].x()+m_drawlist[i].x()+m_drawlist[i+1].x())/3.0;
                       // ay1=(m_drawlist[i-1].y()+m_drawlist[i].y()+m_drawlist[i+1].y())/3.0;
                        ax1=m_drawlist[i].x();
                        ay1=m_drawlist[i].y();
                        lines->add(xst+i,yst-ax1,xst+i,yst-ay1,m_line_color);

                        if (lines->full()) break;
                    }

                } else { // Zoomed in Waveform
//////////////////////////////////////////////////////////////////
// Normal Waveform Plot
//////////////////////////////////////////////////////////////////
                    if (idx>sam) {
                        idx-=sam;
                        time=el.time(idx);
                        //double rate=double(sr)*double(sam);
                    }
                    for (int i=idx;i<siz;i+=sam) {
                        time+=rate;
                        //if (time < minx)
                        //    continue; // Skip stuff before the start of our data window
                        data=dat[i];//el.data(i);

                        px=xst+((time - minx) * xmult);   // Scale the time scale X to pixel scale X
                        py=yst-((data - ymin) * nmult);   // Same for Y scale, with precomputed gain

                        if (firstpx) {
                            lastpx=px;
                            lastpy=py;
                            firstpx=false;
                            continue;
                        }
                        lines->add(lastpx,lastpy,px,py,m_line_color);

                        if (lines->full()) {
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
                for (int i=0;i<siz;i++) {

                    time=start+tim[i];
                    if (first) {
                        if (num_points>15 && (time < minx)) continue; // Skip stuff before the start of our data window
                        first=false;
                        if (i>0)  i--; // Start with the previous sample (which will be in clipping area)
                        time=start+tim[i];
                    }
                    data=dat[i]*gain; //
                    //data=el.data(i); // raw access is faster

                    px=xst+((time - minx) * xmult);   // Scale the time scale X to pixel scale X
                    //py=yst+((data - ymin) * nmult);   // Same for Y scale with precomputed gain
                    py=yst-((data - miny) * ymult);   // Same for Y scale with precomputed gain

                    if (px<left) px=left;
                    if (px>left+width) px=left+width;
                    if (firstpx) {
                        firstpx=false;
                    } else {
                        if (square_plot) {
                            lines->add(lastpx,lastpy,px,lastpy,m_line_color);
                            lines->add(px,lastpy,px,py,m_line_color);
                        } else {
                            lines->add(lastpx,lastpy,px,py,m_line_color);
                        }

                        //lines->add(px,py,m_line_color);

                        if (lines->full()) {
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
            int x,y;
            GetTextExtent(msg,x,y,bigfont);
            //DrawText(w,msg,left+(width/2.0)-(x/2.0),scry-w.GetBottomMargin()-height/2.0+y/2.0,0,Qt::gray,bigfont);
        }
    } else {
        lines->scissor(left,w.flipY(top+height+2),width+1,height+1);
        //lines->draw();
    }
}

