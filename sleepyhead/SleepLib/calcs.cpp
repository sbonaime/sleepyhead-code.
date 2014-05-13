/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Custom CPAP/Oximetry Calculations Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QMutex>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <cmath>
#include <algorithm>

#include "calcs.h"
#include "profiles.h"

extern double round(double number);

bool SearchApnea(Session *session, qint64 time, qint64 dist)
{
    if (session->SearchEvent(CPAP_Obstructive, time, dist))
        return true;

    if (session->SearchEvent(CPAP_Apnea, time, dist))
        return true;

    if (session->SearchEvent(CPAP_ClearAirway, time, dist))
        return true;

    if (session->SearchEvent(CPAP_Hypopnea, time, dist))
        return true;

    return false;
}

// Sort BreathPeak by peak index
bool operator<(const BreathPeak &p1, const BreathPeak &p2)
{
    return p1.start < p2.start;
}

//! \brief Filters input to output with a percentile filter with supplied width.
//! \param samples Number of samples
//! \param width number of surrounding samples to consider
//! \param percentile fractional percentage, between 0 and 1
void percentileFilter(EventDataType *input, EventDataType *output, int samples, int width,
                      EventDataType percentile)
{
    if (samples <= 0) {
        return;
    }

    if (percentile > 1) {
        percentile = 1;
    }

    QVector<EventDataType> buf(width);
    int s, e;
    int z1 = width / 2;
    int z2 = z1 + (width % 2);
    int nm1 = samples - 1;
    //int j;

    // Scan through all of input
    for (int k = 0; k < samples; k++) {

        s = k - z1;
        e = k + z2;

        // Cap bounds
        if (s < 0) { s = 0; }

        if (e > nm1) { e = nm1; }

        //
        int j = 0;

        for (int i = s; i < e; i++) {
            buf[j++] = input[i];
        }

        j--;

        EventDataType val = j * percentile;
        EventDataType fl = floor(val);

        // If even percentile, or already max value..
        if ((val == fl) || (j >= width - 1)) {
            nth_element(buf.begin(), buf.begin() + j, buf.begin() + width - 1);
            val = buf[j];
        } else {
            // Percentile lies between two points, interpolate.
            double v1, v2;
            nth_element(buf.begin(), buf.begin() + j, buf.begin() + width - 1);
            v1 = buf[j];
            nth_element(buf.begin(), buf.begin() + j + 1, buf.begin() + width - 1);
            v2 = buf[j + 1];

            val = v1 + (v2 - v1) * (val - fl);
        }

        output[k] = val;
    }
}

void xpassFilter(EventDataType *input, EventDataType *output, int samples, EventDataType weight)
{
    // prime the first value
    output[0] = input[0];

    for (int i = 1; i < samples; i++) {
        output[i] = weight * input[i] + (1.0 - weight) * output[i - 1];
    }

    //output[samples-1]=input[samples-1];
}

FlowParser::FlowParser()
{
    m_session = nullptr;
    m_flow = nullptr;
    m_filtered = nullptr;
    m_gain = 1;
    m_samples = 0;
    m_startsUpper = true;

    // Allocate filter chain buffers..
    m_filtered = (EventDataType *) malloc(max_filter_buf_size);
}
FlowParser::~FlowParser()
{
    free(m_filtered);
    //    for (int i=0;i<num_filter_buffers;i++) {
    //        free(m_buffers[i]);
    //    }
}
void FlowParser::clearFilters()
{
    m_filters.clear();
}

EventDataType *FlowParser::applyFilters(EventDataType *data, int samples)
{
    EventDataType *in = nullptr, *out = nullptr;

    if (m_filters.size() == 0) {
        //qDebug() << "Trying to apply empty filter list in FlowParser..";
        return nullptr;
    }

    for (int i = 0; i < num_filter_buffers; i++) {
        m_buffers[i] = (EventDataType *) malloc(max_filter_buf_size);
    }

    int numfilt = m_filters.size();

    for (int i = 0; i < numfilt; i++) {
        if (i == 0) {
            in = data;
            out = m_buffers[0];

            if (in == out) {
                //qDebug() << "Error: If you need to use internal m_buffers as initial input, use the second one. No filters were applied";
                return nullptr;
            }
        } else {
            in = m_buffers[(i + 1) % num_filter_buffers];
            out = m_buffers[i % num_filter_buffers];
        }

        // If final link in chain, pass it back out to input data
        if (i == numfilt - 1) {
            out = data;
        }

        Filter &filter = m_filters[i];

        if (filter.type == FilterNone) {
            // Just copy it..
            memcpy(in, out, samples * sizeof(EventDataType));
        } else if (filter.type == FilterPercentile) {
            percentileFilter(in, out, samples, filter.param1, filter.param2);
        } else if (filter.type == FilterXPass) {
            xpassFilter(in, out, samples, filter.param1);
        }
    }

    for (int i = 0; i < num_filter_buffers; i++) {
        free(m_buffers[i]);
    }

    return out;
}
void FlowParser::openFlow(Session *session, EventList *flow)
{
    if (!flow) {
        qDebug() << "called FlowParser::processFlow() with a null EventList!";
        return;
    }

    m_session = session;
    m_flow = flow;
    m_gain = flow->gain();
    m_rate = flow->rate();
    m_samples = flow->count();
    EventStoreType *inraw = flow->rawData();

    // Make sure we won't overflow internal buffers
    if (m_samples > max_filter_buf_size) {
        qDebug() <<
                 "Error: Sample size exceeds max_filter_buf_size in FlowParser::openFlow().. Capping!!!";
        m_samples = max_filter_buf_size;
    }

    // Begin with the second internal buffer
    EventDataType *buf = m_filtered;
    // Apply gain to waveform
    EventStoreType *eptr = inraw + m_samples;

    // Convert from store type to floats..
    for (; inraw < eptr; inraw++) {
        *buf++ = EventDataType(*inraw) * m_gain;
    }

    // Apply the rest of the filters chain
    buf = applyFilters(m_filtered, m_samples);

    calcPeaks(m_filtered, m_samples);
}

// Calculates breath upper & lower peaks for a chunk of EventList data
void FlowParser::calcPeaks(EventDataType *input, int samples)
{
    if (samples <= 0) {
        return;
    }

    EventDataType min = 0, max = 0, c, lastc = 0;

    EventDataType zeroline = 0;

    double rate = m_flow->rate();

    double flowstart = m_flow->first();
    double lasttime, time;

    double peakmax = flowstart, peakmin = flowstart;

    time = lasttime = flowstart;
    breaths.clear();

    // Estimate storage space needed using typical average breaths per minute.
    m_minutes = double(m_flow->last() - m_flow->first()) / 60000.0;
    const double avgbpm = 20;
    int guestimate = m_minutes * avgbpm;
    breaths.reserve(guestimate);

    // Prime min & max, and see which side of the zero line we are starting from.
    c = input[0];
    min = max = c;
    lastc = c;
    m_startsUpper = (c >= zeroline);

    qint32 start = 0, middle = 0; //,end=0;


    int sps = 1000 / m_rate;
    int len = 0, lastk = 0; //lastlen=0

    qint64 sttime = time; //, ettime=time;

    // For each samples, find the peak upper and lower breath components
    for (int k = 0; k < samples; k++) {
        c = input[k];

        if (c >= zeroline) {

            // Did we just cross the zero line going up?
            if (lastc < zeroline) {
                // This helps filter out dirty breaths..
                len = k - start;

                if ((max > 3) && ((max - min) > 8) && (len > sps) && (middle > start))  {

                    // peak detection may not be needed..
                    breaths.push_back(BreathPeak(min, max, start, middle, k)); //, peakmin, peakmax));

                    // Set max for start of the upper breath cycle
                    max = c;
                    peakmax = time;


                    // Starting point of next breath cycle
                    start = k;
                    sttime = time;
                }
            } else if (c > max) {
                // Update upper breath peak
                max = c;
                peakmax = time;
            }
        }

        if (c < zeroline) {
            // Did we just cross the zero line going down?

            if (lastc >= zeroline) {
                // Set min for start of the lower breath cycle
                min = c;
                peakmin = time;
                middle = k;

            } else if (c < min) {
                // Update lower breath peak
                min = c;
                peakmin = time;
            }

        }

        lasttime = time;
        time += rate;
        lastc = c;
        lastk = k;
    }
}


//! \brief Calculate Respiratory Rate, TidalVolume, Minute Ventilation, Ti & Te..
// These are grouped together because, a) it's faster, and b) some of these calculations rely on others.
void FlowParser::calc(bool calcResp, bool calcTv, bool calcTi, bool calcTe, bool calcMv)
{
    if (!m_session) {
        return;
    }

    // Don't even bother if only a few breaths in this chunk
    const int lowthresh = 4;
    int nm = breaths.size();

    if (nm < lowthresh) {
        return;
    }

    const qint64 minute = 60000;


    double start = m_flow->first();
    double time = start;

    int bs, be, bm;
    double st, et, mt;

    /////////////////////////////////////////////////////////////////////////////////
    // Respiratory Rate setup
    /////////////////////////////////////////////////////////////////////////////////
    EventDataType minrr = 0, maxrr = 0;
    EventList *RR = nullptr;
    quint32 *rr_tptr = nullptr;
    EventStoreType *rr_dptr = nullptr;

    if (calcResp) {
        RR = m_session->AddEventList(CPAP_RespRate, EVL_Event);
        minrr = RR->Min(), maxrr = RR->Max();
        RR->setGain(0.2F);
        RR->setFirst(time + minute);
        RR->getData().resize(nm);
        RR->getTime().resize(nm);
        rr_tptr = RR->rawTime();
        rr_dptr = RR->rawData();
    }

    int rr_count = 0;

    double len, st2, et2, adj, stmin, b, rr = 0;
    double len2;
    int idx;



    /////////////////////////////////////////////////////////////////////////////////
    // Inspiratory / Expiratory Time setup
    /////////////////////////////////////////////////////////////////////////////////
    double lastte2 = 0, lastti2 = 0, lastte = 0, lastti = 0, te, ti, ti1, te1, c;
    EventList *Te = nullptr, * Ti = nullptr;

    if (calcTi) {
        Ti = m_session->AddEventList(CPAP_Ti, EVL_Event);
        Ti->setGain(0.02F);
    }

    if (calcTe) {
        Te = m_session->AddEventList(CPAP_Te, EVL_Event);
        Te->setGain(0.02F);
    }


    /////////////////////////////////////////////////////////////////////////////////
    // Tidal Volume setup
    /////////////////////////////////////////////////////////////////////////////////
    EventList *TV = nullptr;
    EventDataType mintv = 0, maxtv = 0, tv = 0;
    double val1, val2;
    quint32 *tv_tptr = nullptr;
    EventStoreType *tv_dptr = nullptr;
    int tv_count = 0;

    if (calcTv) {
        TV = m_session->AddEventList(CPAP_TidalVolume, EVL_Event);
        mintv = TV->Min(), maxtv = TV->Max();
        TV->setGain(20);
        TV->setFirst(start);
        TV->getData().resize(nm);
        TV->getTime().resize(nm);
        tv_tptr = TV->rawTime();
        tv_dptr = TV->rawData();
    }

    /////////////////////////////////////////////////////////////////////////////////
    // Minute Ventilation setup
    /////////////////////////////////////////////////////////////////////////////////
    EventList *MV = nullptr;
    EventDataType mv;

    if (calcMv) {
        MV = m_session->AddEventList(CPAP_MinuteVent, EVL_Event);
        MV->setGain(0.125);
    }

    EventDataType sps = (1000.0 / m_rate); // Samples Per Second


    qint32 timeval = 0; // Time relative to start

    for (idx = 0; idx < nm; idx++) {
        bs = breaths[idx].start;
        bm = breaths[idx].middle;
        be = breaths[idx].end;

        // Calculate start, middle and end time of this breath
        st = start + bs * m_rate;
        mt = start + bm * m_rate;

        timeval = be * m_rate;

        et = start + timeval;



        /////////////////////////////////////////////////////////////////////
        // Calculate Inspiratory Time (Ti) for this breath
        /////////////////////////////////////////////////////////////////////
        if (calcTi) {
            ti = ((mt - st) / 1000.0) * 50.0;
            ti1 = (lastti2 + lastti + ti) / 3.0;
            Ti->AddEvent(mt, ti1);
            lastti2 = lastti;
            lastti = ti;
        }

        /////////////////////////////////////////////////////////////////////
        // Calculate Expiratory Time (Te) for this breath
        /////////////////////////////////////////////////////////////////////
        if (calcTe) {
            te = ((et - mt) / 1000.0) * 50.0;
            // Average last three values..
            te1 = (lastte2 + lastte + te) / 3.0;
            Te->AddEvent(mt, te1);

            lastte2 = lastte;
            lastte = te;
        }

        /////////////////////////////////////////////////////////////////////
        // Calculate TidalVolume
        /////////////////////////////////////////////////////////////////////
        if (calcTv) {
            val1 = 0, val2 = 0;

            // Scan the upper breath
            for (int j = bs; j < bm; j++)  {
                // convert flow to ml/s to L/min and divide by samples per second
                c = double(qAbs(m_filtered[j])) * 1000.0 / 60.0 / sps;
                val2 += c;
                //val2+=c*c; // for RMS
            }

            // calculate root mean square
            //double n=bm-bs;
            //double q=(1/n)*val2;
            //double x=sqrt(q)*2;
            //val2=x;
            tv = val2;

            if (tv < mintv) { mintv = tv; }

            if (tv > maxtv) { maxtv = tv; }

            *tv_tptr++ = timeval;
            *tv_dptr++ = tv / 20.0;
            tv_count++;

        }

        /////////////////////////////////////////////////////////////////////
        // Respiratory Rate Calculations
        /////////////////////////////////////////////////////////////////////
        if (calcResp) {
            stmin = et - minute;

            if (stmin < start) {
                stmin = start;
            }

            len = et - stmin;
            rr = 0;
            len2 = 0;
            //if ( len >= minute) {
            //et2=et;

            // Step back through last minute and count breaths
            for (int i = idx; i >= 0; i--) {
                st2 = start + double(breaths[i].start) * m_rate;
                et2 = start + double(breaths[i].end) * m_rate;

                if (et2 < stmin) {
                    break;
                }

                len = et2 - st2;

                if (st2 < stmin) {
                    // Partial breath
                    st2 = stmin;
                    adj = et2 - st2;
                    b = (1.0 / len) * adj;
                    len2 += adj;
                } else {
                    b = 1;
                    len2 += len;
                }

                rr += b;
            }

            if (len2 < minute) {
                rr *= minute / len2;
            }

            // Calculate min & max
            if (rr < minrr) {
                minrr = rr;
            }

            if (rr > maxrr) {
                maxrr = rr;
            }

            // Add manually.. (much quicker)
            *rr_tptr++ = timeval;

            // Use the same gains as ResMed..

            *rr_dptr++ = rr * 5.0;
            rr_count++;

            //rr->AddEvent(et,br * 50.0);
            //}
        }

        if (calcMv && calcResp && calcTv) {
            mv = (tv / 1000.0) * rr;
            MV->AddEvent(et, mv * 8.0);
        }
    }

    /////////////////////////////////////////////////////////////////////
    // Respiratory Rate post filtering
    /////////////////////////////////////////////////////////////////////

    if (calcResp) {
        RR->setMin(minrr);
        RR->setMax(maxrr);
        RR->setFirst(start);
        RR->setLast(et);
        RR->setCount(rr_count);
    }

    /////////////////////////////////////////////////////////////////////
    // Tidal Volume post filtering
    /////////////////////////////////////////////////////////////////////

    if (calcTv) {
        TV->setMin(mintv);
        TV->setMax(maxtv);
        TV->setFirst(start);
        TV->setLast(et);
        TV->setCount(tv_count);
    }
}

void FlowParser::flagEvents()
{
    if (!PROFILE.cpap->userEventFlagging()) { return; }

    int numbreaths = breaths.size();

    if (numbreaths < 5) { return; }

    EventDataType val, mx, mn;
    QVector<EventDataType> br;

    QVector<qint32> bstart;
    QVector<qint32> bend;
    //QVector<EventDataType> bvalue;

    bstart.reserve(numbreaths * 2);
    bend.reserve(numbreaths * 2);
    //bvalue.reserve(numbreaths*2);
    br.reserve(numbreaths * 2);

    double start = m_flow->first();
    // double sps=1000.0/m_rate;
    double st, et, dur; //mt
    qint64 len;

    bool allowDuplicates = PROFILE.cpap->userEventDuplicates();

    for (int i = 0; i < numbreaths; i++) {
        mx = breaths[i].max;
        mn = breaths[i].min;
        br.push_back(qAbs(mx));
        br.push_back(qAbs(mn));
    }

    //EventList * uf2=m_session->AddEventList(CPAP_UserFlag2,EVL_Event);
    //EventList * uf3=m_session->AddEventList(CPAP_UserFlag3,EVL_Event);

    const EventDataType perc = 0.6F;
    int idx = float(br.size()) * perc;
    nth_element(br.begin(), br.begin() + idx, br.end() - 1);

    EventDataType peak = br[idx]; //*(br.begin()+idx);

    EventDataType cutoffval = peak * (PROFILE.cpap->userFlowRestriction() / 100.0F);

    int bs, bm, be, bs1, bm1, be1;

    for (int i = 0; i < numbreaths; i++) {
        bs = breaths[i].start;
        bm = breaths[i].middle;
        be = breaths[i].end;

        mx = breaths[i].max;
        mn = breaths[i].min;
        val = mx - mn;

        //        if (qAbs(mx) > cutoffval) {
        bs1 = bs;

        for (; bs1 < be; bs1++) {
            if (qAbs(m_filtered[bs1]) > cutoffval) {
                break;
            }
        }

        bm1 = bm;

        for (; bm1 > bs; bm1--) {
            if (qAbs(m_filtered[bm1]) > cutoffval) {
                break;
            }
        }

        if (bm1 >= bs1) {
            bstart.push_back(bs1);
            bend.push_back(bm1);
        }

        //      }
        //    if (qAbs(mn) > cutoffval) {
        bm1 = bm;

        for (; bm1 < be; bm1++) {
            if (qAbs(m_filtered[bm1]) > cutoffval) {
                break;
            }
        }

        be1 = be;

        for (; be1 > bm; be1--) {
            if (qAbs(m_filtered[be1]) > cutoffval) {
                break;
            }
        }

        if (be1 >= bm1) {
            bstart.push_back(bm1);
            bend.push_back(be1);
        }

        //        }
        st = start + bs1 * m_rate;
        et = start + be1  * m_rate;
        //         uf2->AddEvent(st,0);
        //         uf3->AddEvent(et,0);

    }


    EventDataType duration = PROFILE.cpap->userEventDuration();
    //double lastst=start, lastet=start;
    //EventDataType v;
    int bsize = bstart.size();
    EventList *uf1 = nullptr;

    for (int i = 0; i < bsize - 1; i++) {
        bs = bend[i];
        be = bstart[i + 1];
        st = start + bs * m_rate;
        et = start + be  * m_rate;

        len = et - st;
        dur = len / 1000.0;

        if (dur >= duration) {
            if (allowDuplicates || !SearchApnea(m_session, et - len / 2, 15000)) {
                if (!uf1) {
                    uf1 = m_session->AddEventList(CPAP_UserFlag1, EVL_Event);
                }

                uf1->AddEvent(et - len / 2, dur);
            }
        }
    }
}

void calcRespRate(Session *session, FlowParser *flowparser)
{
    if (session->machine()->GetType() != MT_CPAP) { return; }

    //    if (session->machine()->GetClass()!=STR_MACH_PRS1) return;

    if (!session->eventlist.contains(CPAP_FlowRate)) {
        //qDebug() << "calcRespRate called without FlowRate waveform available";
        return; //need flow waveform
    }

    bool trashfp;

    if (!flowparser) {
        flowparser = new FlowParser();
        trashfp = true;
        //qDebug() << "calcRespRate called without valid FlowParser object.. using a slow throw-away!";
        //return;
    } else {
        trashfp = false;
    }

    bool calcResp = !session->eventlist.contains(CPAP_RespRate);
    bool calcTv = !session->eventlist.contains(CPAP_TidalVolume);
    bool calcTi = !session->eventlist.contains(CPAP_Ti);
    bool calcTe = !session->eventlist.contains(CPAP_Te);
    bool calcMv = !session->eventlist.contains(CPAP_MinuteVent);


    int z = (calcResp ? 1 : 0) + (calcTv ? 1 : 0) + (calcMv ? 1 : 0);

    // If any of these three missing, remove all, and switch all on
    if (z > 0 && z < 3) {
        if (!calcResp && !calcTv && !calcMv) {
            calcTv = calcMv = calcResp = true;
        }

        QVector<EventList *> &list = session->eventlist[CPAP_RespRate];

        for (int i = 0; i < list.size(); i++) {
            delete list[i];
        }

        session->eventlist[CPAP_RespRate].clear();

        QVector<EventList *> &list2 = session->eventlist[CPAP_TidalVolume];

        for (int i = 0; i < list2.size(); i++) {
            delete list2[i];
        }

        session->eventlist[CPAP_TidalVolume].clear();

        QVector<EventList *> &list3 = session->eventlist[CPAP_MinuteVent];

        for (int i = 0; i < list3.size(); i++) {
            delete list3[i];
        }

        session->eventlist[CPAP_MinuteVent].clear();
    }

    flowparser->clearFilters();

    // No filters works rather well with the new peak detection algorithm..
    // Although the output could use filtering.

    //flowparser->addFilter(FilterPercentile,7,0.5);
    //flowparser->addFilter(FilterPercentile,5,0.5);
    //flowparser->addFilter(FilterXPass,0.5);
    EventList *flow;

    for (int ws = 0; ws < session->eventlist[CPAP_FlowRate].size(); ws++) {
        flow = session->eventlist[CPAP_FlowRate][ws];

        if (flow->count() > 20) {
            flowparser->openFlow(session, flow);
            flowparser->calc(calcResp, calcTv, calcTi , calcTe, calcMv);
            flowparser->flagEvents();
        }
    }

    if (trashfp) {
        delete flowparser;
    }
}


EventDataType calcAHI(Session *session, qint64 start, qint64 end)
{
    bool rdi = PROFILE.general->calculateRDI();

    double hours, ahi, cnt;

    if (start < 0) {
        // much faster..
        hours = session->hours();
        cnt = session->count(CPAP_Obstructive)
              + session->count(CPAP_Hypopnea)
              + session->count(CPAP_ClearAirway)
              + session->count(CPAP_Apnea);

        if (rdi) {
            cnt += session->count(CPAP_RERA);
        }

        ahi = cnt / hours;
    } else {
        hours = double(end - start) / 3600000L;

        if (hours == 0) { return 0; }

        cnt = session->rangeCount(CPAP_Obstructive, start, end)
              + session->rangeCount(CPAP_Hypopnea, start, end)
              + session->rangeCount(CPAP_ClearAirway, start, end)
              + session->rangeCount(CPAP_Apnea, start, end);

        if (rdi) {
            cnt += session->rangeCount(CPAP_RERA, start, end);
        }

        ahi = cnt / hours;
    }

    return ahi;
}

int calcAHIGraph(Session *session)
{
    bool calcrdi = session->machine()->GetClass() == "PRS1";
    //PROFILE.general->calculateRDI()


    const qint64 window_step = 30000; // 30 second windows
    double window_size = p_profile->cpap->AHIWindow();
    qint64 window_size_ms = window_size * 60000L;

    bool zeroreset = p_profile->cpap->AHIReset();

    if (session->machine()->GetType() != MT_CPAP) { return 0; }

    bool hasahi = session->eventlist.contains(CPAP_AHI);
    bool hasrdi = session->eventlist.contains(CPAP_RDI);

    if (hasahi && hasrdi) {
        return 0;    // abort if already there
    }

    if (!(!hasahi && !hasrdi)) {
        session->destroyEvent(CPAP_AHI);
        session->destroyEvent(CPAP_RDI);
    }

    if (!session->channelExists(CPAP_Obstructive) &&
            !session->channelExists(CPAP_Hypopnea) &&
            !session->channelExists(CPAP_Apnea) &&
            !session->channelExists(CPAP_ClearAirway) &&
            !session->channelExists(CPAP_RERA)
       ) { return 0; }

    qint64 first = session->first(),
           last = session->last(),
           f;

    EventList *AHI = new EventList(EVL_Event);
    AHI->setGain(0.02F);
    session->eventlist[CPAP_AHI].push_back(AHI);

    EventList *RDI = nullptr;

    if (calcrdi) {
        RDI = new EventList(EVL_Event);
        RDI->setGain(0.02F);
        session->eventlist[CPAP_RDI].push_back(RDI);
    }

    EventDataType ahi, rdi;

    qint64 ti = first, lastti = first;

    double avgahi = 0;
    double avgrdi = 0;
    int cnt = 0;

    double events;
    double hours = (window_size / 60.0F);

    if (zeroreset) {
        // I personally don't see the point of resetting each hour.
        do {
            // For each window, in 30 second increments
            for (qint64 t = ti; t < ti + window_size_ms; t += window_step) {
                if (t > last) {
                    break;
                }

                events = session->rangeCount(CPAP_Obstructive, ti, t)
                         + session->rangeCount(CPAP_Hypopnea, ti, t)
                         + session->rangeCount(CPAP_ClearAirway, ti, t)
                         + session->rangeCount(CPAP_Apnea, ti, t);

                ahi = events / hours;

                AHI->AddEvent(t, ahi * 50);
                avgahi += ahi;

                if (calcrdi) {
                    events += session->rangeCount(CPAP_RERA, ti, t);
                    rdi = events / hours;
                    RDI->AddEvent(t, rdi * 50);
                    avgrdi += rdi;
                }

                cnt++;
            }

            lastti = ti;
            ti += window_size_ms;
        } while (ti < last);

    } else {
        for (ti = first; ti < last; ti += window_step) {
            f = ti - window_size_ms;
            //hours=window_size; //double(ti-f)/3600000L;

            events = session->rangeCount(CPAP_Obstructive, f, ti)
                     + session->rangeCount(CPAP_Hypopnea, f, ti)
                     + session->rangeCount(CPAP_ClearAirway, f, ti)
                     + session->rangeCount(CPAP_Apnea, f, ti);

            ahi = events / hours;
            avgahi += ahi;
            AHI->AddEvent(ti, ahi * 50);

            if (calcrdi) {
                events += session->rangeCount(CPAP_RERA, f, ti);
                rdi = events / hours;
                RDI->AddEvent(ti, rdi * 50);
                avgrdi += rdi;
            }

            cnt++;
            lastti = ti;
            ti += window_step;
        }
    }

    AHI->AddEvent(lastti, 0);

    if (calcrdi) {
        RDI->AddEvent(lastti, 0);
    }

    if (!cnt) {
        avgahi = 0;
        avgrdi = 0;
    } else {
        avgahi /= double(cnt);
        avgrdi /= double(cnt);
    }

    cnt++;
    session->setAvg(CPAP_AHI, avgahi);

    if (calcrdi) {
        session->setAvg(CPAP_RDI, avgrdi);
    }

    return cnt;
}
struct TimeValue {
    TimeValue() {
        time = 0;
        value = 0;
    }
    TimeValue(qint64 t, EventStoreType v) {
        time = t;
        value = v;
    }
    TimeValue(const TimeValue &copy) {
        time = copy.time;
        value = copy.value;
    }
    qint64 time;
    EventStoreType value;
};

struct zMaskProfile {
  public:
    zMaskProfile(MaskType type, QString name);
    ~zMaskProfile();

    void reset() { pressureleaks.clear(); }
    void scanLeaks(Session *session);
    void scanPressure(Session *session);
    void updatePressureMin();
    void updateProfile(Session *session);

    void load(Profile *profile);
    void save();

    QMap<EventStoreType, EventDataType> pressuremax;
    QMap<EventStoreType, EventDataType> pressuremin;
    QMap<EventStoreType, EventDataType> pressurecount;
    QMap<EventStoreType, EventDataType> pressuretotal;
    QMap<EventStoreType, EventDataType> pressuremean;
    QMap<EventStoreType, EventDataType> pressurestddev;

    QVector<TimeValue> Pressure;

    EventDataType calcLeak(EventStoreType pressure);

  protected:
    static const quint32 version = 0;
    void scanLeakList(EventList *leak);
    void scanPressureList(EventList *el);

    MaskType    m_type;
    QString     m_name;
    Profile      *m_profile;
    QString     m_filename;

    QMap<EventStoreType, QMap<EventStoreType, quint32> > pressureleaks;
    EventDataType maxP, minP, maxL, minL, m_factor;
};


bool operator<(const TimeValue &p1, const TimeValue &p2)
{
    return p1.time < p2.time;
}

zMaskProfile::zMaskProfile(MaskType type, QString name)
    : m_type(type), m_name(name)
{
}
zMaskProfile::~zMaskProfile()
{
    save();
}

void zMaskProfile::load(Profile *profile)
{
    m_profile = profile;
    m_filename = m_profile->Get("{" + STR_GEN_DataFolder + "}/MaskProfile.mp");
    QFile f(m_filename);

    if (!f.open(QFile::ReadOnly)) {
        return;
    }

    QDataStream in(&f);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 m, v;
    in >> m;
    in >> v;

    if (m != magic) {
        qDebug() << "Magic wrong in zMaskProfile::load";
    }

    in >> pressureleaks;
    f.close();
}
void zMaskProfile::save()
{
    if (m_filename.isEmpty()) {
        return;
    }

    QFile f(m_filename);

    if (!f.open(QFile::WriteOnly)) {
        return;
    }

    QDataStream out(&f);
    out.setVersion(QDataStream::Qt_4_6);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)magic;
    out << (quint32)version;

    out << pressureleaks;
    f.close();
}

void zMaskProfile::scanPressureList(EventList *el)
{
    qint64 start = el->first();
    int count = el->count();
    EventStoreType *dptr = el->rawData();
    EventStoreType *eptr = dptr + count;
    quint32 *tptr = el->rawTime();
    qint64 time;
    EventStoreType pressure;

    for (; dptr < eptr; dptr++) {
        time = start + *tptr++;
        pressure = *dptr;
        Pressure.push_back(TimeValue(time, pressure));
    }
}
void zMaskProfile::scanPressure(Session *session)
{
    Pressure.clear();

    int prescnt = 0;

    //EventStoreType pressure;
    if (session->eventlist.contains(CPAP_Pressure)) {
        prescnt = session->count(CPAP_Pressure);
        Pressure.reserve(prescnt);

        for (int j = 0; j < session->eventlist[CPAP_Pressure].size(); j++) {
            QVector<EventList *> &el = session->eventlist[CPAP_Pressure];

            for (int e = 0; e < el.size(); e++) {
                scanPressureList(el[e]);
            }
        }
    } else if (session->eventlist.contains(CPAP_IPAP)) {
        prescnt = session->count(CPAP_IPAP);
        Pressure.reserve(prescnt);

        for (int j = 0; j < session->eventlist[CPAP_IPAP].size(); j++) {
            QVector<EventList *> &el = session->eventlist[CPAP_IPAP];

            for (int e = 0; e < el.size(); e++) {
                scanPressureList(el[e]);
            }
        }
    }

    qSort(Pressure);
}
void zMaskProfile::scanLeakList(EventList *el)
{
    qint64 start = el->first();
    int count = el->count();
    EventStoreType *dptr = el->rawData();
    EventStoreType *eptr = dptr + count;
    quint32 *tptr = el->rawTime();
    //EventDataType gain=el->gain();

    EventStoreType pressure, leak;

    //EventDataType fleak;
    QMap<EventStoreType, EventDataType>::iterator pmin;
    qint64 ti;
    bool found;

    for (; dptr < eptr; dptr++) {
        leak = *dptr;
        ti = start + *tptr++;

        found = false;
        pressure = Pressure[0].value;

        if (Pressure.size() > 1) {
            for (int i = 0; i < Pressure.size() - 1; i++) {
                const TimeValue &p1 = Pressure[i];
                const TimeValue &p2 = Pressure[i + 1];

                if ((p2.time > ti) && (p1.time <= ti)) {
                    pressure = p1.value;
                    found = true;
                    break;
                } else if (p2.time == ti) {
                    pressure = p2.value;
                    found = true;
                    break;
                }
            }
        } else {
            found = true;
        }

        if (found) {
            pressureleaks[pressure][leak]++;
            //            pmin=pressuremin.find(pressure);
            //            fleak=EventDataType(leak) * gain;
            //            if (pmin==pressuremin.end()) {
            //                pressuremin[pressure]=fleak;
            //                pressurecount[pressure]=pressureleaks[pressure][leak];
            //            } else {
            //                if (pmin.value() > fleak) {
            //                    pmin.value()=fleak;
            //                    pressurecount[pressure]=pressureleaks[pressure][leak];
            //                }
            //            }
        } else {
            //int i=5;
        }
    }


}
void zMaskProfile::scanLeaks(Session *session)
{
    QVector<EventList *> &elv = session->eventlist[CPAP_LeakTotal];

    for (int i = 0; i < elv.size(); i++) {
        scanLeakList(elv[i]);
    }
}
void zMaskProfile::updatePressureMin()
{
    QMap<EventStoreType, QMap<EventStoreType, quint32> >::iterator it;

    EventStoreType pressure;
    //EventDataType perc=0.1;
    //EventDataType tmp,l1,l2;
    //int idx1, idx2,
    int lks;
    double SN;
    double percentile = 0.40;

    double p = 100.0 * percentile;
    double nth, nthi;

    int sum1, sum2, w1, w2, N, k;

    for (it = pressureleaks.begin(); it != pressureleaks.end(); it++) {
        pressure = it.key();
        QMap<EventStoreType, quint32> &leakmap = it.value();
        lks = leakmap.size();
        SN = 0;

        // First sum total counts of all leaks
        for (QMap<EventStoreType, quint32>::iterator lit = leakmap.begin(); lit != leakmap.end(); lit++) {
            SN += lit.value();
        }


        nth = double(SN) * percentile; // index of the position in the unweighted set would be
        nthi = floor(nth);

        sum1 = 0, sum2 = 0;
        w1 = 0, w2 = 0;

        EventDataType v1 = 0, v2;

        N = leakmap.size();
        k = 0;

        bool found = false;
        double total = 0;

        for (QMap<EventStoreType, quint32>::iterator lit = leakmap.begin(); lit != leakmap.end();
                lit++, k++) {
            total += lit.value();
        }

        pressuretotal[pressure] = total;

        for (QMap<EventStoreType, quint32>::iterator lit = leakmap.begin(); lit != leakmap.end();
                lit++, k++) {
            //for (k=0;k < N;k++) {
            v1 = lit.key();
            w1 = lit.value();
            sum1 += w1;

            if (sum1 > nthi) {
                pressuremin[pressure] = v1;
                pressurecount[pressure] = w1;
                found = true;
                break;
            }

            if (sum1 == nthi) {
                break; // boundary condition
            }
        }

        if (found) { continue; }

        if (k >= N) {
            pressuremin[pressure] = v1;
            pressurecount[pressure] = w1;
            continue;
        }

        v2 = (leakmap.begin() + (k + 1)).key();
        w2 = (leakmap.begin() + (k + 1)).value();
        sum2 = sum1 + w2;
        // value lies between v1 and v2

        double px = 100.0 / double(SN); // Percentile represented by one full value

        // calculate percentile ranks
        double p1 = px * (double(sum1) - (double(w1) / 2.0));
        double p2 = px * (double(sum2) - (double(w2) / 2.0));

        // calculate linear interpolation
        double v = v1 + ((p - p1) / (p2 - p1)) * (v2 - v1);
        pressuremin[pressure] = v;
        pressurecount[pressure] = w1;

    }
}

EventDataType zMaskProfile::calcLeak(EventStoreType pressure)
{
    if (maxP == minP) {
        return pressuremin[pressure];
    }

    EventDataType leak = (pressure - minP) * (m_factor) + minL;
    return leak;

}

void zMaskProfile::updateProfile(Session *session)
{
    scanPressure(session);
    scanLeaks(session);
    updatePressureMin();


    if (pressuremin.size() <= 1) {
        maxP = minP = 0;
        maxL = minL = 0;
        m_factor = 0;
        return;
    }

    EventDataType p, l, tmp, mean, sum;
    minP = 250, maxP = 0;
    minL = 1000, maxL = 0;

    long cnt = 0, vcnt;
    int n;

    EventDataType maxcnt, maxval, lastval, lastcnt;

    for (QMap<EventStoreType, QMap<EventStoreType, quint32> >::iterator it = pressureleaks.begin();
            it != pressureleaks.end(); it++)  {
        p = it.key();
        l = pressuremin[p];
        QMap<EventStoreType, quint32> &leakval = it.value();
        cnt = 0;
        vcnt = -1;
        n = leakval.size();
        sum = 0;

        maxcnt = 0, maxval = 0, lastval = 0, lastcnt = 0;

        for (QMap<EventStoreType, quint32>::iterator lit = leakval.begin(); lit != leakval.end(); lit++) {
            cnt += lit.value();

            if (lit.value() > maxcnt) {
                lastcnt = maxcnt;
                maxcnt = lit.value();
                lastval = maxval;
                maxval = lit.key();

            }

            sum += lit.key() * lit.value();

            if (lit.key() == (EventStoreType)l) {
                vcnt = lit.value();
            }
        }

        pressuremean[p] = mean = sum / EventDataType(cnt);

        if (lastval > 0) {
            maxval = qMax(maxval, lastval); //((maxval*maxcnt)+(lastval*lastcnt)) / (maxcnt+lastcnt);
        }

        pressuremax[p] = lastval;
        sum = 0;

        for (QMap<EventStoreType, quint32>::iterator lit = leakval.begin(); lit != leakval.end(); lit++) {
            tmp = lit.key() - mean;
            sum += tmp * tmp;
        }

        pressurestddev[p] = tmp = sqrt(sum / EventDataType(n));

        if (vcnt >= 0) {
            tmp = 1.0 / EventDataType(cnt) * EventDataType(vcnt);

        }
    }

    QMap<EventStoreType, EventDataType> pressureval;
    QMap<EventStoreType, EventDataType> pressureval2;
    EventDataType max = 0, tmp2, tmp3;

    for (QMap<EventStoreType, EventDataType>::iterator it = pressuretotal.begin();
            it != pressuretotal.end(); it++) {
        if (max < it.value()) { max = it.value(); }
    }

    for (QMap<EventStoreType, EventDataType>::iterator it = pressuretotal.begin();
            it != pressuretotal.end(); it++) {
        p = it.key();
        tmp = pressurecount[p];
        tmp2 = it.value();

        tmp3 = (tmp / tmp2) * (tmp2 / max);

        if (tmp3 > 0.05) {
            tmp = pressuremean[p];

            if (tmp > 0) {
                pressureval[p] = tmp; // / tmp2;

                if (p < minP) { minP = p; }

                if (p > maxP) { maxP = p; }

                if (tmp < minL) { minL = tmp; }

                if (tmp > maxL) { maxL = tmp; }
            }
        }
    }

    if ((maxP == minP)  || (minL == maxL) || (minP == 250) || (minL == 1000)) {
        // crappy data set
        maxP = minP = 0;
        maxL = minL = 0;
        m_factor = 0;
        return;
    }

    m_factor = (maxL - minL) / (maxP - minP);

    //    for (QMap<EventStoreType, EventDataType>::iterator it=pressuremin.begin();it!=pressuremin.end()-1;it++) {
    //        p1=it.key();
    //        p2=(it+1).key();
    //        l1=it.value();
    //        l2=(it+1).value();
    //        if (l2 > l1) {
    //            factor=(l2 - l1) / (p2 - p1);
    //            sum+=factor;
    //            cnt++;
    //        }
    //    }

    //    m_factor=sum/double(cnt);

    //    int i=0;
    //    if (i==1) {
    //        QFile f("/home/mark/leaks.csv");
    //        f.open(QFile::WriteOnly);
    //        QTextStream out(&f);
    //        EventDataType p,l,c;
    //        QString fmt;
    //        for (QMap<EventStoreType, EventDataType>::iterator it=pressuremin.begin();it!=pressuremin.end();it++) {
    //            p=EventDataType(it.key()/10.0);
    //            l=it.value();
    //            fmt=QString("%1,%2\n").arg(p,0,'f',1).arg(l);
    //            out << fmt;
    //        }

    // cruft
    //        for (QMap<EventStoreType, QMap<EventStoreType,quint32> >::iterator it=pressureleaks.begin();it!=pressureleaks.end();it++)  {
    //            QMap<EventStoreType,quint32> & leakval=it.value();
    //            for (QMap<EventStoreType,quint32>::iterator lit=leakval.begin();lit!=leakval.end();lit++) {
    //                l=lit.key();
    //                c=lit.value();
    //                fmt=QString("%1,%2,%3\n").arg(p,0,'f',2).arg(l).arg(c);
    //                out << fmt;
    //            }
    //        }
    //        f.close();
    //    }
}

QMutex zMaskmutex;
zMaskProfile mmaskProfile(Mask_NasalPillows, "ResMed Swift FX");
bool mmaskFirst = true;

int calcLeaks(Session *session)
{

    if (session->machine()->GetType() != MT_CPAP) { return 0; }

    if (session->eventlist.contains(CPAP_Leak)) { return 0; } // abort if already there

    if (!session->eventlist.contains(CPAP_LeakTotal)) { return 0; } // can't calculate without this..

    zMaskmutex.lock();
    zMaskProfile *maskProfile = &mmaskProfile;

    if (mmaskFirst) {
        mmaskFirst = false;
        maskProfile->load(p_profile);
    }

    //    if (!maskProfile) {
    //        maskProfile=new zMaskProfile(Mask_NasalPillows,"ResMed Swift FX");
    //    }
    maskProfile->reset();
    maskProfile->updateProfile(session);

    EventList *leak = session->AddEventList(CPAP_Leak, EVL_Event, 1);

    for (int i = 0; i < session->eventlist[CPAP_LeakTotal].size(); i++) {
        EventList &el = *session->eventlist[CPAP_LeakTotal][i];
        EventDataType gain = el.gain(), tmp, val;
        int count = el.count();
        EventStoreType *dptr = el.rawData();
        EventStoreType *eptr = dptr + count;
        quint32 *tptr = el.rawTime();
        qint64 start = el.first(), ti;
        EventStoreType pressure;
        tptr = el.rawTime();
        start = el.first();

        bool found;

        for (; dptr < eptr; dptr++) {
            tmp = EventDataType(*dptr) * gain;
            ti = start + *tptr++;

            found = false;
            pressure = maskProfile->Pressure[0].value;

            for (int i = 0; i < maskProfile->Pressure.size() - 1; i++) {
                const TimeValue &p1 = maskProfile->Pressure[i];
                const TimeValue &p2 = maskProfile->Pressure[i + 1];

                if ((p2.time > ti) && (p1.time <= ti)) {
                    pressure = p1.value;
                    found = true;
                    break;
                } else if (p2.time == ti) {
                    pressure = p2.value;
                    found = true;
                    break;
                }
            }

            if (found) {
                val = tmp - maskProfile->calcLeak(pressure);

                if (val < 0) {
                    val = 0;
                }

                leak->AddEvent(ti, val);
            }
        }
    }

    zMaskmutex.unlock();
    return leak->count();
}


int calcPulseChange(Session *session)
{
    if (session->eventlist.contains(OXI_PulseChange)) { return 0; }

    QHash<ChannelID, QVector<EventList *> >::iterator it = session->eventlist.find(OXI_Pulse);

    if (it == session->eventlist.end()) { return 0; }

    EventDataType val, val2, change, tmp;
    qint64 time, time2;
    qint64 window = PROFILE.oxi->pulseChangeDuration();
    window *= 1000;

    change = PROFILE.oxi->pulseChangeBPM();

    EventList *pc = new EventList(EVL_Event, 1, 0, 0, 0, 0, true);
    pc->setFirst(session->first(OXI_Pulse));
    qint64 lastt;
    EventDataType lv = 0;
    int li = 0;

    int max;

    for (int e = 0; e < it.value().size(); e++) {
        EventList &el = *(it.value()[e]);

        for (unsigned i = 0; i < el.count(); i++) {
            val = el.data(i);
            time = el.time(i);


            lastt = 0;
            lv = change;
            max = 0;

            for (unsigned j = i + 1; j < el.count(); j++) { // scan ahead in the window
                time2 = el.time(j);

                if (time2 > time + window) { break; }

                val2 = el.data(j);
                tmp = qAbs(val2 - val);

                if (tmp > lv) {
                    lastt = time2;

                    if (tmp > max) { max = tmp; }

                    //lv=tmp;
                    li = j;
                }
            }

            if (lastt > 0) {
                qint64 len = (lastt - time) / 1000.0;
                pc->AddEvent(lastt, len, tmp);
                i = li;

            }
        }
    }

    if (pc->count() == 0) {
        delete pc;
        return 0;
    }

    session->eventlist[OXI_PulseChange].push_back(pc);
    session->setMin(OXI_PulseChange, pc->Min());
    session->setMax(OXI_PulseChange, pc->Max());
    session->setCount(OXI_PulseChange, pc->count());
    session->setFirst(OXI_PulseChange, pc->first());
    session->setLast(OXI_PulseChange, pc->last());
    return pc->count();
}


int calcSPO2Drop(Session *session)
{
    if (session->eventlist.contains(OXI_SPO2Drop)) { return 0; }

    QHash<ChannelID, QVector<EventList *> >::iterator it = session->eventlist.find(OXI_SPO2);

    if (it == session->eventlist.end()) { return 0; }

    EventDataType val, val2, change, tmp;
    qint64 time, time2;
    qint64 window = PROFILE.oxi->spO2DropDuration();
    window *= 1000;
    change = PROFILE.oxi->spO2DropPercentage();

    EventList *pc = new EventList(EVL_Event, 1, 0, 0, 0, 0, true);
    qint64 lastt;
    EventDataType lv = 0;
    int li = 0;

    // Fix me.. Time scale varies.
    //const unsigned ringsize=30;
    //EventDataType ring[ringsize]={0};
    //qint64 rtime[ringsize]={0};
    //int rp=0;
    int min;
    int cnt = 0;
    tmp = 0;

    qint64 start = 0;

    // Calculate median baseline
    QList<EventDataType> med;

    for (int e = 0; e < it.value().size(); e++) {
        EventList &el = *(it.value()[e]);

        for (unsigned i = 0; i < el.count(); i++) {
            val = el.data(i);
            time = el.time(i);

            if (val > 0) { med.push_back(val); }

            if (!start) { start = time; }

            if (time > start + 3600000) { break; } // just look at the first hour

            tmp += val;
            cnt++;
        }
    }

    EventDataType baseline = 0;

    if (med.size() > 0) {
        qSort(med);

        int midx = float(med.size()) * 0.90;

        if (midx > med.size() - 1) { midx = med.size() - 1; }

        if (midx < 0) { midx = 0; }

        baseline = med[midx];
    }

    session->settings[OXI_SPO2Drop] = baseline;
    //EventDataType baseline=round(tmp/EventDataType(cnt));
    EventDataType current;
    qDebug() << "Calculated baseline" << baseline;

    for (int e = 0; e < it.value().size(); e++) {
        EventList &el = *(it.value()[e]);

        for (unsigned i = 0; i < el.count(); i++) {
            current = el.data(i);

            if (!current) { continue; }

            time = el.time(i);
            /*ring[rp]=val;
            rtime[rp]=time;
            rp++;
            rp=rp % ringsize;
            if (i<ringsize)  {
                for (unsigned j=i;j<ringsize;j++) {
                    ring[j]=val;
                    rtime[j]=0;
                }
            }
            tmp=0;
            cnt=0;
            for (unsigned j=0;j<ringsize;j++) {
                if (rtime[j] > time-300000) {  // only look at recent entries..
                    tmp+=ring[j];
                    cnt++;
                }
            }
            if (!cnt) {
                unsigned j=abs((rp-1) % ringsize);
                tmp=(ring[j]+val)/2;
            } else tmp/=EventDataType(cnt); */

            val = baseline;
            lastt = 0;
            lv = val;


            min = val;

            for (unsigned j = i; j < el.count(); j++) { // scan ahead in the window
                time2 = el.time(j);
                //if (time2 > time+window) break;
                val2 = el.data(j);

                if (val2 > baseline - change) { break; }

                lastt = time2;
                li = j + 1;
            }

            if (lastt > 0) {
                qint64 len = (lastt - time);

                if (len >= window) {
                    pc->AddEvent(lastt, len / 1000, val - min);
                    i = li;
                }
            }
        }
    }

    if (pc->count() == 0) {
        delete pc;
        return 0;
    }

    session->eventlist[OXI_SPO2Drop].push_back(pc);
    session->setMin(OXI_SPO2Drop, pc->Min());
    session->setMax(OXI_SPO2Drop, pc->Max());
    session->setCount(OXI_SPO2Drop, pc->count());
    session->setFirst(OXI_SPO2Drop, pc->first());
    session->setLast(OXI_SPO2Drop, pc->last());
    return pc->count();
}
