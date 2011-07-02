/********************************************************************
 SleepLib Day Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef DAY_H
#define DAY_H

#include "SleepLib/machine.h"
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

    EventDataType min(MachineCode code,int field=0);
    EventDataType max(MachineCode code,int field=0);
    EventDataType avg(MachineCode code,int field=0);
    EventDataType sum(MachineCode code,int field=0);
    EventDataType count(MachineCode code);
    EventDataType weighted_avg(MachineCode code,int field=0);
    EventDataType percentile(MachineCode mc,int field,double percent);

    // Note, the following convert to doubles without considering the consequences fully.
    EventDataType summary_avg(MachineCode code);
    EventDataType summary_weighted_avg(MachineCode code);
    EventDataType summary_sum(MachineCode code);
    EventDataType summary_min(MachineCode code);
    EventDataType summary_max(MachineCode code);

    qint64 first(MachineCode code);
    qint64 last(MachineCode code);
    qint64 first() { return d_first; };
    qint64 last() { return d_last; };

    qint64 total_time(); // in milliseconds
    double hours() { return double(total_time())/3600000.0; };

    Session *operator [](int i) { return sessions[i]; };

    vector<Session *>::iterator begin() { return sessions.begin(); };
    vector<Session *>::iterator end() { return sessions.end(); };

    size_t size() { return sessions.size(); };
    Machine *machine;

    void OpenEvents();
    void OpenWaveforms();

protected:
    vector<Session *> sessions;
    qint64 d_first,d_last;
    qint64 d_totaltime;
private:
    bool d_firstsession;
};


#endif // DAY_H
