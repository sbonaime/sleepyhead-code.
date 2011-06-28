#ifndef GPIECHART_H
#define GPIECHART_H

#include "graphlayer.h"
class gPieChart : public gLayer
{
public:
    gPieChart(gPointData *d,QColor col=Qt::black);
    virtual ~gPieChart();

    virtual void Plot(gGraphWindow & w,float scrx,float scry);
    void AddName(QString name) { m_names.push_back(name); };

protected:
    vector<QString> m_names;

};

#endif // GPIECHART_H
