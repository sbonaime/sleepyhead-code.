/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Common code
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

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
    ValueCount() { value = 0; count = 0; p = 0; }
    ValueCount(const ValueCount &copy) {
        value = copy.value;
        count = copy.count;
        p = copy.p;
    }
    EventDataType value;
    qint64 count;
    double p;
};

// Primarily sort by value
bool operator <(const ValueCount &a, const ValueCount &b);

const float ounce_convert = 28.3495231F; // grams
const float pound_convert = ounce_convert * 16;

QString weightString(float kg, UnitSystem us = US_Undefined);

//! \brief Mercilessly trash a directory
bool removeDir(const QString &path);


#ifndef nullptr
#define nullptr NULL
#endif

#ifdef TEST_BUILD
const QString STR_TestBuild = "-Testing";
#else
const QString STR_TestBuild = "";
#endif

const QString getAppName();
const QString getDeveloperName();
const QString getDefaultAppRoot();

void initializeStrings();


///////////////////////////////////////////////////////////////////////////////////////////////
// Preference Name Strings
///////////////////////////////////////////////////////////////////////////////////////////////

const QString STR_GEN_Profile = "Profile";
const QString STR_GEN_SkipLogin = "SkipLoginScreen";
const QString STR_GEN_UpdatesLastChecked = "UpdatesLastChecked";
const QString STR_GEN_UpdatesAutoCheck = "Updates_AutoCheck";
const QString STR_GEN_UpdateCheckFrequency = "Updates_CheckFrequency";
const QString STR_GEN_DataFolder = "DataFolder";

const QString STR_GEN_On = QObject::tr("On");
const QString STR_GEN_Off = QObject::tr("Off");

const QString STR_PREF_AllowEarlyUpdates = "AllowEarlyUpdates";
const QString STR_PREF_ReimportBackup = "ReimportBackup";

const QString STR_PROP_Brand = "Brand";
const QString STR_PROP_Model = "Model";
const QString STR_PROP_Series = "Series";
const QString STR_PROP_ModelNumber = "ModelNumber";
const QString STR_PROP_SubModel = "SubModel";
const QString STR_PROP_Serial = "Serial";
const QString STR_PROP_DataVersion = "DataVersion";
const QString STR_PROP_Path = "Path";
const QString STR_PROP_BackupPath = "BackupPath";
const QString STR_PROP_LastImported = "LastImported";

const QString STR_MACH_ResMed = "ResMed";
const QString STR_MACH_PRS1 = "PRS1";
const QString STR_MACH_Journal = "Journal";
const QString STR_MACH_Intellipap = "Intellipap";
const QString STR_MACH_FPIcon = "FPIcon";
const QString STR_MACH_MSeries = "MSeries";
const QString STR_MACH_CMS50 = "CMS50";
const QString STR_MACH_ZEO = "Zeo";

const QString STR_PREF_VersionString = "VersionString";
const QString STR_PREF_Language = "Language";

const QString STR_AppName = "SleepyHead";
const QString STR_DeveloperName = "Jedimark";
const QString STR_AppRoot = "SleepyHeadData";

///////////////////////////////////////////////////////////////////////////////////////////////
// Commonly used translatable text strings
///////////////////////////////////////////////////////////////////////////////////////////////

extern QString STR_UNIT_CM;
extern QString STR_UNIT_INCH;
extern QString STR_UNIT_FOOT;
extern QString STR_UNIT_POUND;
extern QString STR_UNIT_OUNCE;
extern QString STR_UNIT_KG;
extern QString STR_UNIT_CMH2O;
extern QString STR_UNIT_Hours;
extern QString STR_UNIT_BPM;       // Beats per Minute
extern QString STR_UNIT_LPM;       // Litres per Minute

extern QString STR_MESSAGE_ERROR;
extern QString STR_MESSAGE_WARNING;

extern QString STR_TR_BMI;         // Short form of Body Mass Index
extern QString STR_TR_Weight;
extern QString STR_TR_Zombie;
extern QString STR_TR_PulseRate;   // Pulse / Heart rate
extern QString STR_TR_SpO2;
extern QString STR_TR_Plethy;      // Plethysomogram
extern QString STR_TR_Pressure;

extern QString STR_TR_Daily;
extern QString STR_TR_Overview;
extern QString STR_TR_Oximetry;

extern QString STR_TR_Oximeter;
extern QString STR_TR_EventFlags;

extern QString STR_TR_Inclination;
extern QString STR_TR_Orientation;

// Machine type names.
extern QString STR_TR_CPAP;    // Constant Positive Airway Pressure
extern QString STR_TR_BIPAP;   // Bi-Level Positive Airway Pressure
extern QString STR_TR_BiLevel; // Another name for BiPAP
extern QString STR_TR_EPAP;    // Expiratory Positive Airway Pressure
extern QString STR_TR_EPAPLo;  // Expiratory Positive Airway Pressure, Low
extern QString STR_TR_EPAPHi;  // Expiratory Positive Airway Pressure, High
extern QString STR_TR_IPAP;    // Inspiratory Positive Airway Pressure
extern QString STR_TR_IPAPLo;  // Inspiratory Positive Airway Pressure, Low
extern QString STR_TR_IPAPHi;  // Inspiratory Positive Airway Pressure, High
extern QString STR_TR_APAP;    // Automatic Positive Airway Pressure
extern QString STR_TR_ASV;     // Assisted Servo Ventilator
extern QString STR_TR_STASV;

extern QString STR_TR_Humidifier;

extern QString STR_TR_H;       // Short form of Hypopnea
extern QString STR_TR_OA;      // Short form of Obstructive Apnea
extern QString STR_TR_UA;      // Short form of Unspecified Apnea
extern QString STR_TR_CA;      // Short form of Clear Airway Apnea
extern QString STR_TR_FL;      // Short form of Flow Limitation
extern QString STR_TR_LE;      // Short form of Leak Event
extern QString STR_TR_EP;      // Short form of Expiratory Puff
extern QString STR_TR_VS;      // Short form of Vibratory Snore
extern QString
STR_TR_VS2;     // Short form of Secondary Vibratory Snore (Some Philips Respironics Machines have two sources)
extern QString STR_TR_RERA;    // Acronym for Respiratory Effort Related Arousal
extern QString STR_TR_PP;      // Short form for Pressure Pulse
extern QString STR_TR_P;       // Short form for Pressure Event
extern QString STR_TR_RE;      // Short form of Respiratory Effort Related Arousal
extern QString STR_TR_NR;      // Short form of Non Responding event? (forgot sorry)
extern QString STR_TR_NRI;     // Sorry I Forgot.. it's a flag on Intellipap machines
extern QString STR_TR_O2;      // SpO2 Desaturation
extern QString STR_TR_PC;      // Short form for Pulse Change
extern QString STR_TR_UF1;     // Short form for User Flag 1
extern QString STR_TR_UF2;     // Short form for User Flag 2
extern QString STR_TR_UF3;     // Short form for User Flag 3



extern QString STR_TR_PS;     // Short form of Pressure Support
extern QString STR_TR_AHI;    // Short form of Apnea Hypopnea Index
extern QString STR_TR_RDI;    // Short form of Respiratory Distress Index
extern QString STR_TR_AI;     // Short form of Apnea Index
extern QString STR_TR_HI;     // Short form of Hypopnea Index
extern QString STR_TR_UAI;    // Short form of Uncatagorized Apnea Index
extern QString STR_TR_CAI;    // Short form of Clear Airway Index
extern QString STR_TR_FLI;    // Short form of Flow Limitation Index
extern QString STR_TR_REI;    // Short form of RERA Index
extern QString STR_TR_EPI;    // Short form of Expiratory Puff Index
extern QString STR_TR_CSR;    // Short form of Cheyne Stokes Respiration
extern QString STR_TR_PB;     // Short form of Periodic Breathing


// Graph Titles
extern QString STR_TR_IE;              // Inspiratory Expiratory Ratio
extern QString STR_TR_InspTime;        // Inspiratory Time
extern QString STR_TR_ExpTime;         // Expiratory Time
extern QString STR_TR_RespEvent;       // Respiratory Event
extern QString STR_TR_FlowLimitation;
extern QString STR_TR_FlowLimit;
extern QString STR_TR_PatTrigBreath;   // Patient Triggered Breath
extern QString STR_TR_TgtMinVent;      // Target Minute Ventilation
extern QString STR_TR_TargetVent;      // Target Ventilation
extern QString STR_TR_MinuteVent;      // Minute Ventilation
extern QString STR_TR_TidalVolume;
extern QString STR_TR_RespRate;        // Respiratory Rate
extern QString STR_TR_Snore;
extern QString STR_TR_Leak;
extern QString STR_TR_LargeLeak;
extern QString STR_TR_LL;
extern QString STR_TR_Leaks;
extern QString STR_TR_TotalLeaks;
extern QString STR_TR_UnintentionalLeaks;
extern QString STR_TR_MaskPressure;
extern QString STR_TR_FlowRate;
extern QString STR_TR_SleepStage;
extern QString STR_TR_Usage;
extern QString STR_TR_Sessions;
extern QString STR_TR_PrRelief; // Pressure Relief

extern QString STR_TR_NoData;
extern QString STR_TR_Bookmarks;
extern QString STR_TR_SleepyHead;
extern QString STR_TR_SleepyHeadVersion;

extern QString STR_TR_Mode;
extern QString STR_TR_Model;
extern QString STR_TR_Brand;
extern QString STR_TR_Serial;
extern QString STR_TR_Machine;
extern QString STR_TR_Channel;
extern QString STR_TR_Settings;

extern QString STR_TR_Name;
extern QString STR_TR_DOB;    // Date of Birth
extern QString STR_TR_Phone;
extern QString STR_TR_Address;
extern QString STR_TR_Email;
extern QString STR_TR_PatientID;
extern QString STR_TR_Date;

extern QString STR_TR_BedTime;
extern QString STR_TR_WakeUp;
extern QString STR_TR_MaskTime;
extern QString STR_TR_Unknown;
extern QString STR_TR_None;
extern QString STR_TR_Ready;

extern QString STR_TR_First;
extern QString STR_TR_Last;
extern QString STR_TR_Start;
extern QString STR_TR_End;
extern QString STR_TR_On;
extern QString STR_TR_Off;

extern QString STR_TR_Min;    // Minimum
extern QString STR_TR_Max;    // Maximum

extern QString STR_TR_Average;
extern QString STR_TR_Median;
extern QString STR_TR_Avg;    // Short form of Average
extern QString STR_TR_WAvg;   // Short form of Weighted Average

#endif // COMMON_H
