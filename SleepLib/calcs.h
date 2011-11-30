/*
 Custom CPAP/Oximetry Calculations Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/
#ifndef CALCS_H
#define CALCS_H

#include "day.h"

class Calculation
{
public:
    Calculation(ChannelID id,QString name);
    virtual ~Calculation();
    virtual int calculate(Session *session)=0;
protected:
    ChannelID m_id;
    QString m_name;
};

class CalcRespRate:public Calculation
{
public:
    CalcRespRate(ChannelID id=CPAP_RespRate);
    virtual int calculate(Session *session);
protected:
    int filterFlow(EventList *in, EventList *out,EventList *tv, EventList *mv, double rate);
};

class CalcAHIGraph:public Calculation
{
public:
    CalcAHIGraph(ChannelID id=CPAP_AHI);
    virtual int calculate(Session *session);
protected:
};

int calcLeaks(Session *session);

int calcPulseChange(Session *session);
int calcSPO2Drop(Session *session);

EventDataType calcAHI(Session *session,qint64 start=0, qint64 end=0);

#endif // CALCS_H
