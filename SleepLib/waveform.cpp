/********************************************************************
 SleepLib Waveform Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include "waveform.h"

Waveform::Waveform(QDateTime time,MachineCode code, SampleFormat *data,int samples,float duration,SampleFormat min, SampleFormat max)
    :w_time(time),w_code(code),w_data(data),w_samples(samples),w_duration(duration)
{
    w_totalspan=duration;
    double rate=duration/samples;
    w_samplespan=rate;
    Min=min;
    Max=max;
}
Waveform::~Waveform()
{
    delete [] w_data;
}
