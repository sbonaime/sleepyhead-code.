/********************************************************************
 gLineChart Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GLINECHART_H
#define GLINECHART_H

#include "graphwindow.h"
#include "graphlayer.h"

class gLineChart:public gLayer
{
    public:
        gLineChart(gPointData *d=NULL,const QColor col=QColor("black"),int dlsize=4096,bool accelerate=false,bool _hide_axes=false,bool _square_plot=false);
        virtual ~gLineChart();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);

        void SetSquarePlot(bool b) { m_square_plot=b; };
        bool GetSquarePlot() { return m_square_plot; };
        void ReportEmpty(bool b) { m_report_empty=b; };
        bool GetReportEmpty() { return m_report_empty; };

    protected:
        bool m_accelerate;
        int m_drawlist_size;
        bool m_report_empty;
        QPointD *m_drawlist;
        bool m_hide_axes;
        bool m_square_plot;
        //gYAxis * Yaxis;
        //gFooBar *foobar;

};

#endif // GLINECHART_H
