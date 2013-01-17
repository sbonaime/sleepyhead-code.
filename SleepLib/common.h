#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QObject>

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


const QString STR_UNIT_CM=QObject::tr("cm");
const QString STR_UNIT_INCH=QObject::tr("\"");
const QString STR_UNIT_FOOT=QObject::tr("ft");
const QString STR_UNIT_POUND=QObject::tr("lb");
const QString STR_UNIT_OUNCE=QObject::tr("oz");
const QString STR_UNIT_KG=QObject::tr("Kg");
const QString STR_UNIT_CMH2O=QObject::tr("cmH2O");
const QString STR_UNIT_Hours=QObject::tr("Hours");

const QString STR_MESSAGE_ERROR=QObject::tr("Error");
const QString STR_MESSAGE_WARNING=QObject::tr("Warning");

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

const QString STR_TR_BMI=QObject::tr("BMI");
const QString STR_TR_Weight=QObject::tr("Weight");
const QString STR_TR_PulseRate=QObject::tr("Pulse Rate");
const QString STR_TR_SpO2=QObject::tr("SpO2");
const QString STR_TR_Plethy=QObject::tr("Plethy");
const QString STR_TR_FlowRate=QObject::tr("Flow Rate");
const QString STR_TR_Pressure=QObject::tr("Pressure");

const QString STR_TR_Daily=QObject::tr("Daily");
const QString STR_TR_Overview=QObject::tr("Overview");
const QString STR_TR_Oximetry=QObject::tr("Oximetry");

const QString STR_TR_EventFlags=QObject::tr("Event Flags");

#endif // COMMON_H
