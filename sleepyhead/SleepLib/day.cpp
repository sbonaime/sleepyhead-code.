/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Day Class Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "day.h"
#include "profiles.h"
#include <cmath>
#include <QMultiMap>
#include <algorithm>

Day::Day(Machine *m)
    : machine(m)
{
    d_firstsession = true;
}
Day::~Day()
{
    QList<Session *>::iterator s;

    for (s = sessions.begin(); s != sessions.end(); ++s) {
        delete(*s);
    }

}
MachineType Day::machine_type() const
{
    return machine->GetType();
}
Session *Day::find(SessionID sessid)
{
    for (int i = 0; i < size(); i++) {
        if (sessions[i]->session() == sessid) {
            return sessions[i];
        }
    }

    return NULL;
}

void Day::AddSession(Session *s)
{
    if (!s) {
        qWarning("Day::AddSession called with NULL session object");
        return;
    }

    //    if (d_firstsession) {
    //        d_firstsession=false;
    //        d_first=s->first();
    //        d_last=s->last();
    //    } else {
    //        if (d_first > s->first()) d_first = s->first();
    //        if (d_last < s->last()) d_last = s->last();
    //    }
    sessions.push_back(s);
}

EventDataType Day::settings_sum(ChannelID code)
{
    EventDataType val = 0;
    QList<Session *>::iterator s;

    for (s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session &sess = *(*s);
        QHash<ChannelID, QVariant>::iterator i = sess.settings.find(code);

        if (i != sess.settings.end()) {
            val += i.value().toDouble();
        }
    }

    return val;
}

EventDataType Day::settings_max(ChannelID code)
{
    EventDataType val = 0, tmp;

    bool fir = true;
    QList<Session *>::iterator s;

    for (s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) {
            continue;
        }

        Session &sess = *(*s);
        QHash<ChannelID, QVariant>::iterator i = sess.settings.find(code);

        if (i != sess.settings.end()) {
            tmp = i.value().toDouble();

            if (fir) {
                val = tmp;
                fir = false;
            } else if (tmp > val) { val = tmp; }
        }
    }

    return val;
}
EventDataType Day::settings_min(ChannelID code)
{
    EventDataType val = 0, tmp;
    bool fir = true;
    // Cache this?

    QList<Session *>::iterator s;

    for (s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session &sess = *(*s);
        QHash<ChannelID, QVariant>::iterator i = sess.settings.find(code);

        if (i != sess.settings.end()) {
            tmp = i.value().toDouble();

            if (fir) {
                val = tmp;
                fir = false;
            } else {
                if (val < tmp) { val = tmp; }
            }
        }
    }

    return val;
}

EventDataType Day::settings_avg(ChannelID code)
{
    EventDataType val = 0;

    int cnt = 0;
    QList<Session *>::iterator s;

    for (s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session &sess = *(*s);
        QHash<ChannelID, QVariant>::iterator i = sess.settings.find(code);

        if (i != sess.settings.end()) {
            val += i.value().toDouble();
            cnt++;
        }
    }

    val /= EventDataType(cnt);
    return val;
}
EventDataType Day::settings_wavg(ChannelID code)
{
    double s0 = 0, s1 = 0, s2 = 0, tmp;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session &sess = *(*s);

        QHash<ChannelID, QVariant>::iterator i = sess.settings.find(code);

        if (i != sess.settings.end()) {
            s0 = sess.hours();
            tmp = i.value().toDouble();
            s1 += tmp * s0;
            s2 += s0;
        }
    }

    if (s2 == 0) { return 0; }

    tmp = (s1 / s2);
    return tmp;
}


EventDataType Day::percentile(ChannelID code, EventDataType percentile)
{
    //    QHash<ChannelID, QHash<EventDataType, EventDataType> >::iterator pi;
    //    pi=perc_cache.find(code);
    //    if (pi!=perc_cache.end()) {
    //        QHash<EventDataType, EventDataType> & hsh=pi.value();
    //        QHash<EventDataType, EventDataType>::iterator hi=hsh.find(
    //        if (hi!=pi.value().end()) {
    //            return hi.value();
    //        }
    //    }
    // Cache this calculation?

    QList<Session *>::iterator s;

    QHash<EventStoreType, qint64> wmap;

    qint64 SN = 0;

    EventDataType lastgain = 0, gain = 0;
    // First Calculate count of all events
    bool timeweight;

    for (s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session &sess = *(*s);
        QHash<ChannelID, QHash<EventStoreType, EventStoreType> > ::iterator ei = sess.m_valuesummary.find(
                    code);

        if (ei == sess.m_valuesummary.end()) { continue; }

        QHash<ChannelID, QHash<EventStoreType, quint32> > ::iterator tei = sess.m_timesummary.find(code);
        timeweight = (tei != sess.m_timesummary.end());
        gain = sess.m_gain[code];

        // Here's assuming gains don't change accross a days sessions
        // Can't assume this in any multi day calculations..
        if (lastgain > 0) {
            if (gain != lastgain) {
                qDebug() << "Gains differ across sessions: " << gain << lastgain;
            }
        }

        lastgain = gain;

        int value;
        qint64 weight;

        //qint64 tval;
        if (timeweight) {
            for (QHash<EventStoreType, quint32>::iterator i = tei.value().begin(); i != tei.value().end();
                    i++) {
                value = i.key();
                weight = i.value();
                SN += weight;
                wmap[value] += weight;
            }
        } else {
            for (QHash<EventStoreType, EventStoreType>::iterator i = ei.value().begin(); i != ei.value().end();
                    i++) {

                value = i.key();
                weight = i.value();

                SN += weight;

                wmap[value] += weight;
            }
        }
    }

    QVector<ValueCount> valcnt;

    // Build sorted list of value/counts
    for (QHash<EventStoreType, qint64>::iterator n = wmap.begin(); n != wmap.end(); n++) {
        ValueCount vc;
        vc.value = EventDataType(n.key()) * gain;
        vc.count = n.value();
        vc.p = 0;
        valcnt.push_back(vc);
    }

    // sort by weight, then value
    qSort(valcnt);

    //double SN=100.0/double(N); // 100% / overall sum
    double p = 100.0 * percentile;

    double nth = double(SN) * percentile; // index of the position in the unweighted set would be
    double nthi = floor(nth);

    qint64 sum1 = 0, sum2 = 0;
    qint64 w1, w2 = 0;
    double v1 = 0, v2;

    int N = valcnt.size();
    int k = 0;

    for (k = 0; k < N; k++) {
        v1 = valcnt[k].value;
        w1 = valcnt[k].count;
        sum1 += w1;

        if (sum1 > nthi) {
            return v1;
        }

        if (sum1 == nthi) {
            break; // boundary condition
        }
    }

    if (k >= N) {
        return v1;
    }

    v2 = valcnt[k + 1].value;
    w2 = valcnt[k + 1].count;
    sum2 = sum1 + w2;
    // value lies between v1 and v2

    double px = 100.0 / double(SN); // Percentile represented by one full value

    // calculate percentile ranks
    double p1 = px * (double(sum1) - (double(w1) / 2.0));
    double p2 = px * (double(sum2) - (double(w2) / 2.0));

    // calculate linear interpolation
    double v = v1 + ((p - p1) / (p2 - p1)) * (v2 - v1);

    return v;

    //                p1.....p.............p2
    //                37     55            70

}

EventDataType Day::p90(ChannelID code)
{
    return percentile(code, 0.90);
}

EventDataType Day::avg(ChannelID code)
{
    double val = 0;
    // Cache this?
    int cnt = 0;
    QList<Session *>::iterator s;

    // Don't assume sessions are in order.
    for (s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session &sess = *(*s);

        if (sess.m_avg.contains(code)) {
            val += sess.avg(code);
            cnt++;
        }
    }

    if (cnt == 0) { return 0; }

    return EventDataType(val / float(cnt));
}

EventDataType Day::sum(ChannelID code)
{
    // Cache this?
    EventDataType val = 0;
    QList<Session *>::iterator s;

    for (s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session &sess = *(*s);

        if (sess.m_sum.contains(code)) {
            val += sess.sum(code);
        }
    }

    return val;
}

EventDataType Day::wavg(ChannelID code)
{
    double s0 = 0, s1 = 0, s2 = 0;
    qint64 d;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session &sess = *(*s);

        if (sess.m_wavg.contains(code)) {
            d = sess.length(); //.last(code)-sess.first(code);
            s0 = double(d) / 3600000.0;

            if (s0 > 0) {
                s1 += sess.wavg(code) * s0;
                s2 += s0;
            }
        }
    }

    if (s2 == 0) {
        return 0;
    }

    return (s1 / s2);
}
// Total session time in milliseconds
qint64 Day::total_time()
{
    qint64 d_totaltime = 0;
    // Sessions may overlap.. :(
    QMultiMap<qint64, bool> range;

    //range.reserve(size()*2);

    for (QList<Session *>::iterator s = begin(); s != end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session &sess = *(*s);
        range.insert(sess.first(), 0);
        range.insert(sess.last(), 1);
        d_totaltime += sess.length();
    }

    qint64 ti = 0;
    bool b;
    int nest = 0;
    qint64 total = 0;

    // This is my implementation of a typical "brace counting" algorithm mentioned here:
    // http://stackoverflow.com/questions/7468948/problem-calculating-overlapping-date-ranges
    for (QMultiMap<qint64, bool>::iterator it = range.begin(); it != range.end(); it++) {
        b = it.value();

        if (!b) {
            if (!ti) { ti = it.key(); }

            nest++;
        } else {
            if (--nest <= 0) {
                total += it.key() - ti;
                ti = 0;
            }
        }
    }

    if (total != d_totaltime) {
        qDebug() << "Sessions Times overlaps!" << total << d_totaltime;
    }

    return total; //d_totaltime;
}
bool Day::hasEnabledSessions()
{
    bool b = false;

    for (QList<Session *>::iterator s = begin(); s != end(); s++) {
        if ((*s)->enabled()) {
            b = true;
            break;
        }
    }

    return b;
}

/*EventDataType Day::percentile(ChannelID code,double percent)
{
    double val=0;
    int cnt=0;

    for (QList<Session *>::iterator s=sessions.begin();s!=sessions.end();s++) {
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
    qint64 date = 0;
    qint64 tmp;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        tmp = (*s)->first(code);

        if (!tmp) { continue; }

        if (!date) {
            date = tmp;
        } else {
            if (tmp < date) { date = tmp; }
        }
    }

    return date;
}

qint64 Day::last(ChannelID code)
{
    qint64 date = 0;
    qint64 tmp;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        tmp = (*s)->last(code);

        if (!tmp) { continue; }

        if (!date) {
            date = tmp;
        } else {
            if (tmp > date) { date = tmp; }
        }
    }

    return date;
}

EventDataType Day::Min(ChannelID code)
{
    EventDataType min = 0;
    EventDataType tmp;
    bool first = true;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        if (!(*s)->m_min.contains(code)) {
            continue;
        }

        tmp = (*s)->Min(code);

        if (first) {
            min = tmp;
            first = false;
        } else {
            if (tmp < min) { min = tmp; }
        }
    }

    return min;
}

EventDataType Day::physMin(ChannelID code)
{
    EventDataType min = 0;
    EventDataType tmp;
    bool first = true;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        // MW: I meant to check this instead.
        if (!(*s)->m_min.contains(code)) {
            continue;
        }

        tmp = (*s)->physMin(code);

        if (first) {
            min = tmp;
            first = false;
        } else {
            if (tmp < min) { min = tmp; }
        }
    }

    return min;
}

bool Day::hasData(ChannelID code, SummaryType type)
{
    bool has = false;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        Session *sess = *s;

        switch (type) {
        //        case ST_90P:
        //            has=sess->m_90p.contains(code);
        //            break;
        case ST_PERC:
            has = sess->m_valuesummary.contains(code);
            break;

        case ST_MIN:
            has = sess->m_min.contains(code);
            break;

        case ST_MAX:
            has = sess->m_max.contains(code);
            break;

        case ST_CNT:
            has = sess->m_cnt.contains(code);
            break;

        case ST_AVG:
            has = sess->m_avg.contains(code);
            break;

        case ST_WAVG:
            has = sess->m_wavg.contains(code);
            break;

        case ST_CPH:
            has = sess->m_cph.contains(code);
            break;

        case ST_SPH:
            has = sess->m_sph.contains(code);
            break;

        case ST_FIRST:
            has = sess->m_firstchan.contains(code);
            break;

        case ST_LAST:
            has = sess->m_lastchan.contains(code);
            break;

        case ST_SUM:
            has = sess->m_sum.contains(code);
            break;

        default:
            break;
        }

        if (has) { break; }
    }

    return has;
}

EventDataType Day::Max(ChannelID code)
{
    EventDataType max = 0;
    EventDataType tmp;
    bool first = true;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        if (!(*s)->m_max.contains(code)) { continue; }

        //        if ((*s)->eventlist.find(code)==(*s)->eventlist.end()) continue;
        tmp = (*s)->Max(code);

        if (first) {
            max = tmp;
            first = false;
        } else {
            if (tmp > max) { max = tmp; }
        }
    }

    return max;
}

EventDataType Day::physMax(ChannelID code)
{
    EventDataType max = 0;
    EventDataType tmp;
    bool first = true;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) {
            continue;
        }

        // MW: I meant to check this instead.
        if (!(*s)->m_max.contains(code)) {
            continue;
        }

        tmp = (*s)->physMax(code);

        if (first) {
            max = tmp;
            first = false;
        } else {
            if (tmp > max) { max = tmp; }
        }
    }

    return max;
}
EventDataType Day::cph(ChannelID code)
{
    double sum = 0;

    //EventDataType h=0;
    for (int i = 0; i < sessions.size(); i++) {
        if (!sessions[i]->enabled()) { continue; }

        if (!sessions[i]->m_cnt.contains(code)) { continue; }

        sum += sessions[i]->count(code);
        //h+=sessions[i]->hours();
    }

    sum /= hours();
    return sum;
}

EventDataType Day::sph(ChannelID code)
{
    EventDataType sum = 0;
    EventDataType h = 0;

    for (int i = 0; i < sessions.size(); i++) {
        if (!sessions[i]->enabled()) { continue; }

        if (!sessions[i]->m_sum.contains(code)) { continue; }

        sum += sessions[i]->sum(code) / 3600.0; //*sessions[i]->hours();
        //h+=sessions[i]->hours();
    }

    h = hours();
    sum = (100.0 / h) * sum;
    return sum;
}

int Day::count(ChannelID code)
{
    int sum = 0;

    for (int i = 0; i < sessions.size(); i++) {
        if (!sessions[i]->enabled()) { continue; }

        sum += sessions[i]->count(code);
    }

    return sum;
}
bool Day::settingExists(ChannelID id)
{
    for (int j = 0; j < sessions.size(); j++) {
        if (!sessions[j]->enabled()) { continue; }

        QHash<ChannelID, QVariant>::iterator i = sessions[j]->settings.find(id);

        if (i != sessions[j]->settings.end()) {
            return true;
        }
    }

    return false;
}
bool Day::eventsLoaded()
{
    bool r = false;

    for (int i = 0; i < sessions.size(); i++) {
        if (sessions[i]->eventsLoaded()) {
            r = true;
            break;
        }
    }

    return r;
}

bool Day::channelExists(ChannelID id)
{
    bool r = false;

    for (int i = 0; i < sessions.size(); i++) {
        if (!sessions[i]->enabled()) { continue; }

        if (sessions[i]->eventlist.contains(id)) {
            r = true;
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
    bool r = false;

    for (int i = 0; i < sessions.size(); i++) {
        if (!sessions[i]->enabled()) { continue; }

        if (sessions[i]->channelExists(id)) {
            r = true;
            break;
        }

        if (sessions[i]->m_valuesummary.contains(id)) {
            r = true;
            break;
        }
    }

    return r;
}

void Day::OpenEvents()
{
    QList<Session *>::iterator s;

    for (s = sessions.begin(); s != sessions.end(); s++) {
        (*s)->OpenEvents();
    }
}
void Day::CloseEvents()
{
    QList<Session *>::iterator s;

    for (s = sessions.begin(); s != sessions.end(); s++) {
        (*s)->TrashEvents();
    }
}

qint64 Day::first()
{
    qint64 date = 0;
    qint64 tmp;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        tmp = (*s)->first();

        if (!tmp) { continue; }

        if (!date) {
            date = tmp;
        } else {
            if (tmp < date) { date = tmp; }
        }
    }

    return date;
    //    return d_first;
}

//! \brief Returns the last session time of this day
qint64 Day::last()
{
    qint64 date = 0;
    qint64 tmp;

    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); s++) {
        if (!(*s)->enabled()) { continue; }

        tmp = (*s)->last();

        if (!tmp) { continue; }

        if (!date) {
            date = tmp;
        } else {
            if (tmp > date) { date = tmp; }
        }
    }

    return date;
    //    return d_last;
}

void Day::removeSession(Session *sess)
{
    if (sessions.removeAll(sess) < 1) {
        //        int i=5;
    }
}
