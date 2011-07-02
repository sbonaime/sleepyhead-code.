/********************************************************************
 SleepLib Event Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include "event.h"

Event::Event(qint64 time,MachineCode code, EventDataType * data, int fields)
    :e_time(time),e_code(code)
{
    e_fields=fields;
    for (int i=0; i<fields; i++) {
        e_data.push_back(data[i]);
    }

}
Event::~Event()
{

};
