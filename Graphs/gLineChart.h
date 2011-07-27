/********************************************************************
 gLineChart Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GLINECHART_H
#define GLINECHART_H

#include <vector>
#include "SleepLib/event.h"
#include "SleepLib/day.h"
#include "graphwindow.h"
#include "graphlayer.h"

class gLineChart:public gLayer
{
    public:
        gLineChart(MachineCode code,const QColor col=QColor("black"), bool _square_plot=false);
        virtual ~gLineChart();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);

        void SetSquarePlot(bool b) { m_square_plot=b; }
        bool GetSquarePlot() { return m_square_plot; }
        void ReportEmpty(bool b) { m_report_empty=b; }
        bool GetReportEmpty() { return m_report_empty; }

protected:
        bool m_report_empty;
        bool m_square_plot;
};

#endif // GLINECHART_H
