#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QColor>
#include <QObject>
#include "version.h"

#if QT_VERSION >= QT_VERSION_CHECK(4,8,0)

#define DEBUG_EFFICIENCY 1

#endif

enum UnitSystem { US_Undefined, US_Metric, US_Archiac };

typedef float EventDataType;

struct ValueCount {
    ValueCount() { value=0; count=0; p=0; }
    ValueCount(const ValueCount & copy) {
        value=copy.value;
        count=copy.count;
        p=copy.p;
    }
    EventDataType value;
    qint64 count;
    double p;
};

// Primarily sort by value
bool operator <(const ValueCount & a, const ValueCount & b);

const float ounce_convert=28.3495231F; // grams
const float pound_convert=ounce_convert*16;

QString weightString(float kg, UnitSystem us=US_Undefined);

//! \brief Mercilessly trash a directory
bool removeDir(const QString & path);

///////////////////////////////////////////////////////////////////////////////////////////////
// Preference Name Strings
///////////////////////////////////////////////////////////////////////////////////////////////

const QString STR_GEN_Profile="Profile";
const QString STR_GEN_SkipLogin="SkipLoginScreen";
const QString STR_GEN_UpdatesLastChecked="UpdatesLastChecked";
const QString STR_GEN_UpdatesAutoCheck="Updates_AutoCheck";
const QString STR_GEN_UpdateCheckFrequency="Updates_CheckFrequency";
const QString STR_GEN_DataFolder="DataFolder";

const QString STR_GEN_On=QObject::tr("On");
const QString STR_GEN_Off=QObject::tr("Off");

const QString STR_PREF_AllowEarlyUpdates="AllowEarlyUpdates";

const QString STR_PROP_Brand="Brand";
const QString STR_PROP_Model="Model";
const QString STR_PROP_Series="Series";
const QString STR_PROP_ModelNumber="ModelNumber";
const QString STR_PROP_SubModel="SubModel";
const QString STR_PROP_Serial="Serial";
const QString STR_PROP_DataVersion="DataVersion";
const QString STR_PROP_Path="Path";
const QString STR_PROP_BackupPath="BackupPath";
const QString STR_PROP_LastImported="LastImported";

const QString STR_MACH_ResMed="ResMed";
const QString STR_MACH_PRS1="PRS1";
const QString STR_MACH_Journal="Journal";
const QString STR_MACH_Intellipap="Intellipap";
const QString STR_MACH_FPIcon="FPIcon";
const QString STR_MACH_MSeries="MSeries";
const QString STR_MACH_CMS50="CMS50";
const QString STR_MACH_ZEO="Zeo";

const QString STR_PREF_VersionString="VersionString";
const QString STR_PREF_Language="Language";


///////////////////////////////////////////////////////////////////////////////////////////////
// Commonly used translatable text strings
///////////////////////////////////////////////////////////////////////////////////////////////

const QString STR_UNIT_CM=QObject::tr("cm");
const QString STR_UNIT_INCH=QObject::tr("\"");
const QString STR_UNIT_FOOT=QObject::tr("ft");
const QString STR_UNIT_POUND=QObject::tr("lb");
const QString STR_UNIT_OUNCE=QObject::tr("oz");
const QString STR_UNIT_KG=QObject::tr("Kg");
const QString STR_UNIT_CMH2O=QObject::tr("cmH2O");
const QString STR_UNIT_Hours=QObject::tr("Hours");

const QString STR_UNIT_BPM=QObject::tr("bpm");          // Beats per Minute
const QString STR_UNIT_LPM=QObject::tr("L/m");          // Litres per Minute

const QString STR_MESSAGE_ERROR=QObject::tr("Error");
const QString STR_MESSAGE_WARNING=QObject::tr("Warning");

const QString STR_TR_BMI=QObject::tr("BMI");                // Short form of Body Mass Index
const QString STR_TR_Weight=QObject::tr("Weight");
const QString STR_TR_Zombie=QObject::tr("Zombie");
const QString STR_TR_PulseRate=QObject::tr("Pulse Rate");   // Pulse / Heart rate
const QString STR_TR_SpO2=QObject::tr("SpO2");
const QString STR_TR_Plethy=QObject::tr("Plethy");          // Plethysomogram
const QString STR_TR_Pressure=QObject::tr("Pressure");

const QString STR_TR_Daily=QObject::tr("Daily");
const QString STR_TR_Overview=QObject::tr("Overview");
const QString STR_TR_Oximetry=QObject::tr("Oximetry");
const QString STR_TR_Oximeter=QObject::tr("Oximeter");
const QString STR_TR_EventFlags=QObject::tr("Event Flags");

// Machine type names.
const QString STR_TR_CPAP=QObject::tr("CPAP");      // Constant Positive Airway Pressure
const QString STR_TR_BIPAP=QObject::tr("BiPAP");    // Bi-Level Positive Airway Pressure
const QString STR_TR_BiLevel=QObject::tr("Bi-Level"); // Another name for BiPAP
const QString STR_TR_EPAP=QObject::tr("EPAP");      // Expiratory Positive Airway Pressure
const QString STR_TR_IPAP=QObject::tr("IPAP");      // Inspiratory Positive Airway Pressure
const QString STR_TR_IPAPLo=QObject::tr("IPAPLo");  // Inspiratory Positive Airway Pressure, Low
const QString STR_TR_IPAPHi=QObject::tr("IPAPHi");  // Inspiratory Positive Airway Pressure, High
const QString STR_TR_APAP=QObject::tr("APAP");      // Automatic Positive Airway Pressure
const QString STR_TR_ASV=QObject::tr("ASV");        // Assisted Servo Ventilator
const QString STR_TR_STASV=QObject::tr("ST/ASV");

const QString STR_TR_Humidifier=QObject::tr("Humidifier");

const QString STR_TR_H=QObject::tr("H");        // Short form of Hypopnea
const QString STR_TR_OA=QObject::tr("OA");      // Short form of Obstructive Apnea
const QString STR_TR_UA=QObject::tr("A");       // Short form of Unspecified Apnea
const QString STR_TR_CA=QObject::tr("CA");      // Short form of Clear Airway Apnea
const QString STR_TR_FL=QObject::tr("FL");      // Short form of Flow Limitation
const QString STR_TR_LE=QObject::tr("LE");      // Short form of Leak Event
const QString STR_TR_EP=QObject::tr("EP");      // Short form of Expiratory Puff
const QString STR_TR_VS=QObject::tr("VS");      // Short form of Vibratory Snore
const QString STR_TR_VS2=QObject::tr("VS2");    // Short form of Secondary Vibratory Snore (Some Philips Respironics Machines have two sources)
const QString STR_TR_RERA=QObject::tr("RERA");  // Acronym for Respiratory Effort Related Arousal
const QString STR_TR_PP=QObject::tr("PP");      // Short form for Pressure Pulse
const QString STR_TR_P=QObject::tr("P");        // Short form for Pressure Event
const QString STR_TR_RE=QObject::tr("RE");      // Short form of Respiratory Effort Related Arousal
const QString STR_TR_NR=QObject::tr("NR");      // Short form of Non Responding event? (forgot sorry)
const QString STR_TR_NRI=QObject::tr("NRI");    // Sorry I Forgot.. it's a flag on Intellipap machines
const QString STR_TR_O2=QObject::tr("O2");      // SpO2 Desaturation
const QString STR_TR_PC=QObject::tr("PC");      // Short form for Pulse Change
const QString STR_TR_UF1=QObject::tr("UF1");      // Short form for User Flag 1
const QString STR_TR_UF2=QObject::tr("UF2");      // Short form for User Flag 2
const QString STR_TR_UF3=QObject::tr("UF3");      // Short form for User Flag 3



const QString STR_TR_PS=QObject::tr("PS");      // Short form of Pressure Support
const QString STR_TR_AHI=QObject::tr("AHI");    // Short form of Apnea Hypopnea Index
const QString STR_TR_RDI=QObject::tr("RDI");    // Short form of Respiratory Distress Index
const QString STR_TR_AI=QObject::tr("AI");      // Short form of Apnea Index
const QString STR_TR_HI=QObject::tr("HI");      // Short form of Hypopnea Index
const QString STR_TR_UAI=QObject::tr("UAI");    // Short form of Uncatagorized Apnea Index
const QString STR_TR_CAI=QObject::tr("CAI");    // Short form of Clear Airway Index
const QString STR_TR_FLI=QObject::tr("FLI");    // Short form of Flow Limitation Index
const QString STR_TR_REI=QObject::tr("REI");    // Short form of RERA Index
const QString STR_TR_EPI=QObject::tr("EPI");    // Short form of Expiratory Puff Index
const QString STR_TR_CSR=QObject::tr("ÇSR");    // Short form of Cheyne Stokes Respiration
const QString STR_TR_PB=QObject::tr("PB");      // Short form of Periodic Breathing


// Graph Titles
const QString STR_TR_IE=QObject::tr("IE");      // Inspiratory Expiratory Ratio
const QString STR_TR_InspTime=QObject::tr("Insp. Time");    // Inspiratory Time
const QString STR_TR_ExpTime=QObject::tr("Exp. Time");      // Expiratory Time
const QString STR_TR_RespEvent=QObject::tr("Resp. Event");    // Respiratory Event
const QString STR_TR_FlowLimitation=QObject::tr("Flow Limitation");
const QString STR_TR_FlowLimit=QObject::tr("Flow Limit");
const QString STR_TR_PatTrigBreath=QObject::tr("Pat. Trig. Breath"); // Patient Triggered Breath
const QString STR_TR_TgtMinVent=QObject::tr("Tgt. Min. Vent");        // Target Minute Ventilation
const QString STR_TR_TargetVent=QObject::tr("Target Vent.");          // Target Ventilation
const QString STR_TR_MinuteVent=QObject::tr("Minute Vent."); // Minute Ventilation
const QString STR_TR_TidalVolume=QObject::tr("Tidal Volume");
const QString STR_TR_RespRate=QObject::tr("Resp. Rate");    // Respiratory Rate
const QString STR_TR_Snore=QObject::tr("Snore");
const QString STR_TR_Leak=QObject::tr("Leak");
const QString STR_TR_Leaks=QObject::tr("Leaks");
const QString STR_TR_TotalLeaks=QObject::tr("Total Leaks");
const QString STR_TR_UnintentionalLeaks=QObject::tr("Unintentional Leaks");
const QString STR_TR_MaskPressure=QObject::tr("MaskPressure");
const QString STR_TR_FlowRate=QObject::tr("Flow Rate");
const QString STR_TR_SleepStage=QObject::tr("Sleep Stage");
const QString STR_TR_Usage=QObject::tr("Usage");
const QString STR_TR_Sessions=QObject::tr("Sessions");
const QString STR_TR_PrRelief=QObject::tr("Pr. Relief"); // Pressure Relief

const QString STR_TR_NoData=QObject::tr("No Data");
const QString STR_TR_Bookmarks=QObject::tr("Bookmarks");
const QString STR_TR_SleepyHead=QObject::tr("SleepyHead");
const QString STR_TR_SleepyHeadVersion=STR_TR_SleepyHead+" v"+VersionString;

const QString STR_TR_Mode=QObject::tr("Mode");
const QString STR_TR_Model=QObject::tr("Model");
const QString STR_TR_Brand=QObject::tr("Brand");
const QString STR_TR_Serial=QObject::tr("Serial");
const QString STR_TR_Machine=QObject::tr("Machine");
const QString STR_TR_Channel=QObject::tr("Channel");
const QString STR_TR_Settings=QObject::tr("Settings");

const QString STR_TR_Name=QObject::tr("Name");
const QString STR_TR_DOB=QObject::tr("DOB");    // Date of Birth
const QString STR_TR_Phone=QObject::tr("Phone");
const QString STR_TR_Address=QObject::tr("Address");
const QString STR_TR_Email=QObject::tr("Email");
const QString STR_TR_PatientID=QObject::tr("Patient ID");
const QString STR_TR_Date=QObject::tr("Date");

const QString STR_TR_BedTime=QObject::tr("Bedtime");
const QString STR_TR_WakeUp=QObject::tr("Wake-up");
const QString STR_TR_MaskTime=QObject::tr("Mask Time");
const QString STR_TR_Unknown=QObject::tr("Unknown");
const QString STR_TR_None=QObject::tr("None");
const QString STR_TR_Ready=QObject::tr("Ready");

const QString STR_TR_First=QObject::tr("First");
const QString STR_TR_Last=QObject::tr("Last");
const QString STR_TR_Start=QObject::tr("Start");
const QString STR_TR_End=QObject::tr("End");
const QString STR_TR_On=QObject::tr("On");
const QString STR_TR_Off=QObject::tr("Off");

const QString STR_TR_Min=QObject::tr("Min");    // Minimum
const QString STR_TR_Max=QObject::tr("Max");    // Maximum

const QString STR_TR_Average=QObject::tr("Average");
const QString STR_TR_Median=QObject::tr("Median");
const QString STR_TR_Avg=QObject::tr("Avg");        // Average
const QString STR_TR_WAvg=QObject::tr("W-Avg");     // Weighted Average

#endif // COMMON_H
