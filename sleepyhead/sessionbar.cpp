/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SessionBar
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QPainter>
#include <QLinearGradient>
#include <QBrush>
#include <QRect>
#include <QMouseEvent>
#include <QDebug>

#include "sessionbar.h"

SBSeg::SBSeg()
{
    session=NULL;
    color=QColor();
    highlight=false;
}

SBSeg::SBSeg(Session * sess, QColor col)
{
    session=sess;
    color=col;
    highlight=false;
}

//SBSeg::SBSeg(const SBSeg & a)
//{
//    session=(Session *)a.session;
//    color=a.color;
//    highlight=a.highlight;
//}

SessionBar::SessionBar(QWidget *parent) :
    QWidget(parent)
{
    timer.setParent(this);
}
//SessionBar::SessionBar(const SessionBar & copy)
//    :QWidget(this)
//{
//    timer.setParent(this);
//    QList<SBSeg>::const_iterator i;
//    for (i=copy.segments.begin();i!=copy.segments.end();++i) {
//        segments.push_back(*i);
//    }
//}

SessionBar::~SessionBar()
{
}
void SessionBar::updateTimer()
{
    if (!underMouse()) {
        QList<SBSeg>::iterator i;

        for (i=segments.begin();i!=segments.end();++i) {
            (*i).highlight=false;
        }
    } else {
        timer.singleShot(50,this, SLOT(updateTimer()));
    }
    update();
}

SegType SessionBar::min()
{
    if (segments.isEmpty())
        return 0;

    QList<SBSeg>::iterator i=segments.begin();
    SegType min=(*i).session->first();
    i++;

    qint64 val;
    for (;i!=segments.end();++i) {
        val=(*i).session->first();
        if (min > val)
            min=val;
    }
    return min;
}

SegType SessionBar::max()
{
    if (segments.isEmpty())
        return 0;

    QList<SBSeg>::iterator i=segments.begin();
    SegType max=(*i).session->last();
    i++;
    qint64 val;
    for (;i!=segments.end();++i) {
        val=(*i).session->last();
        if (max < val)
            max=val;
    }
    return max;
}

QColor brighten(QColor color);

void SessionBar::mousePressEvent(QMouseEvent * ev)
{
    SegType mn=min();
    SegType mx=max();
    if (mx < mn)
        return;

    SegType total=mx-mn;
    double px=double(width()-5) / double(total);

    double sx,ex;
    QList<SBSeg>::iterator i;

    int cnt=0;
    for (i=segments.begin();i!=segments.end();++i) {
        Session * sess=(*i).session;
        sx=double(sess->first() - mn) * px;
        ex=double(sess->last() - mn) * px;
        if (ex>width()-5) ex=width()-5;
        //ex-=sx;

        if ((ev->x() > sx) && (ev->x() < ex)
            && (ev->y() > 0) && (ev->y() < height())) {
            (*i).session->setEnabled(!(*i).session->enabled());
            emit toggledSession((*i).session);
            break;
        }
        cnt++;
    }
    if (timer.isActive()) timer.stop();
    timer.singleShot(50,this, SLOT(updateTimer()));

}

void SessionBar::mouseMoveEvent(QMouseEvent * ev)
{
    SegType mn=min();
    SegType mx=max();
    if (mx < mn)
        return;

    SegType total=mx-mn;
    double px=double(width()-5) / double(total);

    double sx,ex;
    QList<SBSeg>::iterator i;

    for (i=segments.begin();i!=segments.end();++i) {
        SBSeg & seg=*i;
        sx=double(seg.session->first() - mn) * px;
        ex=double(seg.session->last() - mn) * px;
        if (ex>width()-5) ex=width()-5;
        //ex-=sx;

        if ((ev->x() > sx) && (ev->x() < ex)
            && (ev->y() > 0) && (ev->y() < height())) {
            seg.highlight=true;
        } else seg.highlight=false;
    }
    if (timer.isActive()) timer.stop();
    timer.singleShot(50,this, SLOT(updateTimer()));
}


void SessionBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QRect rect(1,1,width()-2,height()-2);

    painter.drawRect(rect);

    SegType mn=min();
    SegType mx=max();
    if (mx < mn)
        return;

    SegType total=mx-mn;
    double px=double(width()-5) / double(total);

    double sx,ex;
    QList<SBSeg>::iterator i;
    for (i=segments.begin();i!=segments.end();++i) {
        SBSeg & seg=*i;
        qint64 mm=seg.session->first(), MM=seg.session->last(), L=MM-mm;
        sx=double(mm - mn) * px;
        ex=double(MM - mn) * px;
        if (ex>width()-5) ex=width()-5;
        ex-=sx;

        int len=L/1000L;
        int h=len/3600;
        int m=(len/60) % 60;
        //int s=len % 60;

        QString msg=QString("%1h %2m").arg((short)h,1,10,QChar('0')).arg((short)m,1,10,QChar('0'));//.arg((short)s,2,10,QChar('0'));
        //painter.setBrush(QBrush((*i).color);
        QRect segrect(3+sx,3,ex,height()-6);
        if (seg.session->enabled()) {
            QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, height()/2));
            linearGrad.setSpread(QGradient::ReflectSpread);
            QColor col=seg.color;
            if (seg.highlight) col=brighten(col);
            linearGrad.setColorAt(0, col);
            linearGrad.setColorAt(1, brighten(col));
            QBrush brush(linearGrad);
            painter.fillRect(segrect,brush);
        } else {
            if (seg.highlight) {
                QColor col=QColor("#f0f0f0");
                painter.fillRect(segrect,col);
            }
            //msg="Off";
        }
        QRect rect=painter.boundingRect(segrect, Qt::AlignCenter, msg);
        if (rect.width() < segrect.width()) {
            painter.drawText(segrect,Qt::AlignCenter,msg);
        }
        painter.drawRect(segrect);

    }
}
