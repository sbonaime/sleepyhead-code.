#ifndef GPIECHART_H
#define GPIECHART_H

#include "graphlayer.h"
class gPieChart : public gLayer
{
public:
    gPieChart(QColor color=Qt::black);
    virtual ~gPieChart();

    virtual void Plot(gGraphWindow & w,float scrx,float scry);
    virtual void SetDay(Day *d);

    void AddSlice(MachineCode code,QColor col,QString name="");
    void setGradientColor(QColor & color) { m_gradient_color=color; }
    void setOutlineColor(QColor & color) { m_outline_color=color; }

protected:
    map <MachineCode,QString> m_names;
    map <MachineCode,int> m_counts;
    map <MachineCode,QColor> m_colors;
    int m_total;
    QColor m_outline_color;
    QColor m_gradient_color;

};

#endif // GPIECHART_H
