/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gLineOverlayBar Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GLINEOVERLAY_H
#define GLINEOVERLAY_H

#include "gGraphView.h"

/*! \class gLineOverlayBar
    \brief Shows a flag line, a dot, or a span over the top of a 2D line chart.
    */
class gLineOverlayBar: public Layer
{
  public:
    //! \brief Constructs a gLineOverlayBar object of type code, setting the flag/span color, the label to show when zoomed
    //! in, and the display method requested, of either FT_Bar, FT_Span, or FT_Dot
    gLineOverlayBar(ChannelID code, QColor col, QString _label = "", FlagType _flt = FT_Bar);
    virtual ~gLineOverlayBar();

    //! \brief The drawing code that fills the OpenGL vertex GLBuffers
    virtual void paint(QPainter &painter, gGraph &w, const QRegion &region);

    virtual EventDataType Miny() { return 0; }
    virtual EventDataType Maxy() { return 0; }

    //! \brief Returns true if no Channel data available for this code
    virtual bool isEmpty() { return true; }

    //! \brief returns a count of all flags drawn during render. (for gLineOverlaySummary)
    int count() { return m_count; }
    double sum() { return m_sum; }
    FlagType flagtype() { return m_flt; }
  protected:
    //! \brief Mouse moved over this layers area (shows the hover-over tooltips here)
    virtual bool mouseMoveEvent(QMouseEvent *event, gGraph *graph);

    QColor m_flag_color;
    QString m_label;
    FlagType m_flt;
    int m_count;
    double m_sum;
};

/*! \class gLineOverlaySummary
    \brief A container class to hold a group of gLineOverlayBar's, and shows the "Section AHI" over the Flow Rate waveform.
    */
class gLineOverlaySummary: public Layer
{
  public:
    gLineOverlaySummary(QString text, int x, int y);
    virtual ~gLineOverlaySummary();

    virtual void paint(QPainter &painter, gGraph &w, const QRegion &region);
    virtual EventDataType Miny() { return 0; }
    virtual EventDataType Maxy() { return 0; }

    //! \brief Returns true if no Channel data available for this code
    virtual bool isEmpty() { return true; }

    //! \brief Adds a gLineOverlayBar to this list
    gLineOverlayBar *add(gLineOverlayBar *bar) { m_overlays.push_back(bar); return bar; }
  protected:
    QVector<gLineOverlayBar *> m_overlays;
    QString m_text;
    int m_x, m_y;
};


#endif // GLINEOVERLAY_H
