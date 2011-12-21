/*
 SleepLib Machine Loader Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef MACHINE_COMMON_H
#define MACHINE_COMMON_H

#include <QHash>
#include <QColor>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QDebug>
using namespace std;

typedef QString ChannelID;
typedef long MachineID;
typedef long SessionID;
typedef float EventDataType;
typedef qint16 EventStoreType;

//! \brief Exception class for out of Bounds error.. Unused.
class BoundsError {};

//! \brief Exception class for to trap old database versions.
class OldDBVersion {};

const quint32 magic=0xC73216AB; // Magic number for Sleepyhead Data Files.. Don't touch!

//const int max_number_event_fields=10;
// This should probably move somewhere else
//! \fn timezoneOffset();
//! \brief Calculate the timezone Offset in milliseconds between system timezone and UTC
qint64 timezoneOffset();


/*! \enum SummaryType
    \brief Calculation method to select from dealing with summary information
  */
enum SummaryType { ST_CNT, ST_SUM, ST_AVG, ST_WAVG, ST_90P, ST_MIN, ST_MAX, ST_CPH, ST_SPH, ST_FIRST, ST_LAST, ST_HOURS, ST_SESSIONS, ST_SETMIN, ST_SETAVG, ST_SETMAX, ST_SETWAVG, ST_SETSUM };

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
    PR_UNKNOWN=0,PR_NONE,PR_CFLEX,PR_CFLEXPLUS,PR_AFLEX,PR_BIFLEX,PR_EPR,PR_SMARTFLEX
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

// This all needs replacing with actual integer codes.. There will likely be a big speedup when this happens again.
const ChannelID CPAP_IPAP="IPAP";
const ChannelID CPAP_IPAPLo="IPAPLo";
const ChannelID CPAP_IPAPHi="IPAPHi";
const ChannelID CPAP_EPAP="EPAP";
const ChannelID CPAP_Pressure="Pressure";
const ChannelID CPAP_PS="PS";
const ChannelID CPAP_Mode="PAPMode";
const ChannelID CPAP_BrokenSummary="BrokenSummary";
const ChannelID CPAP_PressureMin="PressureMin";
const ChannelID CPAP_PressureMax="PressureMax";
const ChannelID CPAP_RampTime="RampTime";
const ChannelID CPAP_RampPressure="RampPressure";
const ChannelID CPAP_Obstructive="Obstructive";
const ChannelID CPAP_Hypopnea="Hypopnea";
const ChannelID CPAP_ClearAirway="ClearAirway";
const ChannelID CPAP_Apnea="Apnea";
const ChannelID CPAP_CSR="CSR";
const ChannelID CPAP_LeakFlag="LeakFlag";
const ChannelID CPAP_ExP="ExP";
const ChannelID CPAP_NRI="NRI";
const ChannelID CPAP_VSnore="VSnore";
const ChannelID CPAP_VSnore2="VSnore2";
const ChannelID CPAP_RERA="RERA";
const ChannelID CPAP_PressurePulse="PressurePulse";
const ChannelID CPAP_FlowLimit="FlowLimit";
const ChannelID CPAP_FlowRate="FlowRate";
const ChannelID CPAP_MaskPressure="MaskPressure";
const ChannelID CPAP_MaskPressureHi="MaskPressureHi";
const ChannelID CPAP_RespEvent="RespEvent";
const ChannelID CPAP_Snore="Snore";
const ChannelID CPAP_MinuteVent="MinuteVent";
const ChannelID CPAP_RespRate="RespRate";
const ChannelID CPAP_TidalVolume="TidalVolume";
const ChannelID CPAP_PTB="PTB";
const ChannelID CPAP_Leak="Leak";
const ChannelID CPAP_LeakMedian="LeakMedian";
const ChannelID CPAP_LeakTotal="LeakTotal";
const ChannelID CPAP_MaxLeak="MaxLeak";
const ChannelID CPAP_FLG="FLG";
const ChannelID CPAP_IE="IE";
const ChannelID CPAP_Te="Te";
const ChannelID CPAP_Ti="Ti";
const ChannelID CPAP_TgMV="TgMV";
const ChannelID RMS9_E01="RMS9_E01";
const ChannelID RMS9_E02="RMS9_E02";
const ChannelID RMS9_EPR="EPR";
const ChannelID RMS9_EPRSet="EPRSet";
const ChannelID PRS1_00="PRS1_00";
const ChannelID PRS1_01="PRS1_01";
const ChannelID PRS1_08="PRS1_08";
const ChannelID PRS1_0A="PRS1_0A";
const ChannelID PRS1_0B="PRS1_0B";
const ChannelID PRS1_0C="PRS1_0C";
const ChannelID PRS1_0E="PRS1_0E";
const ChannelID PRS1_0F="PRS1_0F";
const ChannelID PRS1_10="PRS1_10";
const ChannelID PRS1_12="PRS1_12";
const ChannelID PRS1_FlexMode="FlexMode";
const ChannelID PRS1_FlexSet="FlexSet";
const ChannelID PRS1_HumidStatus="HumidStat";
const ChannelID PRS1_HumidSetting="HumidSet";
const ChannelID PRS1_SysLock="SysLock";
const ChannelID PRS1_SysOneResistStat="SysOneResistStat";
const ChannelID PRS1_SysOneResistSet="SysOneResistSet";
const ChannelID PRS1_HoseDiam="HoseDiam";
const ChannelID PRS1_AutoOn="AutoOn";
const ChannelID PRS1_AutoOff="AutoOff";
const ChannelID PRS1_MaskAlert="MaskAlert";
const ChannelID PRS1_ShowAHI="ShowAHI";
const ChannelID CPAP_UserFlag1="UserFlag1";
const ChannelID CPAP_UserFlag2="UserFlag2";

const ChannelID OXI_Pulse="Pulse";
const ChannelID OXI_SPO2="SPO2";
const ChannelID OXI_PulseChange="PulseChange";
const ChannelID OXI_SPO2Drop="SPO2Drop";
const ChannelID OXI_Plethy="Plethy";

const ChannelID CPAP_AHI="AHI";
const ChannelID Journal_Notes="Journal";
const ChannelID Journal_Weight="Weight";
const ChannelID Journal_BMI="BMI";
const ChannelID Journal_ZombieMeter="ZombieMeter";
const ChannelID Bookmark_Start="BookmarkStart";
const ChannelID Bookmark_End="BookmarkEnd";
const ChannelID Bookmark_Notes="BookmarkNotes";

#endif // MACHINE_COMMON_H
