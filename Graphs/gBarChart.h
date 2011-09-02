/*
 gBarChart Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GBARCHART_H
#define GBARCHART_H

#include <SleepLib/profiles.h>
#include "gGraphView.h"
#include "gXAxis.h"

class gBarChart:public Layer
{
    public:
        gBarChart(ChannelID code=EmptyChannel,QColor color=QColor("blue"),Qt::Orientation o=Qt::Horizontal);
        virtual ~gBarChart();

        void setProfile(Profile *profile) { m_profile=profile; }
        virtual void paint(gGraph & w,int left, int top, int width, int height);
        virtual void SetDay(Day * day);
        virtual bool isEmpty() { return m_empty; }
        void addSlice(ChannelID code, QColor color) { m_codes.push_back(code); m_colors.push_back(color); }
    protected:
        Qt::Orientation m_orientation;

        // d.Set(i+2400000.5+.000001); // JDN vs MJD vs Rounding errors

        virtual const QString & FormatX(double v) { static QString t; QDateTime d; d=d.fromTime_t(v*86400.0); t=d.toString("MMM dd"); return t; }
        //virtual const wxString & FormatX(double v) { static wxString t; wxDateTime d; d.Set(vi*86400000.0); t=d.Format(wxT("HH:mm")); return t; };
        //virtual const wxString & FormatX(double v) { static wxString t; t=wxString::Format(wxT("%.1f"),v); return t; };
        virtual const QString & FormatY(double v) { static QString t; t.sprintf("%.1f",v); return t; }

        //gXAxis *Xaxis;
        QVector<QColor> m_colors;
        QVector<ChannelID> m_codes;
        QHash<int,QMap<ChannelID,EventDataType> > m_values;
        Profile * m_profile;
        GLBuffer *quads;
        bool m_empty;
        int m_fday;
};

#endif // GBARCHART_H
