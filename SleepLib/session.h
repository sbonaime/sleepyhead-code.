/*
 SleepLib Session Header
 This stuff contains the base calculation smarts
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef SESSION_H
#define SESSION_H

#include <QDebug>
#include <QHash>
#include <QVector>

#include "SleepLib/machine.h"
#include "SleepLib/schema.h"
#include "SleepLib/event.h"
//class EventList;
class Machine;
const quint32 magic=0xC73216AB;

enum SummaryType { ST_CNT, ST_SUM, ST_AVG, ST_WAVG, ST_90P, ST_MIN, ST_MAX, ST_CPH, ST_SPH, ST_FIRST, ST_LAST, ST_HOURS, ST_SESSIONS, ST_SETMIN, ST_SETAVG, ST_SETMAX, ST_SETWAVG, ST_SETSUM };

class Session
{
public:
    Session(Machine *,SessionID);
    virtual ~Session();

    bool Store(QString path);
    bool StoreSummary(QString filename);
    bool StoreEvents(QString filename);
    //bool Load(QString path);
    bool LoadSummary(QString filename);
    bool LoadEvents(QString filename);

    bool OpenEvents();
    void TrashEvents();

    const SessionID & session() {
        return s_session;
    }
    qint64 first() {
        return s_first;
    }
    qint64 last() {
        return s_last;
    }
    qint64 length() {
        return s_last-s_first;
    }
    void SetSessionID(SessionID s) {
        s_session=s;
    }
    void set_first(qint64 d) {
        if (!s_first) s_first=d;
        else if (d<s_first) s_first=d;
    }
    void set_last(qint64 d) {
        if (d<=s_first) {
            qWarning() << "Session::set_last() d<=s_first";
            return;
        }
        if (!s_last) s_last=d;
        else if (s_last<d) s_last=d;
    }

    double hours() {
        double t=(s_last-s_first)/3600000.0;
        return t;
    }

    void SetChanged(bool val) {
        s_changed=val;
        s_events_loaded=val; // dirty hack putting this here
    }
    bool IsChanged() {
        return s_changed;
    }

    QHash<ChannelID,QVector<EventList *> > eventlist;
    QHash<ChannelID,QVariant> settings;
    QHash<ChannelID,int> m_cnt;
    QHash<ChannelID,double> m_sum;
    QHash<ChannelID,EventDataType> m_avg;
    QHash<ChannelID,EventDataType> m_wavg;
    QHash<ChannelID,EventDataType> m_90p;
    QHash<ChannelID,EventDataType> m_min;
    QHash<ChannelID,EventDataType> m_max;
    QHash<ChannelID,EventDataType> m_cph;  // Counts per hour (eg AHI)
    QHash<ChannelID,EventDataType> m_sph;  // % indice (eg % night in CSR)
    QHash<ChannelID,quint64> m_firstchan;
    QHash<ChannelID,quint64> m_lastchan;


    // UpdateSummaries may recalculate all these, but it may be faster setting upfront
    void setCount(ChannelID id,int val) { m_cnt[id]=val; }
    void setSum(ChannelID id,EventDataType val) { m_sum[id]=val; }
    void setMin(ChannelID id,EventDataType val) { m_min[id]=val; }
    void setMax(ChannelID id,EventDataType val) { m_max[id]=val; }
    void setAvg(ChannelID id,EventDataType val) { m_avg[id]=val; }
    void setWavg(ChannelID id,EventDataType val) { m_wavg[id]=val; }
    void set90p(ChannelID id,EventDataType val) { m_90p[id]=val; }
    void setCph(ChannelID id,EventDataType val) { m_cph[id]=val; }
    void setSph(ChannelID id,EventDataType val) { m_sph[id]=val; }
    void setFirst(ChannelID id,qint64 val) { m_firstchan[id]=val; }
    void setLast(ChannelID id,qint64 val) { m_lastchan[id]=val; }

    int count(ChannelID id);

    int rangeCount(ChannelID id, qint64 first,qint64 last);
    double rangeSum(ChannelID id, qint64 first,qint64 last);
    EventDataType rangeMin(ChannelID id, qint64 first,qint64 last);
    EventDataType rangeMax(ChannelID id, qint64 first,qint64 last);

    double sum(ChannelID id);
    EventDataType avg(ChannelID id);
    EventDataType wavg(ChannelID i);
    EventDataType min(ChannelID id);
    EventDataType max(ChannelID id);
    EventDataType p90(ChannelID id);
    EventDataType cph(ChannelID id);
    EventDataType sph(ChannelID id);

    EventDataType percentile(ChannelID id,EventDataType percentile);

    bool channelExists(QString name);// { return (schema::channel.names.contains(name));}

    bool IsLoneSession() { return s_lonesession; }
    void SetLoneSession(bool b) { s_lonesession=b; }
    void SetEventFile(QString & filename) { s_eventfile=filename; }

    inline void updateFirst(qint64 v) { if (!s_first) s_first=v; else if (s_first>v) s_first=v; }
    inline void updateLast(qint64 v) { if (!s_last) s_last=v; else if (s_last<v) s_last=v; }

    qint64 first(ChannelID code);
    qint64 last(ChannelID code);

    void UpdateSummaries();
    EventList * AddEventList(QString chan, EventListType et, EventDataType gain=1.0, EventDataType offset=0.0, EventDataType min=0.0, EventDataType max=0.0, EventDataType rate=0.0);
    Machine * machine() { return s_machine; }
protected:
    SessionID s_session;

    Machine *s_machine;
    qint64 s_first;
    qint64 s_last;
    bool s_changed;
    bool s_lonesession;
    bool _first_session;

    bool s_events_loaded;
    QString s_eventfile;
};


#endif // SESSION_H
