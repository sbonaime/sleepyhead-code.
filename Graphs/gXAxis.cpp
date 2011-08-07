/*
 gXAxis Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <QDebug>
#include "gXAxis.h"

const int divisors[]={86400000,2880000,14400000,7200000,3600000,2700000,1800000,1200000,900000,600000,300000,120000,60000,45000,30000,20000,15000,10000,5000,2000,1000,100,50,10};
const int divcnt=sizeof(divisors)/sizeof(int);

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
    QDateTime d=QDateTime::currentDateTime();
    QTime t1=d.time();
    QTime t2=d.toUTC().time();
    tz_offset=t2.secsTo(t1)/60L;
    tz_offset*=60000L;
    //offset=0;

}
gXAxis::~gXAxis()
{
}
void gXAxis::Plot(gGraphWindow & w,float scrx,float scry)
{
    double px,py;

    int start_px=w.GetLeftMargin();
    int start_py=w.GetBottomMargin();
    int width=scrx-(w.GetLeftMargin()+w.GetRightMargin());
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

    //Most of this could be precalculated when min/max is set..
    QString fd,tmpstr;
    int divmax,dividx;
    int fitmode;
    if (xx>86400000L) {         // Day
        fd="00:00";
        dividx=0;
        divmax=10;
        fitmode=0;
    } else if (xx>600000) {    // Minutes
        fd="00:00";
        dividx=4;
        divmax=18;
        fitmode=1;
    } else if (xx>5000) {      // Seconds
        fd="00:00:00";
        dividx=9;
        divmax=20;
        fitmode=2;
    } else {                   // Microseconds
        fd="00:00:00:000";
        dividx=19;
        divmax=divcnt;
        fitmode=3;
    }

    float x,y;
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

    qint32 vertcnt=0;
    GLshort * vertarray=vertex_array[0];
    if (vertarray==NULL) {
        qWarning() << "VertArray==NULL";
        return;
    }

    while (aligned_start<minx) {
        aligned_start+=step;
    }

    double xmult=double(width)/double(xx);
    double step_pixels=double(step/10.0)*xmult;
    py=start_px+double(aligned_start-minx)*xmult;
    for (int i=0;i<10;i++) {
        py-=step_pixels;
        if (py<start_px) continue;
        vertarray[vertcnt++]=py;
        vertarray[vertcnt++]=start_py;
        vertarray[vertcnt++]=py;
        vertarray[vertcnt++]=start_py-4;
    }
    w.qglColor(Qt::black);
    for (qint64 i=aligned_start;i<maxx;i+=step) {
        px=double(i-minx)*xmult;
        px+=start_px;
        vertarray[vertcnt++]=px;
        vertarray[vertcnt++]=start_py;
        vertarray[vertcnt++]=px;
        vertarray[vertcnt++]=start_py-6;
        qint64 j=i+tz_offset;
        int ms=j % 1000;
        int m=(j/60000L) % 60L;
        int h=(j/3600000L) % 24L;
        int s=(j/1000L) % 60L;

        if (fitmode==0) {
            int day=(j/86400000) % 7;
            tmpstr=QString("XXX %1:%2").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0'));
        } else if (fitmode==1) { // minute
            tmpstr=QString("%1:%2").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0'));
        } else if (fitmode==2) { // second
            tmpstr=QString("%1:%2:%3").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'));
        } else if (fitmode==3) { // milli
            tmpstr=QString("%1:%2:%3:%4").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')).arg(ms,3,10,QChar('0'));
        }

        //w.renderText(px-(x/2),scry-(w.GetBottomMargin()-18),tmpstr);
        DrawText(w,tmpstr,px-(x/2),scry-(w.GetBottomMargin()-18),0);
        py=px;
        for (int j=1;j<10;j++) {
            py+=step_pixels;
            if (py>=scrx-w.GetRightMargin()) break;
            vertarray[vertcnt++]=py;
            vertarray[vertcnt++]=start_py;
            vertarray[vertcnt++]=py;
            vertarray[vertcnt++]=start_py-4;
            if (vertcnt>=maxverts) {
                break;
            }
        }

        if (vertcnt>=maxverts) {
            qWarning() << "maxverts exceeded in gXAxis::Plot()";
            break;
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glLineWidth(1);
    w.qglColor(Qt::black);
    glVertexPointer(2, GL_SHORT, 0, vertarray);
    glDrawArrays(GL_LINES, 0, vertcnt>>1);
    glDisableClientState(GL_VERTEX_ARRAY); // deactivate vertex arrays after drawing

   // glDisable(GL_SCISSOR_TEST);

}

