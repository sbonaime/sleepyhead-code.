/********************************************************************
 SleepLib Day Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef DAY_H
#define DAY_H

#include "SleepLib/machine_common.h"
#include "SleepLib/machine.h"
#include "SleepLib/event.h"
#include "SleepLib/session.h"

class OneTypePerDay
{
};

class Machine;
class Session;
class Day
{
public:
    Day(Machine *m);
    ~Day();
    void AddSession(Session *s);

    MachineType machine_type();

    int count(ChannelID code);
    EventDataType min(ChannelID code);
    EventDataType max(ChannelID code);
    EventDataType cph(ChannelID code);
    EventDataType sph(ChannelID code);
    EventDataType p90(ChannelID code);
    EventDataType avg(ChannelID code);
    EventDataType sum(ChannelID code);
    EventDataType wavg(ChannelID code);

    EventDataType percentile(ChannelID mc,double percent);

    // Note, the following convert to doubles without considering the consequences fully.
    EventDataType settings_avg(ChannelID code);
    EventDataType settings_wavg(ChannelID code);
    EventDataType settings_sum(ChannelID code);
    EventDataType settings_min(ChannelID code);
    EventDataType settings_max(ChannelID code);

    qint64 first() { return d_first; }
    qint64 last() { return d_last; }
    void setFirst(qint64 val) { d_first=val; }
    void setLast(qint64 val) { d_last=val; }
    qint64 first(ChannelID code);
    qint64 last(ChannelID code);


    qint64 total_time(); // in milliseconds
    EventDataType hours() { return EventDataType(double(total_time())/3600000.0); }

    Session *operator [](int i) { return sessions[i]; }

    QVector<Session *>::iterator begin() { return sessions.begin(); }
    QVector<Session *>::iterator end() { return sessions.end(); }

    int size() { return sessions.size(); }
    Machine *machine;

    void OpenEvents();
    QVector<Session *> & getSessions() { return sessions; }
    bool channelExists(ChannelID id);
    bool channelHasData(ChannelID id);
    bool settingExists(ChannelID id);

protected:
    QVector<Session *> sessions;
    qint64 d_first,d_last;
private:
    bool d_firstsession;
};


#endif // DAY_H
