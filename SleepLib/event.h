/********************************************************************
 SleepLib Event Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef EVENT_H
#define EVENT_H

#include <QDateTime>
#include "SleepLib/session.h"
#include "machine_common.h"


enum EventListType { EVL_Waveform, EVL_Event };

class EventList
{
    friend class Session;
public:
    EventList(MachineCode code,EventListType et,EventDataType gain=1.0, EventDataType offset=0.0, EventDataType min=0.0, EventDataType max=0.0, double rate=0.0);
    ~EventList();

    void AddEvent(qint64 time, EventStoreType data);
    void AddWaveform(qint64 start, qint16 * data, int recs, qint64 duration);
    void AddWaveform(qint64 start, unsigned char * data, int recs, qint64 duration);
    void AddWaveform(qint64 start, char * data, int recs, qint64 duration);

    inline const int & count() { return m_count; }
    inline EventStoreType raw(int i) { return m_data[i]; }

    EventDataType data(int i);
    qint64 time(int i);
    inline const qint64 & first() { return m_first; }
    inline const qint64 & last() { return m_last; }
    inline qint64 duration() { return m_last-m_first; }

    void setGain(EventDataType v) { m_gain=v; }
    void setOffset(EventDataType v) { m_offset=v; }
    void setMin(EventDataType v) { m_min=v; }
    void setMax(EventDataType v) { m_max=v; }
    void setRate(EventDataType v) { m_rate=v; }
    inline const EventDataType & min() { return m_min; }
    inline const EventDataType & max() { return m_max; }
    inline const EventDataType & gain() { return m_gain; }
    inline const EventDataType & offset() { return m_offset; }
    inline const EventDataType & rate() { return m_rate; }
    inline const EventListType & type() { return m_type; }
    inline const MachineCode & code() { return m_code; }
    inline const bool & update_minmax() { return m_update_minmax; }
    const vector<EventStoreType> & getData() { return m_data; }
    const vector<qint64> & getTime() { return m_time; }
protected:
    vector<qint64> m_time;
    vector<EventStoreType> m_data;
    MachineCode m_code;
    EventListType m_type;
    int m_count;

    EventDataType m_gain;
    EventDataType m_offset;
    EventDataType m_min;
    EventDataType m_max;
    EventDataType m_rate;     // Waveform sample rate

    qint64 m_first,m_last;
    bool m_update_minmax;
};


#endif // EVENT_H
