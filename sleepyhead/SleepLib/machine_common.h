/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Machine Loader Class Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef MACHINE_COMMON_H
#define MACHINE_COMMON_H

#include <QHash>
#include <QColor>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QDebug>
using namespace std;

// Do not change these without considering the consequences.. For one the Loader needs changing & version increase
typedef quint32 ChannelID;
typedef long MachineID;
typedef long SessionID;
typedef float EventDataType;
typedef qint16 EventStoreType;

//! \brief Exception class for out of Bounds error.. Unused.
class BoundsError {};

//! \brief Exception class for to trap old database versions.
class OldDBVersion {};

const quint32 magic=0xC73216AB; // Magic number for SleepyHead Data Files.. Don't touch!

//const int max_number_event_fields=10;
// This should probably move somewhere else
//! \fn timezoneOffset();
//! \brief Calculate the timezone Offset in milliseconds between system timezone and UTC
qint64 timezoneOffset();


/*! \enum SummaryType
    \brief Calculation/Display method to select from dealing with summary information
  */
enum SummaryType { ST_CNT, ST_SUM, ST_AVG, ST_WAVG, ST_PERC, ST_90P, ST_MIN, ST_MAX, ST_CPH, ST_SPH, ST_FIRST, ST_LAST, ST_HOURS, ST_SESSIONS, ST_SETMIN, ST_SETAVG, ST_SETMAX, ST_SETWAVG, ST_SETSUM, ST_SESSIONID, ST_DATE };

/*! \enum MachineType
    \brief Generalized type of a machine
  */
enum MachineType { MT_UNKNOWN=0,MT_CPAP,MT_OXIMETER,MT_SLEEPSTAGE,MT_JOURNAL };
//void InitMapsWithoutAwesomeInitializerLists();


/*! \enum CPAPMode
    \brief CPAP Machines mode of operation
  */
enum CPAPMode//:short
{
    MODE_UNKNOWN=0,MODE_CPAP,MODE_APAP,MODE_BIPAP,MODE_ASV
};

/*! \enum PRTypes
    \brief Pressure Relief Types, used by CPAP machines
  */
enum PRTypes//:short
{
    PR_UNKNOWN=0,PR_NONE,PR_CFLEX,PR_CFLEXPLUS,PR_AFLEX,PR_BIFLEX,PR_EPR,PR_SMARTFLEX,PR_EASYBREATHE,PR_SENSAWAKE
};
enum PRModes//:short
{
    PM_UNKNOWN=0,PM_RampOnly,PM_FullTime
};


//extern map<ChannelID,QString> DefaultMCShortNames;
//extern map<ChannelID,QString> DefaultMCLongNames;
//extern map<PRTypes,QString> PressureReliefNames;
//extern map<CPAPMode,QString> CPAPModeNames;

/*! \enum MCDataType
    \brief Data Types stored by Profile/Preferences objects, etc..
    */

enum MCDataType
{ MC_bool=0, MC_int, MC_long, MC_float, MC_double, MC_string, MC_datetime };


extern ChannelID NoChannel,SESSION_ENABLED;
extern ChannelID CPAP_IPAP, CPAP_IPAPLo, CPAP_IPAPHi, CPAP_EPAP, CPAP_EPAPLo, CPAP_EPAPHi, CPAP_Pressure, CPAP_PS, CPAP_PSMin, CPAP_PSMax,
CPAP_Mode, CPAP_AHI,
CPAP_PressureMin, CPAP_PressureMax, CPAP_RampTime, CPAP_RampPressure, CPAP_Obstructive, CPAP_Hypopnea,
CPAP_ClearAirway, CPAP_Apnea, CPAP_CSR, CPAP_LeakFlag, CPAP_ExP, CPAP_NRI, CPAP_VSnore, CPAP_VSnore2,
CPAP_RERA, CPAP_PressurePulse, CPAP_FlowLimit, CPAP_FlowRate, CPAP_MaskPressure, CPAP_MaskPressureHi,
CPAP_RespEvent, CPAP_Snore, CPAP_MinuteVent, CPAP_RespRate, CPAP_TidalVolume, CPAP_PTB, CPAP_Leak,
CPAP_LeakMedian, CPAP_LeakTotal, CPAP_MaxLeak, CPAP_FLG, CPAP_IE, CPAP_Te, CPAP_Ti, CPAP_TgMV,
CPAP_UserFlag1, CPAP_UserFlag2, CPAP_UserFlag3, CPAP_BrokenSummary, CPAP_BrokenWaveform, CPAP_RDI,
CPAP_PresReliefSet, CPAP_PresReliefMode, CPAP_PresReliefType, CPAP_Test1, CPAP_Test2;

extern ChannelID RMS9_E01, RMS9_E02, RMS9_EPR, RMS9_EPRSet, RMS9_SetPressure;
extern ChannelID INTP_SmartFlex;
extern ChannelID PRS1_00, PRS1_01, PRS1_08, PRS1_0A, PRS1_0B, PRS1_0C, PRS1_0E, PRS1_0F, PRS1_10, PRS1_12,
PRS1_FlexMode, PRS1_FlexSet, PRS1_HumidStatus, CPAP_HumidSetting, PRS1_SysLock, PRS1_SysOneResistStat,
PRS1_SysOneResistSet, PRS1_HoseDiam, PRS1_AutoOn, PRS1_AutoOff, PRS1_MaskAlert, PRS1_ShowAHI;

extern ChannelID INTELLIPAP_Unknown1, INTELLIPAP_Unknown2;

extern ChannelID OXI_Pulse, OXI_SPO2, OXI_PulseChange, OXI_SPO2Drop, OXI_Plethy;

extern ChannelID Journal_Notes, Journal_Weight, Journal_BMI, Journal_ZombieMeter, Bookmark_Start, Bookmark_End, Bookmark_Notes;

extern ChannelID ZEO_SleepStage, ZEO_ZQ, ZEO_TotalZ, ZEO_TimeToZ, ZEO_TimeInWake, ZEO_TimeInREM, ZEO_TimeInLight, ZEO_TimeInDeep, ZEO_Awakenings,
ZEO_AlarmReason, ZEO_SnoozeTime, ZEO_WakeTone, ZEO_WakeWindow, ZEO_AlarmType, ZEO_MorningFeel, ZEO_FirmwareVersion,
ZEO_FirstAlarmRing, ZEO_LastAlarmRing, ZEO_FirstSnoozeTime, ZEO_LastSnoozeTime, ZEO_SetAlarmTime, ZEO_RiseTime;


#endif // MACHINE_COMMON_H
