/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gFlagsLine Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GFLAGSLINE_H
#define GFLAGSLINE_H

#include "gGraphView.h"
#include "gspacer.h"

class gFlagsGroup;

/*! \class gYSpacer
    \brief A dummy vertical spacer object
   */
class gFlagsLabelArea: public gSpacer
{
  public:
    gFlagsLabelArea(gFlagsGroup *group);
    virtual void paint(QPainter &painter, gGraph &w, const QRegion &region) {
        Q_UNUSED(w);
        Q_UNUSED(painter);
        Q_UNUSED(region);
    }
  protected:
    gFlagsGroup *m_group;
    virtual bool mouseMoveEvent(QMouseEvent *event, gGraph *graph);
};


/*! \class gFlagsLine
    \brief One single line of event flags in the Event Flags chart
    */
class gFlagsLine: public Layer
{
    friend class gFlagsGroup;
  public:
    /*! \brief Constructs an individual gFlagsLine object
        \param code  The Channel the data is sourced from
        \param col   The colour to draw this flag
        \param label The label to show to the left of the Flags line.
        \param always_visible  Whether to always show this line, even if empty
        \param Type of Flag, either FT_Bar, or FT_Span
        */
    gFlagsLine(ChannelID code);
    virtual ~gFlagsLine();

    //! \brief Drawing code to add the flags and span markers to the Vertex buffers.
    virtual void paint(QPainter &painter, gGraph &w, const QRegion &region);

    void setTotalLines(int i) { total_lines = i; }
    void setLineNum(int i) { line_num = i; }
  protected:

    virtual bool mouseMoveEvent(QMouseEvent *event, gGraph *graph);

    bool m_always_visible;
    int total_lines, line_num;
    int m_lx, m_ly;

};

/*! \class gFlagsGroup
    \brief Contains multiple gFlagsLine entries for the Events Flag graph
    */
class gFlagsGroup: public LayerGroup
{
    friend class gFlagsLabelArea;

  public:
    gFlagsGroup();
    virtual ~gFlagsGroup();

    //! Draw filled rectangles behind Event Flag's, and an outlines around them all, Calls the individual paint for each gFlagLine
    virtual void paint(QPainter &painter, gGraph &w, const QRegion &region);

    //! Returns the first time represented by all gFlagLine layers, in milliseconds since epoch
    virtual qint64 Minx();
    //! Returns the last time represented by all gFlagLine layers, in milliseconds since epoch.
    virtual qint64 Maxx();

    //! Checks if each flag has data, and adds valid gFlagLines to the visible layers list
    virtual void SetDay(Day *);

    //! Returns true if none of the gFlagLine objects contain any data for this day
    virtual bool isEmpty() { return m_empty; }

    //! Returns the count of visible flag line entries
    int count() { return lvisible.size(); }

    //! Returns the height in pixels of each bar
    int barHeight() { return m_barh; }

    //! Returns a list of Visible gFlagsLine layers to draw
    QVector<gFlagsLine *> &visibleLayers() { return lvisible; }

    void alwaysVisible(ChannelID code) { m_alwaysvisible.push_back(code); }

  protected:
    virtual bool mouseMoveEvent(QMouseEvent *event, gGraph *graph);

    QList<ChannelID> m_alwaysvisible;

    QList<ChannelID> availableChans;

    QVector<gFlagsLine *> lvisible;
    float m_barh;
    bool m_empty;
    bool m_rebuild_cpap;
};

#endif // GFLAGSLINE_H
