/********************************************************************
 SleepLib Machine Loader Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef MACHINE_COMMON_H
#define MACHINE_COMMON_H

#include <map>
#include <vector>
#include <QString>
using namespace std;

typedef long MachineID;
typedef long SessionID;

class BoundsError {};

const int max_number_event_fields=10;

enum MachineType//: short
{ MT_UNKNOWN=0,MT_CPAP,MT_OXIMETER,MT_SLEEPSTAGE,MT_JOURNAL };
// I wish C++ could extend enums.
// Could be implimented by MachineLoader's register function requesting a block of integer space,
// and a map to name strings.

typedef int ChannelCode;

// Be cautious when extending these.. add to the end of each groups to preserve file formats.
enum MachineCode//:qint16
{
    // General Event Codes
    MC_UNKNOWN=0,

    CPAP_Obstructive, CPAP_Hypopnea, CPAP_ClearAirway, CPAP_RERA, CPAP_VSnore, CPAP_FlowLimit,
    CPAP_Leak, CPAP_Pressure, CPAP_EAP, CPAP_IAP, CPAP_CSR, CPAP_FlowRate, CPAP_MaskPressure,
    CPAP_Snore,CPAP_MinuteVentilation,
    CPAP_BreathsPerMinute,

    // General CPAP Summary Information
    CPAP_PressureMin=0x80, CPAP_PressureMax, CPAP_RampTime, CPAP_RampStartingPressure, CPAP_Mode, CPAP_PressureReliefType,
    CPAP_PressureReliefSetting, CPAP_HumidifierSetting, CPAP_HumidifierStatus, CPAP_PressureMinAchieved,
    CPAP_PressureMaxAchieved, CPAP_PressurePercentValue, CPAP_PressurePercentName, CPAP_PressureAverage, CPAP_PressureMedian,
    CPAP_LeakMedian,CPAP_LeakMinimum,CPAP_LeakMaximum,CPAP_LeakAverage,CPAP_Duration,
    CPAP_SnoreGraph, CPAP_SnoreMinimum, CPAP_SnoreMaximum, CPAP_SnoreAverage, CPAP_SnoreMedian,

    BIPAP_EAPAverage,BIPAP_IAPAverage,BIPAP_EAPMin,BIPAP_EAPMax,BIPAP_IAPMin,BIPAP_IAPMax,CPAP_BrokenSummary,

    // PRS1 Specific Codes
    PRS1_PressurePulse=0x1000,
    PRS1_Unknown00, PRS1_Unknown01, PRS1_Unknown08, PRS1_Unknown09, PRS1_Unknown0B,	PRS1_Unknown0E, PRS1_Unknown10, PRS1_Unknown12,
    PRS1_SystemLockStatus, PRS1_SystemResistanceStatus, PRS1_SystemResistanceSetting, PRS1_HoseDiameter, PRS1_AutoOff, PRS1_MaskAlert, PRS1_ShowAHI,

    // Oximeter Codes
    OXI_Pulse=0x2000, OXI_SPO2, OXI_Plethy, OXI_Signal2, OXI_SignalGood, OXI_PulseAverage, OXI_PulseMin, OXI_PulseMax, OXI_SPO2Average, OXI_SPO2Min, OXI_SPO2Max,

    ZEO_SleepStage=0x2800, ZEO_Waveform,

    GEN_Weight=0x3000, GEN_Notes, GEN_Glucose, GEN_Calories, GEN_Energy, GEN_Mood, GEN_Excercise, GEN_Reminder



};
void InitMapsWithoutAwesomeInitializerLists();

enum FlagType//:short
{ FT_BAR, FT_DOT, FT_SPAN };

enum CPAPMode//:short
{
    MODE_UNKNOWN=0,MODE_CPAP,MODE_APAP,MODE_BIPAP,MODE_ASV
};
enum PRTypes//:short
{
    PR_UNKNOWN=0,PR_NONE,PR_CFLEX,PR_CFLEXPLUS,PR_AFLEX,PR_BIFLEX,PR_EPR,PR_SMARTFLEX
};

extern map<MachineCode,QString> DefaultMCShortNames;
extern map<MachineCode,QString> DefaultMCLongNames;
extern map<PRTypes,QString> PressureReliefNames;
extern map<CPAPMode,QString> CPAPModeNames;

// These are types supported by wxVariant class. To retain compatability, add to the end of this list only..
enum MCDataType//:wxInt8
{ MC_bool=0, MC_int, MC_long, MC_float, MC_double, MC_string, MC_datetime };




#endif // MACHINE_COMMON_H
