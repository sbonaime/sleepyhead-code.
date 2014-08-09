/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gDailySummary Graph Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GDAILYSUMMARY_H
#define GDAILYSUMMARY_H

#include "Graphs/layer.h"
#include "SleepLib/day.h"

class gDailySummary:public Layer
{
public:
    gDailySummary();
    virtual ~gDailySummary() {}

    virtual void SetDay(Day *d);

    virtual bool isEmpty();

    //! Draw filled rectangles behind Event Flag's, and an outlines around them all, Calls the individual paint for each gFlagLine
    virtual void paint(QPainter &painter, gGraph &w, const QRegion &region);

    virtual int minimumHeight() { return m_minimum_height; }
    bool mouseMoveEvent(QMouseEvent *event, gGraph *graph);

protected:
    QList<QString> flag_values;
    QList<QString> flag_labels;
    QList<ChannelID> flag_codes;
    QList<QColor> flag_foreground;
    QList<QColor> flag_background;

    QList<QString> info_labels;
    QList<QString> info_values;

    float flag_height;
    float flag_label_width;
    float flag_value_width;

    int info_height;
    int info_label_width;
    int info_value_width;

    int m_minimum_height;
    bool m_empty;
};

#endif // GDAILYSUMMARY_H
