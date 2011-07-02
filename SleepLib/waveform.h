/********************************************************************
 SleepLib Waveform Header
 This stuff contains the base calculation smarts
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QDateTime>
#include "machine_common.h"

typedef qint16 SampleFormat;

class Waveform
{
    friend class Session;
public:
    Waveform(qint64 time,MachineCode code,SampleFormat * data,int samples,qint64 duration,SampleFormat min, SampleFormat max);
    ~Waveform();
    SampleFormat operator[](int i) {
        if (i<w_samples) return w_data[i];
        else return 0;
    };
    qint64 start() {
        return w_time;
    };
    qint64 end() {
        return w_time+w_totalspan;
    };
    MachineCode code() {
        return w_code;
    };
    int samples() {
        return w_samples;
    };
    qint64 duration() {
        return w_duration;
    };
    SampleFormat min() {
        return Min;
    };
    SampleFormat max() {
        return Max;
    };
    SampleFormat *GetBuffer() { return w_data; };

protected:
    qint64 w_time;
    MachineCode w_code;
    SampleFormat * w_data;
    qint32 w_samples;
    qint64 w_duration;
    SampleFormat Min,Max;
    qint64 w_totalspan;
    qint64 w_samplespan;
};


#endif // WAVEFORM_H
