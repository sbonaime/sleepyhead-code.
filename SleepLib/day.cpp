/*
 SleepLib Day Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "day.h"
#include "profiles.h"
#include <algorithm>

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
}
Session *Day::find(SessionID sessid) {
    for (int i=0;i<size();i++) {
        if (sessions[i]->session()==sessid)
            return sessions[i];
    }
    return NULL;
}

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
        if (!(*s)->enabled()) continue;

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
        if (!(*s)->enabled()) continue;

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
        if (!(*s)->enabled()) continue;

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
        if (!(*s)->enabled()) continue;

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
    double s0=0,s1=0,s2=0,tmp;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;

        Session & sess=*(*s);

        QHash<ChannelID,QVariant>::iterator i=sess.settings.find(code);
        if (i!=sess.settings.end()) {
            s0=sess.hours();
            tmp=i.value().toDouble();
            s1+=tmp*s0;
            s2+=s0;
        }
    }
    if (s2==0) return 0;
    tmp=(s1/s2);
    return tmp;
}






EventDataType Day::percentile(ChannelID code,EventDataType percentile)
{
    // Cache this calculation
    //if (percentile>=1) return 0; // probably better to crash and burn.

    QVector<Session *>::iterator s;

    // Don't assume sessions are in order.
    QVector<EventDataType> ar;
    for (s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;

        Session & sess=*(*s);
        QHash<ChannelID,QHash<EventStoreType, EventStoreType> > ::iterator ei=sess.m_valuesummary.find(code);
        if (ei==sess.m_valuesummary.end()) continue;
        EventDataType gain=sess.m_gain[code];
        for (QHash<EventStoreType, EventStoreType>::iterator i=ei.value().begin();i!=ei.value().end();i++) {
            for (int j=0;j<i.value();j++) {
                ar.push_back(float(i.key())*gain);
            }
        }
    }
    int size=ar.size();
    if (!size)
        return 0;
    size--;

    QVector<EventDataType>::iterator first=ar.begin();
    QVector<EventDataType>::iterator last=ar.end();
    QVector<EventDataType>::iterator middle = first + int((last-first) * percentile);
    std::nth_element(first,middle,last);
    EventDataType val=*middle;

//    qSort(ar);
//    int p=EventDataType(size)*percentile;
//    float p2=EventDataType(size)*percentile;
//    float diff=p2-p;
//    EventDataType val=ar[p];
//    if (diff>0) {
//        int s=p+1;
//        if (s>size-1) s=size-1;
//        EventDataType v2=ar[s];
//        EventDataType v3=v2-val;
//        if (v3>0) {
//            val+=v3*diff;
//        }

//    }

    return val;
}

EventDataType Day::p90(ChannelID code)
{
    return percentile(code,0.90);
}

EventDataType Day::avg(ChannelID code)
{
    double val=0;
    // Cache this?
    int cnt=0;
    QVector<Session *>::iterator s;

    // Don't assume sessions are in order.
    for (s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;

        Session & sess=*(*s);
        if (sess.m_avg.contains(code)) {
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
        if (!(*s)->enabled()) continue;

        Session & sess=*(*s);
        if (sess.m_sum.contains(code)) {
            val+=sess.sum(code);
        }
    }
    return val;
}

EventDataType Day::wavg(ChannelID code)
{
    double s0=0,s1=0,s2=0;
    qint64 d;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;

        Session & sess=*(*s);

        if (sess.m_wavg.contains(code)) {
            d=sess.length();//.last(code)-sess.first(code);
            s0=double(d)/3600000.0;
            if (s0>0) {
                s1+=sess.wavg(code)*s0;
                s2+=s0;
            }
        }
    }
    if (s2==0)
        return 0;

    return (s1/s2);
}
// Total session time in milliseconds
qint64 Day::total_time()
{
    qint64 d_totaltime=0;
    for (QVector<Session *>::iterator s=begin();s!=end();s++) {
        if (!(*s)->enabled()) continue;

        Session & sess=*(*s);
        d_totaltime+=sess.length();
    }
    return d_totaltime;
}
bool Day::hasEnabledSessions()
{
    bool b=false;
    for (QVector<Session *>::iterator s=begin();s!=end();s++) {
        if ((*s)->enabled()) {
            b=true;
            break;
        }
    }
    return b;
}

/*EventDataType Day::percentile(ChannelID code,double percent)
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

}*/

qint64 Day::first(ChannelID code)
{
    qint64 date=0;
    qint64 tmp;

    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;
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
        if (!(*s)->enabled()) continue;
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
EventDataType Day::Min(ChannelID code)
{
    EventDataType min=0;
    EventDataType tmp;
    bool first=true;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;

        if (!(*s)->m_min.contains(code))
            continue;
        tmp=(*s)->Min(code);
        if (first) {
            min=tmp;
            first=false;
        } else {
            if (tmp<min) min=tmp;
        }
    }
    return min;
}

bool Day::hasData(ChannelID code, SummaryType type)
{
    bool has=false;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;
        Session *sess=*s;
        switch(type) {
        case ST_90P:
            has=sess->m_90p.contains(code);
            break;
        case ST_PERC:
            has=sess->m_valuesummary.contains(code);
            break;
        case ST_MIN:
            has=sess->m_min.contains(code);
            break;
        case ST_MAX:
            has=sess->m_max.contains(code);
            break;
        case ST_CNT:
            has=sess->m_cnt.contains(code);
            break;
        case ST_AVG:
            has=sess->m_avg.contains(code);
            break;
        case ST_WAVG:
            has=sess->m_wavg.contains(code);
            break;
        case ST_CPH:
            has=sess->m_cph.contains(code);
            break;
        case ST_SPH:
            has=sess->m_sph.contains(code);
            break;
        case ST_FIRST:
            has=sess->m_firstchan.contains(code);
            break;
        case ST_LAST:
            has=sess->m_lastchan.contains(code);
            break;
        case ST_SUM:
            has=sess->m_sum.contains(code);
            break;
        default:
            break;
        }

        if (has) break;
    }
    return has;
}

EventDataType Day::Max(ChannelID code)
{
    EventDataType max=0;
    EventDataType tmp;
    bool first=true;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;

        if (!(*s)->m_max.contains(code)) continue;
//        if ((*s)->eventlist.find(code)==(*s)->eventlist.end()) continue;
        tmp=(*s)->Max(code);
        if (first) {
            max=tmp;
            first=false;
        } else {
            if (tmp>max) max=tmp;
        }
    }
    return max;
}
EventDataType Day::cph(ChannelID code)
{
    EventDataType sum=0;
    //EventDataType h=0;
    for (int i=0;i<sessions.size();i++) {
        if (!sessions[i]->enabled()) continue;
        if (!sessions[i]->m_cph.contains(code)) continue;
        sum+=sessions[i]->cph(code)*sessions[i]->hours();
        //h+=sessions[i]->hours();
    }
    sum/=hours();
    return sum;
}

EventDataType Day::sph(ChannelID code)
{
    EventDataType sum=0;
    EventDataType h=0;
    for (int i=0;i<sessions.size();i++) {
        if (!sessions[i]->enabled()) continue;
        if (!sessions[i]->m_sum.contains(code)) continue;
        sum+=sessions[i]->sum(code)/3600.0;//*sessions[i]->hours();
        //h+=sessions[i]->hours();
    }
    h=hours();
    sum=(100.0/h)*sum;
    return sum;
}

int Day::count(ChannelID code)
{
    int sum=0;
    for (int i=0;i<sessions.size();i++) {
        if (!sessions[i]->enabled()) continue;
        sum+=sessions[i]->count(code);
    }
    return sum;
}
bool Day::settingExists(ChannelID id)
{
    for (int j=0;j<sessions.size();j++) {
        if (!sessions[j]->enabled()) continue;
        QHash<ChannelID,QVariant>::iterator i=sessions[j]->settings.find(id);
        if (i!=sessions[j]->settings.end()) {
            return true;
        }
    }
    return false;
}
bool Day::eventsLoaded()
{
    bool r=false;
    for (int i=0;i<sessions.size();i++) {
        if (sessions[i]->eventsLoaded()) {
            r=true;
            break;
        }
    }
    return r;
}

bool Day::channelExists(ChannelID id)
{
    bool r=false;
    for (int i=0;i<sessions.size();i++) {
        if (!sessions[i]->enabled()) continue;

        if (sessions[i]->eventlist.contains(id)) {
            r=true;
            break;
        }
    }
    return r;
//    return channelHasData(id);
    //if (machine->hasChannel(id)) return true;
    //return false;
}
bool Day::channelHasData(ChannelID id)
{
    bool r=false;
    for (int i=0;i<sessions.size();i++) {
        if (!sessions[i]->enabled()) continue;

        if (sessions[i]->channelExists(id)) {
            r=true;
            break;
        }
        if (sessions[i]->m_valuesummary.contains(id)) {
            r=true;
            break;
        }
    }
    return r;
}

void Day::OpenEvents()
{
    QVector<Session *>::iterator s;

    for (s=sessions.begin();s!=sessions.end();s++) {
        (*s)->OpenEvents();
    }
}
void Day::CloseEvents()
{
    QVector<Session *>::iterator s;

    for (s=sessions.begin();s!=sessions.end();s++) {
        (*s)->TrashEvents();
    }
}

qint64 Day::first()
{
    qint64 date=0;
    qint64 tmp;

    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;
        tmp=(*s)->first();
        if (!tmp) continue;
        if (!date) {
            date=tmp;
        } else {
            if (tmp<date) date=tmp;
        }
    }
    return date;
//    return d_first;
}

//! \brief Returns the last session time of this day
qint64 Day::last()
{
    qint64 date=0;
    qint64 tmp;

    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->enabled()) continue;
        tmp=(*s)->last();
        if (!tmp) continue;
        if (!date) {
            date=tmp;
        } else {
            if (tmp>date) date=tmp;
        }
    }
    return date;
//    return d_last;
}
