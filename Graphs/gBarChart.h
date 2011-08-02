/*
 gBarChart Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GBARCHART_H
#define GBARCHART_H

#include "graphlayer.h"
#include "gXAxis.h"
class gBarChart:public gLayer
{
    public:
        gBarChart(ChannelID code=EmptyChannel,QColor col=QColor("blue"),Qt::Orientation o=Qt::Horizontal);
        virtual ~gBarChart();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);

    protected:
        Qt::Orientation m_orientation;

        // d.Set(i+2400000.5+.000001); // JDN vs MJD vs Rounding errors

        virtual const QString & FormatX(double v) { static QString t; QDateTime d; d=d.fromMSecsSinceEpoch(v*86400000.0); t=d.toString("MMM dd"); return t; }
        //virtual const wxString & FormatX(double v) { static wxString t; wxDateTime d; d.Set(vi*86400000.0); t=d.Format(wxT("HH:mm")); return t; };
        //virtual const wxString & FormatX(double v) { static wxString t; t=wxString::Format(wxT("%.1f"),v); return t; };
        virtual const QString & FormatY(double v) { static QString t; t.sprintf("%.1f",v); return t; }

        gXAxis *Xaxis;
        QVector<QColor> color;
};

#endif // GBARCHART_H
