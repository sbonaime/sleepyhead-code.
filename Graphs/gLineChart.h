/*
 gLineChart Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GLINECHART_H
#define GLINECHART_H

#include <QVector>
#include "SleepLib/event.h"
#include "SleepLib/day.h"
#include "gGraphView.h"
//#include "graphlayer.h"

class gLineChart:public Layer
{
    public:
        gLineChart(ChannelID code,const QColor col=QColor("black"), bool square_plot=false,bool disable_accel=false);
        virtual ~gLineChart();

        virtual void paint(gGraph & w,int left, int top, int width, int height);

        void SetSquarePlot(bool b) { m_square_plot=b; }
        bool GetSquarePlot() { return m_square_plot; }
        void ReportEmpty(bool b) { m_report_empty=b; }
        bool GetReportEmpty() { return m_report_empty; }
        void setDisableAccel(bool b) { m_disable_accel=b; }
        bool disableAccel() { return m_disable_accel; }
protected:
        bool m_report_empty;
        bool m_square_plot;
        bool m_disable_accel;
        QColor m_line_color;
};

#endif // GLINECHART_H
