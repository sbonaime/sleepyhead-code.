/********************************************************************
 SleepLib Machine Loader Class Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef MACHINE_COMMON_H
#define MACHINE_COMMON_H

#include <QHash>
#include <QColor>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QDebug>
using namespace std;

typedef quint32 ChannelID;
typedef long MachineID;
typedef long SessionID;
typedef float EventDataType;
typedef qint16 EventStoreType;

class BoundsError {};
class OldDBVersion {};

// This is the uber important database version for SleepyHeads internal storage
// Increment this after stuffing with Session's save & load code.
const quint16 dbversion=6;

//const int max_number_event_fields=10;


enum MachineType { MT_UNKNOWN=0,MT_CPAP,MT_OXIMETER,MT_SLEEPSTAGE,MT_JOURNAL };
enum ChannelType { CT_Unknown=0, CT_Event, CT_Graph, CT_Text, CT_Decimal, CT_Integer, CT_Boolean, CT_DateTime, CT_Date, CT_Time, CT_Timespan, CT_Calculation, CT_Lookup };

class Channel
{
protected:
    ChannelID m_id;
    MachineType m_mtype;
    ChannelType m_ctype;
    QString m_details;
    QString m_label;
    QColor m_color;
    QVector<QVariant> m_option;
    EventDataType m_min_value;
    EventDataType m_max_value;
public:
    Channel()
        :m_id(0),m_mtype(MT_UNKNOWN), m_ctype(CT_Unknown), m_details(""), m_label(""),m_color(Qt::black) {}
    Channel(ChannelID id, MachineType mtype, ChannelType ctype, QString details, QString label, QColor color)
        :m_id(id),m_mtype(mtype),m_ctype(ctype),m_details(details),m_label(label),m_color(color) {}
    ChannelID & id() { return m_id; }
    MachineType & machinetype() { return m_mtype; }
    ChannelType & channeltype() { return m_ctype; }
    QString & details() { return m_details; }
    QString & label() { return m_label; }

    EventDataType min() { return m_min_value; }  // Min value allowed for this channel
    EventDataType max() { return m_max_value; }  // Max value allowed for this channel
    void setMin(EventDataType v) { m_min_value=v; }
    void setMax(EventDataType v) { m_max_value=v; }

    // add Option automatically sets the bounds
    void addOption(QVariant v) { m_option.push_back(v); m_min_value=0; m_max_value=m_option.size(); }
    int countOptions() { return m_option.count(); }

    QVariant & option(int i) { if (m_option.contains(i)) return m_option[i]; }
    QVariant & operator [](int i) { if (m_option.contains(i)) return m_option[i]; }

    QString optionString(int i) { if (m_option.contains(i)) return m_option[i].toString(); else return ""; }
    int optionInt(int i) { if (m_option.contains(i)) return m_option[i].toInt(); else return 0; }
    EventDataType optionFloat(int i) { if (m_option.contains(i)) return m_option[i].toFloat(); else return 0; }
};

extern QHash<ChannelID, Channel> channel;
extern QHash<QString,Channel *> channel_lookup;

struct ChannelGroup
{
protected:
    MachineType m_type;
    ChannelID m_first;
    ChannelID m_size;
    ChannelID m_pos;
    QHash<ChannelID, Channel *> m_channel;
    QHash<ChannelType, QHash<ChannelID, Channel *> > m_channelsbytype;
    QHash<QString, Channel *> m_channel_lookup;
public:
    ChannelID first() { return m_first; }
    ChannelID count() { return m_pos; }
    Channel & operator [](ChannelID id) { return *m_channel[id]; }
    Channel & operator [](QString lookup) { return *m_channel_lookup[lookup]; }
    QHash<ChannelID, Channel *> & operator [](ChannelType type) { return m_channelsbytype[type]; }

    ChannelID Get(ChannelType ctype,QString description="",QString label="",QString lookup="",QColor color=Qt::black);
    ChannelGroup();
    ChannelGroup(MachineType type, ChannelID first, ChannelID reserved=0x200);
};
extern QHash<QString,ChannelGroup *> channel_group;

extern ChannelGroup CPAP_CODES;
extern ChannelGroup PRS1_CODES;
extern ChannelGroup RMS9_CODES;
extern ChannelGroup JOURNAL_CODES;
extern ChannelGroup ZEO_CODES;
extern ChannelGroup OXI_CODES;

// ******************** IMPORTANT ********************
// Add to the end of each group or be eaten by a Grue!
// Increment this version number if you mess with this
// ******************** IMPORTANT ********************
const quint32 ChannelVersionNumber=0;

const ChannelID EmptyChannel=0;
extern ChannelID CPAP_Obstructive,CPAP_Hypopnea,CPAP_Apnea, CPAP_ClearAirway, CPAP_RERA, CPAP_FlowLimit,
CPAP_CSR, CPAP_VSnore, CPAP_PressurePulse, CPAP_Mode, CPAP_FlowRate, CPAP_MaskPressure, CPAP_Pressure,
CPAP_EPAP, CPAP_IPAP, CPAP_IPAP_Low, CPAP_IPAP_High, CPAP_PressureSupport, CPAP_Snore, CPAP_Leak,
CPAP_RespiratoryRate, CPAP_TidalVolume, CPAP_MinuteVentilation, CPAP_PatientTriggeredBreaths,
CPAP_FlowLimitGraph, CPAP_TherapyPressure, CPAP_ExpiratoryPressure, CPAP_AHI, CPAP_BrokenSummary,
CPAP_BrokenWaveform, CPAP_Pulse, CPAP_SPO2, CPAP_Plethy,CPAP_RespiratoryEvent,CPAP_IE,CPAP_Ti,CPAP_Te;

extern ChannelID RMS9_PressureReliefType, RMS9_PressureReliefSetting, RMS9_Empty1, RMS9_Empty2;
extern ChannelID PRS1_PressureMin,PRS1_PressureMax, PRS1_PressureMinAchieved, PRS1_PressureMaxAchieved,
PRS1_PressureAvg, PRS1_Pressure90, PRS1_RampTime, PRS1_RampPressure, PRS1_HumidifierStatus,
PRS1_HumidifierSetting, PRS1_PressureReliefType, PRS1_PressureReliefSetting, PRS1_SystemOneResistanceStatus,
PRS1_SystemOneResistanceSetting, PRS1_SystemLockStatus, PRS1_HoseDiameter, PRS1_AutoOff, PRS1_AutoOn,
PRS1_MaskAlert, PRS1_ShowAHI, PRS1_Unknown00, PRS1_Unknown01, PRS1_Unknown08, PRS1_Unknown09,
PRS1_Unknown0A, PRS1_Unknown0B, PRS1_Unknown0C, PRS1_Unknown0E, PRS1_Unknown0F, PRS1_Unknown10,
PRS1_Unknown12, PRS1_VSnore2;

extern ChannelID OXI_Pulse, OXI_SPO2, OXI_Plethysomogram, OXI_PulseChange, OXI_SPO2Drop, OXI_Error, OXI_SignalError;
extern ChannelID ZEO_SleepStage, ZEO_Waveform;
extern ChannelID JOURNAL_Notes, JOURNAL_Weight;






// Be cautious when extending these.. add to the end of each groups to preserve file formats.
/*enum ChannelID//:qint16
{
    // General Event Codes
    MC_UNKNOWN=0,

    CPAP_Obstructive, CPAP_Apnea, CPAP_Hypopnea, CPAP_ClearAirway, CPAP_RERA, CPAP_VSnore, CPAP_FlowLimit,
    CPAP_Leak, CPAP_Pressure, CPAP_EAP, CPAP_IAP, CPAP_CSR, CPAP_FlowRate, CPAP_MaskPressure,
    CPAP_Snore,CPAP_MinuteVentilation, CPAP_RespiratoryRate, CPAP_TidalVolume,CPAP_FlowLimitGraph,
    CPAP_PatientTriggeredBreaths, CPAP_PS, CPAP_IAPLO, CPAP_IAPHI,

    // General CPAP Summary Information
    CPAP_PressureMin=0x80, CPAP_PressureMax, CPAP_RampTime, CPAP_RampStartingPressure, CPAP_Mode, CPAP_PressureReliefType,
    CPAP_PressureReliefSetting, CPAP_HumidifierSetting, CPAP_HumidifierStatus, CPAP_PressureMinAchieved,
    CPAP_PressureMaxAchieved, CPAP_PressurePercentValue, CPAP_PressurePercentName, CPAP_PressureAverage, CPAP_PressureMedian,
    CPAP_LeakMedian,CPAP_LeakMinimum,CPAP_LeakMaximum,CPAP_LeakAverage,CPAP_Duration,
    CPAP_SnoreMinimum, CPAP_SnoreMaximum, CPAP_SnoreAverage, CPAP_SnoreMedian, CPAP_TherapyPressure, CPAP_ExpPressure,

    CPAP_Pressure90,CPAP_Leak90,CPAP_AI, CPAP_HI, CPAP_CAI, CPAP_AHI, CPAP_PBI,

    BIPAP_EAPAverage,BIPAP_IAPAverage,BIPAP_EAPMin,BIPAP_EAPMax,BIPAP_IAPMin,BIPAP_IAPMax,
    BIPAP_PSAverage,BIPAP_PSMin, BIPAP_PSMax, BIPAP_PS90, BIPAP_EAP90, BIPAP_IAP90,

    CPAP_PTBMin, CPAP_PTBMax, CPAP_PTBAverage, CPAP_PTB90,
    CPAP_RRMin, CPAP_RRMax, CPAP_RRAverage, CPAP_RR90,
    CPAP_MVMin, CPAP_MVMax, CPAP_MVAverage, CPAP_MV90,
    CPAP_TVMin, CPAP_TVMax, CPAP_TVAverage, CPAP_TV90,

    CPAP_BrokenSummary, CPAP_BrokenWaveform,

    // PRS1 Specific Codes
    PRS1_PressurePulse=0x1000,PRS1_VSnore2,
    PRS1_Unknown00, PRS1_Unknown01, PRS1_Unknown08, PRS1_Unknown09, PRS1_Unknown0B,	PRS1_Unknown0E, PRS1_Unknown10, PRS1_Unknown12,
    PRS1_SystemLockStatus, PRS1_SystemResistanceStatus, PRS1_SystemResistanceSetting, PRS1_HoseDiameter, PRS1_AutoOff, PRS1_MaskAlert, PRS1_ShowAHI,

    // ASV Unknown Codes
    PRS1_Unknown0A,PRS1_Unknown0C, PRS1_Unknown0F,

    ResMed_Empty1, ResMed_Empty2,

    // Oximeter Codes
    OXI_Pulse=0x2000, OXI_SPO2, OXI_Plethy, OXI_Signal2, OXI_SignalGood, OXI_PulseAverage, OXI_PulseMin, OXI_PulseMax, OXI_SPO2Average, OXI_SPO2Min, OXI_SPO2Max,
    OXI_PulseChange, OXI_SPO2Change,

    ZEO_SleepStage=0x2800, ZEO_Waveform,

    GEN_Weight=0x3000, GEN_Notes, GEN_Glucose, GEN_Calories, GEN_Energy, GEN_Mood, GEN_Excercise, GEN_Reminder

};*/
void InitMapsWithoutAwesomeInitializerLists();


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
enum MCDataType//:wxInt8
{ MC_bool=0, MC_int, MC_long, MC_float, MC_double, MC_string, MC_datetime };


#endif // MACHINE_COMMON_H
