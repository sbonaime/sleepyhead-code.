/********************************************************************
 SleepLib Event Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include <QDebug>
#include "event.h"

EventList::EventList(MachineCode code,EventListType et,EventDataType gain, EventDataType offset, EventDataType min, EventDataType max,double rate)
    :m_code(code),m_type(et),m_gain(gain),m_offset(offset),m_min(min),m_max(max),m_rate(rate)
{
    m_first=m_last=0;
    m_count=0;

    if (min==max) {  // Update Min & Max unless forceably set here..
        m_update_minmax=true;
        m_min=999999999;
        m_max=-999999999;
    } else {
        m_update_minmax=false;
    }

    // Reserve a few to increase performace??
}
EventList::~EventList()
{
}
qint64 EventList::time(int i)
{
    if (m_type==EVL_Waveform) {
        qint64 t=m_first+(double(i)*m_rate);
        return t;
    }
    return m_time[i];
}

EventDataType EventList::data(int i)
{
    return EventDataType(m_data[i])*m_gain;
}

void EventList::AddEvent(qint64 time, EventStoreType data)
{
    // Apply gain & offset
    m_data.push_back(data);
    EventDataType val=data;
    val*=m_gain;
    val+=m_offset;
    m_count++;
    if (m_update_minmax) {
        if (m_min>val) m_min=val;
        if (m_max<val) m_max=val;
    }

    m_time.push_back(time);
    if (!m_first) {
        m_first=time;
        m_last=time;
    }
    if (m_first>time) m_first=time;
    if (m_last<time) m_last=time;
}

// Adds a consecutive waveform chunk
void EventList::AddWaveform(qint64 start, qint16 * data, int recs, qint64 duration)
{
    if (m_type!=EVL_Waveform) {
        qWarning() << "Attempted to add waveform data to non-waveform object";
        return;
    }
    if (!m_rate) {
        qWarning() << "Attempted to add waveform without setting sample rate";
        return;
    }
    // duration=recs*rate;
    qint64 last=start+duration;
    if (!m_first) {
        m_first=start;
        m_last=last;
    }
    if (m_last>start) {
        //qWarning() << "Attempted to add waveform with previous timestamp";
       // return;

        // technically start should equal m_last+1 sample.. check this too.
    }
    if (m_last<last) {
        m_last=last;
    }
    // TODO: Check waveform chunk really is contiguos
    //double rate=duration/recs;

    //realloc buffers.
    m_count+=recs;
    m_data.reserve(m_count);

    EventDataType val;
    for (int i=0;i<recs;i++) {
        val=EventDataType(data[i])*m_gain+m_offset;
        if (m_update_minmax) {
            if (m_min>val) m_min=val;
            if (m_max<val) m_max=val;
        }
        m_data.push_back(data[i]);
    }

}
void EventList::AddWaveform(qint64 start, unsigned char * data, int recs, qint64 duration)
{
    if (m_type!=EVL_Waveform) {
        qWarning() << "Attempted to add waveform data to non-waveform object";
        return;
    }
    if (!m_rate) {
        qWarning() << "Attempted to add waveform without setting sample rate";
        return;
    }
    // duration=recs*rate;
    qint64 last=start+duration;
    if (!m_first) {
        m_first=start;
        m_last=last;
    }
    if (m_last>start) {
        //qWarning() << "Attempted to add waveform with previous timestamp";
       // return;

        // technically start should equal m_last+1 sample.. check this too.
    }
    if (m_last<last) {
        m_last=last;
    }

    // TODO: Check waveform chunk really is contiguos

    //realloc buffers.
    m_count+=recs;
    m_data.reserve(m_count);

    EventDataType val;
    for (int i=0;i<recs;i++) {
        val=EventDataType(data[i])*m_gain+m_offset;
        if (m_update_minmax) {
            if (m_min>val) m_min=val;
            if (m_max<val) m_max=val;
        }
        m_data.push_back(data[i]);
    }

}
void EventList::AddWaveform(qint64 start, char * data, int recs, qint64 duration)
{
    if (m_type!=EVL_Waveform) {
        qWarning() << "Attempted to add waveform data to non-waveform object";
        return;
    }
    if (!m_rate) {
        qWarning() << "Attempted to add waveform without setting sample rate";
        return;
    }
    // duration=recs*rate;
    qint64 last=start+duration;
    if (!m_first) {
        m_first=start;
        m_last=last;
    } else {
        if (m_last>start) {
            //qWarning() << "Attempted to add waveform with previous timestamp";
            //return;

            // technically start should equal m_last+1 sample.. check this too.
        }
        if (m_last<last) {
            m_last=last;
        }
    }

    // TODO: Check waveform chunk really is contiguos

    //realloc buffers.
    m_count+=recs;
    m_data.reserve(m_count);

    EventDataType val;
    for (int i=0;i<recs;i++) {
        val=EventDataType(data[i])*m_gain+m_offset;
        if (m_update_minmax) {
            if (m_min>val) m_min=val;
            if (m_max<val) m_max=val;
        }
        m_data.push_back(data[i]);
    }

}
