/*
 Custom CPAP/Oximetry Calculations Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/
#ifndef CALCS_H
#define CALCS_H

#include "day.h"

int calcRespRate(Session *session);
int calcAHIGraph(Session *session);
EventDataType calcAHI(Session *session,qint64 start=0, qint64 end=0);

int calcLeaks(Session *session);

int calcPulseChange(Session *session);
int calcSPO2Drop(Session *session);


#endif // CALCS_H
