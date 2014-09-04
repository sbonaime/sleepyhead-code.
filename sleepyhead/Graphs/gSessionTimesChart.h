/* gSessionTimesChart Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GSESSIONTIMESCHART_H
#define GSESSIONTIMESCHART_H

#include <SleepLib/day.h>
#include "gGraphView.h"

struct TimeSpan
{
public:
    TimeSpan(): begin(0), end(0) {}
    TimeSpan(float b, float e) : begin(b), end(e) {}
    TimeSpan(const TimeSpan & copy) {
        begin = copy.begin;
        end = copy.end;
    }
    ~TimeSpan() {}
    float begin;
    float end;
};

/*! \class gSessionTimesChart
    \brief Displays a summary of session times
    */
class gSessionTimesChart : public Layer
{
public:
    gSessionTimesChart(QString label, MachineType machtype);
    ~gSessionTimesChart();

    //! \brief Renders the graph to the QPainter object
    virtual void paint(QPainter &painter, gGraph &w, const QRegion &region);

    //! \brief Precalculation code prior to drawing. Day object is not needed here, it's just here for Layer compatability.
    virtual void SetDay(Day *day = nullptr);

    //! \brief Returns true if no data was found for this day during SetDay
    virtual bool isEmpty() { return m_empty; }

    //! \brief Deselect highlighting (the gold bar)
    virtual void deselect() {
        hl_day = -1;
    }

    //! \brief Returns true if currently selected..
    virtual bool isSelected() { return hl_day >= 0; }

protected:
    //! \brief Key was pressed that effects this layer
    virtual bool keyPressEvent(QKeyEvent *event, gGraph *graph);

    //! \brief Mouse moved over this layers area (shows the hover-over tooltips here)
    virtual bool mouseMoveEvent(QMouseEvent *event, gGraph *graph);

    //! \brief Mouse Button was pressed over this area
    virtual bool mousePressEvent(QMouseEvent *event, gGraph *graph);

    //! \brief Mouse Button was released over this area. (jumps to daily view here)
    virtual bool mouseReleaseEvent(QMouseEvent *event, gGraph *graph);

    QString m_label;
    MachineType m_machtype;
    bool m_empty;
    int hl_day;
    int tz_offset;
    float tz_hours;
    QDate firstday;
    QDate lastday;
    QMap<quint32, QList<TimeSpan> > sessiontimes;
};

#endif // GSESSIONTIMESCHART_H
