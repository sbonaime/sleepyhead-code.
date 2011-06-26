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
    Waveform(QDateTime time,MachineCode code,SampleFormat * data,int samples,float duration,SampleFormat min, SampleFormat max);
    ~Waveform();
    SampleFormat operator[](int i) {
        if (i<w_samples) return w_data[i];
        else return 0;
    };
    const QDateTime & start() {
        return w_time;
    };
    const QDateTime end() {
        return w_time.addSecs(w_totalspan);
    };
    MachineCode code() {
        return w_code;
    };
    int samples() {
        return w_samples;
    };
    float duration() {
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
    QDateTime w_time;
    MachineCode w_code;
    SampleFormat * w_data;
    qint32 w_samples;
    float w_duration;
    SampleFormat Min,Max;
    double w_totalspan;
    double w_samplespan;
};


#endif // WAVEFORM_H
