/*
 SleepLib Event Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef EVENT_H
#define EVENT_H

#include <QDateTime>
//#include "SleepLib/session.h"
#include "machine_common.h"

enum EventListType { EVL_Waveform, EVL_Event };

class EventList
{
    friend class Session;
public:
    EventList(EventListType et,EventDataType gain=1.0, EventDataType offset=0.0, EventDataType min=0.0, EventDataType max=0.0, double rate=0.0,bool second_field=false);
    ~EventList();

    void AddEvent(qint64 time, EventStoreType data, EventStoreType data2=0);
    void AddWaveform(qint64 start, qint16 * data, int recs, qint64 duration);
    void AddWaveform(qint64 start, unsigned char * data, int recs, qint64 duration);
    void AddWaveform(qint64 start, char * data, int recs, qint64 duration);

    inline const quint32 & count() { return m_count; }
    void setCount(quint32 count) { m_count=count; }

    inline EventStoreType raw(int i) { return m_data[i]; }
    inline EventStoreType raw2(int i) { return m_data2[i]; }

    EventDataType data(quint32 i);
    EventDataType data2(quint32 i);
    qint64 time(quint32 i);
    bool hasSecondField() { return m_second_field; }
    inline const qint64 & first() { return m_first; }
    inline const qint64 & last() { return m_last; }
    inline qint64 duration() { return m_last-m_first; }
    void setFirst(qint64 val) { m_first=val; }
    void setLast(qint64 val) { m_last=val; }
    void setType(EventListType type) { m_type=type; }

    void setGain(EventDataType v) { m_gain=v; }
    void setOffset(EventDataType v) { m_offset=v; }
    void setMin(EventDataType v) { m_min=v; }
    void setMax(EventDataType v) { m_max=v; }
    void setMin2(EventDataType v) { m_min2=v; }
    void setMax2(EventDataType v) { m_max2=v; }
    void setRate(EventDataType v) { m_rate=v; }
    //void setCode(ChannelID id) { m_code=id; }

    inline const EventDataType & Min() { return m_min; }
    inline const EventDataType & Max() { return m_max; }
    inline const EventDataType & min2() { return m_min2; }
    inline const EventDataType & max2() { return m_max2; }
    inline const EventDataType & gain() { return m_gain; }
    inline const EventDataType & offset() { return m_offset; }
    inline const EventDataType & rate() { return m_rate; }
    inline const EventListType & type() { return m_type; }
    //inline const ChannelID & code() { return m_code; }
    inline const bool & update_minmax() { return m_update_minmax; }

    QString dimension() { return m_dimension; }
    void setDimension(QString dimension) { m_dimension=dimension; }

    QVector<EventStoreType> & getData() { return m_data; }
    QVector<EventStoreType> & getData2() { return m_data2; }
    QVector<quint32> & getTime() { return m_time; }
protected:
    QVector<quint32> m_time; // 32bitalize this.. add offsets to m_first
    QVector<EventStoreType> m_data;
    QVector<EventStoreType> m_data2;
    //ChannelID m_code;
    EventListType m_type;
    quint32 m_count;

    EventDataType m_gain;
    EventDataType m_offset;
    EventDataType m_min,m_min2;
    EventDataType m_max,m_max2;
    EventDataType m_rate;     // Waveform sample rate

    QString m_dimension;

    qint64 m_first,m_last;
    bool m_update_minmax;
    bool m_second_field;
};


#endif // EVENT_H
