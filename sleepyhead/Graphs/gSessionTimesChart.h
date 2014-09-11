/* gSessionTimesChart Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GSESSIONTIMESCHART_H
#define GSESSIONTIMESCHART_H

#include "SleepLib/day.h"
#include "SleepLib/profiles.h"
#include "gGraphView.h"


struct TimeSpan
{
public:
    TimeSpan():begin(0), end(0) {}
    TimeSpan(float b, float e) : begin(b), end(e) {}
    TimeSpan(const TimeSpan & copy) {
        begin = copy.begin;
        end = copy.end;
    }
    ~TimeSpan() {}
    float begin;
    float end;
};

struct SummaryCalcItem {
    SummaryCalcItem() {
        code = 0;
        type = ST_CNT;
    }
    SummaryCalcItem(const SummaryCalcItem & copy) {
        code = copy.code;
        type = copy.type;
    }
    SummaryCalcItem(ChannelID code, SummaryType type)
        :code(code), type(type) {}
    ChannelID code;
    SummaryType type;
};

struct SummaryChartSlice {
    SummaryChartSlice() {
        code = 0;
        height = 0;
        value = 0;
        name = ST_CNT;
    }
    SummaryChartSlice(const SummaryChartSlice & copy) {
        code = copy.code;
        value = copy.value;
        height = copy.height;
        name = copy.name;
        color = copy.color;
    }
    SummaryChartSlice(ChannelID code, EventDataType value, EventDataType height, QString name, QColor color)
        :code(code), value(value), height(height), name(name), color(color) {}
    ChannelID code;
    EventDataType value;
    EventDataType height;
    QString name;
    QColor color;
};

class gSummaryChart : public Layer
{
public:
    gSummaryChart(QString label, MachineType machtype);
    gSummaryChart(ChannelID code, MachineType machtype);
    virtual ~gSummaryChart();

    //! \brief Renders the graph to the QPainter object
    virtual void paint(QPainter &, gGraph &, const QRegion &);

    //! \brief Called whenever data model changes underneath. Day object is not needed here, it's just here for Layer compatability.
    virtual void SetDay(Day *day = nullptr);

    //! \brief Returns true if no data was found for this day during SetDay
    virtual bool isEmpty() { return m_empty; }

    virtual void populate(Day *, int idx);

    //! \brief Override to setup custom stuff before main loop
    virtual void preCalc() {}

    //! \brief Override to call stuff in main loop
    virtual void customCalc(Day *, QList<SummaryChartSlice> &) {}

    //! \brief Override to call stuff after draw is complete
    virtual void afterDraw(QPainter &, gGraph &, QRect) {}

    //! \brief Return any extra data to show beneath the date in the hover over tooltip
    virtual QString tooltipData(Day *, int);

    void addCalc(ChannelID code, SummaryType type) { calcitems.append(SummaryCalcItem(code, type)); }

    virtual Layer * Clone() {
        gSummaryChart * sc = new gSummaryChart(m_label, m_machtype);
        Layer::CloneInto(sc);
        CloneInto(sc);
        return sc;
    }

    void CloneInto(gSummaryChart * layer) {
        layer->m_empty = m_empty;
        layer->firstday = firstday;
        layer->lastday = lastday;
        layer->cache = cache;
        layer->calcitems = calcitems;
        layer->expected_slices = expected_slices;
        layer->nousedays = nousedays;
        layer->totaldays = totaldays;
        layer->peak_value = peak_value;
    }

protected:
    //! \brief Key was pressed that effects this layer
    virtual bool keyPressEvent(QKeyEvent *event, gGraph *graph);

    //! \brief Mouse moved over this layers area (shows the hover-over tooltips here)
    virtual bool mouseMoveEvent(QMouseEvent *event, gGraph *graph);

    //! \brief Mouse Button was pressed over this area
    virtual bool mousePressEvent(QMouseEvent *event, gGraph *graph);

    //! \brief Mouse Button was released over this area. (jumps to daily view here)
    virtual bool mouseReleaseEvent(QMouseEvent *event, gGraph *graph);

    QString m_label;
    MachineType m_machtype;
    bool m_empty;
    int hl_day;
    int tz_offset;
    float tz_hours;
    QDate firstday;
    QDate lastday;

    static QMap<QDate, int> dayindex;
    static QList<Day *> daylist;

    QHash<int, QList<SummaryChartSlice> > cache;
    QList<SummaryCalcItem> calcitems;

    int expected_slices;

    int nousedays;
    int totaldays;

    EventDataType peak_value;
    EventDataType min_value;
};


/*! \class gSessionTimesChart
    \brief Displays a summary of session times
    */
class gSessionTimesChart : public gSummaryChart
{
public:
    gSessionTimesChart()
        :gSummaryChart("SessionTimes", MT_CPAP) {}
    virtual ~gSessionTimesChart() {}

    virtual void SetDay(Day * day = nullptr) {
        gSummaryChart::SetDay(day);
        split = p_profile->session->daySplitTime();

        m_miny = 0;
        m_maxy = 28;
    }

    virtual void preCalc() {
        num_slices = 0;
        num_days = 0;
        total_length = 0;
    }
    virtual void customCalc(Day *, QList<SummaryChartSlice> & slices) {
        int size = slices.size();
        num_slices += size;

        for (int i=0; i<size; ++i) {
            const SummaryChartSlice & slice = slices.at(i);
            total_length += slice.height;
        }

        num_days++;
    }
    virtual void afterDraw(QPainter &, gGraph &, QRect);


    //! \brief Renders the graph to the QPainter object
    virtual void paint(QPainter &painter, gGraph &graph, const QRegion &region);
    virtual Layer * Clone() {
        gSessionTimesChart * sc = new gSessionTimesChart();
        gSummaryChart::CloneInto(sc);
        CloneInto(sc);
        return sc;
    }

    void CloneInto(gSessionTimesChart * layer) {
        layer->split = split;
    }
    QTime split;
    int num_slices;
    int num_days;
    int total_slices;
    double total_length;
};


class gUsageChart : public gSummaryChart
{
public:
    gUsageChart()
        :gSummaryChart("Usage", MT_CPAP) {
        addCalc(NoChannel, ST_HOURS);
    }
    virtual ~gUsageChart() {}

    virtual void preCalc();
    virtual void customCalc(Day *, QList<SummaryChartSlice> &);
    virtual void afterDraw(QPainter &, gGraph &, QRect);
    virtual void populate(Day *day, int idx);

    virtual QString tooltipData(Day * day, int);


    virtual Layer * Clone() {
        gUsageChart * sc = new gUsageChart();
        gSummaryChart::CloneInto(sc);
        CloneInto(sc);
        return sc;
    }

    void CloneInto(gUsageChart * layer) {
        layer->incompdays = incompdays;
        layer->compliance_threshold = compliance_threshold;
    }

private:
    int incompdays;
    EventDataType compliance_threshold;
    double totalhours;
    int totaldays;

};

class gAHIChart : public gSummaryChart
{
public:
    gAHIChart()
        :gSummaryChart("AHIChart", MT_CPAP) {
        channels.append(CPAP_ClearAirway);
        channels.append(CPAP_Obstructive);
        channels.append(CPAP_Apnea);
        channels.append(CPAP_Hypopnea);
        if (p_profile->general->calculateRDI())
            channels.append(CPAP_RERA);
        num_channels = channels.size();
    }
    virtual ~gAHIChart() {}

    virtual void preCalc();
    virtual void customCalc(Day *, QList<SummaryChartSlice> &);
    virtual void afterDraw(QPainter &, gGraph &, QRect);

    virtual void populate(Day *, int idx);

    virtual QString tooltipData(Day * day, int);

    virtual Layer * Clone() {
        gAHIChart * sc = new gAHIChart();
        gSummaryChart::CloneInto(sc);
        CloneInto(sc);
        return sc;
    }

    void CloneInto(gAHIChart * layer) {
        layer->channels = channels;
        layer->num_channels = num_channels;
        layer->indices = indices;
        layer->ahi_total = ahi_total;
        layer->calc_cnt = calc_cnt;
    }

    QList<ChannelID> channels;
    int num_channels;

    QHash<ChannelID, double> indices;
    double ahi_total;
    double total_hours;
    int calc_cnt;
};


class gPressureChart : public gSummaryChart
{
public:
    gPressureChart()
        :gSummaryChart("Pressure", MT_CPAP) {
    }
    virtual ~gPressureChart() {}

    virtual void SetDay(Day * day = nullptr) {
        gSummaryChart::SetDay(day);
        m_miny = 0;
        m_maxy = 24;
    }


    virtual Layer * Clone() {
        gPressureChart * sc = new gPressureChart();
        gSummaryChart::CloneInto(sc);
        return sc;
    }

    virtual void populate(Day * day, int idx);
    virtual QString tooltipData(Day * day, int idx) {
        return day->getCPAPMode() + "\n" + day->getPressureSettings() + gSummaryChart::tooltipData(day, idx);

    }

};

#endif // GSESSIONTIMESCHART_H
