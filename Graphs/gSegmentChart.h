#ifndef GSEGMENTCHART_H
#define GSEGMENTCHART_H

#include "graphlayer.h"

enum GraphSegmentType { GST_Pie, GST_CandleStick };

class gSegmentChart : public gLayer
{
public:
    gSegmentChart(GraphSegmentType gt=GST_Pie, QColor gradient_color=Qt::white,QColor outline_color=Qt::black);
    virtual ~gSegmentChart();

    virtual void Plot(gGraphWindow & w,float scrx,float scry);
    virtual void SetDay(Day *d);

    void AddSlice(MachineCode code,QColor col,QString name="");
    void setGradientColor(QColor & color) { m_gradient_color=color; }
    void setOutlineColor(QColor & color) { m_outline_color=color; }
    const GraphSegmentType & graphType() { return m_graph_type; }
    void setGraphType(GraphSegmentType type) { m_graph_type=type; }

protected:
    vector<MachineCode> m_codes;
    vector<QString> m_names;
    vector<int> m_values;
    vector<QColor> m_colors;

    int m_total;
    GraphSegmentType m_graph_type;
    QColor m_gradient_color;
    QColor m_outline_color;
};

class gTAPGraph:public gSegmentChart
{
public:
    gTAPGraph(MachineCode code,GraphSegmentType gt=GST_CandleStick, QColor gradient_color=Qt::lightGray,QColor outline_color=Qt::black);
    virtual ~gTAPGraph();
    virtual void SetDay(Day *d);
protected:
    MachineCode m_code;
};


#endif // GSEGMENTCHART_H
