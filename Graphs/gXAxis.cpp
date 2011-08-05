/*
 gXAxis Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <QDebug>
#include "gXAxis.h"

gXAxis::gXAxis(QColor col)
:gLayer(EmptyChannel)
{
    m_line_color=col;
    m_text_color=col;
    m_major_color=Qt::darkGray;
    m_minor_color=Qt::lightGray;
    m_show_major_lines=false;
    m_show_minor_lines=false;
    m_show_minor_ticks=true;
    m_show_major_ticks=true;

}
gXAxis::~gXAxis()
{
}
void gXAxis::Plot(gGraphWindow & w,float scrx,float scry)
{
    float px,py;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    float width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
//    float height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    if (width<40)
        return;
    qint64 minx;
    qint64 maxx;
    if (w.BlockZoom()) {
        minx=w.rmin_x;
        maxx=w.rmax_x;
    } else {
        minx=w.min_x;
        maxx=w.max_x;
    }
    qint64 xx=maxx-minx;
    if (xx<=0) return;

    qint64 rxx=w.rmax_x-w.rmin_x;

    QString fd,fmt;
    int divisors[]={86400000,3600000,2700000,1800000,1200000,900000,600000,300000,120000,60000,45000,30000,20000,15000,10000,5000,2000,1000,100,50};
    int divcnt=sizeof(divisors)/sizeof(int);
    int divmax,dividx;
    int fitmode;
    if (xx>86400000L) {         // Day
        fd="00 MMM";
        fmt="dd MMM";
        dividx=0;
        divmax=1;
        fitmode=0;
    } else if (xx>60000) {    // Minutes
        fd="00:00";
        dividx=1;
        divmax=10;
        fitmode=1;
    } else if (xx>5000) {      // Seconds
        fd="00:00:00";
        dividx=9;
        divmax=16;
        fitmode=2;
    } else {                   // Microseconds
        fd="00:00:00:000";
        dividx=15;
        divmax=divcnt;
        fitmode=3;
    }

    float x,y;
    GetTextExtent(fd,x,y);

    if (x<=0) { // font size bug
        qWarning() << "gXAxis::Plot() x<=0";
        return;
    }

    float max_ticks=float(width)/(x+10);  // Max number of ticks that will fit

    float fit_ticks=0;
    int div=-1;
    float closest=0,tmp,tmpft;
    for (int i=dividx;i<divmax;i++){
        tmpft=ceil(float(xx)/float(divisors[i]));
        tmp=max_ticks-tmpft;
        if (tmp<0) continue;
        if (tmpft>closest) {     // Find the closest scale to the number
            closest=tmpft;       // that will fit
            div=i;
            fit_ticks=tmpft;
        }
    }
    if (fit_ticks==0) {
        qDebug() << "FitTicks==0!";
        return;
    }
    if (div<0) {
        qDebug() << "div<0";
        return;
    }
    qint64 step=divisors[div];

    qint64 zqvx=minx-w.rmin_x;              // Amount of time before minx
    qint64 zz=zqvx/step;                  // Number of ticks that fit up to minx

    //Align left minimum to divisor
    qint64 rm=w.rmin_x % step;     // Offset from rminx of an aligned time
    rm=step-rm;
    rm+=w.rmin_x;
    //qint64 rd=w.rmin_x / divisors[div];
    //rd*=divisors[div];
    qint64 aligned_start=(zz*step)+rm; // First location of aligned point.
    aligned_start/=step;
    aligned_start*=step;


    while (aligned_start<minx) {
        aligned_start+=step;
    }
    double xmult=double(width)/double(xx);
    w.qglColor(Qt::black);
    QString tmpstr;
    QTime t1=QDateTime::currentDateTime().time();
    QTime t2=QDateTime::currentDateTimeUtc().time();
    qint64 offset=t2.secsTo(t1)*1000;
    for (qint64 i=aligned_start;i<maxx;i+=step) {
        px=double(i-minx)*xmult;
        px+=start_px;
        glBegin(GL_LINES);
        glVertex2f(px,start_py);
        glVertex2f(px,start_py-6);
        glEnd();


        if (fitmode==0) {
        } else {
            qint64 j=i+offset;
            short m=(j/60000) % 60;
            short h=(j/3600000) % 24;
            short s=(j/1000) % 60;
            if (fitmode==1) { // minute
                tmpstr=QString("%1:%2").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0'));
            } else if (fitmode==2) { // second
                tmpstr=QString("%1:%2:%3").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'));
            } else if (fitmode==3) { // milli
                short ms=j % 1000;
                tmpstr=QString("%1:%2:%3:%4").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')).arg(ms,3,10,QChar('0'));
            }
        }
        DrawText(w,tmpstr,px-(x/2),scry-(w.GetBottomMargin()-12),0);

    }
    int i=0;
    return;
    // subtract 1.. align to first div unit


    float zwv2=zqvx/max_ticks;              // Number of ticks that would fit before
    float gv;
    float res=fmodf(zwv2,gv);



    double jj=1/max_ticks;
    double minor_tick=max_ticks*xx;
    double st2=minx; //double(int(frac*1440.0))/1440.0;
    double st,q;

    bool show_seconds=false;
    bool show_milliseconds=false;
    bool show_time=true;

    double min_tick;
    if (xx<86400000) {
        //double rounding[16]={12,24,48,72,96,144,288,720,1440,2880,5760,8640,17280,86400,172800,345600}; // time rounding
        double rounding[16]={7200000,3600000,1800000,1200000,900000,600000,300000,120000,60000,45000,30000,15000,10000,5000,2000,1000};
        int ri;
        for (ri=0;ri<16;ri++) {
            st=round(st2*rounding[ri])/rounding[ri];
            min_tick=round(minor_tick*rounding[ri])/rounding[ri];
            q=xx/min_tick;  // number of ticks that fits in range
            if (q<=jj) break; // compared to number of ticks that fit on screen.
        }
        if (min_tick<60000) show_seconds=true;
        if (min_tick<10000) show_milliseconds=true;

        if (min_tick<=1000)
            min_tick=1000;
    } else { // Day ticks..
        show_time=false;
        st=st2;
        min_tick=1.0;
        double mtiks=(x+20.0)/width;
        double mt=mtiks*xx;
        min_tick=mt;
        //if (min_tick<1.0) min_tick=1.0;
        //if (min_tick>10) min_tick=10;
    }
    if (min_tick<=0) {
        qWarning() << "gXAxis::Plot() min_tick<=0 :(";
        return;
    }

    double st3=st;
    //while (st3>minx) {
    //    st3-=min_tick/10.0;
    //}
    //st3+=min_tick/10.0;

    py=w.GetBottomMargin();

    qint32 vertcnt=0;
    GLshort * vertarray=vertex_array[0];
    if (vertarray==NULL) {
        qWarning() << "VertArray==NULL";
        return;
    }

    if (m_show_minor_ticks) {
        for (double i=st3; i<=maxx; i+=min_tick/10.0) {
            if (i<minx) continue;
            px=(i-minx)*xmult+w.GetLeftMargin();
            vertarray[vertcnt++]=px;
            vertarray[vertcnt++]=py;
            vertarray[vertcnt++]=px;
            vertarray[vertcnt++]=py-4;
            if (vertcnt>=maxverts) {
                qWarning() << "gXAxis::Plot() maxverts exceeded trying to draw minor ticks";
                return;
            }
        }
    }


    //while (st<minx) {
    //    st+=min_tick/10.0;  // mucking with this changes the scrollyness of the ticker.
    //}

    int hour,minute,second,millisecond;
    QDateTime d;
    // Need to use Qpainter clipping..
    // messy. :(
    glScissor(w.GetLeftMargin()-20,0,width+20,w.GetBottomMargin());
    glEnable(GL_SCISSOR_TEST);

    //const double extra=min_tick/10.0;
    const double extra=0;

    for (double i=st; i<=maxx+extra; i+=min_tick) {
        d=QDateTime::fromMSecsSinceEpoch(i);

        if (show_time) {
            minute=d.time().minute();
            hour=d.time().hour();
            second=d.time().second();
            millisecond=d.time().msec();

            if (show_milliseconds) {
                fd.sprintf("%02i:%02i:%02i:%03i",hour,minute,second,millisecond);
            } else if (show_seconds) {
                fd.sprintf("%02i:%02i:%02i",hour,minute,second);
            } else {
                fd.sprintf("%02i:%02i",hour,minute);
            }
        } else {
            fd=d.toString("MMM dd");
        }

        px=(i-minx)*xmult+w.GetLeftMargin();
        if ((i>=st && i<=maxx) && (m_show_major_ticks)) {
            vertarray[vertcnt++]=px;
            vertarray[vertcnt++]=py;
            vertarray[vertcnt++]=px;
            vertarray[vertcnt++]=py-6;
            if (vertcnt>=maxverts) {
                qWarning() << "gXAxis::Plot() maxverts exceeded trying to draw Major ticks";
                return;
            }
        }

        GetTextExtent(fd,x,y);
        if (!show_time) {
            DrawText(w,fd, px+y, scry-(py-(x/2)-8), 90.0);
        } else {
            DrawText(w,fd, px-(x/2), scry-(py-8-y));
        }

    }
    // Draw the little ticks.
    if (vertcnt>=maxverts) {
        qWarning() << "maxverts exceeded in gYAxis::Plot()";
        return;
    }

    glLineWidth(1);
    glColor3f(0,0,0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_SHORT, 0, vertarray);
    glDrawArrays(GL_LINES, 0, vertcnt>>1);
    glDisableClientState(GL_VERTEX_ARRAY); // deactivate vertex arrays after drawing

    glDisable(GL_SCISSOR_TEST);


}

