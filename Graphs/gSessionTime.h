/*
 gSessionTime Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GSESSIONTIME_H
#define GSESSIONTIME_H

#include "graphlayer.h"
#include "gXAxis.h"
#include "gYAxis.h"

class gTimeYAxis:public gYAxis
{
public:
    gTimeYAxis(QColor col=QColor("black"));
    virtual ~gTimeYAxis();
    virtual const QString & Format(double v);
};


class gSessionTime:public gLayer
{
    public:
        gSessionTime(MachineCode=MC_UNKNOWN,QColor col=QColor("blue"),Qt::Orientation o=Qt::Horizontal);
        virtual ~gSessionTime();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);

    protected:
        Qt::Orientation m_orientation;

        virtual const QString & FormatX(double v) { static QString t; QDateTime d; d=d.fromMSecsSinceEpoch(v*86400000.0); t=d.toString("MMM dd"); return t; }
        //virtual const wxString & FormatX(double v) { static wxString t; wxDateTime d; d.Set(vi*86400000.0); t=d.Format(wxT("HH:mm")); return t; };
        //virtual const wxString & FormatX(double v) { static wxString t; t=wxString::Format(wxT("%.1f"),v); return t; };
        virtual const QString & FormatY(double v) { static QString t; t.sprintf("%.1f",v); return t; }

        gXAxis *Xaxis;
};

#endif // GSESSIONTIME_H
