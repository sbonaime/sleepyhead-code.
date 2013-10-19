/*
 gLineChart Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <QString>
#include <QDebug>
#include <SleepLib/profiles.h>
#include "gLineChart.h"
#include "glcommon.h"

#define EXTRA_ASSERTS 1
gLineChart::gLineChart(ChannelID code,QColor col,bool square_plot, bool disable_accel)
:Layer(code),m_square_plot(square_plot),m_disable_accel(disable_accel)
{
    addPlot(code,col,square_plot);
    m_line_color=col;
    m_report_empty=false;
    addVertexBuffer(lines=new gVertexBuffer(100000,GL_LINES));
    lines->setColor(col);
    lines->setAntiAlias(true);
    lines->setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
gLineChart::~gLineChart()
{
}

bool gLineChart::isEmpty()
{
    if (!m_day) return true;
    for (int j=0;j<m_codes.size();j++) {
        ChannelID code=m_codes[j];
        for (int i=0;i<m_day->size();i++) {
            Session *sess=m_day->getSessions()[i];
            if (sess->channelExists(code))
                return false;
        }
    }
    return true;
}

void gLineChart::SetDay(Day *d)
{
//    Layer::SetDay(d);
    m_day=d;

    m_minx=0,m_maxx=0;
    m_miny=0,m_maxy=0;

    if (!d) return;

    qint64 t64;

    EventDataType min=99999999,max=-999999999,tmp;

    for (int j=0;j<m_codes.size();j++) {
        ChannelID code=m_codes[j];
        for (int i=0;i<d->size();i++) {
            Session *sess=d->getSessions()[i];
            if (!sess->channelExists(code)) continue;

            tmp=sess->Min(code);
            if (min > tmp) min=tmp;

            tmp=sess->Max(code);
            if (max < tmp) max=tmp;

            t64=sess->first(code);
            if (!m_minx || (m_minx > t64)) m_minx=t64;

            t64=sess->last(code);
            if (!m_maxx || (m_maxx < t64)) m_maxx=t64;
        }

    }
    m_miny=min;
    m_maxy=max;

    //if (m_code==CPAP_Leak) {
    // subtract_offset=profile.cpap.[IntentionalLeak].toDouble();
    //} else
    subtract_offset=0;


}
EventDataType gLineChart::Miny()
{
    int m=Layer::Miny();
    if (subtract_offset>0) {
        m-=subtract_offset;
        if (m<0) m=0;
    }
    return m;
}
EventDataType gLineChart::Maxy()
{
    return Layer::Maxy()-subtract_offset;
}

// Time Domain Line Chart
void gLineChart::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible)
        return;

    if (!m_day)
        return;

    //if (!m_day->channelExists(m_code)) return;

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

    // hmmm.. subtract_offset..

    /*if (miny<0) {
        miny=-MAX(fabs(miny),fabs(maxy));
    }*/



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
    bool done;
    double x0,xL;
    double sr;
    int sam;
    int minz,maxz;

    // Draw bounding box
    gVertexBuffer *outlines=w.lines();
    GLuint blk=QColor(Qt::black).rgba();
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
    qint64 clockdrift=qint64(PROFILE.cpap->clockDrift()) * 1000L;
    qint64 drift=0;

    QHash<ChannelID,QVector<EventList *> >::iterator ci;

    //m_line_color=schema::channel[m_code].defaultColor();
    int legendx=left+width;

    int codepoints;
    //GLuint color;
    for (int gi=0;gi<m_codes.size();gi++) {
        ChannelID code=m_codes[gi];
        //m_line_color=m_colors[gi];
        lines->setColor(m_colors[gi]);
        //color=m_line_color.rgba();

        codepoints=0;
        for (int svi=0;svi<m_day->size();svi++) {
            Session *sess=(*m_day)[svi];
            if (!sess) {
                qWarning() << "gLineChart::Plot() NULL Session Record.. This should not happen";
                continue;
            }

            drift=(sess->machine()->GetType()==MT_CPAP) ? clockdrift : 0;

            if (!sess->enabled()) continue;
            schema::Channel ch=schema::channel[code];
            bool fndbetter=false;
            for (QList<schema::Channel *>::iterator l=ch.m_links.begin();l!=ch.m_links.end();l++) {
                schema::Channel *c=*l;
                ci=(*m_day)[svi]->eventlist.find(c->id());
                if (ci!=(*m_day)[svi]->eventlist.end()) {
                    fndbetter=true;
                    break;
                }

            }
            if (!fndbetter) {
                ci=(*m_day)[svi]->eventlist.find(code);
                if (ci==(*m_day)[svi]->eventlist.end()) continue;
            }


            QVector<EventList *> & evec=ci.value();
            num_points=0;
            for (int i=0;i<evec.size();i++)
                num_points+=evec[i]->count();
            total_points+=num_points;
            codepoints+=num_points;
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
                if (accel || num_points>20000) { // Don't square plot if too many points or waveform
                    square_plot=false;
                }

                int siz=evec[n]->count();
                if (siz<=1) continue; // Don't bother drawing 1 point or less.

                x0=el.time(0)+drift;
                xL=el.time(siz-1)+drift;

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
                //EventDataType nmult=ymult*gain;
                //EventDataType ymin=EventDataType(miny)/gain;

                //const QVector<EventStoreType> & dat=el.getData();
                //const QVector<quint32> & tim=el.getTime();
                //quint32 * tptr;


                //qint64 stime=el.first();

                done=false;

//                if (!accel) {
                lines->setSize(1.5);
//                } else lines->setSize(1);

                if (el.type()==EVL_Waveform) {  // Waveform Plot
                    if (idx > sam) idx-=sam;
                    time=el.time(idx) + drift;
                    double rate=double(sr)*double(sam);
                    EventStoreType * ptr=el.rawData()+idx;

                    if (accel) {
                        //////////////////////////////////////////////////////////////////
                        // Accelerated Waveform Plot
                        //////////////////////////////////////////////////////////////////

//                        qint64 tmax=(maxx-time)/rate;
//                        if ((tmax*sam) < siz) {
//                            siz=idx+tmax*sam;
//                            done=true;
//                        }

                        for (int i=idx;i<siz;i+=sam,ptr+=sam) {
                            time+=rate;
                            // This is much faster than QVector access.
                            data=*ptr;
                            data *= gain;

                            // Scale the time scale X to pixel scale X
                            px=((time - minx) * xmult);

                            // Same for Y scale, with gain factored in nmult
                            py=((data - miny) * ymult);

                            // In accel mode, each pixel has a min/max Y value.
                            // m_drawlist's index is the pixel index for the X pixel axis.
                            int z=round(px); // Hmmm... round may screw this up.

                            if (z<minz)
                                minz=z;  // minz=First pixel

                            if (z>maxz)
                                maxz=z;  // maxz=Last pixel

                            if (minz<0) {
                                qDebug() << "gLineChart::Plot() minz<0  should never happen!! minz =" << minz;
                                minz=0;
                            }
                            if (maxz>max_drawlist_size) {
                                qDebug() << "gLineChart::Plot() maxz>max_drawlist_size!!!! maxz = " << maxz << " max_drawlist_size =" << max_drawlist_size;
                                maxz=max_drawlist_size;
                            }

                            // Update the Y pixel bounds.
                            if (py<m_drawlist[z].x())
                                m_drawlist[z].setX(py);
                            if (py>m_drawlist[z].y())
                                m_drawlist[z].setY(py);

                            if (time>maxx) {
                                done=true;
                                break;
                            }

                        }
                        // Plot compressed accelerated vertex list
                        if (maxz>width) {
                            maxz=width;
                        }
                        float ax1,ay1;
                        QPoint * drl=m_drawlist+minz;
                        // Don't need to cap VertexBuffer here, as it's limited to max_drawlist_size anyway


                        // Cap within VertexBuffer capacity, one vertex per line point
                        int np=(maxz-minz)*2;

                        int j=lines->Max()-lines->cnt();
                        if (np < j) {
                            for (int i=minz;i<maxz;i++, drl++) {
                                ax1=drl->x();
                                ay1=drl->y();
                                lines->unsafe_add(xst+i,yst-ax1,xst+i,yst-ay1);

                                //if (lines->full()) break;
                            }
                        } else {
                            qDebug() << "gLineChart full trying to draw" << schema::channel[code].label();
                            done=true;
                        }

                    } else { // Zoomed in Waveform
                        //////////////////////////////////////////////////////////////////
                        // Normal Waveform Plot
                        //////////////////////////////////////////////////////////////////

                        // Cap within VertexBuffer capacity, one vertex per line point
//                        int np=((siz-idx)/sam)*2;
//                        int j=lines->Max()-lines->cnt();
//                        if (np > j) {
//                            siz=j*sam;
//                        }

                        // Prime first point
                        data=*ptr * gain;
                        lastpx=xst+((time - minx) * xmult);
                        lastpy=yst-((data - miny) * ymult);

                        for (int i=idx;i<siz;i+=sam) {
                            ptr+=sam;
                            time+=rate;

                            data=*ptr * gain;

                            px=xst+((time - minx) * xmult);   // Scale the time scale X to pixel scale X
                            py=yst-((data - miny) * ymult);   // Same for Y scale, with precomputed gain
                            //py=yst-((data - ymin) * nmult);   // Same for Y scale, with precomputed gain

                            lines->add(lastpx,lastpy,px,py);

                            lastpx=px;
                            lastpy=py;

                            if (time>maxx) {
                                done=true;
                                break;
                            }
                            if (lines->full())
                                break;
                        }
                    }

                } else  {
                    //////////////////////////////////////////////////////////////////
                    // Standard events/zoomed in Plot
                    //////////////////////////////////////////////////////////////////

                    double start=el.first() + drift;

                    quint32 * tptr=el.rawTime();

                    int idx=0;

                    if (siz>15) {
                        for (;idx<siz;++idx) {
                            time=start + *tptr++;
                            if (time >= minx) {
                                break;
                            }
                        }

                        if (idx > 0) {
                            idx--;
                            //tptr--;
                        }
                    }

                    // Step one backwards if possible (to draw through the left margin)
                    EventStoreType * dptr=el.rawData() + idx;
                    tptr=el.rawTime() + idx;

                    time=start + *tptr++;
                    data=*dptr++ * gain;

                    idx++;

                    lastpx=xst+((time - minx) * xmult);   // Scale the time scale X to pixel scale X
                    lastpy=yst-((data - miny) * ymult);   // Same for Y scale without precomputed gain

                    siz-=idx;

                    // Check if would overflow lines gVertexBuffer
                    int gs=siz << 1;
                    int j=lines->Max()-lines->cnt();
                    if (square_plot)
                        gs <<= 1;
                    if (gs > j) {
                        qDebug() << "Would overflow line points.. increase default VertexBuffer size in gLineChart";
                        siz=(j >> square_plot) ? 2 : 1;
                        done=true; // end after this partial draw..
                    }

                    // Unrolling square plot outside of loop to gain a minor speed improvement.
                    EventStoreType *eptr=dptr+siz;
                    if (square_plot) {
                        for (; dptr < eptr; dptr++) {
                            time=start + *tptr++;
                            data=gain * *dptr;

                            px=xst+((time - minx) * xmult);   // Scale the time scale X to pixel scale X
                            py=yst-((data - miny) * ymult);   // Same for Y scale without precomputed gain

                            // Horizontal lines are easy to cap
                            if (py==lastpy) {
                                // Cap px to left margin
                                if (lastpx<xst) lastpx=xst;

                                // Cap px to right margin
                                if (px>xst+width) px=xst+width;

                                lines->unsafe_add(lastpx,lastpy,px,lastpy,px,lastpy,px,py);
                            } else {
                                // Letting the scissor do the dirty work for non horizontal lines
                                // This really should be changed, as it might be cause that weird
                                // display glitch on Linux..
                                lines->unsafe_add(lastpx,lastpy,px,lastpy,px,lastpy,px,py);
                            }

                            lastpx=px;
                            lastpy=py;

                            if (time > maxx) {
                                done=true; // Let this iteration finish.. (This point will be in far clipping)
                                break;
                            }
                        }
                    } else {
                        for (; dptr < eptr; dptr++) {
                        //for (int i=0;i<siz;i++) {
                            time=start + *tptr++;
                            data=gain  * *dptr;

                            px=xst+((time - minx) * xmult);   // Scale the time scale X to pixel scale X
                            py=yst-((data - miny) * ymult);   // Same for Y scale without precomputed gain

                            // Horizontal lines are easy to cap
                            if (py==lastpy) {
                                // Cap px to left margin
                                if (lastpx<xst) lastpx=xst;

                                // Cap px to right margin
                                if (px>xst+width) px=xst+width;

                                lines->unsafe_add(lastpx,lastpy,px,py);
                            } else {
                                // Letting the scissor do the dirty work for non horizontal lines
                                // This really should be changed, as it might be cause that weird
                                // display glitch on Linux..
                                lines->unsafe_add(lastpx,lastpy,px,py);
                            }

                            lastpx=px;
                            lastpy=py;

                            if (time > maxx) { // Past right edge, abort further drawing..
                                done=true;
                                break;
                            }
                        }
                    }
                }

                if (done) break;
            }
        }

        ////////////////////////////////////////////////////////////////////
        // Draw Legends on the top line
        ////////////////////////////////////////////////////////////////////

        QFontMetrics fm(*defaultfont);
        int bw=fm.xHeight();
        int bh=fm.height()/1.5;

        if ((codepoints>0)) { //(m_codes.size()>1) &&
            QString text=schema::channel[code].label();
            int wid,hi;
            GetTextExtent(text,wid,hi);
            legendx-=wid;
            w.renderText(text,legendx,top-4);

            int tp=top-5-bh/2;
            w.quads()->add(legendx-bw,tp+bh/2,legendx,tp+bh/2,legendx,tp-bh/2,legendx-bw,tp-bh/2,m_colors[gi].rgba());
            legendx-=bw*2;
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


AHIChart::AHIChart(QColor col)
:Layer(NoChannel),m_color(col)
{
    m_miny=m_maxy=0;
    addVertexBuffer(lines=new gVertexBuffer(100000,GL_LINES));
    lines->setColor(col);
    lines->setAntiAlias(true);
    lines->setSize(1.5);
}

AHIChart::~AHIChart()
{
}

void AHIChart::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible)
        return;

    if (!m_day)
        return;

    // Draw bounding box
    gVertexBuffer *outlines=w.lines();
    GLuint blk=QColor(Qt::black).rgba();
    outlines->add(left, top, left, top+height, blk);
    outlines->add(left, top+height, left+width,top+height, blk);
    outlines->add(left+width,top+height, left+width, top, blk);
    outlines->add(left+width, top, left,top, blk);
    width--;
    height-=2;

    EventDataType miny,maxy;
    double minx,maxx;
    miny=w.min_y, maxy=w.max_y;

    maxx=w.max_x, minx=w.min_x;

    // hmmm.. subtract_offset..

    w.roundY(miny,maxy);

    double xx=maxx-minx;
    double xmult=double(width)/xx;

    EventDataType yy=maxy-miny;
    EventDataType ymult=EventDataType(height-3)/yy;   // time to pixel conversion multiplier

    bool first=true;
    double px,py;
    double lastpx,lastpy;
    double top1=top+height;
    bool done=false;
    //GLuint color=m_color.rgba();
    for (int i=0;i<m_time.size();i++) {
        qint64 ti=m_time[i];
        EventDataType v=m_data[i];
        if (ti<minx) continue;
        if (ti>maxx) done=true;
        if (first) {
            if (i>0) {
                ti=m_time[i-1];
                v=m_data[i-1];
                i--;
            }
            px=left+(double(ti-minx)*xmult);
            py=top1-(double(v-miny)*ymult);
            first=false;
        } else {
           px=left+(double(ti-minx)*xmult);
           py=top1-(double(v-miny)*ymult);
           lines->add(px,py,lastpx,lastpy);
        }
        lastpx=px;
        lastpy=py;
        if (done) break;
    }
    lines->scissor(left,w.flipY(top+height+2),width+1,height+1);
}

void AHIChart::SetDay(Day *d)
{
    m_day=d;
    m_data.clear();
    m_time.clear();
    m_maxy=0;
    m_miny=0;

    if (!d) return;
    m_miny=9999;
    QVector<Session *>::iterator s;
    qint64 first=d->first();
    qint64 last=d->last();
    qint64 f;

    qint64 winsize=30000; // 30 second windows
    for (qint64 ti=first;ti<last;ti+=winsize) {
        f=ti-3600000L;
        //if (f<first) f=first;
        EventList *el[6];
        EventDataType ahi=0;
        int cnt=0;

        qint64 clockdrift=(qint64(PROFILE.cpap->clockDrift())*1000L),drift=0;

        bool fnd=false;
        for (s=d->begin();s!=d->end();s++) {
            if (!(*s)->enabled()) continue;
            Session *sess=*s;
            if ((ti < sess->first()) || (f > sess->last())) continue;
            drift=(sess->machine()->GetType()==MT_CPAP) ? clockdrift : 0;

            // Drop off suddenly outside of sessions
            //if (ti>sess->last()) continue;

            fnd=true;
            if (sess->eventlist.contains(CPAP_Obstructive))
                el[0]=sess->eventlist[CPAP_Obstructive][0];
            else el[0]=NULL;
            if (sess->eventlist.contains(CPAP_Apnea))
                el[1]=sess->eventlist[CPAP_Apnea][0];
            else el[1]=NULL;
            if (sess->eventlist.contains(CPAP_Hypopnea))
                el[2]=sess->eventlist[CPAP_Hypopnea][0];
            else el[2]=NULL;
            if (sess->eventlist.contains(CPAP_ClearAirway))
                el[3]=sess->eventlist[CPAP_ClearAirway][0];
            else el[3]=NULL;
            if (sess->eventlist.contains(CPAP_NRI))
                el[4]=sess->eventlist[CPAP_NRI][0];
            else el[4]=NULL;
            int znt=5;
            if (PROFILE.general->calculateRDI()) {
                if (sess->eventlist.contains(CPAP_RERA)) {// What about ExP??
                    el[5]=sess->eventlist[CPAP_RERA][0];
                    znt++;
                } else el[5]=NULL;
            }
            qint64 t;
            for (int i=0;i<znt;i++) {
                if (!el[i]) continue;
                for (quint32 j=0;j<el[i]->count();j++) {
                    t=el[i]->time(j)+drift;
                    if ((t>=f) && (t<ti)) {
                        cnt++;
                    }
                }
            }
        }
        if (!fnd) cnt=0;
        double g=double(ti-f)/3600000.0;
        if (g>0) ahi=cnt/g;

        if (ahi<m_miny) m_miny=ahi;
        if (ahi>m_maxy) m_maxy=ahi;
        m_time.append(ti);
        m_data.append(ahi);
    }
    m_minx=first;
    m_maxx=last;
}
