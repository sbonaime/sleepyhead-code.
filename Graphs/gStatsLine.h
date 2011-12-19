#ifndef GSTATSLINE_H
#define GSTATSLINE_H

#include "SleepLib/machine.h"
#include "gGraphView.h"

/*! \class gStatsLine
    \brief Show a rendered stats area in place of a graph. This is currently unused
    */
class gStatsLine : public Layer
{
public:
    gStatsLine(ChannelID code,QString label="",QColor textcolor=Qt::black);
    virtual void paint(gGraph & w, int left, int top, int width, int height);
    void SetDay(Day *d);

protected:
    QString m_label;
    QColor m_textcolor;
    EventDataType m_min,m_max,m_avg,m_p90;
    QString st_min,st_max,st_avg,st_p90;
    float m_tx,m_ty;
};

#endif // GSTATSLINE_H
