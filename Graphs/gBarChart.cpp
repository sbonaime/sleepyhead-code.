/*
 gBarChart Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <math.h>
#include <QDateTime>
#include "gBarChart.h"

gBarChart::gBarChart(ChannelID code,QColor color,Qt::Orientation o)
:Layer(code),m_orientation(o)
{
    //Xaxis=new gXAxis();
    addGLBuf(quads=new GLBuffer(color,20000,GL_QUADS));
    //addGLBuf(lines=new GLBuffer(col,20,GL_LINES));
    quads->forceAntiAlias(true);
    //lines->setAntiAlias(true);
    //lines->setSize(2);
    m_empty=true;
}
gBarChart::~gBarChart()
{
    //delete Xaxis;
}
void gBarChart::SetDay(Day * day)
{

    if (!m_profile) {
        qWarning() << "Forgot to set profile for gBarChart dummy!";
        m_day=NULL;
        return;
    }
    Layer::SetDay(day);
    //m_empty=true;
   // if (!day) return;
    m_empty=false;
    /*for (int i=0;i<m_codes.size();i++) {
        if (day->count(m_codes[i])>0) {
            m_empty=false;
            break;
        }
    } */

    m_values.clear();
    m_miny=9999999;
    m_maxy=-9999999;
    m_minx=0;
    m_maxx=0;

    int dn;
    EventDataType tmp,total;
    ChannelID code;

    m_fday=0;
    qint64 tt;

    for (QMap<QDate,QVector<Day *> >::iterator d=m_profile->daylist.begin();d!=m_profile->daylist.end();d++) {
        tt=QDateTime(d.key(),QTime(0,0,0),Qt::UTC).toTime_t();
        //tt=QDateTime(d.key(),QTime(12,0,0)).toTime_t();
        dn=tt/86400;
        tt*=1000L;
        if (!m_minx || tt<m_minx) m_minx=tt;
        if (!m_maxx || tt>m_maxx) m_maxx=tt;


        total=0;
        bool fnd=false;
        for (int j=0;j<m_codes.size();j++) {
            code=m_codes[j];
            for (int i=0;i<d.value().size();i++) {
                Day *day=d.value()[i];
                if (day->channelExists(code)) { // too many lookups happening here.. stop the crap..
                    tmp=day->count(code)/day->hours();
                    if (tmp>0) {
                        fnd=true;
                        total+=tmp;
                        m_values[dn][code]=tmp;
                        break;
                    }
                }
            }
        }
        if (fnd) {
            if (!m_fday) m_fday=dn;
            m_values[dn][EmptyChannel]=total;
            if (total<m_miny) m_miny=total;
            if (total>m_maxy) m_maxy=total;
       }
    }
    m_maxy=ceil(m_maxy);
    //m_miny=floor(m_miny);
    m_miny=0;
    //m_minx-=86400000L;

   // m_minx=qint64(QDateTime(m_profile->FirstDay(),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;
    m_maxx=qint64(QDateTime(m_profile->LastDay().addDays(1),QTime(0,0,0),Qt::UTC).toTime_t())*1000L;

    //m_maxx=qint64(QDateTime(m_profile->LastDay().addDays(1),QTime(12,0,0)).toTime_t())*1000L;
    int i=0;

    //set miny & maxy here.. how?
}

void gBarChart::paint(gGraph & w,int left, int top, int width, int height)
{
    if (!m_visible) return;
    //if (!m_day) return;

    GLBuffer *lines=w.lines();
    QColor blk=Qt::black;
    lines->add(left, top, left, top+height, blk);
    lines->add(left, top+height, left+width,top+height, blk);
    lines->add(left+width,top+height, left+width, top, blk);
    lines->add(left+width, top, left, top, blk);

    qint64 minx=w.min_x, maxx=w.max_x;
    //qint64 minx=m_minx, maxx=m_maxx;
    qint64 xx=maxx - minx;
    float days=double(xx)/86400000.0;

    EventDataType maxy=m_maxy;
    EventDataType miny=m_miny;

    w.roundY(miny,maxy);

    EventDataType yy=maxy-miny;
    EventDataType ymult=float(height-2)/yy;

    float barw=(float(width)/float(days));

    qint64 ts;

    float px=left;
    float py;
    EventDataType total;

    int daynum=0;
    float h,tmp;


    qint64 offs=(minx) % 86400000L;
    //zz*=86400000L;
    float offset=(offs)/86400000.0;
    //offset+=float(utcoff)/86400000.0;

    offset*=barw;
    px=left-offset;

    int total_days=0;
    double total_val=0;
    for (qint64 Q=minx;Q<=maxx+86400000L;Q+=86400000L) {
        int zd=Q/86400000L;
        QHash<int,QMap<ChannelID,EventDataType> >::iterator d=m_values.find(zd);
        if (Q<minx) continue;
        if (Q>maxx+86400000) continue;  // break; // out of order if I end up using a hash instead.??
        if (d!=m_values.end()) {
            int x1=px,x2=px+barw;

            if (x1<left) x1=left;
            if (x2>left+width) x2=left+width;
            if (x2<x1) continue;
            ChannelID code;
            total=d.value()[EmptyChannel];


            if (total>0) {
                total_val+=total;
                total_days++;
            }
            py=top+height;
            for (int j=0;j<m_codes.size();j++) {
                code=m_codes[j];
                QMap<ChannelID,EventDataType>::iterator g=d.value().find(code);
                if (g!=d.value().end()) {
                    if (code==EmptyChannel) continue;
                    //look up it's color key
                    QColor & col=m_colors[j];
                    int cr=col.red()*3;
                    int cg=col.green()*3;
                    int cb=col.blue()*3;
                    if (cr<64) cr=64;
                    if (cg<64) cg=64;
                    if (cb<64) cb=64;
                    if (cr>255) cr=255;
                    if (cg>255) cg=255;
                    if (cb>255) cb=255;
                    QColor col2=QColor(cr,cg,cb,255);
                    //col2=QColor(220,220,220,255);

                    tmp=g.value(); //(g.value()/float(total));
                    h=tmp*ymult; //(float(total)*ymult); // height of chunk
                    quads->add(x1,py,col);
                    quads->add(x1,py-h,col);
                    quads->add(x2,py-h,col2);
                    quads->add(x2,py,col2);
                    lines->add(x1,py,x1,py-h,blk);
                    lines->add(x1,py-h,x2,py-h,blk);
                    lines->add(x1,py,x2,py,blk);
                    lines->add(x2,py,x2,py-h,blk);
                    py-=h;
                }
            }
        }
        px+=barw;
        daynum++;
    }
    if (total_days>0) {
        float val=total_val/float(total_days);
        QString z="AHI="+QString::number(val,'f',2)+" days="+QString::number(total_days,'f',0)+" This is going in overview later";
        w.renderText(z,left,top-1);

        // val = AHI for selected area.
    }
}

