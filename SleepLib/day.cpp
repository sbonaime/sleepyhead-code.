/*
 SleepLib Day Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "day.h"
#include "profiles.h"

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
EventDataType Day::percentile(ChannelID code,EventDataType percentile)
{
    // Cache this calculation


    //if (percentile>=1) return 0; // probably better to crash and burn.

    QVector<Session *>::iterator s;

    // Don't assume sessions are in order.
    QVector<EventDataType> ar;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        QHash<ChannelID,QVector<EventList *> >::iterator ei=sess.eventlist.find(code);

        if (ei==sess.eventlist.end())
            continue;
        for (int e=0;e<ei.value().size();e++) {
            EventList *ev=ei.value()[e];
            //if ()
            for (unsigned j=0;j<ev->count();j++) {
                ar.push_back(ev->data(j));
            }
        }
    }
    int size=ar.size();
    if (!size)
        return 0;
    size--;
    qSort(ar);
    int p=EventDataType(size)*percentile;
    float p2=EventDataType(size)*percentile;
    float diff=p2-p;
    EventDataType val=ar[p];
    if (diff>0) {
        int s=p+1;
        if (s>size-1) s=size-1;
        EventDataType v2=ar[s];
        EventDataType v3=v2-val;
        if (v3>0) {
            val+=v3*diff;
        }

    }

    return val;
}

EventDataType Day::p90(ChannelID code) // The "average" p90.. this needs fixing.
{
    int size=sessions.size();

    if (size==0) return 0;
    else if (size==1) return sessions[0]->p90(code);

    QHash<ChannelID,EventDataType>::iterator i=m_p90.find(code);
    if (i!=m_p90.end())
        return i.value();


    QVector<Session *>::iterator s;

    // Don't assume sessions are in order.

    unsigned cnt=0,c;
    EventDataType p;

    QMap<EventDataType, unsigned> pmap;
    QMap<EventDataType, float> tmap;
    QMap<EventDataType, unsigned>::iterator pi;
    for (s=sessions.begin();s!=sessions.end();s++) {
        Session & sess=*(*s);
        c=sess.count(code);
        if (c>0) {
            cnt+=c;
            p=sess.p90(code); //percentile(code,0.9);
            if (!pmap.contains(p)) {
                pmap[p]=c;
                tmap[p]=sess.hours();
            } else {
                pmap[p]+=c;
                tmap[p]+=sess.hours();
            }
        }
    }
    EventDataType val;
    size=pmap.size();
    if (!size) {
        m_p90[code]=val=0;
        return val;
    } else if (size==1) {
        m_p90[code]=val=pmap.begin().key();
        return val;
    }

    OpenEvents();
    EventDataType realp90=percentile(code,0.9);

    val=realp90;
    /*double s0=0,s1=0,s2=0,s3=0;

    for (pi=pmap.begin();pi!=pmap.end();pi++) {
        s2=tmap[pi.key()];
        s3=pi.value();
        s0+=pi.key() * s3 * s2;
        s1+=s3*s2;
    }
    if (s1==0)
        return 0;

    val=s0/s1;
    qDebug() << first() << code << realp90 << val; */

    m_p90[code]=val;
    return val;
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
        Session & sess=*(*s);
        d_totaltime+=sess.length();
    }
    return d_totaltime;
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
EventDataType Day::Min(ChannelID code)
{
    EventDataType min=0;
    EventDataType tmp;
    bool first=true;
    for (QVector<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
        if (!(*s)->m_min.contains(code)) continue;
        //if ((*s)->eventlist.find(code)==(*s)->eventlist.end()) continue;
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
        Session *sess=*s;
        switch(type) {
        case ST_90P:
            has=sess->m_90p.contains(code);
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
        sum+=sessions[i]->count(code);
    }
    return sum;
}
bool Day::settingExists(ChannelID id)
{
    for (int j=0;j<sessions.size();j++) {
        QHash<ChannelID,QVariant>::iterator i=sessions[j]->settings.find(id);
        if (i!=sessions[j]->settings.end()) {
            return true;
        }
    }
    return false;
}
bool Day::channelExists(ChannelID id)
{
    bool r=false;
    for (int i=0;i<sessions.size();i++) {
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
        if (sessions[i]->channelExists(id)) {
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
