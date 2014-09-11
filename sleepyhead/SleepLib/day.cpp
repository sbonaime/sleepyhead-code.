/* SleepLib Day Class Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QMultiMap>

#include <algorithm>
#include <cmath>
#include <limits>

#include "day.h"
#include "profiles.h"

Day::Day()
{
    d_firstsession = true;
    d_summaries_open = false;
    d_events_open = false;
    d_invalidate = true;

}
Day::~Day()
{
    for (QList<Session *>::iterator s = sessions.begin(); s != sessions.end(); ++s) {
        delete(*s);
    }
}

void Day::updateCPAPCache()
{
    d_count.clear();
    d_sum.clear();
    OpenSummary();
    QList<ChannelID> channels = getSortedMachineChannels(MT_CPAP, schema::FLAG | schema::MINOR_FLAG | schema::SPAN);

    int num_channels = channels.size();
    for (int i=0; i< num_channels; ++i) {
        ChannelID code = channels.at(i);
        d_count[code] = count(code);
        d_sum[code] = count(code);
        d_machhours[MT_CPAP] = hours(MT_CPAP);
    }
}

Session * Day::firstSession(MachineType type)
{
    for (int i=0; i<sessions.size(); i++) {
        Session * sess = sessions.at(i);
        if (!sess->enabled()) continue;
        if (sess->machine()->type() == type) {
            return sess;
        }
    }
    return nullptr;
}

bool Day::addMachine(Machine *mach)
{
    invalidate();
    if (!machines.contains(mach->type())) {
        machines[mach->type()] = mach;
        return true;
    }
    return false;
}
Machine *Day::machine(MachineType type)
{
    QHash<MachineType,Machine *>::iterator it = machines.find(type);
    if (it != machines.end())
        return it.value();
    return nullptr;
}

QList<Session *> Day::getSessions(MachineType type, bool ignore_enabled)
{
    QList<Session *>::iterator it;
    QList<Session *>::iterator sess_end = sessions.end();

    QList<Session *> newlist;

    for (it = sessions.begin(); it != sess_end; ++it) {
        if (!ignore_enabled && !(*it)->enabled())
            continue;

        if ((*it)->machine()->type() == type)
            newlist.append((*it));
    }

    return newlist;
}

Session *Day::find(SessionID sessid)
{
    QList<Session *>::iterator end=sessions.end();
    for (QList<Session *>::iterator s = sessions.begin(); s != end; ++s) {
        if ((*s)->session() == sessid) {
            return (*s);
        }
    }

    return nullptr;
}

void Day::addSession(Session *s)
{
    invalidate();
    Q_ASSERT(s!=nullptr);
    QHash<MachineType, Machine *>::iterator mi = machines.find(s->machine()->type());

    if (mi != machines.end()) {
        if (mi.value() != s->machine()) {
            qDebug() << "SleepyHead can't add session" << s->session() << "to this day record, as it already contains a different machine of the same MachineType";
            return;
        }
    } else {
        machines[s->machine()->type()] = s->machine();
    }

    sessions.push_back(s);
}
EventDataType Day::calcMiddle(ChannelID code)
{
    int c = p_profile->general->prefCalcMiddle();

    if (c == 0) {
        return percentile(code, 0.5); // Median
    } else if (c == 1 ) {
        return wavg(code); // Weighted Average
    } else {
        return avg(code); // Average
    }
}
EventDataType Day::calcMax(ChannelID code)
{
    return p_profile->general->prefCalcMax() ? percentile(code, 0.995) : Max(code);
}
EventDataType Day::calcPercentile(ChannelID code)
{
    double p = p_profile->general->prefCalcPercentile() / 100.0;
    return percentile(code, p);
}

QString Day::calcMiddleLabel(ChannelID code)
{
    int c = p_profile->general->prefCalcMiddle();
    if (c == 0) {
        return QObject::tr("%1 %2").arg(STR_TR_Median).arg(schema::channel[code].label());
    } else if (c == 1) {
        return QObject::tr("%1 %2").arg(STR_TR_Average).arg(schema::channel[code].label());
    } else {
        return QObject::tr("%1 %2").arg(STR_TR_Average).arg(schema::channel[code].label());
    }
}
QString Day::calcMaxLabel(ChannelID code)
{
    return QObject::tr("%1 %2").arg(p_profile->general->prefCalcMax() ? QObject::tr("Peak") : STR_TR_Max).arg(schema::channel[code].label());
}
QString Day::calcPercentileLabel(ChannelID code)
{
    return QObject::tr("%1% %2").arg(p_profile->general->prefCalcPercentile(),0, 'f',0).arg(schema::channel[code].label());
}

EventDataType Day::countInsideSpan(ChannelID span, ChannelID code)
{
    QList<Session *>::iterator end = sessions.end();
    int count = 0;
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            count += sess.countInsideSpan(span, code);
        }
    }
    return count;
}

EventDataType Day::lookupValue(ChannelID code, qint64 time, bool square)
{
    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            if ((time > sess.first()) && (time < sess.last())) {
                if (sess.channelExists(code)) {
                    return sess.SearchValue(code,time,square);
                }
            }
        }
    }
    return 0;
}


EventDataType Day::timeAboveThreshold(ChannelID code, EventDataType threshold)
{
    EventDataType val = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled() && sess.m_availableChannels.contains(code)) {
            val += sess.timeAboveThreshold(code,threshold);
        }
    }

    return val;
}

EventDataType Day::timeBelowThreshold(ChannelID code, EventDataType threshold)
{
    EventDataType val = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            val += sess.timeBelowThreshold(code,threshold);
        }
    }

    return val;
}


EventDataType Day::settings_sum(ChannelID code)
{
    EventDataType val = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            QHash<ChannelID, QVariant>::iterator set = sess.settings.find(code);

            if (set != sess.settings.end()) {
                val += set.value().toDouble();
            }
        }
    }

    return val;
}

EventDataType Day::settings_max(ChannelID code)
{
    EventDataType min = -std::numeric_limits<EventDataType>::max();
    EventDataType max = min;
    EventDataType value;

    QList<Session *>::iterator end = sessions.end();
    for(QList<Session *>::iterator it = sessions.begin(); it < end; ++it) {
        Session &sess = *(*it);
        if (sess.enabled()) {
            value = sess.settings.value(code, min).toFloat();
            if (value > max) {
                max = value;
            }
        }
    }

    return max;
}

EventDataType Day::settings_min(ChannelID code)
{
    EventDataType max = std::numeric_limits<EventDataType>::max();
    EventDataType min = max;
    EventDataType value;

    QList<Session *>::iterator end=sessions.end();

    for(QList<Session *>::iterator it = sessions.begin(); it < end; ++it) {
        Session &sess = *(*it);
        if (sess.enabled()) {
            value = sess.settings.value(code, max).toFloat();
            if (value < min) {
                min = value;
            }
        }
    }

    return min;
}

EventDataType Day::settings_avg(ChannelID code)
{
    EventDataType val = 0;
    int cnt = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; it++) {
        Session &sess = *(*it);
        if (sess.enabled()) {
            QHash<ChannelID, QVariant>::iterator set = sess.settings.find(code);

            if (set != sess.settings.end()) {
                val += set.value().toDouble();
                cnt++;
            }
        }
    }

    val = (cnt > 0) ? val /= EventDataType(cnt) : val;

    return val;
}

EventDataType Day::settings_wavg(ChannelID code)
{
    double s0 = 0, s1 = 0, s2 = 0, tmp;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; it++) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            QHash<ChannelID, QVariant>::iterator set = sess.settings.find(code);

            if (set != sess.settings.end()) {
                s0 = sess.hours();
                tmp = set.value().toDouble();
                s1 += tmp * s0;
                s2 += s0;
            }
        }
    }

    if (s2 == 0) { return 0; }

    tmp = (s1 / s2);
    return tmp;
}


EventDataType Day::percentile(ChannelID code, EventDataType percentile)
{
    // Cache this calculation?
    //    QHash<ChannelID, QHash<EventDataType, EventDataType> >::iterator pi;
    //    pi=perc_cache.find(code);
    //    if (pi!=perc_cache.end()) {
    //        QHash<EventDataType, EventDataType> & hsh=pi.value();
    //        QHash<EventDataType, EventDataType>::iterator hi=hsh.find(
    //        if (hi!=pi.value().end()) {
    //            return hi.value();
    //        }
    //    }

    QHash<EventStoreType, qint64> wmap; // weight map

    QHash<EventStoreType, qint64>::iterator wmapit;
    qint64 SN = 0;

    EventDataType lastgain = 0, gain = 0;
    // First Calculate count of all events
    bool timeweight;

    QList<Session *>::iterator sess_end = sessions.end();
    for (QList<Session *>::iterator sess_it = sessions.begin(); sess_it != sess_end; ++sess_it) {
        Session &sess = *(*sess_it);
        if (!sess.enabled()) { continue; }

        QHash<ChannelID, QHash<EventStoreType, EventStoreType> >::iterator ei = sess.m_valuesummary.find(code);

        if (ei == sess.m_valuesummary.end()) { continue; }

        QHash<ChannelID, QHash<EventStoreType, quint32> >::iterator tei = sess.m_timesummary.find(code);
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

        qint64 weight;

        //qint64 tval;
        if (timeweight) {
            QHash<EventStoreType, quint32>::iterator teival_end = tei.value().end();
            wmap.reserve(wmap.size() + tei.value().size());

            for (QHash<EventStoreType, quint32>::iterator it = tei.value().begin(); it != teival_end; ++it) {
                weight = it.value();
                SN += weight;

                wmap[it.key()] += weight;
            }
        } else {
            QHash<EventStoreType, EventStoreType>::iterator eival_end = ei.value().end();

            wmap.reserve(wmap.size() + ei.value().size());
            for (QHash<EventStoreType, EventStoreType>::iterator it = ei.value().begin(); it != eival_end; ++it) {
                weight = it.value();

                SN += weight;

                wmap[it.key()] += weight;
            }
        }
    }

    QVector<ValueCount> valcnt;

    valcnt.resize(wmap.size());
    // Build sorted list of value/counts

    QHash<EventStoreType, qint64>::iterator wmap_end = wmap.end();
    int ii=0;
    for (QHash<EventStoreType, qint64>::iterator it = wmap.begin(); it != wmap_end; ++it) {
        valcnt[ii++]=ValueCount(EventDataType(it.key()) * gain, it.value(), 0);
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
        v1 = valcnt.at(k).value;
        w1 = valcnt.at(k).count;
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

    if (valcnt.size() == 1) {
        return valcnt[0].value;
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
    return percentile(code, 0.90F);
}

EventDataType Day::rangeCount(ChannelID code, qint64 st, qint64 et)
{
    int cnt = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            cnt += sess.rangeCount(code, st, et);
        }
    }

    return cnt;
}
EventDataType Day::rangeSum(ChannelID code, qint64 st, qint64 et)
{
    double val = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            val += sess.rangeSum(code, st, et);
        }
    }

    return val;
}
EventDataType Day::rangeAvg(ChannelID code, qint64 st, qint64 et)
{
    double val = 0;
    int cnt = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            val += sess.rangeSum(code, st, et);
            cnt += sess.rangeCount(code, st,et);
        }
    }

    if (cnt == 0) { return 0; }
    val /= double(cnt);

    return val;
}
EventDataType Day::rangeWavg(ChannelID code, qint64 st, qint64 et)
{
    double sum = 0;
    double cnt = 0;
    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);
        QHash<ChannelID, QVector<EventList *> >::iterator EVEC = sess.eventlist.find(code);
        if (EVEC == sess.eventlist.end()) continue;

        QVector<EventList *>::iterator EL;
        QVector<EventList *>::iterator EVEC_end = EVEC.value().end();
        for (EL = EVEC.value().begin(); EL != EVEC_end; ++EL) {
            EventList * el = *EL;
            if (el->count() < 1) continue;
            EventDataType lastdata = el->data(0);
            qint64 lasttime = el->time(0);

            if (lasttime < st)
                lasttime = st;

            for (unsigned i=1; i<el->count(); i++)  {
                double data = el->data(i);
                qint64 time = el->time(i);

                if (time < st) {
                    lasttime = st;
                    lastdata = data;
                    continue;
                }

                if (time > et) {
                    time = et;
                }

                double duration = double(time - lasttime) / 1000.0;
                sum += data * duration;
                cnt += duration;

                if (time >= et) break;

                lasttime = time;
                lastdata = data;
            }
        }
    }
    if (cnt < 0.000001)
        return 0;
    return sum / cnt;
}


// Boring non weighted percentile
EventDataType Day::rangePercentile(ChannelID code, float p, qint64 st, qint64 et)
{
    int count = rangeCount(code, st,et);
    QVector<EventDataType> list;
    list.resize(count);
    int idx = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);
        QHash<ChannelID, QVector<EventList *> >::iterator EVEC = sess.eventlist.find(code);
        if (EVEC == sess.eventlist.end()) continue;

        QVector<EventList *>::iterator EL;
        QVector<EventList *>::iterator EVEC_end = EVEC.value().end();
        for (EL = EVEC.value().begin(); EL != EVEC_end; ++EL) {
            EventList * el = *EL;
            for (unsigned i=0; i<el->count(); i++)  {
                qint64 time = el->time(i);
                if ((time < st) || (time > et)) continue;
                list[idx++] = el->data(i);
            }
        }
    }

    // TODO: use nth_element instead..
    qSort(list);

    float b = float(idx) * p;
    int a = floor(b);
    int c = ceil(b);

    if ((a == c) || (c >= idx)) {
        return list[a];
    }

    EventDataType v1 = list[a];
    EventDataType v2 = list[c];

    EventDataType diff = v2 - v1;  // the whole               == C-A

    double ba = b - float(a);     // A....B...........C      == B-A

    double val = v1 + diff * ba;

    return val;
}

EventDataType Day::avg(ChannelID code)
{
    double val = 0;
    // Cache this?
    int cnt = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled()) {
            val += sess.sum(code);
            cnt += sess.count(code);
        }
    }

    if (cnt == 0) { return 0; }
    val /= double(cnt);

    return val;
}

EventDataType Day::sum(ChannelID code)
{
    // Cache this?
    EventDataType val = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled() && sess.m_sum.contains(code)) {
            val += sess.sum(code);
        }
    }

    return val;
}

EventDataType Day::wavg(ChannelID code)
{
    double s0 = 0, s1 = 0, s2 = 0;
    qint64 d;

    QList<Session *>::iterator end = sessions.end();

    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled() && sess.m_wavg.contains(code)) {
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
    QMultiMap<qint64, bool> range;
    //range.reserve(size()*2);

    // Remember sessions may overlap..

    qint64 first, last;
    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if (sess.enabled() && (sess.machine()->type() != MT_JOURNAL)) {
            first = sess.first();
            last = sess.last();

            // This algorithm relies on non zero length, and correctly ordered sessions
            if (last > first) {
                range.insert(first, 0);
                range.insert(last, 1);
                d_totaltime += sess.length();
            }
        }
    }

    bool b;
    int nest = 0;
    qint64 ti = 0;
    qint64 total = 0;

    // This is my implementation of a typical "brace counting" algorithm mentioned here:
    // http://stackoverflow.com/questions/7468948/problem-calculating-overlapping-date-ranges

    QMultiMap<qint64, bool>::iterator rend = range.end();
    for (QMultiMap<qint64, bool>::iterator rit = range.begin(); rit != rend; ++rit) {
        b = rit.value();

        if (!b) {
            if (!ti) {
                ti = rit.key();
            }

            nest++;
        } else {
            if (--nest <= 0) {
                total += rit.key() - ti;
                ti = 0;
            }
        }
    }

    if (total != d_totaltime) {
        // They can overlap.. tough.
//        qDebug() << "Sessions Times overlaps!" << total << d_totaltime;
    }

    return total; //d_totaltime;
}

// Total session time in milliseconds, only considering machinetype
qint64 Day::total_time(MachineType type)
{
    qint64 d_totaltime = 0;
    QMultiMap<qint64, bool> range;
    //range.reserve(size()*2);

    // Remember sessions may overlap..

    qint64 first, last;
    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session &sess = *(*it);

        if ((sess.machine()->type() == type) && sess.enabled()) {
            first = sess.first();
            last = sess.last();

            // This algorithm relies on non zero length, and correctly ordered sessions
            if (last > first) {
                range.insert(first, 0);
                range.insert(last, 1);
                d_totaltime += sess.length();
            }
        }
    }

    bool b;
    int nest = 0;
    qint64 ti = 0;
    qint64 total = 0;

    // This is my implementation of a typical "brace counting" algorithm mentioned here:
    // http://stackoverflow.com/questions/7468948/problem-calculating-overlapping-date-ranges

    QMultiMap<qint64, bool>::iterator rend = range.end();
    for (QMultiMap<qint64, bool>::iterator rit = range.begin(); rit != rend; ++rit) {
        b = rit.value();

        if (!b) {
            if (!ti) {
                ti = rit.key();
            }

            nest++;
        } else {
            if (--nest <= 0) {
                total += rit.key() - ti;
                ti = 0;
            }
        }
    }

    if (total != d_totaltime) {
        // They can overlap.. tough.
//        qDebug() << "Sessions Times overlaps!" << total << d_totaltime;
    }

    return total; //d_totaltime;
}


bool Day::hasEnabledSessions()
{
    QList<Session *>::iterator end = sessions.end();

    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        if ((*it)->enabled()) {
            return true;
        }
    }

    return false;
}

bool Day::hasEnabledSessions(MachineType type)
{
    QList<Session *>::iterator end = sessions.end();

    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        if (((*it)->machine()->type() == type) && (*it)->enabled()) {
            return true;
        }
    }

    return false;
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

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess=*(*it);

        if (sess.enabled()) {
            tmp = sess.first(code);

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp < date) { date = tmp; }
            }
        }
    }

    return date;
}

qint64 Day::last(ChannelID code)
{
    qint64 date = 0;
    qint64 tmp;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; it++) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            tmp = sess.last(code);

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp > date) { date = tmp; }
            }
        }
    }

    return date;
}

EventDataType Day::Min(ChannelID code)
{
    EventDataType min = 0;
    EventDataType tmp;
    bool first = true;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; it++) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_min.contains(code)) {

            tmp = sess.Min(code);

            if (first) {
                min = tmp;
                first = false;
            } else {
                if (tmp < min) { min = tmp; }
            }
        }
    }

    return min;
}

EventDataType Day::physMin(ChannelID code)
{
    EventDataType min = 0;
    EventDataType tmp;
    bool first = true;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_min.contains(code)) {

            tmp = sess.physMin(code);

            if (first) {
                min = tmp;
                first = false;
            } else {
                if (tmp < min) { min = tmp; }
            }
        }
    }

    return min;
}

bool Day::hasData(ChannelID code, SummaryType type)
{
    bool has = false;

    QList<Session *>::iterator end = sessions.end();

    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);
        if (sess.machine()->type() == MT_JOURNAL) continue;

        if (sess.enabled()) {
            switch (type) {
            //        case ST_90P:
            //            has=sess->m_90p.contains(code);
            //            break;
            case ST_PERC:
                has = sess.m_valuesummary.contains(code);
                break;

            case ST_MIN:
                has = sess.m_min.contains(code);
                break;

            case ST_MAX:
                has = sess.m_max.contains(code);
                break;

            case ST_CNT:
                has = sess.m_cnt.contains(code);
                break;

            case ST_AVG:
                has = sess.m_avg.contains(code);
                break;

            case ST_WAVG:
                has = sess.m_wavg.contains(code);
                break;

            case ST_CPH:
                has = sess.m_cph.contains(code);
                break;

            case ST_SPH:
                has = sess.m_sph.contains(code);
                break;

            case ST_FIRST:
                has = sess.m_firstchan.contains(code);
                break;

            case ST_LAST:
                has = sess.m_lastchan.contains(code);
                break;

            case ST_SUM:
                has = sess.m_sum.contains(code);
                break;

            default:
                break;
            }

            if (has) { break; }
        }
    }

    return has;
}

EventDataType Day::Max(ChannelID code)
{
    EventDataType max = 0;
    EventDataType tmp;
    bool first = true;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_max.contains(code)) {

            tmp = sess.Max(code);

            if (first) {
                max = tmp;
                first = false;
            } else {
                if (tmp > max) { max = tmp; }
            }
        }
    }

    return max;
}

EventDataType Day::physMax(ChannelID code)
{
    EventDataType max = 0;
    EventDataType tmp;
    bool first = true;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_max.contains(code)) {
            tmp = sess.physMax(code);

            if (first) {
                max = tmp;
                first = false;
            } else {
                if (tmp > max) { max = tmp; }
            }
        }
    }

    return max;
}
EventDataType Day::cph(ChannelID code)
{
    double sum = 0;

    //EventDataType h=0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_cnt.contains(code)) {
            sum += sess.count(code);
        }
    }

    sum /= hours();
    return sum;
}

EventDataType Day::sph(ChannelID code)
{
    EventDataType sum = 0;
    EventDataType h = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_sum.contains(code)) {
            sum += sess.sum(code) / 3600.0; //*sessions[i]->hours();
            //h+=sessions[i]->hours();
        }
    }

    h = hours();
    sum = (100.0 / h) * sum;
    return sum;
}

EventDataType Day::count(ChannelID code)
{
    EventDataType total = 0;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.m_cnt.contains(code)) {
            total += sess.count(code);
        }
    }

    return total;
}

bool Day::summaryOnly()
{
    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);
        if (sess.summaryOnly())
            return true;
    }
    return false;
}

bool Day::settingExists(ChannelID id)
{
    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            QHash<ChannelID, QVariant>::iterator set = sess.settings.find(id);

            if (set != sess.settings.end()) {
                return true;
            }
        }
    }

    return false;
}

bool Day::eventsLoaded()
{
    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.eventsLoaded()) {
            return true;
        }
    }

    return false;
}

bool Day::channelExists(ChannelID id)
{
    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled() && sess.eventlist.contains(id)) {
            return true;
        }
    }

    return false;
}
bool Day::hasEvents() {
    int s=sessions.size();
    for (int i=0; i<s; ++i) {
        if (sessions.at(i)->eventlist.size() > 0) return true;
    }
    return false;
}

bool Day::channelHasData(ChannelID id)
{
    QList<Session *>::iterator end = sessions.end();

    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if (sess.enabled()) {
            if (sess.channelExists(id)) {
                return true;
            }

            if (sess.m_valuesummary.contains(id)) {
                return true;
            }
            if (sess.m_cnt.contains(id)) {
                return true;
            }
        }
    }

    return false;
}

void Day::OpenEvents()
{
    if (d_events_open)
        return;
    Q_FOREACH(Session * session, sessions) {
        if (session->machine()->type() != MT_JOURNAL)
            session->OpenEvents();
    }
    d_events_open = true;
}

void Day::OpenSummary()
{
    if (d_summaries_open) return;
    Q_FOREACH(Session * session, sessions) {
        session->LoadSummary();
    }
    d_summaries_open = true;
}


void Day::CloseEvents()
{
    Q_FOREACH(Session * session, sessions) {
        session->TrashEvents();
    }
    d_events_open = false;
}

QList<ChannelID> Day::getSortedMachineChannels(MachineType type, quint32 chantype)
{
    QList<ChannelID> available;
    QHash<MachineType, Machine *>::iterator mi_end = machines.end();
    for (QHash<MachineType, Machine *>::iterator mi = machines.begin(); mi != mi_end; mi++) {
        if (mi.key() != type) continue;
        available.append(mi.value()->availableChannels(chantype));
    }

    QMultiMap<int, ChannelID> order;

    for (int i=0; i < available.size(); ++i) {
        ChannelID code = available.at(i);
        order.insert(schema::channel[code].order(), code);
    }

    QMultiMap<int, ChannelID>::iterator it;

    QList<ChannelID> channels;
    for (it = order.begin(); it != order.end(); ++it) {
        ChannelID code = it.value();
        channels.append(code);
    }
    return channels;
}


QList<ChannelID> Day::getSortedMachineChannels(quint32 chantype)
{
    QList<ChannelID> available;
    QHash<MachineType, Machine *>::iterator mi_end = machines.end();
    for (QHash<MachineType, Machine *>::iterator mi = machines.begin(); mi != mi_end; mi++) {
        if (mi.key() == MT_JOURNAL) continue;
        available.append(mi.value()->availableChannels(chantype));
    }

    QMultiMap<int, ChannelID> order;

    for (int i=0; i < available.size(); ++i) {
        ChannelID code = available.at(i);
        order.insert(schema::channel[code].order(), code);
    }

    QMultiMap<int, ChannelID>::iterator it;

    QList<ChannelID> channels;
    for (it = order.begin(); it != order.end(); ++it) {
        ChannelID code = it.value();
        channels.append(code);
    }
    return channels;
}

qint64 Day::first(MachineType type)
{
    qint64 date = 0;
    qint64 tmp;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if ((sess.machine()->type() == type) && sess.enabled()) {
            tmp = sess.first();

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp < date) { date = tmp; }
            }
        }
    }

    return date;
}

qint64 Day::first()
{
    qint64 date = 0;
    qint64 tmp;

    QList<Session *>::iterator end = sessions.end();
    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);
        if (sess.machine()->type() == MT_JOURNAL) continue;
        if (sess.enabled()) {
            tmp = sess.first();

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp < date) { date = tmp; }
            }
        }
    }

    return date;
}

//! \brief Returns the last session time of this day
qint64 Day::last()
{
    qint64 date = 0;
    qint64 tmp;

    QList<Session *>::iterator end = sessions.end();

    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);
        if (sess.machine()->type() == MT_JOURNAL) continue;

        if (sess.enabled()) {
            tmp = sess.last();

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp > date) { date = tmp; }
            }
        }
    }

    return date;
}

qint64 Day::last(MachineType type)
{
    qint64 date = 0;
    qint64 tmp;

    QList<Session *>::iterator end = sessions.end();


    for (QList<Session *>::iterator it = sessions.begin(); it != end; ++it) {
        Session & sess = *(*it);

        if ((sess.machine()->type() == type) && sess.enabled()) {
            tmp = sess.last();

            if (!tmp) { continue; }

            if (!date) {
                date = tmp;
            } else {
                if (tmp > date) { date = tmp; }
            }
        }
    }

    return date;
}

bool Day::removeSession(Session *sess)
{
    return sessions.removeAll(sess) > 0;
}

QString Day::getCPAPMode()
{
    Machine * mach = machine(MT_CPAP);
    if (!mach) return STR_MessageBox_Error;

    CPAPLoader * loader = qobject_cast<CPAPLoader *>(mach->loader());

    ChannelID modechan = loader->CPAPModeChannel();

    schema::Channel & chan = schema::channel[modechan];

    int mode = (CPAPMode)(int)qRound(settings_wavg(modechan));

    return chan.option(mode);


//    if (mode == MODE_CPAP) {
//        return QObject::tr("Fixed");
//    } else if (mode == MODE_APAP) {
//        return QObject::tr("Auto");
//    } else if (mode == MODE_BILEVEL_FIXED ) {
//        return QObject::tr("Fixed Bi-Level");
//    } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
//        return QObject::tr("Auto Bi-Level (Fixed PS)");
//    } else if (mode == MODE_BILEVEL_AUTO_VARIABLE_PS) {
//        return QObject::tr("Auto Bi-Level (Variable PS)");
//    }  else if (mode == MODE_ASV) {
//        return QObject::tr("ASV Fixed EPAP");
//    } else if (mode == MODE_ASV_VARIABLE_EPAP) {
//        return QObject::tr("ASV Variable EPAP");
//    }
//    return STR_TR_Unknown;
}

QString Day::getPressureRelief()
{
    Machine * mach = machine(MT_CPAP);
    if (!mach) return STR_MessageBox_Error;

    CPAPLoader * loader = qobject_cast<CPAPLoader *>(mach->loader());

    if (!loader) return STR_MessageBox_Error;

    QString pr_str;

    ChannelID pr_level_chan = loader->PresReliefLevel();
    ChannelID pr_mode_chan = loader->PresReliefMode();

    if ((pr_mode_chan != NoChannel) && settingExists(pr_mode_chan)) {
        int pr_mode = qRound(settings_wavg(pr_mode_chan));
        pr_str = QObject::tr("%1%2").arg(loader->PresReliefLabel()).arg(schema::channel[pr_mode_chan].option(pr_mode));

        int pr_level = -1;
        if (pr_level_chan != NoChannel && settingExists(pr_level_chan)) {
            pr_level = qRound(settings_wavg(pr_level_chan));
        }
        if (pr_level >= 0) pr_str += QString(" %1").arg(schema::channel[pr_level_chan].option(pr_level));
    } else pr_str = STR_TR_None;
    return pr_str;
}


QString Day::getPressureSettings()
{
    Q_ASSERT(machine(MT_CPAP) != nullptr);

    CPAPMode mode = (CPAPMode)(int)settings_max(CPAP_Mode);
    QString units = schema::channel[CPAP_Pressure].units();

    if (mode == MODE_CPAP) {
        return QObject::tr("Fixed %1 (%2)").arg(settings_min(CPAP_Pressure)).arg(units);
    } else if (mode == MODE_APAP) {
        return QObject::tr("Min %1 Max %2 (%3)").arg(settings_min(CPAP_PressureMin)).arg(settings_max(CPAP_PressureMax)).arg(units);
    } else if (mode == MODE_BILEVEL_FIXED ) {
        return QObject::tr("EPAP %1 IPAP %2 (%3)").arg(settings_min(CPAP_EPAP),0,'f',1).arg(settings_max(CPAP_IPAP),0,'f',1).arg(units);
    } else if (mode == MODE_BILEVEL_AUTO_FIXED_PS) {
        return QObject::tr("PS %1 over %2-%3 (%4)").arg(settings_max(CPAP_PS),0,'f',1).arg(settings_min(CPAP_EPAPLo),0,'f',1).arg(settings_max(CPAP_IPAPHi),0,'f',1).arg(units);
    } else if (mode == MODE_BILEVEL_AUTO_VARIABLE_PS) {
        return QObject::tr("Min EPAP %1 Max IPAP %2 PS %3-%4 (%5)").arg(settings_min(CPAP_EPAPLo),0,'f',1).arg(settings_max(CPAP_IPAPHi),0,'f',1).arg(settings_min(CPAP_PSMin),0,'f',1).arg(settings_max(CPAP_PSMax),0,'f',1).arg(units);
    } else if (mode == MODE_ASV) {
        return QObject::tr("EPAP %1 PS %2-%3 (%6)").arg(settings_min(CPAP_EPAP),0,'f',1).arg(settings_min(CPAP_PSMin),0,'f',1).arg(settings_max(CPAP_PSMax),0,'f',1).arg(units);
    } else if (mode == MODE_ASV_VARIABLE_EPAP) {
        return QObject::tr("Min EPAP %1 Max IPAP %2 PS %3-%4 (%5)").
                arg(settings_min(CPAP_EPAPLo),0,'f',1).
                arg(settings_max(CPAP_IPAPHi),0,'f',1).
                arg(settings_max(CPAP_PSMin),0,'f',1).
                arg(settings_min(CPAP_PSMax),0,'f',1).
                arg(units);
    }

    return STR_TR_Unknown;
}


EventDataType Day::calc(ChannelID code, ChannelCalcType type)
{
    EventDataType value;

    switch(type) {
    case Calc_Min:
        value = Min(code);
        break;
    case Calc_Middle:
        value = calcMiddle(code);
        break;
    case Calc_Perc:
        value = calcPercentile(code);
        break;
    case Calc_Max:
        value = calcMax(code);
        break;
    case Calc_UpperThresh:
        value = schema::channel[code].upperThreshold();
        break;
    case Calc_LowerThresh:
        value = schema::channel[code].lowerThreshold();
        break;
    case Calc_Zero:
    default:
        value = 0;
        break;
    };
    return value;
}
