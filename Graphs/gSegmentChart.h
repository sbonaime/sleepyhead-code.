/*
 gSegmentChart Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GSEGMENTCHART_H
#define GSEGMENTCHART_H

#include "gGraphView.h"

enum GraphSegmentType { GST_Pie, GST_CandleStick, GST_Line };

class gSegmentChart : public Layer
{
public:
    gSegmentChart(GraphSegmentType gt=GST_Pie, QColor gradient_color=Qt::white,QColor outline_color=Qt::black);
    virtual ~gSegmentChart();

    virtual void paint(gGraph & w,int left, int top, int width, int height);
    virtual void SetDay(Day *d);
    virtual bool isEmpty();

    void AddSlice(ChannelID code,QColor col,QString name="");
    void setGradientColor(QColor & color) { m_gradient_color=color; }
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
    GLFloatBuffer *poly,*lines;
};

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
