/* Minutes At Pressure Graph Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef MINUTESATPRESSURE_H
#define MINUTESATPRESSURE_H

#include "Graphs/layer.h"
#include "SleepLib/day.h"

class MinutesAtPressure;
class RecalcMAP:public QRunnable
{
    friend class MinutesAtPressure;
public:
    explicit RecalcMAP(MinutesAtPressure * map) :map(map), m_quit(false), m_done(false) {}
    virtual ~RecalcMAP();
    virtual void run();
    void quit();
protected:
    MinutesAtPressure * map;
    volatile bool m_quit;
    volatile bool m_done;
};

class MinutesAtPressure:public Layer
{
    friend class RecalcMAP;
public:
    MinutesAtPressure();
    virtual ~MinutesAtPressure();

    virtual void recalculate(gGraph * graph);

    virtual void SetDay(Day *d);

    virtual bool isEmpty();
    virtual int minimumHeight();

    //! Draw filled rectangles behind Event Flag's, and an outlines around them all, Calls the individual paint for each gFlagLine
    virtual void paint(QPainter &painter, gGraph &w, const QRegion &region);


    bool mouseMoveEvent(QMouseEvent *event, gGraph *graph);
    bool mousePressEvent(QMouseEvent *event, gGraph *graph);
    bool mouseReleaseEvent(QMouseEvent *event, gGraph *graph);

    virtual void recalcFinished();

protected:
    QMutex timelock;
    QMutex mutex;

    bool m_empty;
    int m_minimum_height;

    qint64 m_lastminx;
    qint64 m_lastmaxx;
    gGraph * m_graph;
    RecalcMAP * m_remap;
    QMap<EventStoreType, int> times;
    QList<ChannelID> chans;
    QHash<ChannelID, QMap<EventStoreType, EventDataType> > events;
    int maxtime;
    ChannelID m_presChannel;
    EventStoreType m_minpressure;
    EventStoreType m_maxpressure;

    QMap<EventStoreType, EventDataType> ahis;
};

#endif // MINUTESATPRESSURE_H
