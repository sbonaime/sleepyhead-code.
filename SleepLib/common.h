#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QObject>

enum UnitSystem { US_Undefined, US_Metric, US_Archiac };

const float ounce_convert=28.3495231; // grams
const float pound_convert=ounce_convert*16;

QString weightString(float kg, UnitSystem us);

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

const QString STR_PROP_Brand="Brand";
const QString STR_PROP_Model="Model";
const QString STR_PROP_ModelNumber="ModelNumber";
const QString STR_PROP_SubModel="SubModel";
const QString STR_PROP_Serial="Serial";
const QString STR_PROP_DataVersion="DataVersion";
const QString STR_PROP_Path="Path";

const QString STR_MACH_ResMed="ResMed";
const QString STR_MACH_PRS1="PRS1";
const QString STR_MACH_Journal="Journal";
const QString STR_MACH_Intellipap="Intellipap";
const QString STR_MACH_CMS50="CMS50";
const QString STR_MACH_ZEO="Zeo";

const QString STR_TR_BMI=QObject::tr("BMI");
const QString STR_TR_Weight=QObject::tr("Weight");

const QString STR_TR_Daily=QObject::tr("Daily");
const QString STR_TR_Overview=QObject::tr("Overview");
const QString STR_TR_Oximetry=QObject::tr("Oximetry");

const QString STR_TR_EventFlags=QObject::tr("Event Flags");

#endif // COMMON_H
