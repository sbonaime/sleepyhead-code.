/*
 gXAxis Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <QDebug>
#include "gXAxis.h"

const qint64 divisors[]={2419200000,1814400000,1209600000,604800000,259200000, 172800000, 86400000,2880000,14400000,7200000,3600000,2700000,1800000,1200000,900000,600000,300000,120000,60000,45000,30000,20000,15000,10000,5000,2000,1000,100,50,10};
const int divcnt=sizeof(divisors)/sizeof(int);

gXAxis::gXAxis(QColor col,bool fadeout)
:Layer(EmptyChannel)
{
    m_line_color=col;
    m_text_color=col;
    m_major_color=Qt::darkGray;
    m_minor_color=Qt::lightGray;
    m_show_major_lines=false;
    m_show_minor_lines=false;
    m_show_minor_ticks=true;
    m_show_major_ticks=true;
    m_fadeout=fadeout;
    QDateTime d=QDateTime::currentDateTime();
    QTime t1=d.time();
    QTime t2=d.toUTC().time();
    tz_offset=t2.secsTo(t1)/60L;
    tz_offset*=60000L;

}
gXAxis::~gXAxis()
{
}
void gXAxis::paint(gGraph & w,int left,int top, int width, int height)
{
    double px,py;

    int start_px=left;
    //int start_py=top;
    //int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
//    float height=scry-(w.GetTopMargin()+w.GetBottomMargin());

    if (width<40)
        return;
    qint64 minx;
    qint64 maxx;
    if (w.blockZoom()) {
        minx=w.rmin_x;
        maxx=w.rmax_x;
    } else {
        minx=w.min_x;
        maxx=w.max_x;
    }
    qint64 xx=maxx-minx;
    if (xx<=0) return;

    //Most of this could be precalculated when min/max is set..
    QString fd,tmpstr;
    int divmax,dividx;
    int fitmode;
    /*if (xx>2*86400000L) {         // Day
        dividx=0;
        divmax=10;
        //fitmode=-1;
        fd="MMM dd";
    } else */
    if (xx>86400000L) {         // Day
        fd="MMM 00";
        dividx=0;
        divmax=15;
        fitmode=0;
    } else if (xx>600000) {    // Minutes
        fd="00:00";
        dividx=6;
        divmax=24;
        fitmode=1;
    } else if (xx>5000) {      // Seconds
        fd="00:00:00";
        dividx=13;
        divmax=24;
        fitmode=2;
    } else {                   // Microseconds
        fd="00:00:00:000";
        dividx=25;
        divmax=divcnt;
        fitmode=3;
    }

    int x,y;
    GetTextExtent(fd,x,y);

    if (x<=0) {
        qWarning() << "gXAxis::Plot() x<=0 font size bug";
        return;
    }

    int max_ticks=width/(x+15);  // Max number of ticks that will fit

    int fit_ticks=0;
    int div=-1;
    qint64 closest=0,tmp,tmpft;
    for (int i=dividx;i<divmax;i++){
        tmpft=xx/divisors[i];
        tmp=max_ticks-tmpft;
        if (tmp<0) continue;
        if (tmpft>closest) {     // Find the closest scale to the number
            closest=tmpft;       // that will fit
            div=i;
            fit_ticks=tmpft;
        }
    }
    if (fit_ticks==0) {
        qDebug() << "gXAxis::Plot() Couldn't fit ticks.. Too short?" << minx << maxx << xx;
        return;
    }
    if ((div<0) || (div>divcnt)) {
        qDebug() << "gXAxis::Plot() div out of bounds";
        return;
    }
    qint64 step=divisors[div];

    //Align left minimum to divisor by losing precision
    qint64 aligned_start=minx/step;
    aligned_start*=step;

    while (aligned_start<minx) {
        aligned_start+=step;
    }

    QColor linecol=Qt::black;
    GLBuffer *lines=w.backlines();

    double xmult=double(width)/double(xx);
    double step_pixels=double(step/10.0)*xmult;
    py=left+double(aligned_start-minx)*xmult;
    for (int i=0;i<10;i++) {
        py-=step_pixels;
        if (py<start_px) continue;
        lines->add(py,top,py,top+4,linecol);
    }

    for (qint64 i=aligned_start;i<maxx;i+=step) {
        px=double(i-minx)*xmult;
        px+=left;
        lines->add(px,top,px,top+6,linecol);
        qint64 j=i+tz_offset;
        int ms=j % 1000;
        int m=(j/60000L) % 60L;
        int h=(j/3600000L) % 24L;
        int s=(j/1000L) % 60L;

        if (fitmode==0) {
            int d=(j/1000);
            QDateTime dt=QDateTime::fromTime_t(d);
            tmpstr=dt.toString("MMM dd");
        //} else if (fitmode==0) {
//            static QString dow[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
//            int d=(j/86400000) % 7;
//            tmpstr=QString("%1 %2:%3").arg(dow[d]).arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0'));
        } else if (fitmode==1) { // minute
            tmpstr=QString("%1:%2").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0'));
        } else if (fitmode==2) { // second
            tmpstr=QString("%1:%2:%3").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'));
        } else if (fitmode==3) { // milli
            tmpstr=QString("%1:%2:%3:%4").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')).arg(ms,3,10,QChar('0'));
        }

        w.renderText(tmpstr,px-(x/2),top+18);
        py=px;
        for (int j=1;j<10;j++) {
            py+=step_pixels;
            if (py>=left+width) break;
            lines->add(py,top,py,top+4,linecol);
        }

        if (lines->full()) {
            qWarning() << "maxverts exceeded in gXAxis::Plot()";
            break;
        }
    }
}

