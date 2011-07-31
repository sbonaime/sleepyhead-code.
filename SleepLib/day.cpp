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
}
Day::~Day()
{
    QVector<Session *>::iterator s;
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

EventDataType Day::settings_sum(ChannelID code)
{
    EventDataType val=0;
    QVector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        QHash<ChannelID,QVariant>::iterator i=sess.settings.find(code);
        if (i!=sess.settings.end()) {
            val+=i.value().toDouble();
        }
    }
    return val;
}

EventDataType Day::settings_max(ChannelID code)
{
    EventDataType val=0,tmp;

    QVector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        QHash<ChannelID,QVariant>::iterator i=sess.settings.find(code);
        if (i!=sess.settings.end()) {
            tmp=i.value().toDouble();
            if (tmp>val) val=tmp;
        }
    }
    return val;
}
EventDataType Day::settings_min(ChannelID code)
{
    EventDataType val=0,tmp;
    bool fir=true;
    // Cache this?

    QVector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        QHash<ChannelID,QVariant>::iterator i=sess.settings.find(code);
        if (i!=sess.settings.end()) {
            tmp=i.value().toDouble();
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

EventDataType Day::settings_avg(ChannelID code)
{
    EventDataType val=0;

    int cnt=0;
    QVector<Session *>::iterator s;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        QHash<ChannelID,QVariant>::iterator i=sess.settings.find(code);
        if (i!=sess.settings.end()) {
            val+=i.value().toDouble();
            cnt++;
        }
    }
    val/=EventDataType(cnt);
    return val;
}
EventDataType Day::settings_wavg(ChannelID code)
{
    double s0=0,s1=0,s2=0;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        QHash<ChannelID,QVariant>::iterator i=sess.settings.find(code);
        if (i!=sess.settings.end()) {
            s0=sess.hours();
            s1+=i.value().toDouble()*s0;
            s2+=s0;
        }
    }
    if (s2==0) return 0;
    return (s1/s2);

}
EventDataType Day::p90(ChannelID code) // The "average" p90.. this needs fixing.
{
    double val=0;
    // Cache this?
    int cnt=0;
    QVector<Session *>::iterator s;

    // Don't assume sessions are in order.
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        QHash<ChannelID,QVector<EventList *> >::iterator i=sess.eventlist.find(code);
        if (i!=sess.eventlist.end()) {
            val+=sess.p90(code);
            cnt++;
        }
    }
    if (cnt==0) return 0;
    return EventDataType(val/float(cnt));
}

EventDataType Day::avg(ChannelID code)
{
    double val=0;
    // Cache this?
    int cnt=0;
    QVector<Session *>::iterator s;

    // Don't assume sessions are in order.
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.eventlist.find(code)!=sess.eventlist.end()) {
            val+=sess.avg(code);
            cnt++;
        }
    }
    if (cnt==0) return 0;
    return EventDataType(val/float(cnt));
}

EventDataType Day::sum(ChannelID code)
{
    // Cache this?
    EventDataType val=0;
    QVector<Session *>::iterator s;

    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.eventlist.find(code)!=sess.eventlist.end()) {
            val+=sess.sum(code);
        }
    }
    return val;
}

EventDataType Day::wavg(ChannelID code)
{
    double s0=0,s1=0,s2=0;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.eventlist.find(code)!=sess.eventlist.end()) {
            s0=sess.hours();
            s1+=sess.wavg(code)*s0;
            s2+=s0;
        }
    }
    if (s2==0) return 0;
    return (s1/s2);
}
// Total session time in milliseconds
qint64 Day::total_time()
{
    qint64 d_totaltime=0;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        d_totaltime+=sess.last()-sess.first();
    }
    return d_totaltime;
}
EventDataType Day::percentile(ChannelID code,double percent)
{
    double val=0;
    int cnt=0;

    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        if (sess.eventlist.find(code)!=sess.eventlist.end()) {
            val+=sess.percentile(code,percent);
            cnt++;
        }
    }
    if (cnt==0) return 0;
    return EventDataType(val/cnt);

}

qint64 Day::first(ChannelID code)
{
    qint64 date=0;
    qint64 tmp;

    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        tmp=(*s)->first(code);
        if (!tmp) continue;
        if (!date) {
            date=tmp;
        } else {
            if (tmp<date) date=tmp;
        }
    }
    return date;
}

qint64 Day::last(ChannelID code)
{
    qint64 date=0;
    qint64 tmp;

    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        tmp=(*s)->last(code);
        if (!tmp) continue;
        if (!date) {
            date=tmp;
        } else {
            if (tmp>date) date=tmp;
        }
    }
    return date;
}
EventDataType Day::min(ChannelID code)
{
    EventDataType min=0;
    EventDataType tmp;
    bool first=true;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if ((*s)->eventlist.find(code)==(*s)->eventlist.end()) continue;
        tmp=(*s)->min(code);
        if (first) {
            min=tmp;
            first=false;
        } else {
            if (tmp<min) min=tmp;
        }
    }
    return min;
}

EventDataType Day::max(ChannelID code)
{
    EventDataType max=0;
    EventDataType tmp;
    bool first=true;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if ((*s)->eventlist.find(code)==(*s)->eventlist.end()) continue;
        tmp=(*s)->max(code);
        if (first) {
            max=tmp;
            first=false;
        } else {
            if (tmp>max) max=tmp;
        }
    }
    return max;
}
int Day::count(ChannelID code)
{
    int sum=0;
    for (int i=0;i<sessions.size();i++) {
        sum+=sessions[i]->count(code);
    }
    return sum;
}
void Day::OpenEvents()
{
    QVector<Session *>::iterator s;

    for (s=sessions.begin();s!=sessions.end();s++) {
        (*s)->OpenEvents();
    }

}
