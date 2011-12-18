/*
 Custom CPAP/Oximetry Calculations Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/
#ifndef CALCS_H
#define CALCS_H

#include "day.h"

//! \brief Calculate Respiratory Rate, Tidal Volume & Minute Ventilation for PRS1 data
int calcRespRate(Session *session);

//! \brief Calculates the sliding window AHI graph
int calcAHIGraph(Session *session);

//! \brief Calculates AHI for a session between start & end (a support function for the sliding window graph)
EventDataType calcAHI(Session *session,qint64 start=0, qint64 end=0);

//! \brief Leaks calculations for PRS1
int calcLeaks(Session *session);

//! \brief Calculate Pulse change flagging, according to preferences
int calcPulseChange(Session *session);

//! \brief Calculate SPO2 Drop flagging, according to preferences
int calcSPO2Drop(Session *session);


#endif // CALCS_H
