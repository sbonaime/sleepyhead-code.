/********************************************************************
 SleepLib Day Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include "day.h"

Day::Day(Machine *m)
:machine(m)
{

    d_firstsession=true;
    sessions.clear();
}
Day::~Day()
{
    vector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        delete (*s);
    }

}
MachineType Day::machine_type()
{
    return machine->GetType();
};

void Day::AddSession(Session *s)
{
    if (!s) {
        qWarning("Day::AddSession called with NULL session object");
        return;
    }
    if (d_firstsession) {
        d_firstsession=false;
        d_first=s->first();
        d_last=s->last();
    } else {
        if (d_first > s->first()) d_first = s->first();
        if (d_last < s->last()) d_last = s->last();
    }
    sessions.push_back(s);
}

EventDataType Day::summary_sum(MachineCode code)
{
    EventDataType val=0;
    vector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.summary.find(code)!=sess.summary.end()) {
            val+=sess.summary[code].toDouble();
        }
    }
    return val;
}

EventDataType Day::summary_max(MachineCode code)
{
    EventDataType val=0,tmp;

    vector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.summary.find(code)!=sess.summary.end()) {
            tmp=sess.summary[code].toDouble();
            if (tmp>val) val=tmp;
        }
    }
    return val;
}
EventDataType Day::summary_min(MachineCode code)
{
    EventDataType val=0,tmp;
    bool fir=true;
    // Cache this?

    vector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.summary.find(code)!=sess.summary.end()) {
            tmp=sess.summary[code].toDouble();
            if (fir) {
                val=tmp;
                fir=false;
            } else {
                if (val<tmp) val=tmp;
            }
        }
    }
    return val;
}

EventDataType Day::summary_avg(MachineCode code)
{
    EventDataType val=0,tmp=0;

    int cnt=0;
    vector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.summary.find(code)!=sess.summary.end()) {
            tmp+=sess.summary[code].toDouble();
            cnt++;
        }
    }
    val=tmp/EventDataType(cnt);
    return val;
}
EventDataType Day::summary_weighted_avg(MachineCode code)
{
    double s0=0,s1=0,s2=0;
    for (vector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.summary.find(code)!=sess.summary.end()) {
            s0=sess.hours();
            s1+=sess.summary[code].toDouble()*s0;
            s2+=s0;
        }
    }
    if (s2==0) return 0;
    return (s1/s2);

}

EventDataType Day::min(MachineCode code,int field)
{
    EventDataType val=0,tmp;
    bool fir=true;
    // Cache this?

    vector<Session *>::iterator s;

    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            tmp=sess.min_event_field(code,field);
            if (fir) {
                val=tmp;
                fir=false;
            } else {
                if (val>tmp) val=tmp;
            }
        }
    }
    return val;
}

EventDataType Day::max(MachineCode code,int field)
{
    EventDataType val=0,tmp;
    bool fir=true;
    // Cache this?

    // Don't assume sessions are in order.
    vector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            tmp=sess.max_event_field(code,field);
            if (fir) {
                val=tmp;
                fir=false;
            } else {
                if (val<tmp) val=tmp;
            }
        }
    }
    return val;
}

EventDataType Day::avg(MachineCode code,int field)
{
    double val=0;
    // Cache this?
    int cnt=0;
    vector<Session *>::iterator s;

    // Don't assume sessions are in order.
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            val+=sess.avg_event_field(code,field);
            cnt++;
        }
    }
    if (cnt==0) return 0;
    return EventDataType(val/float(cnt));
}

EventDataType Day::sum(MachineCode code,int field)
{
    // Cache this?
    EventDataType val=0;
    vector<Session *>::iterator s;

    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            val+=sess.sum_event_field(code,field);
        }
    }
    return val;
}

int Day::count(MachineCode code)
{
    int val=0;

    for (vector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            val+=sess.count_events(code);
        }
    }
    return val;
}
EventDataType Day::weighted_avg(MachineCode code,int field)
{
    double s0=0,s1=0,s2=0;
    for (vector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            s0=sess.hours();
            s1+=sess.weighted_avg_event_field(code,field)*s0;
            s2+=s0;
        }
    }
    if (s2==0) return 0;
    return (s1/s2);
}
// Total session time in milliseconds
qint64 Day::total_time()
{
    d_totaltime=0;
    for (vector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        d_totaltime+=sess.last()-sess.first();
    }
    return d_totaltime;
}
EventDataType Day::percentile(MachineCode code,int field,double percent)
{
    double val=0;
    int cnt=0;

    for (vector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            val+=sess.percentile(code,field,percent);
            cnt++;
        }
    }
    if (cnt==0) return 0;
    return EventDataType(val/cnt);

}

qint64 Day::first(MachineCode code)
{
    qint64 date=0;
    qint64 tmp;

    for (vector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            tmp=sess.events[code][0]->time();
            if (!date) {
                date=tmp;
            } else {
                if (tmp<date) date=tmp;
            }
        }
    }
    return date;
}

qint64 Day::last(MachineCode code)
{
    qint64 date=0;
    qint64 tmp;

    for (vector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.events.find(code)!=sess.events.end()) {
            vector<Event *>::reverse_iterator i=sess.events[code].rbegin();
            assert(i!=sess.events[code].rend());
            tmp=(*i)->time();
            if (!date) {
                date=tmp;
            } else {
                if (tmp>date) date=tmp;
            }
        }
    }
    return date;
}

void Day::OpenEvents()
{
    vector<Session *>::iterator s;

    for (s=sessions.begin();s!=sessions.end();s++) {
        (*s)->OpenEvents();
    }

}
void Day::OpenWaveforms()
{
    vector<Session *>::iterator s;

    for (s=sessions.begin();s!=sessions.end();s++) {
        (*s)->OpenWaveforms();
    }
}
