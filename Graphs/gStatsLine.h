#ifndef GSTATSLINE_H
#define GSTATSLINE_H

#include "SleepLib/machine.h"
#include <QStaticText>
#include "gGraphView.h"

class gStatsLine : public Layer
{
public:
    gStatsLine(ChannelID code,QString label="",QColor textcolor=Qt::black);
    virtual void paint(gGraph & w, int left, int top, int width, int height);
    void SetDay(Day *d);
    //bool isEmpty();

protected:
    QColor m_textcolor;
    //bool m_empty;
    EventDataType m_min,m_max,m_avg,m_p90;
    QString m_label;
    QString m_text;
    QStaticText st_label,st_min,st_max,st_avg,st_p90;
    float m_tx,m_ty;
};

#endif // GSTATSLINE_H
