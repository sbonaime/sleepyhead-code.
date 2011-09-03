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

        QVector<QColor> m_colors;
        QVector<ChannelID> m_codes;
        QHash<int,QHash<short,EventDataType> > m_values;
        Profile * m_profile;
        GLBuffer *quads;
        bool m_empty;
        int m_fday;
        QString m_label;
};

class AHIChart:public gBarChart
{
public:
    AHIChart(Profile *profile);
    virtual void SetDay(Day * day);
};
class UsageChart:public gBarChart
{
public:
    UsageChart(Profile *profile);
    virtual void SetDay(Day * day);
};

#endif // GBARCHART_H
