/*
 SleepLib Machine Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QApplication>
#include <QDir>
#include <QProgressBar>
#include <QDebug>
#include <QString>
#include <QObject>
#include <tr1/random>
#include <sys/time.h>

#include "machine.h"
#include "profiles.h"
#include <algorithm>

ChannelGroup CPAP_CODES;
ChannelGroup PRS1_CODES;
ChannelGroup RMS9_CODES;
ChannelGroup OXI_CODES;
ChannelGroup ZEO_CODES;
ChannelGroup JOURNAL_CODES;


ChannelID CPAP_Obstructive,CPAP_Hypopnea,CPAP_Apnea, CPAP_ClearAirway, CPAP_RERA, CPAP_FlowLimit,
CPAP_CSR, CPAP_VSnore, CPAP_PressurePulse, CPAP_Mode, CPAP_FlowRate, CPAP_MaskPressure, CPAP_Pressure,
CPAP_EPAP, CPAP_IPAP, CPAP_IPAP_Low, CPAP_IPAP_High, CPAP_PressureSupport, CPAP_Snore, CPAP_Leak,
CPAP_RespiratoryRate, CPAP_TidalVolume, CPAP_MinuteVentilation, CPAP_PatientTriggeredBreaths,
CPAP_FlowLimitGraph, CPAP_TherapyPressure, CPAP_ExpiratoryPressure, CPAP_AHI,CPAP_RespiratoryEvent,
CPAP_IE,CPAP_Ti,CPAP_Te,
CPAP_BrokenSummary,CPAP_BrokenWaveform, CPAP_Pulse, CPAP_SPO2, CPAP_Plethy;

ChannelID RMS9_PressureReliefType, RMS9_PressureReliefSetting, RMS9_Empty1, RMS9_Empty2;
ChannelID PRS1_PressureMin,PRS1_PressureMax, PRS1_PressureMinAchieved, PRS1_PressureMaxAchieved,
PRS1_PressureAvg, PRS1_Pressure90, PRS1_RampTime, PRS1_RampPressure, PRS1_HumidifierStatus,
PRS1_HumidifierSetting, PRS1_PressureReliefType, PRS1_PressureReliefSetting, PRS1_SystemOneResistanceStatus,
PRS1_SystemOneResistanceSetting, PRS1_SystemLockStatus, PRS1_HoseDiameter, PRS1_AutoOff, PRS1_AutoOn,
PRS1_MaskAlert, PRS1_ShowAHI, PRS1_Unknown00, PRS1_Unknown01, PRS1_Unknown08, PRS1_Unknown09,
PRS1_Unknown0A, PRS1_Unknown0B, PRS1_Unknown0C, PRS1_Unknown0E, PRS1_Unknown0F, PRS1_Unknown10,
PRS1_Unknown12, PRS1_VSnore2;

ChannelID OXI_Pulse, OXI_SPO2, OXI_Plethysomogram, OXI_PulseChange, OXI_SPO2Drop, OXI_Error, OXI_SignalError;
ChannelID ZEO_SleepStage, ZEO_Waveform;
ChannelID JOURNAL_Notes, JOURNAL_Weight;

ChannelID ChannelGroup::Get(ChannelType ctype,QString description,QString label,QString lookup,QColor color)
{
    ChannelID id=m_first+m_pos;
    if (++m_pos>=m_size) {
        qCritical("Not enough slots allocated for channel group");
        abort();
    }
    channel[id]=Channel(id,m_type,ctype,description,label,color);
    m_channelsbytype[ctype][id]=channel_lookup[lookup]=m_channel[id]=&channel[id];

    return id;
}
ChannelGroup::ChannelGroup()
{
    m_pos=0; m_first=0xf000, m_size=0x1000;
}
ChannelGroup::ChannelGroup(MachineType type, ChannelID first, ChannelID reserved)
    :m_type(type),m_first(first),m_size(reserved)
{
    m_pos=0;
}


extern QProgressBar * qprogress;

// Master Lists..
QHash<ChannelID, Channel> channel;
QHash<QString,Channel *> channel_lookup;
QHash<QString,ChannelGroup *> channel_group;


void InitMapsWithoutAwesomeInitializerLists()
{
    CPAP_CODES=ChannelGroup(MT_CPAP,0x1000,0x400);
    PRS1_CODES=ChannelGroup(MT_CPAP,0x1400,0x200);
    RMS9_CODES=ChannelGroup(MT_CPAP,0x1600,0x200);
    OXI_CODES=ChannelGroup(MT_OXIMETER,0x2000,0x800);
    ZEO_CODES=ChannelGroup(MT_SLEEPSTAGE,0x2800,0x800);
    JOURNAL_CODES=ChannelGroup(MT_JOURNAL,0x3000,0x800);

    // ******************** IMPORTANT ********************
    // Add to the end of each group or be eaten by a Grue!
    // ******************** IMPORTANT ********************

    // Flagable Events
        CPAP_Obstructive=CPAP_CODES.Get(CT_Event,QObject::tr("Obstructive Apnea"),QObject::tr("OA"),"OA"),
        CPAP_Hypopnea=CPAP_CODES.Get(CT_Event,QObject::tr("Hypopnea"),QObject::tr("H"),"OA"),
        CPAP_Apnea=CPAP_CODES.Get(CT_Event,QObject::tr("Unspecified Apnea"),QObject::tr("UA"),"UA"),
        CPAP_ClearAirway=CPAP_CODES.Get(CT_Event,QObject::tr("Clear Airway Apnea"),QObject::tr("CA"),"CA"),
        CPAP_RERA=CPAP_CODES.Get(CT_Event,QObject::tr("RERA"),QObject::tr("RE"),"RE"),
        CPAP_FlowLimit=CPAP_CODES.Get(CT_Event,QObject::tr("Flow Limitation"),QObject::tr("FL"),"FL"),
        CPAP_CSR=CPAP_CODES.Get(CT_Event,QObject::tr("Periodic Breathing"),QObject::tr("PB"),"PB"),
        CPAP_VSnore=CPAP_CODES.Get(CT_Event,QObject::tr("Vibratory Snore"),QObject::tr("VS"),"VS"),
        CPAP_PressurePulse=CPAP_CODES.Get(CT_Event,QObject::tr("Pressure Pulse"),QObject::tr("PP"),"PP"),
        CPAP_Mode=CPAP_CODES.Get(CT_Lookup,QObject::tr("CPAP Mode"),QObject::tr("CPAPMode"),"CPAPMode"),
        // Alternate names at the end of each section

    // Graphable Events & Waveforms
        CPAP_FlowRate=CPAP_CODES.Get(CT_Graph,QObject::tr("Flow Rate Waveform"),QObject::tr("Flow Rate"),"FR"),
        CPAP_MaskPressure=CPAP_CODES.Get(CT_Graph,QObject::tr("Mask Pressure"),QObject::tr("Mask Pressure"),"MP"),
        CPAP_Pressure=CPAP_CODES.Get(CT_Graph,QObject::tr("Pressure"),QObject::tr("Pressure"),"P"),
        CPAP_EPAP=CPAP_CODES.Get(CT_Graph,QObject::tr("Expiratory Pressure"),QObject::tr("EPAP"),"EPAP"),
        CPAP_IPAP=CPAP_CODES.Get(CT_Graph,QObject::tr("Inhalation Pressure"),QObject::tr("IPAP"),"IPAP"),
        CPAP_IPAP_Low=CPAP_CODES.Get(CT_Graph,QObject::tr("IPAP Low"),QObject::tr("IPAP Low"),"IPAPL"),
        CPAP_IPAP_High=CPAP_CODES.Get(CT_Graph,QObject::tr("IPAP Low"),QObject::tr("IPAP High"),"IPAPH"),
        CPAP_PressureSupport=CPAP_CODES.Get(CT_Graph,QObject::tr("Pressure Support"),QObject::tr("Pressure Support"),"PS"),
        CPAP_Snore=CPAP_CODES.Get(CT_Graph,QObject::tr("Snore"),QObject::tr("Snore"),"Snore"),
        CPAP_Leak=CPAP_CODES.Get(CT_Graph,QObject::tr("Leak Rate"),QObject::tr("Leak"),"Leak"),
        CPAP_RespiratoryRate=CPAP_CODES.Get(CT_Graph,QObject::tr("Respiratory Rate"),QObject::tr("Resp. Rate"),"RR"),
        CPAP_TidalVolume=CPAP_CODES.Get(CT_Graph,QObject::tr("Tidal Volume"),QObject::tr("Tidal Volume"),"TV"),
        CPAP_MinuteVentilation=CPAP_CODES.Get(CT_Graph,QObject::tr("Minute Ventilation"),QObject::tr("Minute Vent."),"MV"),
        CPAP_PatientTriggeredBreaths=CPAP_CODES.Get(CT_Graph,QObject::tr("Patient Triggered Breaths"),QObject::tr("Patient Trig Breaths"),"PTB"),
        CPAP_FlowLimitGraph=CPAP_CODES.Get(CT_Graph,QObject::tr("Flow Limitation Graph"),QObject::tr("Flow Limit."),"FLG"),
        CPAP_TherapyPressure=CPAP_CODES.Get(CT_Graph,QObject::tr("Therapy Pressure"),QObject::tr("Therapy Pressure"),"TP"),
        CPAP_ExpiratoryPressure=CPAP_CODES.Get(CT_Graph,QObject::tr("Expiratory Pressure"),QObject::tr("Expiratory Pressure"),"EXP"),
        CPAP_RespiratoryEvent=CPAP_CODES.Get(CT_Graph,QObject::tr("Respiratory Event"),QObject::tr("Respiratory Event"),"RESPEv"),
        CPAP_IE=CPAP_CODES.Get(CT_Graph,QObject::tr("I:E"),QObject::tr("I:E"),"IE"),
        CPAP_Ti=CPAP_CODES.Get(CT_Graph,QObject::tr("Ti"),QObject::tr("Ti"),"Ti"),
        CPAP_Te=CPAP_CODES.Get(CT_Graph,QObject::tr("Te"),QObject::tr("Te"),"Te"),


        CPAP_AHI=CPAP_CODES.Get(CT_Calculation,QObject::tr("Apnea-Hypopnea Index"),QObject::tr("AHI"),"AHI"),
        /*CPAP_AI=CPAP_CODES.Get(CT_Calculation,QObject::tr("Apnea Index"),QObject::tr("AI"),"AI"),
        CPAP_HI=CPAP_CODES.Get(CT_Calculation,QObject::tr("Hypopnea Index"),QObject::tr("HI"),"HI"),
        CPAP_CAI=CPAP_CODES.Get(CT_Calculation,QObject::tr("Central Apnea Index"),QObject::tr("CAI"),"CAI"),
        CPAP_RAI=CPAP_CODES.Get(CT_Calculation,QObject::tr("RERA+AHI Index"),QObject::tr("RAI"),"RAI"),
        CPAP_RI=CPAP_CODES.Get(CT_Calculation,QObject::tr("RERA Index"),QObject::tr("RI"),"RI"),
        CPAP_FLI=CPAP_CODES.Get(CT_Calculation,QObject::tr("Flow Limitation Index"),QObject::tr("FLI"),"FLI"),
        CPAP_VSI=CPAP_CODES.Get(CT_Calculation,QObject::tr("Vibratory Snore Index"),QObject::tr("VSI"),"VSI"),
        CPAP_PBI=CPAP_CODES.Get(CT_Calculation,QObject::tr("% Night in Periodic Breathing"),QObject::tr("PBI"),"PBI"), */
        //CPAP_PressureReliefType=CPAP_CODES.Get(CT_Lookup,QObject::tr("Pressure Relief Type"),"","PRType"),
        //CPAP_PressureReliefSetting=CPAP_CODES.Get(CT_Lookup,QObject::tr("Pressure Relief Setting"),"","PRSet"),
        CPAP_BrokenSummary=CPAP_CODES.Get(CT_Boolean,QObject::tr("Broken Summary"),QObject::tr(""),"BrokenSummary"),
        CPAP_BrokenWaveform=CPAP_CODES.Get(CT_Boolean,QObject::tr("Broken Waveform"),QObject::tr(""),"BrokenWaveform");


        RMS9_PressureReliefType=RMS9_CODES.Get(CT_Lookup,QObject::tr("Pressure Relief Type"),"","RMS9_PRType"),
        RMS9_PressureReliefSetting=RMS9_CODES.Get(CT_Decimal,QObject::tr("Pressure Relief Setting"),"","RMS9_PRSet"),
        RMS9_Empty1=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown Empty 1"),"","RMS9_UE1"),
        RMS9_Empty2=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown Empty 2"),"","RMS9_UE2"),

        PRS1_PressureMin=PRS1_CODES.Get(CT_Decimal,QObject::tr("Minimum Pressure"),"","PRS1_MinPres"),
        PRS1_PressureMax=PRS1_CODES.Get(CT_Decimal,QObject::tr("Maximum Pressure"),"","PRS1_MaxPres"),
        PRS1_PressureMinAchieved=PRS1_CODES.Get(CT_Decimal,QObject::tr("Minimum Pressure Achieved"),"","PRS1_MinPresAch"),
        PRS1_PressureMaxAchieved=PRS1_CODES.Get(CT_Decimal,QObject::tr("Maximum Pressure Achieved"),"","PRS1_MaxPresAch"),
        PRS1_PressureAvg=PRS1_CODES.Get(CT_Decimal,QObject::tr("Average Pressure"),"","PRS1_AvgPres"),
        PRS1_Pressure90=PRS1_CODES.Get(CT_Decimal,QObject::tr("90% Pressure"),"","PRS1_90Pres"),
        PRS1_RampTime=PRS1_CODES.Get(CT_Timespan,QObject::tr("Ramp Time"),"","PRS1_RampTime"),
        PRS1_RampPressure=PRS1_CODES.Get(CT_Decimal,QObject::tr("Ramp Pressure"),"","PRS1_RampPressure"),
        PRS1_HumidifierStatus=PRS1_CODES.Get(CT_Boolean,QObject::tr("Humidifier Status"),"","PRS1_HumiStat"),
        PRS1_HumidifierSetting=PRS1_CODES.Get(CT_Lookup,QObject::tr("Humidifier Setting"),"","PRS1_HumiSet"),
        PRS1_PressureReliefType=PRS1_CODES.Get(CT_Lookup,QObject::tr("Pressure Relief Type"),"","PRS1_PRType"),
        PRS1_PressureReliefSetting=PRS1_CODES.Get(CT_Lookup,QObject::tr("Pressure Relief Setting"),"","PRS1_PRSet"),
        PRS1_SystemOneResistanceStatus=PRS1_CODES.Get(CT_Boolean,QObject::tr("System-One Resistance Status"),"","PRS1_ResistStat"),
        PRS1_SystemOneResistanceSetting=PRS1_CODES.Get(CT_Lookup,QObject::tr("System-One Resistance Setting"),"","PRS1_ResistSet"),
        PRS1_SystemLockStatus=PRS1_CODES.Get(CT_Boolean,QObject::tr("System Lock Status"),"","PRS1_SysLockStat"),
        PRS1_HoseDiameter=PRS1_CODES.Get(CT_Lookup,QObject::tr("Hose Diameter"),"","PRS1_HoseDiam"),
        PRS1_AutoOff=PRS1_CODES.Get(CT_Boolean,QObject::tr("Auto Off"),"","PRS1_AutoOff"),
        PRS1_AutoOn=PRS1_CODES.Get(CT_Boolean,QObject::tr("Auto On"),"","PRS1_AutoOn"),
        PRS1_MaskAlert=PRS1_CODES.Get(CT_Boolean,QObject::tr("Mask Alert"),"","PRS1_MaskAlert"),
        PRS1_ShowAHI=PRS1_CODES.Get(CT_Boolean,QObject::tr("Show AHI"),"","PRS1_ShowAHI"),
        PRS1_Unknown00=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 00 Event"),QObject::tr("U00"),"PRS1_U00"),
        PRS1_Unknown01=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 01 Event"),QObject::tr("U01"),"PRS1_U01"),
        PRS1_Unknown08=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 08 Event"),QObject::tr("U08"),"PRS1_U08"),
        PRS1_Unknown09=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 09 Event"),QObject::tr("U09"),"PRS1_U09"),
        PRS1_Unknown0A=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 0A Event"),QObject::tr("U0A"),"PRS1_U0A"),
        PRS1_Unknown0B=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 0B Event"),QObject::tr("U0B"),"PRS1_U0B"),
        PRS1_Unknown0C=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 0C Event"),QObject::tr("U0C"),"PRS1_U0C"),
        PRS1_Unknown0E=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 0E Event"),QObject::tr("U0E"),"PRS1_U0E"),
        PRS1_Unknown0F=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 0F Event"),QObject::tr("U0F"),"PRS1_U0F"),
        PRS1_Unknown10=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 10 Event"),QObject::tr("U10"),"PRS1_U10"),
        PRS1_Unknown12=CPAP_CODES.Get(CT_Event,QObject::tr("Unknown 12 Event"),QObject::tr("U12"),"PRS1_U12"),
        PRS1_VSnore2=CPAP_CODES.Get(CT_Event,QObject::tr("Vibratory Snore (Type 2)"),QObject::tr("VS2"),"VS2");

        // CPAP Integrated oximetery codes..
        CPAP_Pulse=CPAP_CODES.Get(CT_Graph,QObject::tr("Pulse Rate"),QObject::tr("Pulse"),"CPPR");
        CPAP_SPO2=CPAP_CODES.Get(CT_Graph,QObject::tr("SpO2"),QObject::tr("SpO2"),"CPSP");
        CPAP_Plethy=CPAP_CODES.Get(CT_Graph,QObject::tr("Plethysomagram"),QObject::tr("Plethysomagram"),"CPPL");

        OXI_Pulse=OXI_CODES.Get(CT_Graph,QObject::tr("Pulse Rate"),QObject::tr("PR"),"PR"),
        OXI_SPO2=OXI_CODES.Get(CT_Graph,QObject::tr("Oxygen Saturation"),QObject::tr("SPO2"),"SPO2"),
        OXI_Plethysomogram=OXI_CODES.Get(CT_Graph,QObject::tr("Plethysomogram"),QObject::tr("PLE"),"PLE"),

        OXI_PulseChange=OXI_CODES.Get(CT_Event,QObject::tr("Pulse Change"),QObject::tr("PC"),"PC"),
        OXI_SPO2Drop=OXI_CODES.Get(CT_Event,QObject::tr("Oxigen Saturation Drop"),QObject::tr("SD"),"SD"),
        OXI_Error=OXI_CODES.Get(CT_Event,QObject::tr("Oximeter Error"),QObject::tr("ER1"),"ER1"),
        OXI_SignalError=OXI_CODES.Get(CT_Event,QObject::tr("Oximeter Signal Error"),QObject::tr("ER2"),"ER2");


        ZEO_SleepStage=ZEO_CODES.Get(CT_Graph,QObject::tr("Sleep Stage"),QObject::tr("SS"),"SS"),
        ZEO_Waveform=ZEO_CODES.Get(CT_Graph,QObject::tr("Brainwaves"),QObject::tr("BW"),"BW");



        JOURNAL_Notes=JOURNAL_CODES.Get(CT_Text,"Journal/Notes","JN"),
        JOURNAL_Weight=JOURNAL_CODES.Get(CT_Decimal,"Weight","W");

    channel[CPAP_Mode].addOption("CPAP");
    channel[CPAP_Mode].addOption("Auto");
    channel[CPAP_Mode].addOption("BIPAP/VPAP");
    channel[CPAP_Mode].addOption("ASV");

    channel[PRS1_HoseDiameter].addOption("22mm");
    channel[PRS1_HoseDiameter].addOption("15mm");
    channel[PRS1_HumidifierSetting].addOption("Off");
    channel[PRS1_HumidifierSetting].addOption("1");
    channel[PRS1_HumidifierSetting].addOption("2");
    channel[PRS1_HumidifierSetting].addOption("3");
    channel[PRS1_HumidifierSetting].addOption("4");
    channel[PRS1_HumidifierSetting].addOption("5");
    channel[PRS1_SystemOneResistanceSetting].addOption("x1");
    channel[PRS1_SystemOneResistanceSetting].addOption("x2");
    channel[PRS1_SystemOneResistanceSetting].addOption("x3");
    channel[PRS1_SystemOneResistanceSetting].addOption("x4");
    channel[PRS1_SystemOneResistanceSetting].addOption("x5");
    channel[PRS1_PressureReliefType].addOption("None");
    channel[PRS1_PressureReliefType].addOption("C-Flex");
    channel[PRS1_PressureReliefType].addOption("C-Flex+");
    channel[PRS1_PressureReliefType].addOption("A-Flex");
    channel[PRS1_PressureReliefType].addOption("Bi-Flex");
    channel[PRS1_PressureReliefType].addOption("Bi-Flex+");
    channel[PRS1_PressureReliefSetting].addOption("Off");
    channel[PRS1_PressureReliefSetting].addOption("1");
    channel[PRS1_PressureReliefSetting].addOption("2");
    channel[PRS1_PressureReliefSetting].addOption("3");
    channel[RMS9_PressureReliefType].addOption("None");
    channel[RMS9_PressureReliefType].addOption("EPR");
    channel[RMS9_PressureReliefSetting].addOption("0");
    channel[RMS9_PressureReliefSetting].addOption("0.5");
    channel[RMS9_PressureReliefSetting].addOption("1");
    channel[RMS9_PressureReliefSetting].addOption("1.5");
    channel[RMS9_PressureReliefSetting].addOption("2");
    channel[RMS9_PressureReliefSetting].addOption("2.5");
    channel[RMS9_PressureReliefSetting].addOption("3.0");

    channel[ZEO_SleepStage].addOption("Unknown");
    channel[ZEO_SleepStage].addOption("Awake");
    channel[ZEO_SleepStage].addOption("REM");
    channel[ZEO_SleepStage].addOption("Light Sleep");
    channel[ZEO_SleepStage].addOption("Deep Sleep");

/*    CPAPModeNames[MODE_UNKNOWN]=QObject::tr("Undetermined");
    CPAPModeNames[MODE_CPAP]=QObject::tr("CPAP");
    CPAPModeNames[MODE_APAP]=QObject::tr("Auto");
    CPAPModeNames[MODE_BIPAP]=QObject::tr("BIPAP");
    CPAPModeNames[MODE_ASV]=QObject::tr("ASV");

    PressureReliefNames[PR_UNKNOWN]=QObject::tr("Undetermined");
    PressureReliefNames[PR_NONE]=QObject::tr("None");
    PressureReliefNames[PR_CFLEX]=QObject::tr("C-Flex");
    PressureReliefNames[PR_CFLEXPLUS]=QObject::tr("C-Flex+");
    PressureReliefNames[PR_AFLEX]=QObject::tr("A-Flex");
    PressureReliefNames[PR_BIFLEX]=QObject::tr("Bi-Flex");
    PressureReliefNames[PR_EPR]=QObject::tr("Exhalation Pressure Relief (EPR)");
    PressureReliefNames[PR_SMARTFLEX]=QObject::tr("SmartFlex");

    DefaultMCShortNames[CPAP_Obstructive]=QObject::tr("OA");
    DefaultMCShortNames[CPAP_Apnea]=QObject::tr("A");
    DefaultMCShortNames[CPAP_Hypopnea]=QObject::tr("H");
    DefaultMCShortNames[CPAP_RERA]=QObject::tr("RE");
    DefaultMCShortNames[CPAP_ClearAirway]=QObject::tr("CA");
    DefaultMCShortNames[CPAP_CSR]=QObject::tr("CSR/PB");
    DefaultMCShortNames[CPAP_VSnore]=QObject::tr("VS");
    DefaultMCShortNames[PRS1_VSnore2]=QObject::tr("VS2");
    DefaultMCShortNames[CPAP_FlowLimit]=QObject::tr("FL");
    DefaultMCShortNames[CPAP_Pressure]=QObject::tr("P");
    DefaultMCShortNames[CPAP_Leak]=QObject::tr("LR");
    DefaultMCShortNames[CPAP_EAP]=QObject::tr("EPAP");
    DefaultMCShortNames[CPAP_IAP]=QObject::tr("IPAP");
    DefaultMCShortNames[PRS1_PressurePulse]=QObject::tr("PP");

    DefaultMCLongNames[CPAP_Obstructive]=QObject::tr("Obstructive Apnea");
    DefaultMCLongNames[CPAP_Hypopnea]=QObject::tr("Hypopnea");
    DefaultMCLongNames[CPAP_Apnea]=QObject::tr("Apnea");
    DefaultMCLongNames[CPAP_RERA]=QObject::tr("RERA");
    DefaultMCLongNames[CPAP_ClearAirway]=QObject::tr("Clear Airway Apnea");
    DefaultMCLongNames[CPAP_CSR]=QObject::tr("Periodic Breathing");
    DefaultMCLongNames[CPAP_VSnore]=QObject::tr("Vibratory Snore"); // flags type
    DefaultMCLongNames[CPAP_FlowLimit]=QObject::tr("Flow Limitation");
    DefaultMCLongNames[CPAP_Pressure]=QObject::tr("Pressure");
    DefaultMCLongNames[CPAP_Leak]=QObject::tr("Leak Rate");
    DefaultMCLongNames[CPAP_PS]=QObject::tr("Pressure Support");
    DefaultMCLongNames[CPAP_EAP]=QObject::tr("BIPAP EPAP");
    DefaultMCLongNames[CPAP_IAP]=QObject::tr("BIPAP IPAP");
    DefaultMCLongNames[CPAP_Snore]=QObject::tr("Vibratory Snore");  // Graph data
    DefaultMCLongNames[PRS1_VSnore2]=QObject::tr("Vibratory Snore (Graph)");
    DefaultMCLongNames[PRS1_PressurePulse]=QObject::tr("Pressure Pulse");
    DefaultMCLongNames[PRS1_Unknown0E]=QObject::tr("Unknown 0E");
    DefaultMCLongNames[PRS1_Unknown00]=QObject::tr("Unknown 00");
    DefaultMCLongNames[PRS1_Unknown01]=QObject::tr("Unknown 01");
    DefaultMCLongNames[PRS1_Unknown0B]=QObject::tr("Unknown 0B");
    DefaultMCLongNames[PRS1_Unknown10]=QObject::tr("Unknown 10"); */
}


//////////////////////////////////////////////////////////////////////////////////////////
// Machine Base-Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Machine::Machine(Profile *p,MachineID id)
{
    day.clear();
    highest_sessionid=0;
    profile=p;
    if (!id) {
        std::tr1::minstd_rand gen;
        std::tr1::uniform_int<MachineID> unif(1, 0x7fffffff);
        gen.seed((unsigned int) time(NULL));
        MachineID temp;
        do {
            temp = unif(gen); //unif(gen) << 32 |
        } while (profile->machlist.find(temp)!=profile->machlist.end());

        m_id=temp;

    } else m_id=id;
    qDebug() << "Create Machine: " << hex << m_id; //%lx",m_id);
    m_type=MT_UNKNOWN;
    firstsession=true;
}
Machine::~Machine()
{
    qDebug() << "Destroy Machine";
    for (QMap<QDate,Day *>::iterator d=day.begin();d!=day.end();d++) {
        delete d.value();
    }
}
Session *Machine::SessionExists(SessionID session)
{
    if (sessionlist.find(session)!=sessionlist.end()) {
        return sessionlist[session];
    } else {
        return NULL;
    }
}

Day *Machine::AddSession(Session *s,Profile *p)
{
    if (!s) {
        qWarning() << "Empty Session in Machine::AddSession()";
        return NULL;
    }
    if (!p) {
        qWarning() << "Empty Profile in Machine::AddSession()";
        return NULL;
    }
    if (s->session()>highest_sessionid)
        highest_sessionid=s->session();


    QTime split_time(12,0,0);
    if (pref.Exists("DaySplitTime")) {
        split_time=pref["DaySplitTime"].toTime();
    }
    int combine_sessions;
    if (pref.Exists("CombineCloserSessions")) {
        combine_sessions=pref["CombineCloserSessions"].toInt(); // In Minutes
    } else combine_sessions=0;

    int ignore_sessions;
    if (pref.Exists("IgnoreShorterSessions")) {
        ignore_sessions=pref["IgnoreShorterSessions"].toInt(); // In Minutes
    } else ignore_sessions=0;

    int session_length=s->last()-s->first();
    session_length/=60000;

    sessionlist[s->session()]=s; // To make sure it get's saved later even if it's not wanted.

    QDateTime d1,d2=QDateTime::fromTime_t(s->first()/1000);

    QDate date=d2.date();
    QTime time=d2.time();

    QMap<QDate,Day *>::iterator dit,nextday;


    bool combine_next_day=false;
    int closest_session=0;

    if (time<split_time) {
        date=date.addDays(-1);
    } else if (combine_sessions > 0) {
        dit=day.find(date.addDays(-1)); // Check Day Before
        if (dit!=day.end()) {
            QDateTime lt=QDateTime::fromTime_t(dit.value()->last()/1000);
            closest_session=lt.secsTo(d2)/60;
            if (closest_session<combine_sessions) {
                date=date.addDays(-1);
            }
        } else {
            nextday=day.find(date.addDays(1));// Check Day Afterwards
            if (nextday!=day.end()) {
                QDateTime lt=QDateTime::fromTime_t(nextday.value()->first()/1000);
                closest_session=d2.secsTo(lt)/60;
                if (closest_session < combine_sessions) {
                    // add todays here. pull all tomorrows records to this date.
                    combine_next_day=true;
                }
            }
        }
    }

    if (session_length<ignore_sessions) {
        //if (!closest_session || (closest_session>=60))
        return NULL;
    }

    if (!firstsession) {
        if (firstday>date) firstday=date;
        if (lastday<date) lastday=date;
    } else {
        firstday=lastday=date;
        firstsession=false;
    }


    Day *dd=NULL;
    dit=day.find(date);
    if (dit==day.end()) {
        //QString dstr=date.toString("yyyyMMdd");
        //qDebug("Adding Profile Day %s",dstr.toAscii().data());
        dd=new Day(this);
        day[date]=dd;
        // Add this Day record to profile
        p->AddDay(date,dd,m_type);
    } else {
        dd=*dit;
    }
    dd->AddSession(s);

    if (combine_next_day) {
        for (QVector<Session *>::iterator i=nextday.value()->begin();i!=nextday.value()->end();i++) {
            dd->AddSession(*i);
        }
        QMap<QDate,QVector<Day *> >::iterator nd=p->daylist.find(date.addDays(1));
        for (QVector<Day *>::iterator i=nd->begin();i!=nd->end();i++) {
            if (*i==nextday.value()) {
                nd.value().erase(i);
            }
        }
        day.erase(nextday);
    }
    return dd;
}

// This functions purpose is murder and mayhem... It deletes all of a machines data.
// Therefore this is the most dangerous function in this software..
bool Machine::Purge(int secret)
{
    // Boring api key to stop this function getting called by accident :)
    if (secret!=3478216) return false;


    // It would be joyous if this function screwed up..
    QString path=profile->Get("DataFolder")+"/"+hexid();

    QDir dir(path);

    if (!dir.exists()) // It doesn't exist anyway.
        return true;
    if (!dir.isReadable())
        return false;


    qDebug() << "Purging " << QDir::toNativeSeparators(path);

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QFileInfoList list=dir.entryInfoList();
    int could_not_kill=0;

    for (int i=0;i<list.size();++i) {
        QFileInfo fi=list.at(i);
        QString fullpath=fi.canonicalFilePath();
        //int j=fullpath.lastIndexOf(".");

        QString ext_s=fullpath.section('.',-1);//right(j);
        bool ok;
        ext_s.toInt(&ok,10);
        if (ok) {
            qDebug() << "Deleting " << fullpath;
            dir.remove(fullpath);
        } else could_not_kill++;

    }
    dir.remove(path+"/channels.dat");
    if (could_not_kill>0) {
      //  qWarning() << "Could not purge path\n" << path << "\n\n" << could_not_kill << " file(s) remain.. Suggest manually deleting this path\n";
    //    return false;
    }

    return true;
}

const quint32 channel_version=1;


bool Machine::Load()
{
    QString path=profile->Get("DataFolder")+"/"+hexid();

    QDir dir(path);
    qDebug() << "Loading " << path;

    if (!dir.exists() || !dir.isReadable())
        return false;

    QString fn=path+"/channels.dat";
    QFile cf(fn);
    cf.open(QIODevice::ReadOnly);
    QDataStream in(&cf);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 tmp;
    in >> tmp;
    if (magic!=tmp) {
        qDebug() << "Machine Channel file format is wrong" << fn;
    }
    in >> tmp;
    if (tmp!=channel_version) {
        qDebug() << "Machine Channel file format is wrong" << fn;
    }
    qint32 tmp2;
    in >> tmp2;
    if (tmp2!=m_id) {
        qDebug() << "Machine Channel file format is wrong" << fn;
    }
    in >> m_channels;
    cf.close();


    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);

    QFileInfoList list=dir.entryInfoList();

    typedef QVector<QString> StringList;
    QMap<SessionID,StringList> sessfiles;
    QMap<SessionID,StringList>::iterator s;

    QString fullpath,ext_s,sesstr;
    int ext;
    SessionID sessid;
    bool ok;
    for (int i=0;i<list.size();i++) {
        QFileInfo fi=list.at(i);
        fullpath=fi.canonicalFilePath();
        ext_s=fi.fileName().section(".",-1);
        ext=ext_s.toInt(&ok,10);
        if (!ok) continue;
        sesstr=fi.fileName().section(".",0,-2);
        sessid=sesstr.toLong(&ok,16);
        if (!ok) continue;
        if (sessfiles[sessid].capacity()==0) sessfiles[sessid].resize(3);
        sessfiles[sessid][ext]=fi.canonicalFilePath();
    }

    int size=sessfiles.size();
    int cnt=0;
    for (s=sessfiles.begin(); s!=sessfiles.end(); s++) {
        cnt++;
        if ((cnt % 10)==0)
            if (qprogress) qprogress->setValue((float(cnt)/float(size)*100.0));

        Session *sess=new Session(this,s.key());

        if (sess->LoadSummary(s.value()[0])) {
             sess->SetEventFile(s.value()[1]);
             AddSession(sess,profile);
        } else {
            qWarning() << "Error unpacking summary data";
            delete sess;
        }
    }
    if (qprogress) qprogress->setValue(100);
    return true;
}
bool Machine::SaveSession(Session *sess)
{
    QString path=profile->Get("DataFolder")+"/"+hexid();
    if (sess->IsChanged()) sess->Store(path);
    return true;
}

bool Machine::Save()
{
    int size=0;
    int cnt=0;

    QString path=profile->Get("DataFolder")+"/"+hexid();
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkdir(path);
    }

    QString fn=path+"/channels.dat";
    QFile cf(fn);
    if (cf.open(QIODevice::WriteOnly)) {
        int i=4;
    }
    QDataStream out(&cf);
    out.setVersion(QDataStream::Qt_4_6);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)magic;          // Magic Number
    out << (quint32)channel_version;// File Version
    out << (quint32)m_id;// Machine ID

    out << m_channels;
    cf.close();


    // Calculate size for progress bar
    size=sessionlist.size();

    QHash<SessionID,Session *>::iterator s;

    m_savelist.clear();
    for (s=sessionlist.begin(); s!=sessionlist.end(); s++) {
        cnt++;
        if ((*s)->IsChanged()) {
            m_savelist.push_back(*s);
            //(*s)->UpdateSummaries();
            //(*s)->Store(path);
            //(*s)->TrashEvents();
        }
    }
    savelistCnt=0;
    savelistSize=m_savelist.size();
    if (!pref["EnableMultithreading"].toBool()) {
        for (int i=0;i<savelistSize;i++) {
            qprogress->setValue(66.0+(float(savelistCnt)/float(savelistSize)*33.0));
            QApplication::processEvents();
            Session *s=m_savelist.at(i);
            s->UpdateSummaries();
            s->Store(path);
            s->TrashEvents();
            savelistCnt++;

        }
        return true;
    }
    int threads=QThread::idealThreadCount();
    savelistSem=new QSemaphore(threads);
    savelistSem->acquire(threads);
    QVector<SaveThread*>thread;
    for (int i=0;i<threads;i++) {
        thread.push_back(new SaveThread(this,path));
        QObject::connect(thread[i],SIGNAL(UpdateProgress(int)),qprogress,SLOT(setValue(int)));
        thread[i]->start();
    }
    while (!savelistSem->tryAcquire(threads,250)) {
        //qDebug() << savelistSem->available();
        if (qprogress) {
        //    qprogress->setValue(66.0+(float(savelistCnt)/float(savelistSize)*33.0));
           QApplication::processEvents();
        }
    }

    for (int i=0;i<threads;i++) {
        while (thread[i]->isRunning()) {
            SaveThread::msleep(250);
            QApplication::processEvents();
        }
        delete thread[i];
    }

    delete savelistSem;
    return true;
}

/*SaveThread::SaveThread(Machine *m,QString p)
{
    machine=m;
    path=p;
} */

void SaveThread::run()
{
    while (Session *sess=machine->popSaveList()) {
        int i=66.0+(float(machine->savelistCnt)/float(machine->savelistSize)*33.0);
        emit UpdateProgress(i);
        sess->UpdateSummaries();
        sess->Store(path);
        sess->TrashEvents();
    }
    machine->savelistSem->release(1);
}

Session *Machine::popSaveList()
{

    Session *sess=NULL;
    savelistMutex.lock();
    if (m_savelist.size()>0) {
        sess=m_savelist.at(0);
        m_savelist.pop_front();
        savelistCnt++;
    }
    savelistMutex.unlock();
    return sess;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CPAP implmementation
//////////////////////////////////////////////////////////////////////////////////////////
CPAP::CPAP(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_CPAP;

    registerChannel(CPAP_Obstructive);
    registerChannel(CPAP_Hypopnea);
    registerChannel(CPAP_ClearAirway);
    registerChannel(CPAP_Snore);
    registerChannel(CPAP_Leak);
}

CPAP::~CPAP()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Oximeter Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
Oximeter::Oximeter(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_OXIMETER;
    registerChannel(OXI_Pulse);
    registerChannel(OXI_SPO2);
    registerChannel(OXI_Plethysomogram);
}

Oximeter::~Oximeter()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// SleepStage Class implmementation
//////////////////////////////////////////////////////////////////////////////////////////
SleepStage::SleepStage(Profile *p,MachineID id):Machine(p,id)
{
    m_type=MT_SLEEPSTAGE;
}
SleepStage::~SleepStage()
{
}





