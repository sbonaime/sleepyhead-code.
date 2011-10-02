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

enum GraphType { GT_BAR, GT_LINE };

class SummaryChart:public Layer
{
    public:
        SummaryChart(Profile *profile, QString label, GraphType type=GT_BAR);
        virtual ~SummaryChart();

        void setProfile(Profile *profile) { m_profile=profile; }
        virtual void paint(gGraph & w,int left, int top, int width, int height);
        virtual void SetDay(Day * day=NULL);
        virtual bool isEmpty() { return m_empty; }
        void addSlice(ChannelID code, QColor color, SummaryType type) { m_codes.push_back(code); m_colors.push_back(color); m_type.push_back(type); }
        void deselect() { hl_day=-1; }
        void setMachineType(MachineType type) { m_machinetype=type; }
        MachineType machineType() { return m_machinetype; }
    protected:
        Qt::Orientation m_orientation;

        QVector<QColor> m_colors;
        QVector<ChannelID> m_codes;
        QVector<SummaryType> m_type;
        QHash<int,QHash<short,EventDataType> > m_values;
        QHash<int,Day *> m_days;

        Profile * m_profile;
        GLShortBuffer *quads,*lines;
        bool m_empty;
        int m_fday;
        QString m_label;

        float barw; // bar width from last draw
        qint64 l_offset; // last offset
        float offset;    // in pixels;
        int l_left,l_top,l_width,l_height;
        int rtop;
        qint64 l_minx,l_maxx;
        int hl_day;
        gGraph * graph;
        GraphType m_graphtype;
        MachineType m_machinetype;
        virtual bool mouseMoveEvent(QMouseEvent * event);
        virtual bool mousePressEvent(QMouseEvent * event);
        virtual bool mouseReleaseEvent(QMouseEvent * event);

};

/*
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

class AvgChart:public gBarChart
{
public:
    AvgChart(Profile *profile);
    virtual void SetDay(Day * day);
};*/

#endif // GBARCHART_H
