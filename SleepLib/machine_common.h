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

class BoundsError {};
class OldDBVersion {};

const quint32 magic=0xC73216AB; // Magic number for Sleepyhead Data Files.. Don't touch!

//const int max_number_event_fields=10;


enum MachineType { MT_UNKNOWN=0,MT_CPAP,MT_OXIMETER,MT_SLEEPSTAGE,MT_JOURNAL };
//void InitMapsWithoutAwesomeInitializerLists();


enum CPAPMode//:short
{
    MODE_UNKNOWN=0,MODE_CPAP,MODE_APAP,MODE_BIPAP,MODE_ASV
};
enum PRTypes//:short
{
    PR_UNKNOWN=0,PR_NONE,PR_CFLEX,PR_CFLEXPLUS,PR_AFLEX,PR_BIFLEX,PR_EPR,PR_SMARTFLEX
};

//extern map<ChannelID,QString> DefaultMCShortNames;
//extern map<ChannelID,QString> DefaultMCLongNames;
//extern map<PRTypes,QString> PressureReliefNames;
//extern map<CPAPMode,QString> CPAPModeNames;

// These are types supported by wxVariant class. To retain compatability, add to the end of this list only..
enum MCDataType
{ MC_bool=0, MC_int, MC_long, MC_float, MC_double, MC_string, MC_datetime };

const QString CPAP_IPAP="IPAP";
const QString CPAP_IPAPLo="IPAPLo";
const QString CPAP_IPAPHi="IPAPHi";
const QString CPAP_EPAP="EPAP";
const QString CPAP_Pressure="Pressure";
const QString CPAP_PS="PS";
const QString PRS1_FlexMode="FlexMode";
const QString PRS1_FlexSet="FlexSet";
const QString CPAP_Mode="PAPMode";
const QString CPAP_BrokenSummary="BrokenSummary";
const QString CPAP_PressureMin="PressureMin";
const QString CPAP_PressureMax="PressureMin";
const QString CPAP_RampTime="RampTime";
const QString CPAP_RampPressure="RampPressure";
const QString CPAP_Obstructive="Obstructive";
const QString CPAP_Hypopnea="Hypopnea";
const QString CPAP_ClearAirway="ClearAirway";
const QString CPAP_Apnea="Apnea";
const QString CPAP_CSR="CSR";
const QString CPAP_LeakFlag="LeakFlag";
const QString CPAP_ExP="ExP";
const QString CPAP_NRI="NRI";
const QString CPAP_VSnore="VSnore";
const QString CPAP_VSnore2="VSnore2";
const QString CPAP_RERA="RERA";
const QString CPAP_PressurePulse="PressurePulse";
const QString CPAP_FlowLimit="FlowLimit";
const QString CPAP_FlowRate="FlowRate";
const QString CPAP_MaskPressure="MaskPressure";
const QString CPAP_MaskPressureHi="MaskPressureHi";
const QString CPAP_RespEvent="RespEvent";
const QString CPAP_Snore="Snore";
const QString CPAP_MinuteVent="MinuteVent";
const QString CPAP_RespRate="RespRate";
const QString CPAP_TidalVolume="TidalVolume";
const QString CPAP_PTB="PTB";
const QString CPAP_Leak="Leak";
const QString CPAP_LeakMedian="LeakMedian";
const QString CPAP_LeakTotal="LeakTotal";
const QString CPAP_MaxLeak="MaxLeak";
const QString CPAP_FLG="FLG";
const QString CPAP_IE="IE";
const QString CPAP_Te="Te";
const QString CPAP_Ti="Ti";
const QString CPAP_TgMV="TgMV";
const QString RMS9_E01="RMS9_E01";
const QString RMS9_E02="RMS9_E02";
const QString PRS1_00="PRS1_00";
const QString PRS1_01="PRS1_01";
const QString PRS1_08="PRS1_08";
const QString PRS1_0A="PRS1_0A";
const QString PRS1_0B="PRS1_0B";
const QString PRS1_0C="PRS1_0C";
const QString PRS1_0E="PRS1_0E";
const QString PRS1_0F="PRS1_0F";
const QString PRS1_10="PRS1_10";
const QString PRS1_12="PRS1_12";

const QString OXI_Pulse="Pulse";
const QString OXI_SPO2="SPO2";
const QString OXI_PulseChange="PulseChange";
const QString OXI_SPO2Drop="SPO2Drop";
const QString OXI_Plethy="Plethy";

const QString CPAP_AHI="AHI";
const QString Journal_Notes="Journal";


#endif // MACHINE_COMMON_H
