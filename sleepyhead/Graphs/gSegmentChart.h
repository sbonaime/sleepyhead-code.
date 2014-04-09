/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gSegmentChart Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GSEGMENTCHART_H
#define GSEGMENTCHART_H

#include "gGraphView.h"

enum GraphSegmentType { GST_Pie, GST_CandleStick, GST_Line };

/*! \class gSegmentChart
    \brief Draws a PieChart, CandleStick or 2D Line plots containing multiple Channel 'slices'
  */
class gSegmentChart : public Layer
{
public:
    gSegmentChart(GraphSegmentType gt=GST_Pie, QColor gradient_color=Qt::white,QColor outline_color=Qt::black);
    virtual ~gSegmentChart();

    //! \brief The drawing code that fills the Vertex buffers
    virtual void paint(gGraph & w,int left, int top, int width, int height);

    //! \brief Pre-fills a buffer with the data needed to draw
    virtual void SetDay(Day *d);

    //! \brief Returns true if no data available for drawing
    virtual bool isEmpty();

    //! \brief Adds a channel slice, and sets the color and label
    void AddSlice(ChannelID code,QColor col,QString name="");

    //! \brief Sets the fade-out color to make the graphs look more attractive
    void setGradientColor(QColor & color) { m_gradient_color=color; }

    //! \brief Sets the outline color for the edges drawn around the Pie slices
    void setOutlineColor(QColor & color) { m_outline_color=color; }
    const GraphSegmentType & graphType() { return m_graph_type; }
    void setGraphType(GraphSegmentType type) { m_graph_type=type; }

protected:
    QVector<ChannelID> m_codes;
    QVector<QString> m_names;
    QVector<int> m_values;
    QVector<QColor> m_colors;

    int m_total;
    GraphSegmentType m_graph_type;
    QColor m_gradient_color;
    QColor m_outline_color;
    bool m_empty;

    // gah.. can't convert these
    GLFloatBuffer *poly,*lines;
};

/*! \class gTAPGraph
    \brief Time at Pressure chart, derived from gSegmentChart
    \notes Currently unused
    */
class gTAPGraph:public gSegmentChart
{
public:
    gTAPGraph(ChannelID code,GraphSegmentType gt=GST_CandleStick, QColor gradient_color=Qt::lightGray,QColor outline_color=Qt::black);
    virtual ~gTAPGraph();
    virtual void SetDay(Day *d);
protected:
    ChannelID m_code;
};


#endif // GSEGMENTCHART_H
