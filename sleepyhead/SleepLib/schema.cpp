/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Schema Implementation (Parse Channel XML data)
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QFile>
#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QMessageBox>
#include <QApplication>

#include "common.h"
#include "schema.h"

namespace schema {

ChannelList channel;
Channel EmptyChannel;
Channel *SessionEnabledChannel;

QHash<QString, ChanType> ChanTypes;
QHash<QString, DataType> DataTypes;
QHash<QString, ScopeType> Scopes;

bool schema_initialized = false;

void init()
{
    if (schema_initialized) { return; }

    schema_initialized = true;

    EmptyChannel = Channel(0, DATA, DAY, "Empty", "Empty", "Empty Channel", "", "");
    SessionEnabledChannel = new Channel(1, DATA, DAY, "Enabled", "Enabled", "Session Enabled", "", "");

    channel.channels[1] = SessionEnabledChannel;
    channel.names["Enabled"] = SessionEnabledChannel;
    SESSION_ENABLED = 1;
    ChanTypes["data"] = DATA;
    //Types["waveform"]=WAVEFORM;
    ChanTypes["setting"] = SETTING;

    Scopes["session"] = SESSION;
    Scopes["day"] = DAY;
    Scopes["machine"] = MACHINE;
    Scopes["global"] = GLOBAL;

    DataTypes[""] = DEFAULT;
    DataTypes["bool"] = BOOL;
    DataTypes["double"] = DOUBLE;
    DataTypes["integer"] = INTEGER;
    DataTypes["string"] = STRING;
    DataTypes["richtext"] = RICHTEXT;
    DataTypes["date"] = DATE;
    DataTypes["datetime"] = DATETIME;
    DataTypes["time"] = TIME;

    if (!schema::channel.Load(":/docs/channels.xml")) {
        QMessageBox::critical(0, QObject::tr("Error"),
                              QObject::tr("Couldn't parse Channels.xml, this build is seriously borked, no choice but to abort!!"),
                              QMessageBox::Ok);
        QApplication::exit(-1);
    }

    //    <channel id="0x110c" class="data" name="Pressure" details="Pressure" label="Pressure" unit="cmH20" color="dark green"/>
    //    <channel id="0x110d" class="data" name="IPAP" details="Inspiratory Pressure" label="IPAP" unit="cmH20" color="orange"/>
    //    <channel id="0x110e" class="data" name="EPAP" details="Expiratory Pressure" label="EPAP" unit="cmH20" color="light blue"/>
    //    <channel id="0x110f" class="data" name="PS" details="Pressure Support" label="PS" unit="cmH20" color="dark blue"/>
    //    <channel id="0x1110" class="data" name="IPAPLo" details="Inspiratory Pressure Lo" label="IPAP Lo" unit="cmH20" color="grey"/>
    //    <channel id="0x1111" class="data" name="IPAPHi" details="Inspiratory Pressure Hi" label="IPAP Hi" unit="cmH20" color="grey"/>
    //    <channel id="0x111a" class="setting" name="PSMin" details="Pressure Support Min" label="PS Min" unit="cmH20" color="dark cyan"/>
    //    <channel id="0x111b" class="setting" name="PSMax" details="Pressure Support Max" label="PS Max" unit="cmH20" color="dark magenta"/>
    //    <channel id="0x1020" class="data" name="PressureMin" details="Min Therapy Pressure" label="Pressure" color="black"/>
    //    <channel id="0x1021" class="data" name="PressureMax" details="Max Therapy Pressure" label="Pressure" color="black"/>
    //    <channel id="0x1022" class="data" name="RampTime" details="Ramp Time" label="Ramp Time" color="black"/>
    //    <channel id="0x1023" class="data" name="RampPressure" details="Ramp Starting Pressure" label="Ramp Pr." color="black"/>

    QString GRP_CPAP = "CPAP";
    QString GRP_POS = "POS";
    QString GRP_OXI = "OXI";
    // Pressure Related Settings

    // Lookup Code strings are used internally and not meant to be tranlsated

    //                  Group                ChannelID            Code    Type     Scope    Lookup Code      Translable Name                 Description                                   Shortened Name              Units String            FieldType   Default Color
    schema::channel.add(GRP_CPAP, new Channel(CPAP_Pressure      = 0x110C, DATA,    SESSION,
                        "Pressure",      STR_TR_Pressure,                QObject::tr("Therapy Pressure"),
                        STR_TR_Pressure,            STR_UNIT_CMH2O,         DEFAULT,    QColor("dark green")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_IPAP          = 0x110D, DATA,    SESSION, "IPAP",
                        STR_TR_IPAP,                    QObject::tr("Inspiratory Pressure"),          STR_TR_IPAP,
                        STR_UNIT_CMH2O,         DEFAULT,    QColor("orange")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_IPAPLo        = 0x1110, DATA,    SESSION, "IPAPLo",
                        STR_TR_IPAPLo,                  QObject::tr("Lower Inspiratory Pressure"),    STR_TR_IPAPLo,
                        STR_UNIT_CMH2O,         DEFAULT,    QColor("orange")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_IPAPHi        = 0x1111, DATA,    SESSION, "IPAPHi",
                        STR_TR_IPAPHi,                  QObject::tr("Higher Inspiratory Pressure"),   STR_TR_IPAPHi,
                        STR_UNIT_CMH2O,         DEFAULT,    QColor("orange")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_EPAP          = 0x110E, DATA,    SESSION, "EPAP",
                        STR_TR_EPAP,                    QObject::tr("Expiratory Pressure"),           STR_TR_EPAP,
                        STR_UNIT_CMH2O,         DEFAULT,    QColor("light blue")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_EPAPLo        = 0x111C, DATA,    SESSION, "EPAPLo",
                        STR_TR_EPAPLo,                  QObject::tr("Lower Expiratory Pressure"),     STR_TR_EPAPLo,
                        STR_UNIT_CMH2O,         DEFAULT,    QColor("light blue")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_EPAPHi        = 0x111D, DATA,    SESSION, "EPAPHi",
                        STR_TR_EPAPHi,                  QObject::tr("Higher Expiratory Pressure"),    STR_TR_EPAPHi,
                        STR_UNIT_CMH2O,         DEFAULT,    QColor("aqua")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_PS            = 0x110F, DATA,    SESSION, "PS",
                        STR_TR_PS,                      QObject::tr("Pressure Support"),              STR_TR_PS,
                        STR_UNIT_CMH2O,         DEFAULT,    QColor("grey")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_PSMin         = 0x111A, SETTING, SESSION, "PSMin",
                        QObject::tr("PS Min") ,         QObject::tr("Pressure Support Minimum"),
                        QObject::tr("PS Min"),      STR_UNIT_CMH2O,         DEFAULT,    QColor("dark cyan")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_PSMax         = 0x111B, SETTING, SESSION, "PSMax",
                        QObject::tr("PS Max"),          QObject::tr("Pressure Support Maximum"),
                        QObject::tr("PS Max"),      STR_UNIT_CMH2O,         DEFAULT,    QColor("dark magenta")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_PressureMin   = 0x1020, SETTING, SESSION,
                        "PressureMin",   QObject::tr("Min Pressure") ,   QObject::tr("Minimum Therapy Pressure"),
                        QObject::tr("Pr. Min"),     STR_UNIT_CMH2O,         DEFAULT,    QColor("black")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_PressureMax   = 0x1021, SETTING, SESSION,
                        "PressureMax",   QObject::tr("Max Pressure"),    QObject::tr("Maximum Therapy Pressure"),
                        QObject::tr("Pr. Max"),     STR_UNIT_CMH2O,         DEFAULT,    QColor("black")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_RampTime      = 0x1022, SETTING, SESSION,
                        "RampTime",      QObject::tr("Ramp Time") ,      QObject::tr("Ramp Delay Period"),
                        QObject::tr("Ramp Time"),   QObject::tr("minutes"), DEFAULT,    QColor("black")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_RampPressure  = 0x1023, SETTING, SESSION,
                        "RampPressure",  QObject::tr("Ramp Pressure"),   QObject::tr("Starting Ramp Pressure"),
                        QObject::tr("Ramp Pr."),    STR_UNIT_CMH2O,         DEFAULT,    QColor("black")));

    //      <channel id="0x1000" class="data" name="CSR" details="Periodic Breathing" label="PB" unit="% of night" color="light green"/>
    //      <channel id="0x1001" class="data" name="ClearAirway" details="Clear Airway Apnea" label="CA" unit="events/hour" color="purple"/>
    //      <channel id="0x1002" class="data" name="Obstructive" details="Obstructive Apnea" label="OA" unit="events/hour" color="#40c0ff"/>
    //      <channel id="0x1003" class="data" name="Hypopnea" details="Hypopnea" label="H" unit="events/hour" color="blue"/>
    //      <channel id="0x1004" class="data" name="Apnea" details="Unspecified Apnea" label="UA" unit="events/hour" color="dark green"/>
    //      <channel id="0x1005" class="data" name="FlowLimit" details="Flow Limitation" label="FL" unit="events/hour" color="#404040"/>
    //      <channel id="0x1006" class="data" name="RERA" details="Resp. Effort Related Arousal" unit="events/hour" label="RERA" color="gold"/>
    //      <channel id="0x1007" class="data" name="VSnore" details="Vibratory Snore" label="VS" unit="events/hour" color="red"/>
    //      <channel id="0x1008" class="data" name="VSnore2" details="Vibratory Snore #2" label="VS2" unit="events/hour" color="orange"/>
    //      <channel id="0x1009" class="data" name="PressurePulse" details="Pressure Pulse" label="PP" unit="events/hour" color="dark red"/>
    //      <channel id="0x100a" class="data" name="LeakFlag" details="Leak Event" label="L" unit="events/hour" color="dark blue"/>
    //      <channel id="0x100b" class="data" name="NRI" details="Non-Responding Event" label="NRI" unit="events/hour" color="orange"/>
    //      <channel id="0x100c" class="data" name="ExP" details="Exhale Puff" label="EP" unit="events/hour" color="dark magenta"/>
    //      <channel id="0x101e" class="data" name="UserFlag1" details="User Flag #1" label="UF1" unit="events/hour" color="dark cyan"/>
    //      <channel id="0x101f" class="data" name="UserFlag2" details="User Flag #2" label="UF2" unit="events/hour" color="dark blue"/>
    //      <channel id="0x1024" class="data" name="UserFlag3" details="User Flag #3" label="UF3" unit="events/hour" color="dark grey"/>
    schema::channel.add(GRP_CPAP, new Channel(CPAP_CSR           = 0x1000, DATA,    SESSION, "CSR",
                        QObject::tr("Periodic Breathing"),
                        QObject::tr("A period of periodic breathing"),
                        QObject::tr("PB"),       QObject::tr("%"),            DEFAULT,    QColor("light green")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_ClearAirway   = 0x1001, DATA,    SESSION,
                        "ClearAirway",    QObject::tr("Clear Airway Apnea"),
                        QObject::tr("An apnea where the airway is open"),
                        QObject::tr("CA"),       QObject::tr("events/hr"),    DEFAULT,    QColor("purple")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_Obstructive   = 0x1002, DATA,    SESSION,
                        "Obstructive",    QObject::tr("Obstructive Apnea"),
                        QObject::tr("An apnea caused by airway obstruction"),
                        QObject::tr("OA"),       QObject::tr("events/hr"),    DEFAULT,    QColor("#40c0ff")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_Hypopnea      = 0x1003, DATA,    SESSION,
                        "Hypopnea",       QObject::tr("Hypopnea"),
                        QObject::tr("A partially obstructed airway"),
                        QObject::tr("H"),        QObject::tr("events/hr"),    DEFAULT,    QColor("blue")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_Apnea         = 0x1004, DATA,    SESSION, "Apnea",
                        QObject::tr("Unclassified Apnea"),
                        QObject::tr("An apnea that could not fit into a category"),
                        QObject::tr("UA"),       QObject::tr("events/hr"),    DEFAULT,    QColor("dark green")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_FlowLimit     = 0x1005, DATA,    SESSION,
                        "FlowLimit",      QObject::tr("Flow Limitation"),
                        QObject::tr("An restriction in breathing from normal, causing a flattening of the flow waveform."),
                        QObject::tr("FL"),       QObject::tr("events/hr"),    DEFAULT,    QColor("#404040")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_RERA          = 0x1006, DATA,    SESSION, "RERA",
                        QObject::tr("Respiratory Effort Related Arousal"),
                        QObject::tr("An restriction in breathing that causes an either an awakening or sleep disturbance."),
                        QObject::tr("RE"),       QObject::tr("events/hr"),    DEFAULT,    QColor("gold")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_VSnore        = 0x1007, DATA,    SESSION, "VSnore",
                        QObject::tr("Vibratory Snore"),                       QObject::tr("A vibratory snore"),
                        QObject::tr("VS"),       QObject::tr("events/hr"),    DEFAULT,    QColor("red")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_VSnore2       = 0x1008, DATA,    SESSION, "VSnore2",
                        QObject::tr("Vibratory Snore"),
                        QObject::tr("A vibratory snore as detcted by a System One machine"),
                        QObject::tr("VS2"),      QObject::tr("events/hr"),    DEFAULT,    QColor("red")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_PressurePulse = 0x1009, DATA,    SESSION,
                        "PressurePulse",  QObject::tr("Pressure Pulse"),
                        QObject::tr("A pulse of pressure 'pinged' to detect a closed airway."),
                        QObject::tr("PP"),       QObject::tr("events/hr"),    DEFAULT,    QColor("dark red")));

    // This Large Leak record is just a flag marker, used by Intellipap for one
    schema::channel.add(GRP_CPAP, new Channel(CPAP_LeakFlag      = 0x100a, DATA,    SESSION,
                        "LeakFlag",       QObject::tr("Large Leak"),
                        QObject::tr("A large mask leak affecting machine performance."),
                        QObject::tr("LL"),       QObject::tr("events/hr"),    DEFAULT,    QColor("light gray")));

    // The following is a Large Leak record that references a waveform span
    schema::channel.add(GRP_CPAP, new Channel(PRS1_10 = 0x1158, DATA,    SESSION,
                        "LeakFlagSpan",       QObject::tr("Large Leak"),
                        QObject::tr("A large mask leak affecting machine performance."),
                        QObject::tr("LL"),       QObject::tr("events/hr"),    DEFAULT,    QColor("light gray")));

    schema::channel.add(GRP_CPAP, new Channel(CPAP_NRI           = 0x100b, DATA,    SESSION, "NRI",
                        QObject::tr("Non Responding Event"),
                        QObject::tr("A type of respiratory event that won't respond to a pressure increase."),
                        QObject::tr("NR"),       QObject::tr("events/hr"),    DEFAULT,    QColor("orange")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_ExP           = 0x100c, DATA,    SESSION, "ExP",
                        QObject::tr("Expiratory Puff"),
                        QObject::tr("Intellipap event where you breathe out your mouth."),
                        QObject::tr("EP"),       QObject::tr("events/hr"),    DEFAULT,    QColor("dark magenta")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_SensAwake     = 0x100d, DATA,    SESSION,
                        "SensAwake",      QObject::tr("SensAwake"),
                        QObject::tr("SensAwake feature will reduce pressure when waking is detected."),
                        QObject::tr("SA"),       QObject::tr("events/hr"),    DEFAULT,    QColor("gold")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_UserFlag1     = 0x101e, DATA,    SESSION,
                        "UserFlag1",      QObject::tr("User Flag #1"),
                        QObject::tr("A user definable event detected by SleepyHead's flow waveform processor."),
                        QObject::tr("UF1"),      QObject::tr("events/hr"),    DEFAULT,    QColor("dark cyan")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_UserFlag2     = 0x101f, DATA,    SESSION,
                        "UserFlag2",      QObject::tr("User Flag #2"),
                        QObject::tr("A user definable event detected by SleepyHead's flow waveform processor."),
                        QObject::tr("UF2"),      QObject::tr("events/hr"),    DEFAULT,    QColor("dark blue")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_UserFlag3     = 0x1024, DATA,    SESSION,
                        "UserFlag3",      QObject::tr("User Flag #3"),
                        QObject::tr("A user definable event detected by SleepyHead's flow waveform processor."),
                        QObject::tr("UF3"),      QObject::tr("events/hr"),    DEFAULT,    QColor("dark grey")));

    //    <channel id="0x1800" class="data" name="Pulse" details="Pulse Rate" label="Pulse Rate" unit="bpm" color="red"/>
    //     <channel id="0x1801" class="data" name="SPO2" details="Oxygen Saturation" label="SpO2" unit="%" color="blue"/>
    //     <channel id="0x1802" class="data" name="Plethy" details="Plethysomogram" label="Plethy" color="black"/>
    //     <channel id="0x1803" class="data" name="PulseChange" details="Pulse Change" label="Pulse Change" color="light gray"/>
    //     <channel id="0x1804" class="data" name="SPO2Drop" details="SPO2Drop" label="SPO2 Drop" color="light blue"/>
    schema::channel.add(GRP_OXI, new Channel(OXI_Pulse           = 0x1800, DATA,    SESSION, "Pulse",
                        QObject::tr("Pulse Rate"),                    QObject::tr("Heart rate in beats per minute"),
                        QObject::tr("Pulse Rate"), QObject::tr("%"),            DEFAULT,    QColor("red")));
    schema::channel.add(GRP_OXI, new Channel(OXI_SPO2            = 0x1801, DATA,    SESSION, "SPO2",
                        QObject::tr("SpO2 %"),                        QObject::tr("Blood-oxygen saturation percentage"),
                        QObject::tr("SpO2"),       QObject::tr("bpm"),          DEFAULT,    QColor("blue")));
    schema::channel.add(GRP_OXI, new Channel(OXI_Plethy          = 0x1802, DATA,    SESSION, "Plethy",
                        QObject::tr("Plethysomogram"),
                        QObject::tr("An optical Photo-plethysomogram showing heart rhythm"),
                        QObject::tr("Plethy"),     QObject::tr("hz"),           DEFAULT,    QColor("#404040")));
    schema::channel.add(GRP_OXI, new Channel(OXI_PulseChange     = 0x1803, DATA,    SESSION,
                        "PulseChange",      QObject::tr("Pulse Change"),
                        QObject::tr("A sudden (user definable) change in heart rate"),
                        QObject::tr("PC"),         QObject::tr("events/hr"),    DEFAULT,    QColor("light grey")));
    schema::channel.add(GRP_OXI, new Channel(OXI_SPO2Drop        = 0x1804, DATA,    SESSION,
                        "SPO2Drop",         QObject::tr("SpO2 Drop"),
                        QObject::tr("A sudden (user definable) drop in blood oxygen saturation"),
                        QObject::tr("SD"),         QObject::tr("events/hr"),    DEFAULT,    QColor("light blue")));

    //      <channel id="0x1100" class="data" name="FlowRate" details="Flow Rate" label="Flow Rate" unit="L/min" color="black"/>
    //      <channel id="0x1101" class="data" name="MaskPressure" details="Mask Pressure" label="Mask Pressure" unit="cmH20" color="blue"/>
    //      <channel id="0x1102" class="data" name="MaskPressureHi" details="Mask Pressure" label="Mask Pressure (Hi-Res)" unit="cmH20" color="blue" link="0x1101"/>
    //      <channel id="0x1103" class="data" name="TidalVolume" details="Tidal Volume" label="Tidal Volume" unit="ml" color="magenta"/>
    //      <channel id="0x1104" class="data" name="Snore" details="Snore" label="Snore" unit="unknown" color="grey"/>
    //      <channel id="0x1105" class="data" name="MinuteVent" details="Minute Ventilation" label="Minute Vent." unit="L/min" color="dark cyan"/>
    //      <channel id="0x1106" class="data" name="RespRate" details="Respiratory Rate" label="Resp. Rate" unit="breaths/min" color="dark magenta"/>
    //      <channel id="0x1107" class="data" name="PTB" details="Patient Triggered Breaths" label="Pat. Trig. Breaths" unit="%" color="dark grey"/>
    //      <channel id="0x1108" class="data" name="Leak" details="Leak Rate" label="Leaks" unit="L/min" color="dark green"/>

    //      <channel id="0x1109" class="data" name="IE" details="Inspiratory:Expiratory" label="I:E" unit="ratio" color="dark red"/>
    //      <channel id="0x110a" class="data" name="Te" details="Expiratory Time" label="Exp Time" unit="seconds" color="dark green"/>
    //      <channel id="0x110b" class="data" name="Ti" details="Inspiratory Time" label="Insp Time" unit="seconds" color="dark blue"/>
    //      <channel id="0x1112" class="data" name="RespEvent" details="Respiratory Events" label="Resp Events" unit="" color="black"/>
    //      <channel id="0x1113" class="data" name="FLG" details="Flow Limit Graph" label="Flow Limit" unit="0-1" color="dark grey"/>
    //      <channel id="0x1114" class="data" name="TgMV" details="Target Minute Ventilation" label="Target Vent." unit="" color="dark cyan"/>
    //      <channel id="0x1115" class="data" name="MaxLeak" details="Maximum Leak" label="MaxLeaks" unit="L/min" color="dark red"/>
    //      <channel id="0x1116" class="data" name="AHI" details="Apnea / Hypopnea Index" label="AHI/Hr" unit="events/hr" color="dark red"/>
    //      <channel id="0x1117" class="data" name="LeakTotal" details="Total Leak Rate" label="Total Leaks" unit="L/min" color="dark green"/>
    //      <channel id="0x1118" class="data" name="LeakMedian" details="Median Leak Rate" label="Median Leaks" unit="L/min" color="dark green"/>
    //      <channel id="0x1119" class="data" name="RDI" details="Respiratory Disturbance Index" label="RDI" unit="events/hr" color="dark red"/>

    schema::channel.add(GRP_CPAP, new Channel(CPAP_FlowRate          = 0x1100, DATA,    SESSION,
                        "FlowRate",          QObject::tr("Flow Rate"),
                        QObject::tr("Breathing flow rate waveform"),                 QObject::tr("Flow Rate"),
                        QObject::tr("L/min"),    DEFAULT,    QColor("black")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_MaskPressure      = 0x1101, DATA,    SESSION,
                        "MaskPressure",      QObject::tr("Mask Pressure"),
                        QObject::tr("Mask Pressure"),                                QObject::tr("Mask Pressure"),
                        QObject::tr("cmH2O"),    DEFAULT,    QColor("black")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_MaskPressureHi    = 0x1102, DATA,    SESSION,
                        "MaskPressureHi",    QObject::tr("Mask Pressure"),
                        QObject::tr("Mask Pressure (High resolution)"),              QObject::tr("Mask Pressure"),
                        QObject::tr("cmH2O"),    DEFAULT,    QColor("black"), 0x1101)); // linked to CPAP_MaskPressure
    schema::channel.add(GRP_CPAP, new Channel(CPAP_TidalVolume       = 0x1103, DATA,    SESSION,
                        "TidalVolume",       QObject::tr("Tidal Volume"),
                        QObject::tr("Amount of air displaced per breath"),           QObject::tr("Tidal Volume"),
                        QObject::tr("L/min"),    DEFAULT,    QColor("magenta")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_Snore             = 0x1104, DATA,    SESSION,
                        "Snore",             QObject::tr("Snore"),
                        QObject::tr("Graph displaying snore volume"),                QObject::tr("Snore"),
                        QObject::tr("??"),       DEFAULT,    QColor("grey")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_MinuteVent        = 0x1105, DATA,    SESSION,
                        "MinuteVent",        QObject::tr("Minute Ventilation"),
                        QObject::tr("Amount of air displaced per minute"),           QObject::tr("Minute Vent."),
                        QObject::tr("L/min"),    DEFAULT,    QColor("dark cyan")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_RespRate          = 0x1106, DATA,    SESSION,
                        "RespRate",          QObject::tr("Respiratory Rate"),
                        QObject::tr("Rate of breaths per minute"),                   QObject::tr("Resp. Rate"),
                        QObject::tr("Bpm"),      DEFAULT,    QColor("dark magenta")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_PTB               = 0x1107, DATA,    SESSION, "PTB",
                        QObject::tr("Patient Triggered Breaths"),
                        QObject::tr("Percentage of breaths triggered by patient"),   QObject::tr("Pat. Trig. Breaths"),
                        QObject::tr("%"),        DEFAULT,    QColor("dark grey")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_Leak              = 0x1108, DATA,    SESSION,
                        "Leak",              QObject::tr("Leak Rate"),
                        QObject::tr("Rate of detected mask leakage"),                QObject::tr("Leak Rate"),
                        QObject::tr("L/min"),    DEFAULT,    QColor("dark green")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_IE                = 0x1109, DATA,    SESSION, "IE",
                        QObject::tr("I:E Ratio"),
                        QObject::tr("Ratio between Inspiratory and Expiratory time"), QObject::tr("I:E Ratio"),
                        QObject::tr("ratio"),    DEFAULT,    QColor("dark red")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_Te                = 0x110A, DATA,    SESSION, "Te",
                        QObject::tr("Expiratory Time"),                    QObject::tr("Time taken to breathe out"),
                        QObject::tr("Exp. Time"),          QObject::tr("seconds"),  DEFAULT,    QColor("dark green")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_Ti                = 0x110B, DATA,    SESSION, "Ti",
                        QObject::tr("Inspiratory Time"),                   QObject::tr("Time taken to breathe in"),
                        QObject::tr("Insp. Time"),         QObject::tr("seconds"),  DEFAULT,    QColor("dark blue")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_RespEvent         = 0x1112, DATA,    SESSION,
                        "RespEvent",         QObject::tr("Respiratory Event"),
                        QObject::tr("A ResMed data source showing Respiratory Events"),  QObject::tr("Resp. Event"),
                        QObject::tr("events"),   DEFAULT,    QColor("black")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_FLG               = 0x1113, DATA,    SESSION, "FLG",
                        QObject::tr("Flow Limitation"),
                        QObject::tr("Graph showing severity of flow limitations"),   QObject::tr("Flow Limit."),
                        QObject::tr("0-1"),      DEFAULT,    QColor("dark gray")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_TgMV              = 0x1114, DATA,    SESSION,
                        "TgMV",              QObject::tr("Target Minute Ventilation"),
                        QObject::tr("Target Minute Ventilation?"),                   QObject::tr("Target Vent."),
                        QObject::tr("??"),       DEFAULT,    QColor("dark cyan")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_MaxLeak           = 0x1115, DATA,    SESSION,
                        "MaxLeak",           QObject::tr("Maximum Leak"),
                        QObject::tr("The maximum rate of mask leakage"),             QObject::tr("Max Leaks"),
                        QObject::tr("L/min"),    DEFAULT,    QColor("dark red")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_AHI               = 0x1116, DATA,    SESSION, "AHI",
                        QObject::tr("Apnea Hypopnea Index"),
                        QObject::tr("Graph showing running AHI for the past hour"),  QObject::tr("AHI"),
                        QObject::tr("events/hour"), DEFAULT,  QColor("dark red")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_LeakTotal         = 0x1117, DATA,    SESSION,
                        "LeakTotal",         QObject::tr("Total Leak Rate"),
                        QObject::tr("Detected mask leakage including natural Mask leakages"),  QObject::tr("Total Leaks"),
                        QObject::tr("L/min"),    DEFAULT,    QColor("dark green")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_LeakMedian        = 0x1118, DATA,    SESSION,
                        "LeakMedian",        QObject::tr("Median Leak Rate"),
                        QObject::tr("Median rate of detected mask leakage"),         QObject::tr("Median Leaks"),
                        QObject::tr("L/min"),    DEFAULT,    QColor("dark green")));
    schema::channel.add(GRP_CPAP, new Channel(CPAP_RDI               = 0x1119, DATA,    SESSION, "RDI",
                        QObject::tr("Respiratory Disturbance Index"),
                        QObject::tr("Graph showing running RDI for the past hour"),  QObject::tr("RDI"),
                        QObject::tr("events/hour"), DEFAULT,  QColor("dark red")));

    schema::channel.add(GRP_POS, new Channel(POS_Orientation         = 0x2990, DATA,    SESSION,
                        "Orientation",      QObject::tr("Orientation"),
                        QObject::tr("Sleep position in degrees"),  QObject::tr("Orientation"),  QObject::tr("degrees"),
                        DEFAULT,  QColor("dark blue")));
    schema::channel.add(GRP_POS, new Channel(POS_Inclination         = 0x2991, DATA,    SESSION,
                        "Inclination",      QObject::tr("Inclination"),
                        QObject::tr("Upright angle in degrees"),  QObject::tr("Inclination"),  QObject::tr("degrees"),
                        DEFAULT,  QColor("dark magenta")));



    NoChannel = 0;
    //    CPAP_IPAP=schema::channel["IPAP"].id();
    //    CPAP_IPAPLo=schema::channel["IPAPLo"].id();
    //    CPAP_IPAPHi=schema::channel["IPAPHi"].id();
    //    CPAP_EPAP=schema::channel["EPAP"].id();
    //    CPAP_Pressure=schema::channel["Pressure"].id();
    //    CPAP_PS=schema::channel["PS"].id();
    //    CPAP_PSMin=schema::channel["PSMin"].id();
    //    CPAP_PSMax=schema::channel["PSMax"].id();
    CPAP_Mode = schema::channel["PAPMode"].id();
    CPAP_BrokenSummary = schema::channel["BrokenSummary"].id();
    CPAP_BrokenWaveform = schema::channel["BrokenWaveform"].id();
    //    CPAP_PressureMin=schema::channel["PressureMin"].id();
    //    CPAP_PressureMax=schema::channel["PressureMax"].id();
    //    CPAP_RampTime=schema::channel["RampTime"].id();
    //    CPAP_RampPressure=schema::channel["RampPressure"].id();
    //    CPAP_Obstructive=schema::channel["Obstructive"].id();
    //    CPAP_Hypopnea=schema::channel["Hypopnea"].id();
    //    CPAP_ClearAirway=schema::channel["ClearAirway"].id();
    //    CPAP_Apnea=schema::channel["Apnea"].id();
    //    CPAP_CSR=schema::channel["CSR"].id();
    //    CPAP_LeakFlag=schema::channel["LeakFlag"].id();
    //    CPAP_ExP=schema::channel["ExP"].id();
    //    CPAP_NRI=schema::channel["NRI"].id();
    //    CPAP_VSnore=schema::channel["VSnore"].id();
    //    CPAP_VSnore2=schema::channel["VSnore2"].id();
    //    CPAP_RERA=schema::channel["RERA"].id();
    //    CPAP_PressurePulse=schema::channel["PressurePulse"].id();
    //    CPAP_FlowLimit=schema::channel["FlowLimit"].id();
    //    CPAP_FlowRate=schema::channel["FlowRate"].id();
    //    CPAP_MaskPressure=schema::channel["MaskPressure"].id();
    //    CPAP_MaskPressureHi=schema::channel["MaskPressureHi"].id();
    //    CPAP_RespEvent=schema::channel["RespEvent"].id();
    //    CPAP_Snore=schema::channel["Snore"].id();
    //    CPAP_MinuteVent=schema::channel["MinuteVent"].id();
    //    CPAP_RespRate=schema::channel["RespRate"].id();
    //    CPAP_TidalVolume=schema::channel["TidalVolume"].id();
    //    CPAP_PTB=schema::channel["PTB"].id();
    //    CPAP_Leak=schema::channel["Leak"].id();
    //    CPAP_LeakMedian=schema::channel["LeakMedian"].id();
    //    CPAP_LeakTotal=schema::channel["LeakTotal"].id();
    //    CPAP_MaxLeak=schema::channel["MaxLeak"].id();
    //    CPAP_FLG=schema::channel["FLG"].id();
    //    CPAP_IE=schema::channel["IE"].id();
    //    CPAP_Te=schema::channel["Te"].id();
    //    CPAP_Ti=schema::channel["Ti"].id();
    //    CPAP_TgMV=schema::channel["TgMV"].id();
    CPAP_Test1 = schema::channel["TestChan1"].id();
    CPAP_Test2 = schema::channel["TestChan2"].id();

    CPAP_PresReliefSet = schema::channel["PresRelSet"].id();
    CPAP_PresReliefMode = schema::channel["PresRelMode"].id();
    CPAP_PresReliefType = schema::channel["PresRelType"].id();

    //    CPAP_UserFlag1=schema::channel["UserFlag1"].id();
    //    CPAP_UserFlag2=schema::channel["UserFlag2"].id();
    //    CPAP_UserFlag3=schema::channel["UserFlag3"].id();
    RMS9_E01 = schema::channel["RMS9_E01"].id();
    RMS9_E02 = schema::channel["RMS9_E02"].id();
    RMS9_EPR = schema::channel["EPR"].id();
    RMS9_EPRSet = schema::channel["EPRSet"].id();
    RMS9_SetPressure = schema::channel["SetPressure"].id();
    PRS1_00 = schema::channel["PRS1_00"].id();
    PRS1_01 = schema::channel["PRS1_01"].id();
    PRS1_08 = schema::channel["PRS1_08"].id();
    PRS1_0A = schema::channel["PRS1_0A"].id();
    PRS1_0B = schema::channel["PRS1_0B"].id();
    PRS1_0C = schema::channel["PRS1_0C"].id();
    PRS1_0E = schema::channel["PRS1_0E"].id();
    PRS1_0F = schema::channel["PRS1_0F"].id();
    PRS1_12 = schema::channel["PRS1_12"].id();
    PRS1_FlexMode = schema::channel["FlexMode"].id();
    PRS1_FlexSet = schema::channel["FlexSet"].id();
    PRS1_HumidStatus = schema::channel["HumidStat"].id();
    CPAP_HumidSetting = schema::channel["HumidSet"].id();
    PRS1_SysLock = schema::channel["SysLock"].id();
    PRS1_SysOneResistStat = schema::channel["SysOneResistStat"].id();
    PRS1_SysOneResistSet = schema::channel["SysOneResistSet"].id();
    PRS1_HoseDiam = schema::channel["HoseDiam"].id();
    PRS1_AutoOn = schema::channel["AutoOn"].id();
    PRS1_AutoOff = schema::channel["AutoOff"].id();
    PRS1_MaskAlert = schema::channel["MaskAlert"].id();
    PRS1_ShowAHI = schema::channel["ShowAHI"].id();
    INTELLIPAP_Unknown1 = schema::channel["IntUnk1"].id();
    INTELLIPAP_Unknown2 = schema::channel["IntUnk2"].id();
    //    OXI_Pulse=schema::channel["Pulse"].id();
    //    OXI_SPO2=schema::channel["SPO2"].id();
    //    OXI_PulseChange=schema::channel["PulseChange"].id();
    //    OXI_SPO2Drop=schema::channel["SPO2Drop"].id();
    //    OXI_Plethy=schema::channel["Plethy"].id();
    //    CPAP_AHI=schema::channel["AHI"].id();
    //    CPAP_RDI=schema::channel["RDI"].id();
    Journal_Notes = schema::channel["Journal"].id();
    Journal_Weight = schema::channel["Weight"].id();
    Journal_BMI = schema::channel["BMI"].id();
    Journal_ZombieMeter = schema::channel["ZombieMeter"].id();
    Bookmark_Start = schema::channel["BookmarkStart"].id();
    Bookmark_End = schema::channel["BookmarkEnd"].id();
    Bookmark_Notes = schema::channel["BookmarkNotes"].id();

    ZEO_SleepStage = schema::channel["SleepStage"].id();
    ZEO_ZQ = schema::channel["ZeoZQ"].id();
    ZEO_Awakenings = schema::channel["Awakenings"].id();
    ZEO_MorningFeel = schema::channel["MorningFeel"].id();
    ZEO_TimeInWake = schema::channel["TimeInWake"].id();
    ZEO_TimeInREM = schema::channel["TimeInREM"].id();
    ZEO_TimeInLight = schema::channel["TimeInLight"].id();
    ZEO_TimeInDeep = schema::channel["TimeInDeep"].id();
    ZEO_TimeToZ = schema::channel["TimeToZ"].id();
}

Channel::Channel(ChannelID id, ChanType type, ScopeType scope, QString code, QString fullname,
                 QString description, QString label, QString unit, DataType datatype, QColor color, int link):
    m_id(id),
    m_type(type),
    m_scope(scope),
    m_code(code),
    m_fullname(fullname),
    m_description(description),
    m_label(label),
    m_unit(unit),
    m_datatype(datatype),
    m_defaultcolor(color),
    m_link(link),
    m_upperThreshold(0),
    m_lowerThreshold(0),
    m_upperThresholdColor(Qt::red),
    m_lowerThresholdColor(Qt::green)
{
}
bool Channel::isNull()
{
    return (this == &EmptyChannel);
}

ChannelList::ChannelList()
    : m_doctype("channels")
{
}
ChannelList::~ChannelList()
{
    for (QHash<ChannelID, Channel *>::iterator i = channels.begin(); i != channels.end(); i++) {
        delete i.value();
    }
}
bool ChannelList::Load(QString filename)
{
    QDomDocument doc(m_doctype);
    QFile file(filename);
    qDebug() << "Opening " << filename;

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open" << filename;
        return false;
    }

    QString errorMsg;
    int errorLine;

    if (!doc.setContent(&file, false, &errorMsg, &errorLine)) {
        qWarning() << "Invalid XML Content in" << filename;
        qWarning() << "Error line" << errorLine << ":" << errorMsg;
        return false;
    }

    file.close();


    QDomElement root = doc.documentElement();

    if (root.tagName().toLower() != "channels") {
        return false;
    }

    QString language = root.attribute("language", "en");
    QString version = root.attribute("version", "");

    if (version.isEmpty()) {
        qWarning() << "No Version Field in" << m_doctype << "Schema, assuming 1.0" << filename;
        version = "1.0";
    }

    qDebug() << "Processing xml file:" << m_doctype << language << version;
    QDomNodeList grp = root.elementsByTagName("group");
    QDomNode node, n, ch;
    QDomElement e;

    bool ok;
    int id, linkid;
    QString chantype, scopestr, typestr, name, group, idtxt, details, label, unit, datatypestr,
            defcolor, link;
    ChanType type;
    DataType datatype;
    Channel *chan;
    QColor color;
    //bool multi;
    ScopeType scope;
    int line;

    for (int i = 0; i < grp.size(); i++) {
        node = grp.at(i);
        group = node.toElement().attribute("name");
        //qDebug() << "Group Name" << group;
        // Why do I have to skip the first node here? (shows up empty)
        n = node.firstChildElement();

        while (!n.isNull()) {
            line = n.lineNumber();
            e = n.toElement();

            if (e.nodeName().toLower() != "channel") {
                qWarning() << "Ignoring unrecognized schema type " << e.nodeName() << "in" << filename << "line" <<
                           line;
                continue;
            }

            ch = n.firstChild();
            n = n.nextSibling();

            idtxt = e.attribute("id");
            id = idtxt.toInt(&ok, 16);

            if (!ok) {
                qWarning() << "Dodgy ID number " << e.nodeName() << "in" << filename << "line" << line;
                continue;
            }

            chantype = e.attribute("class", "data").toLower();

            if (!ChanTypes.contains(chantype)) {
                qWarning() << "Dodgy class " << chantype << "in" << filename << "line" << line;
                continue;
            }

            type = ChanTypes[chantype];

            scopestr = e.attribute("scope", "session");

            if (scopestr.at(0) == QChar('!')) {
                scopestr = scopestr.mid(1);
                //multi=true;
            } //multi=false;

            if (!Scopes.contains(scopestr)) {
                qWarning() << "Dodgy Scope " << scopestr << "in" << filename << "line" << line;
                continue;
            }

            scope = Scopes[scopestr];
            name = e.attribute("name", "");
            details = e.attribute("details", "");
            label = e.attribute("label", "");

            if (name.isEmpty() || details.isEmpty() || label.isEmpty()) {
                qWarning() << "Missing name,details or label attribute in" << filename << "line" << line;
                continue;
            }

            unit = e.attribute("unit");

            defcolor = e.attribute("color", "black");
            color = QColor(defcolor);

            if (!color.isValid()) {
                qWarning() << "Invalid Color " << defcolor << "in" << filename << "line" << line;
                color = Qt::black;
            }

            datatypestr = e.attribute("type", "").toLower();

            link = e.attribute("link", "");

            if (!link.isEmpty()) {
                linkid = link.toInt(&ok, 16);

                if (!ok) {
                    qWarning() << "Dodgy Link ID number " << e.nodeName() << "in" << filename << " line" << line;
                }
            } else { linkid = 0; }

            if (DataTypes.contains(datatypestr)) {
                datatype = DataTypes[typestr];
            } else {
                qWarning() << "Ignoring unrecognized schema datatype in" << filename << "line" << line;
                continue;
            }

            if (channels.contains(id)) {
                qWarning() << "Schema already contains id" << id << "in" << filename << "line" << line;
                continue;
            }

            if (names.contains(name)) {
                qWarning() << "Schema already contains name" << name << "in" << filename << "line" << line;
                continue;
            }

            chan = new Channel(id, type, scope, name, name, details, label, unit, datatype, color, linkid);
            channels[id] = chan;
            names[name] = chan;
            //qDebug() << "Channel" << id << name << label;
            groups[group][name] = chan;

            if (linkid > 0) {
                if (channels.contains(linkid)) {
                    Channel *it = channels[linkid];
                    it->m_links.push_back(chan);
                    //int i=0;
                } else {
                    qWarning() << "Linked channel must be defined first in" << filename << "line" << line;
                }
            }

            // process children
            while (!ch.isNull()) {
                e = ch.toElement();
                QString sub = ch.nodeName().toLower();
                QString id2str, name2str;
                int id2;

                if (sub == "option") {
                    id2str = e.attribute("id");
                    id2 = id2str.toInt(&ok, 10);
                    name2str = e.attribute("value");
                    //qDebug() << sub << id2 << name2str;
                    chan->addOption(id2, name2str);
                } else if (sub == "color") {
                }

                ch = ch.nextSibling();
            }
        }
    }

    return true;
}

void ChannelList::add(QString group, Channel *chan)
{
    Q_ASSERT(chan != nullptr);

    if (channels.contains(chan->id())) {
        qWarning() << "Channels already contains id" << chan->id() << chan->code();
        Q_ASSERT(false);
        return;
    }

    if (names.contains(chan->code())) {
        qWarning() << "Channels already contains name" << chan->id() << chan->code();
        Q_ASSERT(false);
        return;
    }

    channels[chan->id()] = chan;
    names[chan->code()] = chan;
    groups[group][chan->code()] = chan;

    if (channels.contains(chan->linkid())) {
        Channel *it = channels[chan->linkid()];
        it->m_links.push_back(chan);
        //int i=0;
    } else {
        if (chan->linkid()>0) {
            qWarning() << "Linked channel must be defined first for" << chan->code();
        }
    }
}

bool ChannelList::Save(QString filename)
{
    Q_UNUSED(filename)
    return false;
}

}

//typedef schema::Channel * ChannelID;
