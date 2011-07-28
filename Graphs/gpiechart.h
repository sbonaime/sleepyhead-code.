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

protected:
    map <MachineCode,QString> m_names;
    map <MachineCode,int> m_counts;
    map <MachineCode,QColor> m_colors;
    int m_total;
    QColor m_outline_color;
};

#endif // GPIECHART_H
