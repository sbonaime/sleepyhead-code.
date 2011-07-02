/********************************************************************
 SleepLib Event Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef EVENT_H
#define EVENT_H

#include <QDateTime>
#include "machine_common.h"

typedef double EventDataType;

class Event
{
    friend class Session;
public:
    Event(qint64 time,MachineCode code,EventDataType * data,int fields);
    ~Event();
    EventDataType operator[](short i) {
        if (i<e_fields) return e_data[i];
        else return 0;
    };
    const qint64 & time() {
        return e_time;
    };
    MachineCode code() {
        return e_code;
    };
    short fields() {
        return e_fields;
    };
protected:
    qint64 e_time;
    MachineCode e_code;
    short e_fields;
    vector<EventDataType> e_data;
};

#endif // EVENT_H
