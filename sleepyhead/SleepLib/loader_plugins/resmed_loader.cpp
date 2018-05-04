/* SleepLib ResMed Loader Implementation
 *
 * Copyright (c) 2011-2018 Mark Watkins <mark@jedimark.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QApplication>
#include <QString>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProgressBar>
#include <QTextStream>
#include <QDebug>
#include <QStringList>
#include <cmath>

#include "resmed_loader.h"

#include "SleepLib/session.h"
#include "SleepLib/calcs.h"

#ifdef DEBUG_EFFICIENCY
#include <QElapsedTimer>  // only available in 4.8
#endif

extern QProgressBar *qprogress;

QHash<QString, QList<quint16> > Resmed_Model_Map;

ChannelID RMS9_EPR, RMS9_EPRLevel, RMS9_Mode, RMS9_SmartStart, RMS9_HumidStatus, RMS9_HumidLevel,
         RMS9_PtAccess, RMS9_Mask, RMS9_ABFilter, RMS9_ClimateControl, RMS9_TubeType,
         RMS9_Temp, RMS9_TempEnable, RMS9_RampEnable;

const QString STR_ResMed_AirSense10 = "AirSense 10";
const QString STR_ResMed_AirCurve10= "AirCurve 10";
const QString STR_ResMed_S9 = "S9";
const QString STR_UnknownModel = "Resmed S9 ???";

// Return the model name matching the supplied model number.
const QString & lookupModel(quint16 model)
{

    QHash<QString, QList<quint16> >::iterator end = Resmed_Model_Map.end();
    for (QHash<QString, QList<quint16> >::iterator it = Resmed_Model_Map.begin(); it != end; ++it) {
        QList<quint16> & list = it.value();
        for (int i=0; i < list.size(); ++i) {
            if (list.at(i) == model) {
                return it.key();
            }
        }
    }
    return STR_UnknownModel;
}

QHash<ChannelID, QStringList> resmed_codes;

const QString STR_ext_TGT = "tgt";
const QString STR_ext_CRC = "crc";


ResMedEDFParser::ResMedEDFParser(QString filename)
    :EDFParser(filename)
{
}
ResMedEDFParser::~ResMedEDFParser()
{
}

// Looks up foreign language Signal names that match this channelID
EDFSignal *ResMedEDFParser::lookupSignal(ChannelID ch)
{
    // Get list of all known foreign language names for this channel
    QHash<ChannelID, QStringList>::iterator channames = resmed_codes.find(ch);

    if (channames == resmed_codes.end()) {
        // no alternatives strings found for this channel
        return nullptr;
    }

    // This is bad, because ResMed thinks it was a cool idea to use two channels with the same name.

    // Scan through EDF's list of signals to see if any match
    for (int i = 0; i < channames.value().size(); i++) {
        EDFSignal *sig = lookupLabel(channames.value()[i]);
        if (sig) return sig;
    }

    // Failed
    return nullptr;
}

// Check if given string matches any alternative signal names for this channel
bool matchSignal(ChannelID ch, const QString & name)
{
    QHash<ChannelID, QStringList>::iterator channames = resmed_codes.find(ch);

    if (channames == resmed_codes.end()) {
        return false;
    }

    QStringList & strings = channames.value();
    int size = strings.size();

    for (int i=0; i < size; ++i) {
        // Using starts with, because ResMed is very lazy about consistency
        if (name.startsWith(strings.at(i), Qt::CaseInsensitive)) {
                return true;
        }
    }
    return false;
}

// This function parses a list of STR files and creates a date ordered map of individual records
void ResmedLoader::ParseSTR(Machine *mach, QMap<QDate, STRFile> & STRmap)
{
    if (!qprogress) {
        qWarning() << "What happened to qprogress object in ResmedLoader::ParseSTR()";
        return;
    }

    QMap<QDate, STRFile>::iterator it;

    for (it = STRmap.begin(); it!= STRmap.end(); ++it) {
        STRFile & file = it.value();
        QString & strfile = file.filename;
        ResMedEDFParser & str = *file.edf;

        QDate date = str.startdate_orig.date(); // each STR.edf record starts at 12 noon

        qDebug() << "Parsing" << strfile << date << str.GetNumDataRecords() << str.GetNumSignals();

        // ResMed and their consistent naming and spacing... :/
        EDFSignal *maskon = str.lookupLabel("Mask On");
        if (!maskon) {
            maskon = str.lookupLabel("MaskOn");
        }
        EDFSignal *maskoff = str.lookupLabel("Mask Off");
        if (!maskoff) {
             maskoff = str.lookupLabel("MaskOff");
        }

        EDFSignal *sig = nullptr;

        int size = str.GetNumDataRecords();
        int cnt=0;

        // For each data record, representing 1 day each
        for (int rec = 0; rec < size; ++rec, date = date.addDays(1)) {

            QMap<QDate, ResMedDay>::iterator rit = resdayList.find(date);
            if (rit != resdayList.end()) {
                // Already seen this record.. should check if the data is the same, but meh.
                continue;
            }

            int recstart = rec * maskon->nr;

            bool validday = false;
            for (int s = 0; s < maskon->nr; ++s) {
                qint32 on = maskon->data[recstart + s];
                qint32 off = maskoff->data[recstart + s];

                if ((on >= 0) && (off >= 0)) validday=true;
            }
            if (!validday) {
                // There are no mask on/off events, so this STR day is useless.
                continue;
            }

            rit = resdayList.insert(date, ResMedDay());

            STRRecord &R = rit.value().str;

            uint timestamp = QDateTime(date,QTime(12,0,0)).toTime_t();
            R.date = date;

           // skipday = false;
            if ((cnt++ % 10) == 0) {
                qprogress->setValue(10.0 + (float(cnt) / float(size) * 90.0));
                QApplication::processEvents();
            }

            // For every mask on, there will be a session within 1 minute either way
            // We can use that for data matching
            // Scan the mask on/off events by minute
            R.maskon.resize(maskon->nr);
            R.maskoff.resize(maskoff->nr);
            for (int s = 0; s < maskon->nr; ++s) {
                qint32 on = maskon->data[recstart + s];
                qint32 off = maskoff->data[recstart + s];

                R.maskon[s] = (on>0) ? (timestamp + (on * 60)) : 0;
                R.maskoff[s] = (off>0) ? (timestamp + (off * 60)) : 0;
            }


            // two conditions that need dealing with, mask running at noon start, and finishing at noon start..
            // (Sessions are forcibly split by resmed.. why the heck don't they store it that way???)
            if ((R.maskon[0]==0) && (R.maskoff[0]>0)) {
                R.maskon[0] = timestamp;
            }
            if ((R.maskon[maskon->nr-1] > 0) && (R.maskoff[maskoff->nr-1] == 0)) {
                R.maskoff[maskoff->nr-1] = QDateTime(date,QTime(12,0,0)).addDays(1).toTime_t() - 1;
            }

            CPAPMode mode = MODE_UNKNOWN;

            if ((sig = str.lookupSignal(CPAP_Mode))) {
                int mod = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                R.rms9_mode = mod;

                if (mod == 11) {
                    mode = MODE_APAP; // For her
                } else if (mod >= 8) {       // mod 8 == vpap adapt variable epap
                    mode = MODE_ASV_VARIABLE_EPAP;
                } else if (mod >= 7) {       // mod 7 == vpap adapt
                    mode = MODE_ASV;
                } else if (mod >= 6) { // mod 6 == vpap auto (Min EPAP, Max IPAP, PS)
                    mode = MODE_BILEVEL_AUTO_FIXED_PS;
                } else if (mod >= 3) {// mod 3 == vpap s fixed pressure (EPAP, IPAP, No PS)
                    mode = MODE_BILEVEL_FIXED;
                    // 4,5 are S/T types...

                } else if (mod >= 1) {
                    mode = MODE_APAP; // mod 1 == apap
                    // not sure what mode 2 is ?? split ?
                } else {
                    mode = MODE_CPAP; // mod 0 == cpap
                }
                R.mode = mode;

                // Settings.CPAP.Starting Pressure
                if ((mod == 0) && (sig = str.lookupLabel("S.C.StartPress"))) {
                    R.ramp_pressure = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                // Settings.Adaptive Starting Pressure? // mode 11 = APAP for her?
                if (((mod == 1) || (mod == 11)) && (sig = str.lookupLabel("S.AS.StartPress"))) {
                    R.ramp_pressure = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }

                if ((R.mode == MODE_BILEVEL_FIXED) && (sig = str.lookupLabel("S.BL.StartPress"))) {
                    // Bilevel Starting Pressure
                    R.ramp_pressure = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if (((R.mode == MODE_ASV) || (R.mode == MODE_ASV_VARIABLE_EPAP)) && (sig = str.lookupLabel("S.VA.StartPress"))) {
                    // Bilevel Starting Pressure
                    R.ramp_pressure = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
            }

            if ((sig = str.lookupLabel("Mask Dur"))) {
                R.maskdur = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if ((sig = str.lookupLabel("Leak Med")) || (sig = str.lookupLabel("Leak.50"))) {
                float gain = sig->gain * 60.0;
                R.leak50 = EventDataType(sig->data[rec]) * gain + sig->offset;
            }
            if ((sig = str.lookupLabel("Leak Max"))|| (sig = str.lookupLabel("Leak.Max"))) {
                float gain = sig->gain * 60.0;
                R.leakmax = EventDataType(sig->data[rec]) * gain + sig->offset;
            }
            if ((sig = str.lookupLabel("Leak 95")) || (sig = str.lookupLabel("Leak.95"))) {
                float gain = sig->gain * 60.0;
                R.leak95 = EventDataType(sig->data[rec]) * gain + sig->offset;
            }

            if ((sig = str.lookupLabel("RespRate.50"))) {
                R.rr50 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("RespRate.Max"))) {
                R.rrmax = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("RespRate.95"))) {
                R.rr95 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("MinVent.50"))) {
                R.mv50 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("MinVent.Max"))) {
                R.mvmax = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("MinVent.95"))) {
                R.mv95 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("TidVol.50"))) {
                R.tv50 = EventDataType(sig->data[rec]) * (sig->gain*1000.0) + sig->offset;
            }
            if ((sig = str.lookupLabel("TidVol.Max"))) {
                R.tvmax = EventDataType(sig->data[rec]) * (sig->gain*1000.0) + sig->offset;
            }
            if ((sig = str.lookupLabel("TidVol.95"))) {
                R.tv95 = EventDataType(sig->data[rec]) * (sig->gain*1000.0) + sig->offset;
            }

            if ((sig = str.lookupLabel("MaskPress.50"))) {
                R.mp50 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("MaskPress.Max"))) {
                R.mpmax = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("MaskPress.95"))) {
                R.mp95 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if ((sig = str.lookupLabel("TgtEPAP.50"))) {
                R.tgtepap50 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("TgtEPAP.Max"))) {
                R.tgtepapmax = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("TgtEPAP.95"))) {
                R.tgtepap95 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if ((sig = str.lookupLabel("TgtIPAP.50"))) {
                R.tgtipap50 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("TgtIPAP.Max"))) {
                R.tgtipapmax = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("TgtIPAP.95"))) {
                R.tgtipap95 = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }


            bool haveipap = false;
//                if (R.mode == MODE_BILEVEL_FIXED) {
                if ((sig = str.lookupSignal(CPAP_IPAP))) {
                    R.ipap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                    haveipap = true;
                }

                if ((sig = str.lookupSignal(CPAP_EPAP))) {
                    R.epap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }


            if (R.mode == MODE_ASV) {
                if ((sig = str.lookupLabel("S.AV.StartPress"))) {
                    EventDataType sp = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                    R.ramp_pressure = sp;
                }
                if ((sig = str.lookupLabel("S.AV.EPAP"))) {
                    R.min_epap = R.max_epap = R.epap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;

                }
                if ((sig = str.lookupLabel("S.AV.MinPS"))) {
                    R.min_ps = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if ((sig = str.lookupLabel("S.AV.MaxPS"))) {
                    R.max_ps = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                    R.max_ipap = R.epap + R.max_ps;
                    R.min_ipap = R.epap + R.min_ps;
                }
            }
            if (R.mode == MODE_ASV_VARIABLE_EPAP) {
                if ((sig = str.lookupLabel("S.AA.StartPress"))) {
                    EventDataType sp = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                    R.ramp_pressure = sp;
                }
                if ((sig = str.lookupLabel("S.AA.MinEPAP"))) {
                    R.min_epap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if ((sig = str.lookupLabel("S.AA.MaxEPAP"))) {
                    R.max_epap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if ((sig = str.lookupLabel("S.AA.MinPS"))) {
                    R.min_ps = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if ((sig = str.lookupLabel("S.AA.MaxPS"))) {
                    R.max_ps = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                    R.max_ipap = R.max_epap + R.max_ps;
                    R.min_ipap = R.min_epap + R.min_ps;
                }
            }

            if ((sig = str.lookupSignal(CPAP_PressureMax))) {
                R.max_pressure = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupSignal(CPAP_PressureMin))) {
                R.min_pressure = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupSignal(RMS9_SetPressure))) {
                R.set_pressure = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if ((sig = str.lookupSignal(CPAP_EPAPHi))) {
                R.max_epap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupSignal(CPAP_EPAPLo))) {
                R.min_epap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if ((sig = str.lookupSignal(CPAP_IPAPHi))) {
                R.max_ipap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                haveipap = true;
            }
            if ((sig = str.lookupSignal(CPAP_IPAPLo))) {
                R.min_ipap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                haveipap = true;
            }
            if ((sig = str.lookupSignal(CPAP_PS))) {
                R.ps = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            //  }


            // Okay, problem here: THere are TWO PSMin & MAX values on the 36037 with the same string
            // One is for ASV mode, and one is for ASVAuto
            int psvar = (mode == MODE_ASV_VARIABLE_EPAP) ? 1 : 0;

            if ((sig = str.lookupLabel("Max PS", psvar))) {
                R.max_ps = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("Min PS", psvar))) {
                R.min_ps = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if (!haveipap) {
            }

            if (mode == MODE_ASV_VARIABLE_EPAP) {
                R.min_ipap = R.min_epap + R.min_ps;
                R.max_ipap = R.max_epap + R.max_ps;
            } else if (mode == MODE_ASV) {
                R.min_ipap = R.epap + R.min_ps;
                R.max_ipap = R.epap + R.max_ps;
            }

            EventDataType epr = -1, epr_level = -1;
            bool a10 = false;
            if ((mode == MODE_CPAP) || (mode == MODE_APAP)) {
                if ((sig = str.lookupSignal(RMS9_EPR))) {
                    epr= EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if ((sig = str.lookupSignal(RMS9_EPRLevel))) {
                    epr_level= EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }

                if ((sig = str.lookupLabel("S.EPR.EPRType"))) {
                    a10 = true;
                    epr = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                    epr += 1;
                }
                int epr_on=0, clin_epr_on=0;
                if ((sig = str.lookupLabel("S.EPR.EPREnable"))) { // first check machines opinion
                    a10 = true;
                    epr_on = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if (epr_on && (sig = str.lookupLabel("S.EPR.ClinEnable"))) {
                    a10 = true;
                    clin_epr_on = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if (a10 && !(epr_on && clin_epr_on)) {
                    epr = 0;
                    epr_level = 0;
                }
            }

            if ((epr >= 0) && (epr_level >= 0)) {
                R.epr_level = epr_level;
                R.epr = epr;
            } else {
                if (epr >= 0) {
                    static bool warn=false;
                    if (!warn) { // just nag once
                        qDebug() << "If you can read this, please tell Jedimark you found a ResMed with EPR but no EPR_LEVEL so he can remove this warning";
                        warn = true;
                    }

                    R.epr = (epr > 0) ? 1 : 0;
                    R.epr_level = epr;
                } else if (epr_level >= 0) {
                    R.epr_level = epr_level;
                    R.epr = (epr_level > 0) ? 1 : 0;
                }
            }

            if ((sig = str.lookupLabel("AHI"))) {
                R.ahi = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("AI"))) {
                R.ai = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("HI"))) {
                R.hi = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("UAI"))) {
                R.uai = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("CAI"))) {
                R.cai = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if ((sig = str.lookupLabel("S.RampTime"))) {
                R.s_RampTime = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.RampEnable"))) {
                R.s_RampEnable = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.EPR.ClinEnable"))) {
                R.s_EPR_ClinEnable = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.EPR.EPREnable"))) {
                R.s_EPREnable = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if ((sig = str.lookupLabel("S.ABFilter"))) {
                R.s_ABFilter = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if ((sig = str.lookupLabel("S.ClimateControl"))) {
                R.s_ClimateControl = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }

            if ((sig = str.lookupLabel("S.Mask"))) {
                R.s_Mask = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.PtAccess"))) {
                R.s_PtAccess = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.SmartStart"))) {
                R.s_SmartStart = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.HumEnable"))) {
                R.s_HumEnable = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.HumLevel"))) {
                R.s_HumLevel = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.TempEnable"))) {
                R.s_TempEnable = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.Temp"))) {
                R.s_Temp = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
            if ((sig = str.lookupLabel("S.Tube"))) {
                R.s_Tube = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
            }
        }
    }
}




/*void ResmedImport::run()
{
    loader->saveMutex.lock();

    Session * sess = mach->SessionExists(sessionid);
    if (sess) {
        if (sess->summaryOnly()) {
            // Reuse this session
            sess->wipeSummary();
        } else {
            // Already imported
            loader->saveMutex.unlock();
            return;
        }
    } else {
        // Could be importing from an older backup.. if so, destroy the summary only records
        quint32 key = int(sessionid / 60) * 60;

        sess = mach->SessionExists(key);
        if (sess) {
            if (sess->summaryOnly()) {

                sess->Destroy();
                delete sess;
            }
        }

        // Create the session
        sess = new Session(mach, sessionid);
    }
    loader->saveMutex.unlock();

    if (files.contains(EDF_PLD)) {
        QStringList & sl = files[EDF_PLD];
        for (int i=0; i<sl.size(); ++i) {
            QString file = sl.at(i);
            loader->LoadPLD(sess, file);
#ifdef SESSION_DEBUG
            sess->session_files.append(file);
#endif
        }
    }
    if (files.contains(EDF_BRP)) {
        QStringList & sl = files[EDF_BRP];
        for (int i=0; i<sl.size(); ++i) {
            QString file = sl.at(i);

            loader->LoadBRP(sess, file);
#ifdef SESSION_DEBUG
            sess->session_files.append(file);
#endif
        }
    }
    if (files.contains(EDF_SAD)) {
        QStringList & sl = files[EDF_SAD];
        for (int i=0; i<sl.size(); ++i) {
            QString file = sl.at(i);
            loader->LoadSAD(sess, file);
#ifdef SESSION_DEBUG
            sess->session_files.append(file);
#endif
        }
    }

    if (files.contains(EDF_CSL)) {
        QStringList & sl = files[EDF_CSL];
        for (int i=0; i<sl.size(); ++i) {
            QString file = sl.at(i);
            loader->LoadCSL(sess, file);
#ifdef SESSION_DEBUG
            sess->session_files.append(file);
#endif
        }
    }

    bool haveeve = false;
    if (files.contains(EDF_EVE)) {
        QStringList & sl = files[EDF_EVE];
        for (int i=0; i<sl.size(); ++i) {
            QString file = sl.at(i);
            loader->LoadEVE(sess, file);
#ifdef SESSION_DEBUG
            sess->session_files.append(file);
#endif

            haveeve = true;
        }
    }

    if (!haveeve) {
        sess->AddEventList(CPAP_Obstructive, EVL_Event);
        sess->AddEventList(CPAP_ClearAirway, EVL_Event);
        sess->AddEventList(CPAP_Apnea, EVL_Event);
        sess->AddEventList(CPAP_Hypopnea, EVL_Event);
    }


    if (sess->first() == 0) {
       // loader->saveMutex.lock();

        //if (mach->sessionlist.contains(sess->session())) {
       // sess->Destroy();
            //mach->sessionlist.remove(sess->session());
        //}
        delete sess;
        //loader->saveMutex.unlock();
        return;
    }

    sess->setSummaryOnly(false);
    sess->SetChanged(true);

    /////////////////////////////////////////////////////////////////////////////////
    // Process STR.edf now all valid Session data is imported
    /////////////////////////////////////////////////////////////////////////////////

    quint32 key = quint32(sessionid / 60) * 60; // round to 1 minute

    QMap<quint32, STRRecord>::iterator strsess_end = loader->strsess.end();
    QMap<quint32, STRRecord>::iterator it = loader->strsess.find(key);

    if (it == strsess_end) {
        // ResMed merges mask on/off groups that are less than a minute apart
        // this means have to jump back to the last session closest.

        it = loader->strsess.lowerBound(key);
        if (it != loader->strsess.begin()) it--;
    }

    if (it != strsess_end) {
        STRRecord & R = it.value();

        // calculate the time between session record and mask-on record.
        int gap = sessionid - R.maskon;

        if (gap > 3600*6) {
            QDateTime dt = QDateTime::fromTime_t(sessionid);
            QDateTime rt = QDateTime::fromTime_t(R.maskon);

            QString msg = QString("Warning: Closest matching STR record for %1 is %2 by %3 seconds").
                    arg(dt.toString(Qt::ISODate)).
                    arg(sess->length() / 1000.0L,0,'f',1).
                    arg(gap);
            qDebug() << msg;
        }


        // Claim this session
        R.sessionid = sessionid;


        // Save maskon time in session setting so we can use it later to avoid doubleups.
        sess->settings[RMS9_MaskOnTime] = R.maskon;

#ifdef SESSION_DEBUG
        sess->session_files.append("STR.edf");
#endif

        if (R.mode >= 0) {
            sess->settings[CPAP_Mode] = R.mode;
            sess->settings[RMS9_Mode] = R.rms9_mode;
            if (R.mode == MODE_CPAP) {
                if (R.set_pressure >= 0) {
                    sess->settings[CPAP_Pressure] = R.set_pressure;
                }
            } else if (R.mode == MODE_APAP) {
                if (R.min_pressure >= 0) sess->settings[CPAP_PressureMin] = R.min_pressure;
                if (R.max_pressure >= 0) sess->settings[CPAP_PressureMax] = R.max_pressure;
            } else if (R.mode == MODE_BILEVEL_FIXED) {
                if (R.epap >= 0) sess->settings[CPAP_EPAP] = R.epap;
                if (R.ipap >= 0) sess->settings[CPAP_IPAP] = R.ipap;
                if (R.ps >= 0) sess->settings[CPAP_PS] = R.ps;
            } else if (R.mode == MODE_BILEVEL_AUTO_FIXED_PS) {
                if (R.min_epap >= 0) sess->settings[CPAP_EPAPLo] = R.min_epap;
                if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
                if (R.ps >= 0) sess->settings[CPAP_PS] = R.ps;
            } else if (R.mode == MODE_ASV) {
                if (R.epap >= 0) sess->settings[CPAP_EPAP] = R.epap;
                if (R.min_ps >= 0) sess->settings[CPAP_PSMin] = R.min_ps;
                if (R.max_ps >= 0) sess->settings[CPAP_PSMax] = R.max_ps;
                if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
            } else if (R.mode == MODE_ASV_VARIABLE_EPAP) {
                if (R.max_epap >= 0) sess->settings[CPAP_EPAPHi] = R.max_epap;
                if (R.min_epap >= 0) sess->settings[CPAP_EPAPLo] = R.min_epap;
                if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
                if (R.min_ipap >= 0) sess->settings[CPAP_IPAPLo] = R.min_ipap;
                if (R.min_ps >= 0) sess->settings[CPAP_PSMin] = R.min_ps;
                if (R.max_ps >= 0) sess->settings[CPAP_PSMax] = R.max_ps;
            }
        } else {
            if (R.set_pressure >= 0) sess->settings[CPAP_Pressure] = R.set_pressure;
            if (R.min_pressure >= 0) sess->settings[CPAP_PressureMin] = R.min_pressure;
            if (R.max_pressure >= 0) sess->settings[CPAP_PressureMax] = R.max_pressure;
            if (R.max_epap >= 0) sess->settings[CPAP_EPAPHi] = R.max_epap;
            if (R.min_epap >= 0) sess->settings[CPAP_EPAPLo] = R.min_epap;
            if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
            if (R.min_ipap >= 0) sess->settings[CPAP_IPAPLo] = R.min_ipap;
            if (R.min_ps >= 0) sess->settings[CPAP_PSMin] = R.min_ps;
            if (R.max_ps >= 0) sess->settings[CPAP_PSMax] = R.max_ps;
            if (R.ps >= 0) sess->settings[CPAP_PS] = R.ps;
            if (R.epap >= 0) sess->settings[CPAP_EPAP] = R.epap;
            if (R.ipap >= 0) sess->settings[CPAP_IPAP] = R.ipap;
        }

        if (R.epr >= 0) {
            sess->settings[RMS9_EPR] = (int)R.epr;
            if (R.epr > 0) {
                if (R.epr_level >= 0) {
                    sess->settings[RMS9_EPRLevel] = (int)R.epr_level;
                }
            }
        }

        if (R.s_RampEnable >= 0) {
            sess->settings[RMS9_RampEnable] = R.s_RampEnable;

            if (R.s_RampEnable >= 1) {
                if (R.s_RampTime >= 0) {
                    sess->settings[CPAP_RampTime] = R.s_RampTime;
                }
                if (R.ramp_pressure >= 0) {
                    sess->settings[CPAP_RampPressure] = R.ramp_pressure;
                }
            }
        }

        if (R.s_SmartStart >= 0) {
            sess->settings[RMS9_SmartStart] = R.s_SmartStart;
        }
        if (R.s_ABFilter >= 0) {
            sess->settings[RMS9_ABFilter] = R.s_ABFilter;
        }
        if (R.s_ClimateControl >= 0) {
            sess->settings[RMS9_ClimateControl] = R.s_ClimateControl;
        }
        if (R.s_Mask >= 0) {
            sess->settings[RMS9_Mask] = R.s_Mask;
        }
        if (R.s_PtAccess >= 0) {
            sess->settings[RMS9_PtAccess] = R.s_PtAccess;
        }

        if (R.s_HumEnable >= 0) {
            sess->settings[RMS9_HumidStatus] = (short)R.s_HumEnable;
            if ((R.s_HumEnable >= 1) && (R.s_HumLevel >= 0)) {
                sess->settings[RMS9_HumidLevel] = (short)R.s_HumLevel;
            }
        }
        if (R.s_TempEnable >= 0) {
            sess->settings[RMS9_TempEnable] = (short)R.s_TempEnable;
            if ((R.s_TempEnable >= 1) && (R.s_Temp >= 0)){
                sess->settings[RMS9_Temp] = (short)R.s_Temp;
            }
        }

        // Ignore all the rest of the sumary data, because there is enough available to calculate it with higher accuracy.

        if (sess->length() > 0) {
            loader->addSession(sess);
        } else {
            delete sess;
            return;
        }
    }
    CPAPMode mode = (CPAPMode)sess->settings[CPAP_Mode].toInt();

    if ((mode >= MODE_BILEVEL_FIXED) && (!sess->eventlist.contains(CPAP_IPAP))) {
        QVector<EventList *> & evl = sess->eventlist[CPAP_Pressure];

        for (int i=0; i<evl.size(); ++i) {
            EventList * el = evl[i];
            sess->eventlist[CPAP_IPAP].push_back(el);
        }

        sess->eventlist.remove(CPAP_Pressure);
    }

    // Update indexes, process waveform and perform flagging
    sess->UpdateSummaries();

    // Save is not threadsafe?
   // loader->saveMutex.lock();
    sess->Store(mach->getDataPath());
   // loader->saveMutex.unlock();

    // Free the memory used by this session
    sess->TrashEvents();
} */

ResmedLoader::ResmedLoader()
{
    const QString RMS9_ICON = ":/icons/rms9.png";
    const QString RM10_ICON = ":/icons/airsense10.png";
    const QString RM10C_ICON = ":/icons/aircurve.png";

    m_pixmaps[STR_ResMed_S9] = QPixmap(RMS9_ICON);
    m_pixmap_paths[STR_ResMed_S9] = RMS9_ICON;
    m_pixmaps[STR_ResMed_AirSense10] = QPixmap(RM10_ICON);
    m_pixmap_paths[STR_ResMed_AirSense10] = RM10_ICON;
    m_pixmaps[STR_ResMed_AirCurve10] = QPixmap(RM10C_ICON);
    m_pixmap_paths[STR_ResMed_AirCurve10] = RM10C_ICON;
    m_type = MT_CPAP;
}
ResmedLoader::~ResmedLoader()
{
}

/*void ResmedImportStage2::run()
{
    if (R.maskon == R.maskoff) return;
    Session * sess = new Session(mach, R.maskon);


    sess->really_set_first(qint64(R.maskon) * 1000L);
    sess->really_set_last(qint64(R.maskoff) * 1000L);

    // Claim this record for future imports
    sess->settings[RMS9_MaskOnTime] = R.maskon;
    sess->setSummaryOnly(true);

#ifdef SESSION_DEBUG
    sess->session_files.append("STR.edf");
#endif
    sess->SetChanged(true);

    // First take the settings

    if (R.mode >= 0) {
        sess->settings[CPAP_Mode] = R.mode;
        sess->settings[RMS9_Mode] = R.rms9_mode;

        if (R.mode == MODE_CPAP) {
            if (R.set_pressure >= 0) {
                sess->settings[CPAP_Pressure] = R.set_pressure;
            }
        } else if (R.mode == MODE_APAP) {
            if (R.min_pressure >= 0) sess->settings[CPAP_PressureMin] = R.min_pressure;
            if (R.max_pressure >= 0) sess->settings[CPAP_PressureMax] = R.max_pressure;
        } else if (R.mode == MODE_BILEVEL_FIXED) {
            if (R.epap >= 0) sess->settings[CPAP_EPAP] = R.epap;
            if (R.ipap >= 0) sess->settings[CPAP_IPAP] = R.ipap;
            if (R.ps >= 0) sess->settings[CPAP_PS] = R.ps;
        } else if (R.mode == MODE_BILEVEL_AUTO_FIXED_PS) {
            if (R.min_epap >= 0) sess->settings[CPAP_EPAPLo] = R.min_epap;
            if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
            if (R.ps >= 0) sess->settings[CPAP_PS] = R.ps;
        } else if (R.mode == MODE_ASV) {
            if (R.epap >= 0) sess->settings[CPAP_EPAP] = R.epap;
            if (R.min_ps >= 0) sess->settings[CPAP_PSMin] = R.min_ps;
            if (R.max_ps >= 0) sess->settings[CPAP_PSMax] = R.max_ps;
            if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
        } else if (R.mode == MODE_ASV_VARIABLE_EPAP) {
            if (R.max_epap >= 0) sess->settings[CPAP_EPAPHi] = R.max_epap;
            if (R.min_epap >= 0) sess->settings[CPAP_EPAPLo] = R.min_epap;
            if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
            if (R.min_ipap >= 0) sess->settings[CPAP_IPAPLo] = R.min_ipap;
            if (R.min_ps >= 0) sess->settings[CPAP_PSMin] = R.min_ps;
            if (R.max_ps >= 0) sess->settings[CPAP_PSMax] = R.max_ps;
        }
    } else {
        if (R.set_pressure >= 0) sess->settings[CPAP_Pressure] = R.set_pressure;
        if (R.min_pressure >= 0) sess->settings[CPAP_PressureMin] = R.min_pressure;
        if (R.max_pressure >= 0) sess->settings[CPAP_PressureMax] = R.max_pressure;
        if (R.max_epap >= 0) sess->settings[CPAP_EPAPHi] = R.max_epap;
        if (R.min_epap >= 0) sess->settings[CPAP_EPAPLo] = R.min_epap;
        if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
        if (R.min_ipap >= 0) sess->settings[CPAP_IPAPLo] = R.min_ipap;
        if (R.min_ps >= 0) sess->settings[CPAP_PSMin] = R.min_ps;
        if (R.max_ps >= 0) sess->settings[CPAP_PSMax] = R.max_ps;
        if (R.ps >= 0) sess->settings[CPAP_PS] = R.ps;
        if (R.epap >= 0) sess->settings[CPAP_EPAP] = R.epap;
        if (R.ipap >= 0) sess->settings[CPAP_IPAP] = R.ipap;
    }


    if (R.epr >= 0) {
        sess->settings[RMS9_EPR] = (int)R.epr;
        if (R.epr > 0) {
            if (R.epr_level >= 0) {
                sess->settings[RMS9_EPRLevel] = (int)R.epr_level;
            }
        }
    }
    if (R.leakmax >= 0) sess->setMax(CPAP_Leak, R.leakmax);
    if (R.leakmax >= 0) sess->setMin(CPAP_Leak, 0);
    if ((R.leakmed >= 0) && (R.leak95 >= 0) && (R.leakmax >= 0)) {
        sess->m_timesummary[CPAP_Leak][short(R.leakmax / R.leakgain)]=1;
        sess->m_timesummary[CPAP_Leak][short(R.leak95 / R.leakgain)]=9;
        sess->m_timesummary[CPAP_Leak][short(R.leakmed / R.leakgain)]=65;
        sess->m_timesummary[CPAP_Leak][0]=25;
    }


    // Find the matching date group for this record
    QMap<QDate, QList<STRRecord *> >::iterator dtit = loader->strdate.find(R.date);

    // should not be possible, but my brain hurts...
    if (dtit == loader->strdate.end()) {
        qWarning() << "ResmedImportStage2::run() ASSERT(dtit != loader->strdate.end()) failed";
        return;
    }

    if (dtit != loader->strdate.end()) {
        QList<STRRecord *> & dayrecs = dtit.value();
        bool hasdatasess=false;
        EventDataType time=0, totaltime=0;

        for (int c=0; c < dayrecs.size(); ++c) {
            STRRecord *r = dayrecs[c];
            if (r->sessionid > 0) {
                // get complicated.. calculate all the counts for valid sessions, and use the summary to make up the rest
                hasdatasess = true;
            }
            totaltime += r->maskoff - r->maskon;
        }


        if (!hasdatasess) {
            for (int c=0; c < dayrecs.size(); ++c) {
                STRRecord *r = dayrecs[c];
                time = r->maskoff - r->maskon;
                float ratio = time / totaltime;

                // Add the time weighted proportion of the events counts
                if (r->ai >= 0) {
                    sess->setCount(CPAP_Obstructive, r->ai * ratio);
                    sess->setCph(CPAP_Obstructive, (r->ai * ratio) / (time / 3600.0));
                }
                if (r->uai >= 0) {
                    sess->setCount(CPAP_Apnea, r->uai * ratio);
                    sess->setCph(CPAP_Apnea, (r->uai * ratio) / (time / 3600.0));
                }
                if (r->hi >= 0) {
                    sess->setCount(CPAP_Hypopnea, r->hi * ratio);
                    sess->setCph(CPAP_Hypopnea, (r->hi * ratio) / (time / 3600.0));
                }
                if (r->cai >= 0) {
                    sess->setCount(CPAP_ClearAirway, r->cai * ratio);
                    sess->setCph(CPAP_ClearAirway, (r->ai * ratio) / (time / 3600.0));
                }

            }

        }
    }

    loader->addSession(sess);
    //loader->saveMutex.lock();
    sess->Store(mach->getDataPath());
    //loader->saveMutex.unlock();
} */


long event_cnt = 0;

const QString RMS9_STR_datalog = "DATALOG";
const QString RMS9_STR_idfile = "Identification.";
const QString RMS9_STR_strfile = "STR.";

bool ResmedLoader::Detect(const QString & givenpath)
{
    QDir dir(givenpath);

    if (!dir.exists()) {
        return false;
    }

    // ResMed drives contain a folder named "DATALOG".
    if (!dir.exists(RMS9_STR_datalog)) {
        return false;
    }

    // They also contain a file named "STR.edf".
    if (!dir.exists("STR.edf")) {
        return false;
    }

    return true;
}


MachineInfo ResmedLoader::PeekInfo(const QString & path)
{
    if (!Detect(path)) return MachineInfo();

    QFile f(path+"/"+RMS9_STR_idfile+"tgt");

    // Abort if this file is dodgy..
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        return MachineInfo();
    }
    MachineInfo info = newInfo();

    // Parse # entries into idmap.
    while (!f.atEnd()) {
        QString line = f.readLine().trimmed();

        if (!line.isEmpty()) {
            QString key = line.section(" ", 0, 0).section("#", 1);
            QString value = line.section(" ", 1);

            if (key == "SRN") { // Serial Number
                info.serial = value;

            } else if (key == "PNA") {  // Product Name
                value.replace("_"," ");

                if (value.contains(STR_ResMed_S9)) {
                    value.replace(STR_ResMed_S9, "");
                    info.series = STR_ResMed_S9;
                } else if (value.contains(STR_ResMed_AirSense10)) {
                    value.replace(STR_ResMed_AirSense10, "");
                    info.series = STR_ResMed_AirSense10;
                } else if (value.contains(STR_ResMed_AirCurve10)) {
                    value.replace(STR_ResMed_AirCurve10, "");
                    info.series = STR_ResMed_AirCurve10;
                }
                value.replace("(","");
                value.replace(")","");
                if (value.contains("Adapt", Qt::CaseInsensitive)) {
                    if (!value.contains("VPAP")) {
                        value.replace("Adapt", QObject::tr("VPAP Adapt"));
                    }
                }
                info.model = value.trimmed();
            } else if (key == "PCD") { // Product Code
                info.modelnumber = value;
            }
        }
    }

    return info;
}




EDFType lookupEDFType(const QString & text)
{
    if (text == "EVE") {
        return EDF_EVE;
    } else if (text =="BRP") {
        return EDF_BRP;
    } else if (text == "PLD") {
        return EDF_PLD;
    } else if (text == "SAD") {
        return EDF_SAD;
    } else if (text == "CSL") {
        return EDF_CSL;
    } else return EDF_UNKNOWN;
}

// Pretend to parse the EVE file to get the duration out of it.
int PeekAnnotations(const QString & path, quint32 &start, quint32 &end)
{
    ResMedEDFParser edf(path);
    if (!edf.Parse())
        return -1;

    QString t;

    //double duration;
    char *data;
    char c;
    long pos;
    bool sign, ok;
    double d;
    double tt;

    int recs = 0;
    int goodrecs = 0;

    // Notes: Event records have useless duration record.

    start = edf.startdate / 1000L;
    // Process event annotation records
    for (int s = 0; s < edf.GetNumSignals(); s++) {
        recs = edf.edfsignals[s].nr * edf.GetNumDataRecords() * 2;

        data = (char *)edf.edfsignals[s].data;
        pos = 0;
        tt = edf.startdate;
        //duration = 0;

        while (pos < recs) {
            c = data[pos];

            if ((c != '+') && (c != '-')) {
                break;
            }

            if (data[pos++] == '+') { sign = true; }
            else { sign = false; }

            t = "";
            c = data[pos];

            do {
                t += c;
                pos++;
                c = data[pos];
            } while ((c != 20) && (c != 21)); // start code

            d = t.toDouble(&ok);

            if (!ok) {
                qDebug() << "Faulty EDF Annotations file " << edf.filename;
                break;
            }

            if (!sign) { d = -d; }

            tt = edf.startdate + qint64(d * 1000.0);

            //duration = 0;
            // First entry

            if (data[pos] == 21) {
                pos++;
                // get duration.
                t = "";

                do {
                    t += data[pos];
                    pos++;
                } while ((data[pos] != 20) && (pos < recs)); // start code

                //duration = t.toDouble(&ok);

                if (!ok) {
                    qDebug() << "Faulty EDF Annotations file (at %" << pos << ") " << edf.filename;
                    break;
                }
            }
            end = (tt / 1000.0);

            while ((data[pos] == 20) && (pos < recs)) {
                t = "";
                pos++;

                if (data[pos] == 0) {
                    break;
                }

                if (data[pos] == 20) {
                    pos++;
                    break;
                }

                do {
                    t += tolower(data[pos++]);
                } while ((data[pos] != 20) && (pos < recs)); // start code

                if (!t.isEmpty() && (t!="recording starts")) {
                    goodrecs++;
                }

                if (pos >= recs) {
                    qDebug() << "Short EDF Annotations file" << edf.filename;
                    break;
                }

            }

            while ((data[pos] == 0) && (pos < recs)) { pos++; }

            if (pos >= recs) { break; }
        }

    }
    return goodrecs;
}

///////////////////////////////////////////////////////////////////////////////
// Looks inside an EDF or EDF.gz and grabs the start and duration
///////////////////////////////////////////////////////////////////////////////
EDFduration getEDFDuration(const QString & filename)
{
    QString ext = filename.section("_", -1).section(".",0,0).toUpper();

    bool ok1, ok2;

    int num_records;
    double rec_duration;

    QDateTime startDate;

    if (!filename.endsWith(".gz", Qt::CaseInsensitive)) {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly)) {
            return EDFduration(0, 0, filename);
        }

        if (!file.seek(0xa8)) {
            file.close();
            return EDFduration(0, 0, filename);
        }

        QByteArray bytes = file.read(16).trimmed();

        startDate = QDateTime::fromString(QString::fromLatin1(bytes, 16), "dd.MM.yyHH.mm.ss");

        if (!file.seek(0xec)) {
            file.close();
            return EDFduration(0, 0, filename);
        }

        bytes = file.read(8).trimmed();
        num_records = bytes.toInt(&ok1);
        bytes = file.read(8).trimmed();
        rec_duration = bytes.toDouble(&ok2);

        file.close();
    } else {
        gzFile f = gzopen(filename.toLatin1(), "rb");
        if (!f) {
            return EDFduration(0, 0, filename);
        }

        if (!gzseek(f, 0xa8, SEEK_SET)) {
            gzclose(f);
            return EDFduration(0, 0, filename);
        }
        char datebytes[17] = {0};
        gzread(f, (char *)&datebytes, 16);
        QString str = QString(QString::fromLatin1(datebytes,16)).trimmed();

        startDate = QDateTime::fromString(str, "dd.MM.yyHH.mm.ss");

        if (!gzseek(f, 0xec-0xa8-16, SEEK_CUR)) { // 0xec
            gzclose(f);
            return EDFduration(0, 0, filename);
        }

        // Decompressed header and data block
        char cbytes[9] = {0};
        gzread(f, (char *)&cbytes, 8);
        str = QString(cbytes).trimmed();
        num_records = str.toInt(&ok1);

        gzread(f, (char *)&cbytes, 8);
        str = QString(cbytes).trimmed();
        rec_duration = str.toDouble(&ok2);

        gzclose(f);

    }

    QDate d2 = startDate.date();

    if (d2.year() < 2000) {
        d2.setDate(d2.year() + 100, d2.month(), d2.day());
        startDate.setDate(d2);
    }
    if (!startDate.isValid()) {
        qDebug() << "Invalid date time retreieved parsing EDF duration for" << filename;
        return EDFduration(0, 0, filename);
    }

    if (!(ok1 && ok2)) {
        return EDFduration(0, 0, filename);
    }

    quint32 start = startDate.toTime_t();
    quint32 end = start + rec_duration * num_records;

    QString filedate = filename.section("/",-1).section("_",0,1);

    QDateTime dt2 = QDateTime::fromString(filedate, "yyyyMMdd_hhmmss");
    quint32 st2 = dt2.toTime_t();

    start = qMin(st2, start);

    if (end < start) end = qMax(st2, start);

    if ((ext == "EVE") || (ext == "CSL")) {
        // S10 Forces us to parse EVE files to find their real durations
        quint32 en2;

        // Have to get the actual duration of the EVE file by parsing the annotations. :(


        // Can we cache the stupid EDFParser file for later ???

        int recs = PeekAnnotations(filename, st2, en2);
        if (recs > 0) {
            start = qMin(st2, start);
            end = qMax(en2, end);
            EDFduration dur(start, end, filename);

            dur.type = lookupEDFType(ext.toUpper());

            return dur;
        } else {
            // empty annotations file, don't give a crap about it...
            return EDFduration(0, 0, filename);
        }
        // A Firmware bug causes (perhaps with failing SD card) sessions to sometimes take a long time to write
    }

    EDFduration dur(start, end, filename);

    dur.type = lookupEDFType(ext.toUpper());

    return dur;
}


///////////////////////////////////////////////////////////////////////////////////////////
// Sorted EDF files that need processing into date records according to ResMed noon split
///////////////////////////////////////////////////////////////////////////////////////////
int ResmedLoader::scanFiles(Machine * mach, const QString & datalog_path)
{
    QTime time;

    bool create_backups = p_profile->session->backupCardData();
    QString backup_path = mach->getBackupPath();

    if (datalog_path == (backup_path + RMS9_STR_datalog + "/")) {
        // Don't create backups if importing from backup folder
        create_backups = false;
    }


    ///////////////////////////////////////////////////////////////////////////////////////
    // Generate list of files for later processing
    ///////////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_EFFICIENCY
    time.start();
#endif

    QDir dir(datalog_path);

    // First list any EDF files in DATALOG folder
    QStringList filter;
    filter << "*.edf";
    dir.setNameFilters(filter);
    QFileInfoList EDFfiles = dir.entryInfoList();

    dir.setNameFilters(QStringList());
    dir.setFilter(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
    QString filename;
    bool ok;

    QFileInfoList flist = dir.entryInfoList();
    int flistSize = flist.size();

    qDebug() << "Generating list of EDF files";
    // Scan for any sub folders and create files lists
    for (int i = 0; i < flistSize ; i++) {
        const QFileInfo & fi = flist.at(i);
        filename = fi.fileName();

        int len = filename.length();
        if ((len == 4) || (len == 8)) {
            filename.toInt(&ok);
            if (ok) {
                // Get file lists under this directory
                dir.setPath(fi.canonicalFilePath());
                dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
                dir.setSorting(QDir::Name);

                // Append all files to one big QFileInfoList
                EDFfiles.append(dir.entryInfoList());
            }
        }
    }
#ifdef DEBUG_EFFICIENCY
    qDebug() << "Generating EDF files list took" << time.elapsed() << "ms";
#endif

    ////////////////////////////////////////////////////////////////////////////////////////
    // Scan through EDF files, Extracting EDF Durations, and skipping already imported files
    // Check for duplicates along the way from compressed/uncompressed files
    ////////////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_EFFICIENCY
    time.start();
#endif
    QString datestr;
    QDateTime datetime;
    QDate date;
    int totalfiles = EDFfiles.size();

    // Calculate number of files for progress bar for this stage
    int pbarFreq = totalfiles / 50;
    if (pbarFreq < 1) pbarFreq = 1; // stop a divide by zero

    qprogress->setMaximum(totalfiles);
    qprogress->setValue(0);

    int cnt = 0;
    // Scan through all folders looking for EDF files, skip any already imported and peek inside to get durations

    qDebug() << "Starting EDF duration scan pass";
    for (int i=0; i < totalfiles; ++i) {
        const QFileInfo & fi = EDFfiles.at(i);

        // Update progress bar
        if ((cnt++ % pbarFreq) == 0) {
            qprogress->setValue(cnt);
            QApplication::processEvents();
        }

        // Forget about it if it can't be read.
        if (!fi.isReadable()) {
            continue;
        }

        filename = fi.fileName();

        datestr = filename.section("_", 0, 1);
        datetime = QDateTime().fromString(datestr,"yyyyMMdd_HHmmss");
        date = datetime.date();
        // ResMed splits days at noon and now so do we, so all times before noon
        // go to the previous day
        if (datetime.time().hour() < 12) {
            date = date.addDays(-1);
        }

        // Chop off the .gz component if it exists, it's not needed at this stage
        if (filename.endsWith(STR_ext_gz)) {
            filename.chop(3);
        }
        QString fullpath = fi.filePath();

        QString newpath = create_backups ? backup(fullpath, backup_path) : fullpath;


        // Accept only .edf and .edf.gz files
        if (filename.right(4).toLower() != ("."+STR_ext_EDF)) {
            continue;
        }

//        QString ext = key.section("_", -1).section(".",0,0).toUpper();
//        EDFType type = lookupEDFType(ext);

        // Find or create ResMedDay object for this date
        QMap<QDate, ResMedDay>::iterator rd = resdayList.find(date);
        if (rd == resdayList.end()) {
            rd = resdayList.insert(date, ResMedDay());
            rd.value().date = date;

            // We have data files without STR.edf record... the user MAY be planning on importing from another backup
            // later which could cause problems if we don't deal with it.
            // Best solution I can think of is import and tag the day No Settings and skip the day from overview.
        }
        ResMedDay & resday = rd.value();

        if (!resday.files.contains(filename)) {
            resday.files[filename] = newpath;
        }
    }
#ifdef DEBUG_EFFICIENCY
    qDebug() << "Scanning EDF files took" << time.elapsed() << "ms";
#endif

    return resdayList.size();
}
        /*// Check for duplicates
        if (newfiles.contains(filename)) {
            // Not sure what to do with it.. delete it? check compress status and delete the other one?
            // Either way we don't want to process so skip it
            qDebug() << "Duplicate EDF file detected" << filename;
            continue;
        }

        QString fullname = fi.canonicalFilePath();

        // Peek inside the EDF file and get the EDFDuration record for the session matching that follows
        EDFduration dur = getEDFDuration(fullname);
        dur.filename = filename;

        if (dur.start != dur.end) { // make sure empty EVE's are skipped

            // Insert into newfiles map for later processing
            QMap<QString, EDFduration>::iterator it = newfiles.insert(filename, dur);

            // And index in filesbytype for quick lookup by type
            filesbytype[dur.type].append(&it.value());
        }
    }
#ifdef DEBUG_EFFICIENCY
    qDebug() << "ResmedLoader::scanFiles() EDF Duration scan took" << time.elapsed() << "ms";
#endif

    QList<EDFType> EDForder;
    EDForder.push_back(EDF_PLD);
    EDForder.push_back(EDF_BRP);
    EDForder.push_back(EDF_SAD);
    QHash<EDFType, QStringList>::iterator gi;

    qprogress->setMaximum(filesbytype[EDF_PLD].size() + filesbytype[EDF_BRP].size() + filesbytype[EDF_SAD].size());
    cnt = 0;

    for (int i=0; i<3; i++) {
        EDFType basetype = EDForder.takeFirst();

        // Process PLD files
        QList<EDFduration *> & LIST = filesbytype[basetype];
        int base_size = LIST.size();
        for (int f=0; f < base_size; ++f) {
            if ((cnt % 50) == 0) {
                qprogress->setValue(cnt);
                QApplication::processEvents();
            }
            cnt++;
            const EDFduration * dur = LIST.at(f);

            quint32 start = dur->start;
            if (start == 0) continue;

            quint32 end = dur->end;
            QHash<EDFType, QStringList> grp;

//            grp[EDF_PLD] = QStringList();
//            grp[EDF_SAD] = QStringList();
//            grp[EDF_BRP] = QStringList();
//            grp[EDF_EVE] = QStringList();
//            grp[EDF_CSL] = QStringList();


            grp[basetype].append(create_backups ? backup(dur->path, backup_path) : dur->path);


            QStringList files;
            files.append(dur->filename);

            for (int o=0; o<EDForder.size(); ++o) {
                EDFType type = EDForder.at(o);

                QList<EDFduration *> & EDF_list = filesbytype[type];
                QList<EDFduration *>::iterator item;
                QList<EDFduration *>::iterator list_end = EDF_list.end();
                for (item = EDF_list.begin(); item != list_end; ++item) {
                    const EDFduration * dur2 = *item;
                    if (dur2->start == 0) continue;

                    // Do the sessions Overlap?
                    if ((start < dur2->end) && ( dur2->start < end)) {
                        start = qMin(start, dur2->start);
                        end = qMax(end, dur2->end);

                        files.append(dur2->filename);

                        grp[type].append(create_backups ? backup(dur2->path, backup_path) : dur2->path);

                        filesbytype[type].erase(item);
                    }
                }

            }

            // EVE annotation files can cover multiple sessions
            QList<EDFduration *> & EDF_list = filesbytype[EDF_EVE];
            QList<EDFduration *>::iterator item;
            QList<EDFduration *>::iterator list_end = EDF_list.end();
            for (item = EDF_list.begin(); item != list_end; ++item) {
                const EDFduration * dur2 = *item;
                if (dur2->start == 0) continue;

                // Do the sessions Overlap?
                if ((start < dur2->end) && ( dur2->start < end)) {
//                    start = qMin(start, dur2->start);
//                    end = qMax(end, dur2->end);

                    files.append(dur2->filename);

                    grp[EDF_EVE].append(create_backups ? backup(dur2->path, backup_path) : dur2->path);
                }
            }

            // CSL files contain CSR flags
            QList<EDFduration *> & CSL_list = filesbytype[EDF_CSL];
            list_end = CSL_list.end();
            for (item = CSL_list.begin(); item != list_end; ++item) {
                const EDFduration * dur2 = *item;
                if (dur2->start == 0) continue;

                // Do the sessions Overlap?
                if ((start < dur2->end) && ( dur2->start < end)) {
//                    start = qMin(start, dur2->start);
//                    end = qMax(end, dur2->end);

                    files.append(dur2->filename);

                    grp[EDF_CSL].append(create_backups ? backup(dur2->path, backup_path) : dur2->path);
                }
            }



            if (mach->SessionExists(start) == nullptr) {
                //EDFGroup group(grp[EDF_BRP], grp[EDF_EVE], grp[EDF_PLD], grp[EDF_SAD], grp[EDF_CSL]);
                if (grp.size() > 0) {
                    queTask(new ResmedImport(this, start, grp, mach));
                    for (int i=0; i<files.size(); i++) skipfiles[files.at(i)] = start;
                }
            }
        }
    }


    // No PLD files

    QMap<QString, EDFduration>::iterator it;
    QMap<QString, EDFduration>::iterator itn;
    QMap<QString, EDFduration>::iterator it_end = newfiles.end();

    // Now scan through all new files, and group together into sessions
    for (it = newfiles.begin(); it != it_end; ++it) {
        quint32 start = it.value().start;

        if (start == 0)
            continue;

        const QString & file = it.key();

        quint32 end = it.value().end;


        QString type = file.section("_", -1).section(".", 0, 0).toUpper();

        QString newpath = create_backups ? backup(it.value().path, backup_path) : it.value().path;

        EDFGroup group;

        if (type == "BRP") group.BRP = newpath;
        else if (type == "EVE") {
            if (group.BRP.isEmpty()) {
                qDebug() << "Jedimark's Order theory was wrong.. EVE's need to be parsed seperately!";
            }
            group.EVE = newpath;
        }

        else if (type == "PLD") group.PLD = newpath;
        else if (type == "SAD") group.SAD = newpath;
        else continue;

        QStringList sessfiles;
        sessfiles.push_back(file);

        for (itn = it+1; itn != it_end; ++itn) {
            if (itn.value().start == 0) continue;  // already processed
            const EDFduration & dur2 = itn.value();

            // Do the sessions Overlap?
            if ((start < dur2.end) && ( dur2.start < end)) {

                start = qMin(start, dur2.start);
                end = qMax(end, dur2.end);

                type = itn.key().section("_",-1).section(".",0,0).toUpper();

                newpath = create_backups ? backup(dur2.path, backup_path) : dur2.path;

                if (type == "BRP") {
                    if (!group.BRP.isEmpty()) {
                        itn.value().start = 0;
                        continue;
                    }
                    group.BRP = newpath;
                } else if (type == "EVE") {
                    if (!group.EVE.isEmpty()) {
                        itn.value().start = 0;
                        continue;
                    }
                    group.EVE = newpath;
                } else if (type == "PLD") {
                    if (!group.PLD.isEmpty()) {
                        itn.value().start = 0;
                        continue;
                    }
                    group.PLD = newpath;
                } else if (type == "SAD") {
                    if (!group.SAD.isEmpty()) {
                        itn.value().start = 0;
                        continue;
                    }
                    group.SAD = newpath;
                } else {
                    itn.value().start = 0;
                    continue;
                }
                sessfiles.push_back(itn.key());

                itn.value().start = 0;
            }
        }

        if (mach->SessionExists(start) == nullptr) {
            queTask(new ResmedImport(this, start, group, mach));
            for (int i=0; i < sessfiles.size(); ++i) {
                skipfiles[sessfiles.at(i)] = start;
            }
        }
    } */

    // Run the tasks...
   /* int c = countTasks();
    runTasks(AppSetting->multithreading());

    newSkipFiles.append(skipfiles.keys());
    impfile.remove();

    if (impfile.open(QFile::WriteOnly)) {
        QTextStream out(&impfile);
        out << mach->serial() << "\n";
        QHash<QString, SessionID>::iterator skit;
        QHash<QString, SessionID>::iterator skit_end = skipfiles.end();
        for (skit = skipfiles.begin(); skit != skit_end; ++skit) {
            QString a = QString("%1,%2\n").arg(skit.key()).arg(skit.value());;
            out << a;
        }
        out.flush();
    }
    impfile.close();

    return c;
}*/
void DetectPAPMode(Session *sess)
{
    if (sess->channelDataExists(CPAP_Pressure)) {
        // Determine CPAP or APAP?
        EventDataType min = sess->Min(CPAP_Pressure);
        EventDataType max = sess->Max(CPAP_Pressure);
        if ((max-min)<0.1) {
            sess->settings[CPAP_Mode] = MODE_CPAP;
            sess->settings[CPAP_Pressure] = qRound(max * 10.0)/10.0;
            // early call.. It's CPAP mode
        } else {
            // Ramp is ugly
            if (sess->length() > 1800000L) {  // half an our
            }
            sess->settings[CPAP_Mode] = MODE_APAP;
            sess->settings[CPAP_PressureMin] = qRound(min * 10.0)/10.0;
            sess->settings[CPAP_PressureMax] = qRound(max * 10.0)/10.0;
        }

    } else if (sess->eventlist.contains(CPAP_IPAP)) {
        sess->settings[CPAP_Mode] = MODE_BILEVEL_AUTO_VARIABLE_PS;
       // Determine BiPAP or ASV
    }

}
void StoreSummarySettings(Session * sess, STRRecord & R)
{
    if (R.mode >= 0) {
        if (R.mode == MODE_CPAP) {
        } else if (R.mode == MODE_APAP) {
        }
    }

    if (R.leak95 >= 0) {
//        sess->setp95(CPAP_Leak, R.leak95);
    }
    if (R.leak50 >= 0) {
//        sess->setp50(CPAP_Leak, R.leak50);
    }
    if (R.leakmax >= 0) {
        sess->setMax(CPAP_Leak, R.leakmax);
    }

    if (R.rr95 >= 0) {
//        sess->setp95(CPAP_RespRate, R.rr95);
    }
    if (R.rr50 >= 0) {
//        sess->setp50(CPAP_RespRate, R.rr50);
    }
    if (R.rrmax >= 0) {
        sess->setMax(CPAP_RespRate, R.rrmax);
    }

    if (R.mv95 >= 0) {
//        sess->setp95(CPAP_MinuteVent, R.mv95);
    }
    if (R.mv50 >= 0) {
//        sess->setp50(CPAP_MinuteVent, R.mv50);
    }
    if (R.mvmax >= 0) {
        sess->setMax(CPAP_MinuteVent, R.mvmax);
    }

    if (R.tv95 >= 0) {
  //      sess->setp95(CPAP_TidalVolume, R.tv95);
    }
    if (R.tv50 >= 0) {
//        sess->setp50(CPAP_TidalVolume, R.tv50);
    }
    if (R.tvmax >= 0) {
        sess->setMax(CPAP_TidalVolume, R.tvmax);
    }

    if (R.mp95 >= 0) {
//        sess->setp95(CPAP_MaskPressure, R.mp95);
    }
    if (R.mp50 >= 0) {
//        sess->setp50(CPAP_MaskPressure, R.mp50);
    }
    if (R.mpmax >= 0) {
        sess->setMax(CPAP_MaskPressure, R.mpmax);
    }



}

void StoreSettings(Session * sess, STRRecord & R)
{
    if (R.mode >= 0) {
        sess->settings[CPAP_Mode] = R.mode;
        sess->settings[RMS9_Mode] = R.rms9_mode;
        if (R.mode == MODE_CPAP) {
            if (R.set_pressure >= 0) {
                sess->settings[CPAP_Pressure] = R.set_pressure;
            }
        } else if (R.mode == MODE_APAP) {
            if (R.min_pressure >= 0) sess->settings[CPAP_PressureMin] = R.min_pressure;
            if (R.max_pressure >= 0) sess->settings[CPAP_PressureMax] = R.max_pressure;
        } else if (R.mode == MODE_BILEVEL_FIXED) {
            if (R.epap >= 0) sess->settings[CPAP_EPAP] = R.epap;
            if (R.ipap >= 0) sess->settings[CPAP_IPAP] = R.ipap;
            if (R.ps >= 0) sess->settings[CPAP_PS] = R.ps;
        } else if (R.mode == MODE_BILEVEL_AUTO_FIXED_PS) {
            if (R.min_epap >= 0) sess->settings[CPAP_EPAPLo] = R.min_epap;
            if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
            if (R.ps >= 0) sess->settings[CPAP_PS] = R.ps;
        } else if (R.mode == MODE_ASV) {
            if (R.epap >= 0) sess->settings[CPAP_EPAP] = R.epap;
            if (R.min_ps >= 0) sess->settings[CPAP_PSMin] = R.min_ps;
            if (R.max_ps >= 0) sess->settings[CPAP_PSMax] = R.max_ps;
            if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
        } else if (R.mode == MODE_ASV_VARIABLE_EPAP) {
            if (R.max_epap >= 0) sess->settings[CPAP_EPAPHi] = R.max_epap;
            if (R.min_epap >= 0) sess->settings[CPAP_EPAPLo] = R.min_epap;
            if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
            if (R.min_ipap >= 0) sess->settings[CPAP_IPAPLo] = R.min_ipap;
            if (R.min_ps >= 0) sess->settings[CPAP_PSMin] = R.min_ps;
            if (R.max_ps >= 0) sess->settings[CPAP_PSMax] = R.max_ps;
        }
    } else {
        if (R.set_pressure >= 0) sess->settings[CPAP_Pressure] = R.set_pressure;
        if (R.min_pressure >= 0) sess->settings[CPAP_PressureMin] = R.min_pressure;
        if (R.max_pressure >= 0) sess->settings[CPAP_PressureMax] = R.max_pressure;
        if (R.max_epap >= 0) sess->settings[CPAP_EPAPHi] = R.max_epap;
        if (R.min_epap >= 0) sess->settings[CPAP_EPAPLo] = R.min_epap;
        if (R.max_ipap >= 0) sess->settings[CPAP_IPAPHi] = R.max_ipap;
        if (R.min_ipap >= 0) sess->settings[CPAP_IPAPLo] = R.min_ipap;
        if (R.min_ps >= 0) sess->settings[CPAP_PSMin] = R.min_ps;
        if (R.max_ps >= 0) sess->settings[CPAP_PSMax] = R.max_ps;
        if (R.ps >= 0) sess->settings[CPAP_PS] = R.ps;
        if (R.epap >= 0) sess->settings[CPAP_EPAP] = R.epap;
        if (R.ipap >= 0) sess->settings[CPAP_IPAP] = R.ipap;
    }

    if (R.epr >= 0) {
        sess->settings[RMS9_EPR] = (int)R.epr;
        if (R.epr > 0) {
            if (R.epr_level >= 0) {
                sess->settings[RMS9_EPRLevel] = (int)R.epr_level;
            }
        }
    }

    if (R.s_RampEnable >= 0) {
        sess->settings[RMS9_RampEnable] = R.s_RampEnable;

        if (R.s_RampEnable >= 1) {
            if (R.s_RampTime >= 0) {
                sess->settings[CPAP_RampTime] = R.s_RampTime;
            }
            if (R.ramp_pressure >= 0) {
                sess->settings[CPAP_RampPressure] = R.ramp_pressure;
            }
        }
    }

    if (R.s_SmartStart >= 0) {
        sess->settings[RMS9_SmartStart] = R.s_SmartStart;
    }
    if (R.s_ABFilter >= 0) {
        sess->settings[RMS9_ABFilter] = R.s_ABFilter;
    }
    if (R.s_ClimateControl >= 0) {
        sess->settings[RMS9_ClimateControl] = R.s_ClimateControl;
    }
    if (R.s_Mask >= 0) {
        sess->settings[RMS9_Mask] = R.s_Mask;
    }
    if (R.s_PtAccess >= 0) {
        sess->settings[RMS9_PtAccess] = R.s_PtAccess;
    }

    if (R.s_HumEnable >= 0) {
        sess->settings[RMS9_HumidStatus] = (short)R.s_HumEnable;
        if ((R.s_HumEnable >= 1) && (R.s_HumLevel >= 0)) {
            sess->settings[RMS9_HumidLevel] = (short)R.s_HumLevel;
        }
    }
    if (R.s_TempEnable >= 0) {
        sess->settings[RMS9_TempEnable] = (short)R.s_TempEnable;
        if ((R.s_TempEnable >= 1) && (R.s_Temp >= 0)){
            sess->settings[RMS9_Temp] = (short)R.s_Temp;
        }
    }

}

struct OverlappingEDF {
    quint32 start;
    quint32 end;
    QMultiMap<quint32, QString> filemap;
    Session * sess;
};

void ResDayTask::run()
{

    /*loader->sessionMutex.lock();
    Day *day = p_profile->FindDay(resday->date, MT_CPAP);
    if (day) {
        if (day->summaryOnly(mach)) {
            if (resday->files.size() == 0) {
                // Summary only, and no new data files detected so we are done.
                loader->sessionMutex.unlock();
                return;
            }
            QList<Session *> sessions = day->getSessions(MT_CPAP);

            // Delete sessions for this day so they recreate with a clean slate.
            for (int i=0;i<sessions.size();++i) {
                Session * sess = sessions[i];
                day->removeSession(sess);
                delete sess;
            }

        } else {
            loader->sessionMutex.unlock();
            return;
        }
    }
    loader->sessionMutex.unlock(); */
    if (resday->files.size() == 0) {
        if (!resday->str.date.isValid()) {
            // No STR or files???
            // This condition should be impossible, but just in case something gets fudged up elsewhere later
            return;
        }
        // Summary only day, create one session and tag it summary only
        STRRecord & R = resday->str;

        for (int i=0;i<resday->str.maskon.size();++i) {
            quint32 maskon = resday->str.maskon[i];
            quint32 maskoff = resday->str.maskoff[i];
            if ((maskon>0) && (maskoff>0)) {
                Session * sess = new Session(mach, maskon);
                sess->set_first(quint64(maskon) * 1000L);
                sess->set_last(quint64(maskoff) * 1000L);
                // Process the STR.edf settings
                StoreSettings(sess, R);
                // We want the summary information too otherwise we've got nothing.
                StoreSummarySettings(sess, R);

                sess->setSummaryOnly(true);
                sess->SetChanged(true);
                sess->Store(mach->getDataPath());

                loader->sessionMutex.lock();
                mach->AddSession(sess);
                loader->sessionCount++;
                loader->sessionMutex.unlock();

                //sess->TrashEvents();
            }
        }
        return;
    }

    // sooo... at this point we have
    // resday record populated with correct STR.edf settings for this date
    // files list containing unsorted EDF files that match this day
    // guaranteed no sessions for this day for this machine.

    // Need to overlap check sessions

    QList<OverlappingEDF> overlaps;

    int maskevents = resday->str.maskon.size();
    if (resday->str.date.isValid()) {

        //First populate Overlaps with Mask ON/OFF events
        for (int i=0; i < maskevents; ++i) {
            if ((resday->str.maskon[i]>0) || (resday->str.maskoff[i]>0)) {
                OverlappingEDF ov;
                ov.start = resday->str.maskon[i];
                ov.end = resday->str.maskoff[i];
                ov.sess = nullptr;
                overlaps.append(ov);
            }
        }
    }

    QMap<quint32, QString> EVElist, CSLlist;

    QHash<QString, QString>::iterator fit;
    for (fit=resday->files.begin(); fit!=resday->files.end(); ++fit) {
        const QString & key = fit.key();
        const QString & fullpath = fit.value();
        QString ext = key.section("_", -1).section(".",0,0).toUpper();
        EDFType type = lookupEDFType(ext);

        QString datestr = key.section("_", 0, 1);
        QDateTime datetime = QDateTime().fromString(datestr,"yyyyMMdd_HHmmss");

        quint32 tt = datetime.toTime_t();
        if (type == EDF_EVE) {
            EVElist[tt] = key;
            continue;
        } else if (type == EDF_CSL) {
            CSLlist[tt] = key;
            continue;
        }

        bool added = false;
        for (int i=0; i< overlaps.size(); i++) {
            OverlappingEDF & ovr = overlaps[i];
            if ((tt >= (ovr.start)) && (tt < ovr.end)) {
                ovr.filemap.insert(tt, key);

                added = true;
                break;
            }
        }
        if (!added) {
            // Didn't get a hit, look at the actual EDF files duration and check for an overlap
            EDFduration dur = getEDFDuration(fullpath);
            for (int i=overlaps.size()-1; i>=0; --i) {
                OverlappingEDF & ovr = overlaps[i];
                if ((ovr.start < dur.end) && (dur.start < ovr.end)) {
                    ovr.filemap.insert(tt, key);
                    // Expand ovr's scope
                    //ovr.start = min(ovr.start, dur.start);
                    //ovr.end = max(ovr.end, dur.end);
                    added = true;
                    break;
                }
            }
            if (!added) {
                // Couldn't fit it in anywhere, create a new Overlap entry/session
                OverlappingEDF ov;
                ov.start = dur.start;
                ov.end = dur.end;
                ov.filemap.insert(tt, key);
                overlaps.append(ov);
            }
        }
    }


    // Create an ordered map and see how far apart the sessions really are.
    QMap<quint32, OverlappingEDF> mapov;
    for (int i=0;i<overlaps.size();++i) {
        OverlappingEDF & ovr = overlaps[i];
        mapov[ovr.start] = ovr;
    }

    // Examine the gaps in between to see if we should merge sessions
    QMap<quint32, OverlappingEDF>::iterator oit;
    for (oit=mapov.begin(); oit != mapov.end(); ++oit) {
        // Get next in line
        QMap<quint32, OverlappingEDF>::iterator next_oit = oit+1;
        if (next_oit != mapov.end()) {
            OverlappingEDF & A = oit.value();
            OverlappingEDF & B = next_oit.value();
            int gap = B.start - A.end;
            if (gap < 60) {
//                qDebug() << "Only a" << gap << "s sgap between ResMed sessions on" << resday->date.toString();
            }
        }
    }


    if (overlaps.size()==0) return;


    // Now overlaps is populated with zero or more individual session groups of EDF files (zero because of sucky summary only days)
    for (int s=0; s<overlaps.size(); ++s) {
        OverlappingEDF & ovr = overlaps[s];
        if (ovr.filemap.size() == 0) continue;
        Session * sess = new Session(mach, ovr.start);
        ovr.sess = sess;

        QMultiMap<quint32,QString>::iterator mit;
        for (mit = ovr.filemap.begin(); mit != ovr.filemap.end(); ++mit) {
            const QString & filename = mit.value();
            const QString & fullpath = resday->files[filename];
            QString ext = filename.section("_", -1).section(".",0,0).toUpper();
            EDFType type = lookupEDFType(ext);

#ifdef SESSION_DEBUG
            sess->session_files.append(filename);
#endif
            switch (type) {
            case EDF_BRP:
                loader->LoadBRP(sess, fullpath);
                break;
            case EDF_PLD:
                loader->LoadPLD(sess, fullpath);
                break;
            case EDF_SAD:
                loader->LoadSAD(sess, fullpath);
                break;
            case EDF_EVE:
            case EDF_CSL:
                break;
            default:
                qWarning() << "Unrecognized file type for" << filename;
            }
        }

        // Turns out there is only one or sometimes two EVE's per day, and they store data for the whole day
        // So we have to extract Annotations data and apply it for all sessions
        QMap<quint32, QString>::iterator eit;
        for (eit = EVElist.begin(); eit != EVElist.end(); ++eit) {
            const QString & fullpath = resday->files[eit.value()];

            loader->LoadEVE(ovr.sess, fullpath);
        }
        for (eit = CSLlist.begin(); eit != CSLlist.end(); ++eit) {
            const QString & fullpath = resday->files[eit.value()];
            loader->LoadCSL(ovr.sess, fullpath);
        }

        if (EVElist.size() == 0) {
            sess->AddEventList(CPAP_Obstructive, EVL_Event);
            sess->AddEventList(CPAP_ClearAirway, EVL_Event);
            sess->AddEventList(CPAP_Apnea, EVL_Event);
            sess->AddEventList(CPAP_Hypopnea, EVL_Event);
        }
        sess->setSummaryOnly(false);
        sess->SetChanged(true);

        if (sess->length()>0) {
            // we want empty sessions even though they are crap
        }

        if (resday->str.date.isValid()) {
            STRRecord & R = resday->str;

            // Claim this session
            R.sessionid = sess->session();

            // Save maskon time in session setting so we can use it later to avoid doubleups.
            //sess->settings[RMS9_MaskOnTime] = R.maskon;

    #ifdef SESSION_DEBUG
            sess->session_files.append("STR.edf");
    #endif
            StoreSettings(sess, R);

        } else {
            // No corresponding STR.edf record, but we have EDF files

            bool foundprev = false;
            // This is yuck.. we need to find the LAST date with valid settings data
            QDate first = p_profile->FirstDay(MT_CPAP);
            for (QDate d = resday->date.addDays(-1); d >= first; d = d.addDays(-1)) {
                loader->sessionMutex.lock();
                Day * day = p_profile->GetDay(d, MT_CPAP);
                bool hasmachine = day && day->hasMachine(mach);
                loader->sessionMutex.unlock();

                if (!day) continue;
                if (!hasmachine) continue;

                QList<Session *> sessions = day->getSessions(MT_CPAP);

                if (sessions.size() > 0) {
                    Session *chksess = sessions[0];
                    sess->settings = chksess->settings;
                    foundprev = true;
                    break;
                }

            }
            sess->settings[CPAP_BrokenSummary] = true;

            if (!foundprev) {
                // We have no Summary or Settings data... we need to do something to indicate this, and detect the mode
                if (sess->channelDataExists(CPAP_Pressure)) {
                    DetectPAPMode(sess);
                }
            }
        }

        sess->UpdateSummaries();

        // Save is not threadsafe? (meh... it seems to be)
       // loader->saveMutex.lock();
        sess->Store(mach->getDataPath());
       // loader->saveMutex.unlock();

        loader->sessionMutex.lock();
        mach->AddSession(sess);  // AddSession definitely ain't.
        loader->sessionMutex.unlock();

        // Free the memory used by this session
        sess->TrashEvents();

        loader->sessionMutex.lock();
        loader->sessionCount++;
        loader->sessionMutex.unlock();
    }
}

int ResmedLoader::Open(const QString & dirpath)
{

    QString key, value;
    QString line;
    QString newpath;
    QString filename;

    QHash<QString, QString> idmap;  // Temporary properties hash

    QString path(dirpath);
    path = path.replace("\\", "/");

    // Strip off end "/" if any
    if (path.endsWith("/")) {
        path = path.section("/", 0, -2);
    }

    // Strip off DATALOG from path, and set newpath to the path contianing DATALOG
    if (path.endsWith(RMS9_STR_datalog)) {
        newpath = path + "/";
        path = path.section("/", 0, -2);
    } else {
        newpath = path + "/" + RMS9_STR_datalog + "/";
    }

    // Add separator back
    path += "/";

    // Check DATALOG folder exists and is readable
    if (!QDir().exists(newpath)) {
        return -1;
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Parse Identification.tgt file (containing serial number and machine information)
    ///////////////////////////////////////////////////////////////////////////////////
    filename = path + RMS9_STR_idfile + STR_ext_TGT;
    QFile f(filename);

    // Abort if this file is dodgy..
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        return -1;
    }
    MachineInfo info = newInfo();

    // Parse # entries into idmap.
    while (!f.atEnd()) {
        line = f.readLine().trimmed();

        if (!line.isEmpty()) {
            key = line.section(" ", 0, 0).section("#", 1);
            value = line.section(" ", 1);

            if (key == "SRN") { // Serial Number
                info.serial = value;
                continue;

            } else if (key == "PNA") {  // Product Name
                value.replace("_"," ");
                if (value.contains(STR_ResMed_S9)) {
                    value.replace(STR_ResMed_S9, "");
                    info.series = STR_ResMed_S9;
                } else if (value.contains(STR_ResMed_AirSense10)) {
                    value.replace(STR_ResMed_AirSense10, "");
                    info.series = STR_ResMed_AirSense10;
                } else if (value.contains(STR_ResMed_AirCurve10)) {
                    value.replace(STR_ResMed_AirCurve10, "");
                    info.series = STR_ResMed_AirCurve10;
                }
                value.replace("(","");
                value.replace(")","");

                if (value.contains("Adapt", Qt::CaseInsensitive)) {
                    if (!value.contains("VPAP")) {
                        value.replace("Adapt", QObject::tr("VPAP Adapt"));
                    }
                }
                info.model = value.trimmed();
                continue;

            } else if (key == "PCD") { // Product Code
                info.modelnumber = value;
                continue;
            }

            idmap[key] = value;
        }
    }

    f.close();

    // Abort if no serial number
    if (info.serial.isEmpty()) {
        qDebug() << "S9 Data card has no valid serial number in Indentification.tgt";
        return -1;
    }

    // Early check for STR.edf file, so we can early exit before creating faulty machine record.
    QString strpath = path + RMS9_STR_strfile + STR_ext_EDF; // STR.edf file
    f.setFileName(strpath);

    if (!f.exists()) { // No STR.edf.. Do we have a STR.edf.gz?
        strpath += STR_ext_gz;
        f.setFileName(strpath);

        if (!f.exists()) {
            qDebug() << "Missing STR.edf file";
            return -1;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Create machine object (unless it's already registered)
    ///////////////////////////////////////////////////////////////////////////////////
    Machine *mach = p_profile->CreateMachine(info);

    bool importing_backups = false;
    bool create_backups = p_profile->session->backupCardData();
    bool compress_backups = p_profile->session->compressBackupData();

    QString backup_path = mach->getBackupPath();

    if (path == backup_path) {
        // Don't create backups if importing from backup folder
        importing_backups = true;
        create_backups = false;
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Parse the idmap into machine objects properties, (overwriting any old values)
    ///////////////////////////////////////////////////////////////////////////////////
    for (QHash<QString, QString>::iterator i = idmap.begin(); i != idmap.end(); i++) {
        mach->properties[i.key()] = i.value();
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Open and Parse STR.edf files (including those listed in STR_Backup)
    ///////////////////////////////////////////////////////////////////////////////////

    resdayList.clear();

    // List all STR.edf backups and tag on latest for processing
    QMap<QDate, STRFile> STRmap;

    QDir dir;

    // Create the STR_Backup folder if it doesn't exist
    QString strBackupPath = backup_path + "STR_Backup";
    if (!dir.exists(strBackupPath)) dir.mkpath(strBackupPath);

    if (!importing_backups ) {
        QStringList strfiles;
        // add primary STR.edf
        strfiles.push_back(strpath);

        // Just in case we are importing into a new folder, process SleepyHead backup structures
        dir.setPath(path + "STR_Backup");
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::Readable);
        QFileInfoList flist = dir.entryInfoList();

        int size = flist.size();
        // Add any STR_Backup versions to the file list
        for (int i = 0; i < size; i++) {
            QFileInfo fi = flist.at(i);
            filename = fi.fileName();
            if (!filename.startsWith("STR", Qt::CaseInsensitive))
                continue;
            if (!(filename.endsWith("edf.gz", Qt::CaseInsensitive) || filename.endsWith("edf", Qt::CaseInsensitive)))
                continue;
            strfiles.push_back(fi.canonicalFilePath());
        }

        // Now place any of these files in the Backup folder sorted by the file date
        for (int i=0;i<strfiles.size();i++) {
            QString filename = strfiles.at(i);
            ResMedEDFParser * stredf = new ResMedEDFParser(filename);
            if (!stredf->Parse()) {
                qDebug() << "Faulty STR file" << filename;
                delete stredf;
                continue;
            }

            if (stredf->serialnumber != info.serial) {
                qDebug() << "Identification.tgt Serial number doesn't match" << filename;
                delete stredf;
                continue;
            }

            QDate date = stredf->startdate_orig.date();
            date = QDate(date.year(), date.month(), 1);
            if (STRmap.contains(date)) {
                delete stredf;
                continue;
            }
            QString newname = "STR-"+date.toString("yyyyMM")+"."+STR_ext_EDF;

            QString backupfile = strBackupPath+"/"+newname;

            QString gzfile = backupfile + STR_ext_gz;
            QString nongzfile = backupfile;

            backupfile = compress_backups ? gzfile : nongzfile;

            if (!QFile::exists(backupfile)) {
                if (filename.endsWith(STR_ext_gz,Qt::CaseInsensitive)) {
                    if (compress_backups) {
                        QFile::copy(filename, backupfile);
                    } else {
                        uncompressFile(filename, backupfile);
                    }
                } else {
                    if (compress_backups) {
                        // already compressed, keep it.
                        compressFile(filename, backupfile);
                    } else {
                        QFile::copy(filename, backupfile);
                    }
                }
            }
            // Remove any duplicate compressed/uncompressed
            if (compress_backups) {
                QFile::exists(nongzfile) && QFile::remove(nongzfile);
            } else {
                QFile::exists(gzfile) && QFile::remove(gzfile);
            }


            STRmap[date] = STRFile(backupfile, stredf);
        }
    }

    // Now we open the REAL STR_Backup, and open the rest for later parsing

    dir.setPath(backup_path + "STR_Backup");
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::Readable);
    QFileInfoList flist = dir.entryInfoList();
    QDate date;

    int size = flist.size();
    // Add any STR_Backup versions to the file list
    for (int i = 0; i < size; i++) {
        QFileInfo fi = flist.at(i);
        filename = fi.fileName();
        if (!filename.startsWith("STR", Qt::CaseInsensitive))
            continue;
        if (!(filename.endsWith("edf.gz", Qt::CaseInsensitive) || filename.endsWith("edf", Qt::CaseInsensitive)))
            continue;
        QString datestr = filename.section("STR-",-1).section(".edf",0,0)+"01";
        date = QDate().fromString(datestr,"yyyyMMdd");

        if (STRmap.contains(date)) {
            continue;
        }

        ResMedEDFParser * stredf = new ResMedEDFParser(fi.canonicalFilePath());
        if (!stredf->Parse()) {
            qDebug() << "Faulty STR file" << filename;
            delete stredf;
            continue;
        }

        if (stredf->serialnumber != info.serial) {
            qDebug() << "Identification.tgt Serial number doesn't match" << filename;
            delete stredf;
            continue;
        }

        // Don't trust the filename date, pick the one inside the STR...
        date = stredf->startdate_orig.date();
        date = QDate(date.year(), date.month(), 1);

        STRmap[date] = STRFile(fi.canonicalFilePath(), stredf);
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Build a Date map of all records in STR.edf files, populating ResDayList
    ///////////////////////////////////////////////////////////////////////////////////
    ParseSTR(mach, STRmap);

    // We are done with the Parsed STR EDF objects, so delete them
    QMap<QDate, STRFile>::iterator it;
    for (it=STRmap.begin(); it!= STRmap.end(); ++it) {
        delete it.value().edf;
    }


    ///////////////////////////////////////////////////////////////////////////////////
    // Create the backup folder for storing a copy of everything in..
    // (Unless we are importing from this backup folder)
    ///////////////////////////////////////////////////////////////////////////////////
    dir.setPath(newpath);
    if (create_backups) {
        if (!dir.exists(backup_path)) {
            if (!dir.mkpath(backup_path + RMS9_STR_datalog)) {
                qDebug() << "Could not create S9 backup directory :-/";
            }
        }

        compress_backups ? compressFile(path + "STR.edf", backup_path + "STR.edf.gz") : QFile::copy(path + "STR.edf", backup_path + "STR.edf");

        // Copy Identification files to backup folder
        QFile::copy(path + RMS9_STR_idfile + STR_ext_TGT, backup_path + RMS9_STR_idfile + STR_ext_TGT);
        QFile::copy(path + RMS9_STR_idfile + STR_ext_CRC, backup_path + RMS9_STR_idfile + STR_ext_CRC);

        // Meh.. these can be calculated if ever needed for ResScan SDcard export
        QFile::copy(path + "STR.crc", backup_path + "STR.crc");
    }


    ///////////////////////////////////////////////////////////////////////////////////
    // Scan DATALOG files, sort, and import any new sessions
    ///////////////////////////////////////////////////////////////////////////////////

    // First remove a legacy file if present...
    QFile impfile(mach->getDataPath()+"/imported_files.csv");
    if (impfile.exists()) impfile.remove();

    scanFiles(mach, newpath);

    // Now at this point we have resdayList populated with processable summary and EDF files data
    // that can be processed in threads..

    QMap<QDate, ResMedDay>::iterator rdi;

    for (rdi = resdayList.begin(); rdi != resdayList.end(); rdi++) {
        QDate date = rdi.key();

        ResMedDay & resday = rdi.value();
        resday.date = date;

        Day * day = p_profile->FindDay(date, MT_CPAP);
        bool reimporting = false;
        if (day && day->hasMachine(mach)) {
            // Sessions found for this machine, check if only summary info

            if (day->summaryOnly(mach) && (resday.files.size()> 0)) {
                // Note: if this isn't an EDF file, there's really no point doing this here,
                // but the worst case scenario is this session is deleted and reimported.. this just slows things down a bit in that case

                // This day was first imported as a summary from STR.edf, so we now totally want to redo this day
                QList<Session *> sessions = day->getSessions(MT_CPAP);

                // Delete sessions for this day so they recreate with a clean slate.
                for (int i=0;i<sessions.size();++i) {
                    Session * sess = sessions[i];
                    day->removeSession(sess);
                    delete sess;
                }

                reimporting = true;
            } else {
                continue;
            }
        }

        ResDayTask * rdt = new ResDayTask(this, mach, &resday);
        queTask(rdt);
        rdt->reimporting = reimporting;
    }

    sessionCount = 0;
    runTasks();
    int num_new_sessions = sessionCount;


    ////////////////////////////////////////////////////////////////////////////////////
    // Now look for any new summary data that can be extracted from STR.edf records
    ////////////////////////////////////////////////////////////////////////////////////


    //int size = m->sessionlist.size();
    //int cnt=0;
//    Session * sess;


    // Scan through all sessions, and remove any strsess records that have a matching session already
//    for (sessit = m->sessionlist.begin(); sessit != sessend; ++sessit) {
//        sess = *sessit;
//        quint32 key = sess->settings[RMS9_MaskOnTime].toUInt();

//        // Ugly.. need to check sessions overlaps..

//        QMap<quint32, STRRecord>::iterator e = strsess.find(key);
//        if (e != end) {
//            strsess.erase(e);
//        }
//    }
///

/*    QHash<SessionID, Session *>::iterator sessit;
    QHash<SessionID, Session *>::iterator sessend = mach->sessionlist.end();;

    QMap<SessionID, Session *>::iterator sit;
    QMap<SessionID, Session *>::iterator ns_end = new_sessions.end();


    QMap<quint32, STRRecord>::iterator it;
    QMap<quint32, STRRecord>::iterator end = strsess.end();

    QList<quint32> strlist;
    for (it = strsess.begin(); it != end; ++it) {
        STRRecord & R = it.value();
        quint32 s1 = R.maskon;
        quint32 e1 = R.maskoff;
        bool fnd = false;
        for (sessit = mach->sessionlist.begin(); sessit != sessend; ++sessit) {
            sess = sessit.value();
            quint32 s2 = sess->session();
            quint32 e2 = s2 + (sess->length() / 1000L);

            if ((s1 < e2) && (s2 < e1)) {
                strlist.push_back(it.key());
                fnd = true;
                break;
            }
        }
        if (!fnd) for (sit = new_sessions.begin(); sit != ns_end; ++sit) {
            sess = sit.value();
            quint32 s2 = sess->session();
            quint32 e2 = s2 + (sess->length() / 1000L);

            if ((s1 < e2) && (s2 < e1)) {
                strlist.push_back(it.key());
                fnd = true;
                break;
            }

        }
    }

    for (int i=0; i<strlist.size(); i++) {
        int k = strlist.at(i);
        strsess.remove(k);
    }

    ///

    //size = strsess.size();
    //cnt=0;
    quint32 ignoreolder = p_profile->session->ignoreOlderSessionsDate().toTime_t();

    bool ignoreold = p_profile->session->ignoreOlderSessions();
    // strsess end can change above.
    end = strsess.end();

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Scan through unmatched strsess records, and attempt to get at summary data
    /////////////////////////////////////////////////////////////////////////////////////////////
    for (it = strsess.begin(); it != end; ++it) {
        STRRecord & R = it.value();

        if (ignoreold && (R.maskon < ignoreolder)) {
            mach->skipSaveTask();
            continue;
        }

        //noooo ASSERTS!!!! Q _ASSERT(R.sessionid == 0);

        // the following should not happen
        if (R.sessionid > 0) {
            mach->skipSaveTask();
            continue;
        }


        queTask(new ResmedImportStage2(this, R, mach));
    }

    num_new_sessions += countTasks();
    */



    finishAddingSessions();

#ifdef DEBUG_EFFICIENCY
    {
        qint64 totalbytes = 0;
        qint64 totalns = 0;
        qDebug() << "Time Delta Efficiency Information";

        for (QHash<ChannelID, qint64>::iterator it = channel_efficiency.begin();
                it != channel_efficiency.end(); it++) {
            ChannelID code = it.key();
            qint64 value = it.value();
            qint64 ns = channel_time[code];
            totalbytes += value;
            totalns += ns;
            double secs = double(ns) / 1000000000.0L;
            QString s = value < 0 ? "saved" : "cost";
            qDebug() << "Time-Delta conversion for " + schema::channel[code].label() + " " + s + " " +
                     QString::number(qAbs(value)) + " bytes and took " + QString::number(secs, 'f', 4) + "s";
        }

        qDebug() << "Total toTimeDelta function usage:" << totalbytes << "in" << double(
                     totalns) / 1000000000.0 << "seconds";
    }
#endif

    if (qprogress) { qprogress->setValue(100); }

    sessfiles.clear();
    strsess.clear();

    strdate.clear();
    channel_efficiency.clear();
    channel_time.clear();

    qDebug() << "Total Events " << event_cnt;
    return num_new_sessions;
}


QString ResmedLoader::backup(const QString & fullname, const QString & backup_path)
{
    QDir dir;
    QString filename, yearstr, newname, oldname;

    bool compress = p_profile->session->compressBackupData();

    bool ok;
    bool gz = (fullname.right(3).toLower() == STR_ext_gz);  // Input file is a .gz?


    filename = fullname.section("/", -1);
    if (gz) {
        filename.chop(3);
    }

    yearstr = filename.left(4);
    yearstr.toInt(&ok, 10);

    if (!ok) {
        qDebug() << "Invalid EDF filename given to ResMedLoader::backup()" << fullname;
        return "";
    }

    QString newpath = backup_path + RMS9_STR_datalog + "/" + yearstr;
    !dir.exists(newpath) && dir.mkpath(newpath);

    newname = newpath+"/"+filename;

    QString tmpname = newname;

    QString newnamegz = newname + STR_ext_gz;
    QString newnamenogz = newname;

    newname = compress ? newnamegz : newnamenogz;

    // First make sure the correct backup exists in the right place
    if (!QFile::exists(newname)) {
        if (compress) {
            // If input file is already compressed.. copy it to the right location, otherwise compress it
            gz ? QFile::copy(fullname, newname) : compressFile(fullname, newname);
        } else {
            // If inputs a gz, uncompress it, otherwise copy is raw
            gz ? uncompressFile(fullname, newname) : QFile::copy(fullname, newname);
        }
    } // else backup already exists... good.

    // Now the correct backup is in place, we can trash any
    if (compress) {
        // Remove any uncompressed duplicate
        QFile::exists(newnamenogz) && QFile::remove(newnamenogz);
    } else {
        // Delete the non compressed copy and choose it instead.
        QFile::exists(newnamegz) && QFile::remove(newnamegz);
    }

    // Used to store it under Backup\Datalog
    // Remove any traces from old backup directory structure
    oldname = backup_path + RMS9_STR_datalog + "/" + filename;
    QFile::exists(oldname) && QFile::remove(oldname);
    QFile::exists(oldname + STR_ext_gz) && QFile::remove(oldname + STR_ext_gz);

    return newname;
}

bool ResmedLoader::LoadCSL(Session *sess, const QString & path)
{
    ResMedEDFParser edf(path);
    if (!edf.Parse())
        return false;

    QString t;

    long recs;
    double duration;
    char *data;
    char c;
    long pos;
    bool sign, ok;
    double d;
    double tt;

    // Notes: Event records have useless duration record.
   // sess->updateFirst(edf.startdate);

    EventList *CSR = nullptr;

    // Allow for empty sessions..
    qint64 csr_starts = 0;


    // Process event annotation records
    for (int s = 0; s < edf.GetNumSignals(); s++) {
        recs = edf.edfsignals[s].nr * edf.GetNumDataRecords() * 2;

        data = (char *)edf.edfsignals[s].data;
        pos = 0;
        tt = edf.startdate;
    //    sess->updateFirst(tt);
        duration = 0;

        while (pos < recs) {
            c = data[pos];

            if ((c != '+') && (c != '-')) {
                break;
            }

            if (data[pos++] == '+') { sign = true; }
            else { sign = false; }

            t = "";
            c = data[pos];

            do {
                t += c;
                pos++;
                c = data[pos];
            } while ((c != 20) && (c != 21)); // start code

            d = t.toDouble(&ok);

            if (!ok) {
                qDebug() << "Faulty EDF CSL file " << edf.filename;
                break;
            }

            if (!sign) { d = -d; }

            tt = edf.startdate + qint64(d * 1000.0);
            duration = 0;
            // First entry

            if (data[pos] == 21) {
                pos++;
                // get duration.
                t = "";

                do {
                    t += data[pos];
                    pos++;
                } while ((data[pos] != 20) && (pos < recs)); // start code

                duration = t.toDouble(&ok);

                if (!ok) {
                    qDebug() << "Faulty EDF CSL file (at %" << pos << ") " << edf.filename;
                    break;
                }
            }

            while ((data[pos] == 20) && (pos < recs)) {
                t = "";
                pos++;

                if (data[pos] == 0) {
                    break;
                }

                if (data[pos] == 20) {
                    pos++;
                    break;
                }

                do {
                    t += tolower(data[pos++]);
                } while ((data[pos] != 20) && (pos < recs)); // start code

                if (!t.isEmpty()) {
                    if (t == "csr start") {
                        csr_starts = tt;
                    } else if (t == "csr end") {
                        if (!CSR) {
                            CSR = sess->AddEventList(CPAP_CSR, EVL_Event);
                        }

                        if (csr_starts > 0) {
                            if (sess->checkInside(csr_starts)) {
                                CSR->AddEvent(tt, double(tt - csr_starts) / 1000.0);
                            }
                            csr_starts = 0;
                        } else {
                            qDebug() << "If you can read this, ResMed sucks and split CSR flagging!";
                        }
                    } else if (t != "recording starts") {
                        qDebug() << "Unobserved ResMed CSL annotation field: " << t;
                    }
                }

                if (pos >= recs) {
                    qDebug() << "Short EDF CSL file" << edf.filename;
                    break;
                }

                // pos++;
            }

            while ((data[pos] == 0) && (pos < recs)) { pos++; }

            if (pos >= recs) { break; }
        }

    //    sess->updateLast(tt);
    }

    Q_UNUSED(duration)

    return true;
}

bool ResmedLoader::LoadEVE(Session *sess, const QString & path)
{
    ResMedEDFParser edf(path);
    if (!edf.Parse())
        return false;

    QString t;

    long recs;
    double duration=0;
    char *data;
    char c;
    long pos;
    bool sign, ok;
    double d;
    double tt;

    // Notes: Event records have useless duration record.
    // Do not update session start / end times because they are needed to determine if events belong in this session or not...

    EventList *OA = nullptr, *HY = nullptr, *CA = nullptr, *UA = nullptr, *RE = nullptr;

    // Allow for empty sessions..

    // Create EventLists
    OA = sess->AddEventList(CPAP_Obstructive, EVL_Event);
    HY = sess->AddEventList(CPAP_Hypopnea, EVL_Event);
    UA = sess->AddEventList(CPAP_Apnea, EVL_Event);

    // Process event annotation records
    for (int s = 0; s < edf.GetNumSignals(); s++) {
        recs = edf.edfsignals[s].nr * edf.GetNumDataRecords() * 2;

        data = (char *)edf.edfsignals[s].data;
        pos = 0;
        tt = edf.startdate;

        while (pos < recs) {
            c = data[pos];

            if ((c != '+') && (c != '-')) {
                break;
            }

            if (data[pos++] == '+') { sign = true; }
            else { sign = false; }

            t = "";
            c = data[pos];

            do {
                t += c;
                pos++;
                c = data[pos];
            } while ((c != 20) && (c != 21)); // start code

            d = t.toDouble(&ok);

            if (!ok) {
                qDebug() << "Faulty EDF EVE file " << edf.filename;
                break;
            }

            if (!sign) { d = -d; }

            tt = edf.startdate + qint64(d * 1000.0);
            duration = 0;
            // First entry

            if (data[pos] == 21) {
                pos++;
                // get duration.
                t = "";

                do {
                    t += data[pos];
                    pos++;
                } while ((data[pos] != 20) && (pos < recs)); // start code

                duration = t.toDouble(&ok);

                if (!ok) {
                    qDebug() << "Faulty EDF EVE file (at %" << pos << ") " << edf.filename;
                    break;
                }
            }

            while ((data[pos] == 20) && (pos < recs)) {
                t = "";
                pos++;

                if (data[pos] == 0) {
                    break;
                }

                if (data[pos] == 20) {
                    pos++;
                    break;
                }

                do {
                    t += tolower(data[pos++]);
                } while ((data[pos] != 20) && (pos < recs)); // start code

                if (!t.isEmpty()) {
                    if (matchSignal(CPAP_Obstructive, t)) {

                        if (sess->checkInside(tt)) OA->AddEvent(tt, duration);
                    } else if (matchSignal(CPAP_Hypopnea, t)) {
                        if (sess->checkInside(tt)) HY->AddEvent(tt, duration + 10); // Only Hyponea's Need the extra duration???
                    } else if (matchSignal(CPAP_Apnea, t)) {
                        if (sess->checkInside(tt)) UA->AddEvent(tt, duration);
                    } else if (matchSignal(CPAP_RERA, t)) {
                        // Not all machines have it, so only create it when necessary..
                        if (!RE) {
                            if (!(RE = sess->AddEventList(CPAP_RERA, EVL_Event))) {
                                return false;
                            }
                        }
                        if (sess->checkInside(tt)) RE->AddEvent(tt, duration);
                    } else if (matchSignal(CPAP_ClearAirway, t)) {
                        // Not all machines have it, so only create it when necessary..
                        if (!CA) {
                            if (!(CA = sess->AddEventList(CPAP_ClearAirway, EVL_Event))) {
                                return false;
                            }
                        }

                        if (sess->checkInside(tt)) CA->AddEvent(tt, duration);
                    } else {
                        if (t != "recording starts") {
                            qDebug() << "Unobserved ResMed annotation field: " << t;
                        }
                    }
                }

                if (pos >= recs) {
                    qDebug() << "Short EDF EVE file" << edf.filename;
                    break;
                }

                // pos++;
            }

            while ((data[pos] == 0) && (pos < recs)) { pos++; }

            if (pos >= recs) { break; }
        }

    }

    return true;
}

bool ResmedLoader::LoadBRP(Session *sess, const QString & path)
{
    ResMedEDFParser edf(path);
    if (!edf.Parse())
        return false;

    sess->updateFirst(edf.startdate);

    qint64 duration = edf.GetNumDataRecords() * edf.GetDuration();
    sess->updateLast(edf.startdate + duration);

    for (int s = 0; s < edf.GetNumSignals(); s++) {
        EDFSignal &es = edf.edfsignals[s];

        long recs = es.nr * edf.GetNumDataRecords();
        if (recs < 0)
            continue;
        ChannelID code;

        if (matchSignal(CPAP_FlowRate, es.label)) {
            code = CPAP_FlowRate;
            es.gain *= 60.0;
            es.physical_minimum *= 60.0;
            es.physical_maximum *= 60.0;
            es.physical_dimension = "L/M";

        } else if (matchSignal(CPAP_MaskPressureHi, es.label)) {
            code = CPAP_MaskPressureHi;

        } else if (matchSignal(CPAP_RespEvent, es.label)) {
            code = CPAP_RespEvent;

        } else if (es.label != "Crc16") {
            qDebug() << "Unobserved ResMed BRP Signal " << es.label;
            continue;
        } else continue;

        if (code) {
            double rate = double(duration) / double(recs);
            EventList *a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->setDimension(es.physical_dimension);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            EventDataType min = a->Min();
            EventDataType max = a->Max();

            // Cap to physical dimensions, because there can be ram glitches/whatever that throw really big outliers.
            if (min < es.physical_minimum) min = es.physical_minimum;
            if (max > es.physical_maximum) max = es.physical_maximum;

            sess->setMin(code, min);
            sess->setMax(code, max);
            sess->setPhysMin(code, es.physical_minimum);
            sess->setPhysMax(code, es.physical_maximum);
        }
    }

    return true;
}

// Convert EDFSignal data to sleepyheads Time-Delta Event format
void ResmedLoader::ToTimeDelta(Session *sess, ResMedEDFParser &edf, EDFSignal &es, ChannelID code,
                               long recs, qint64 duration, EventDataType t_min, EventDataType t_max, bool square)
{
    if (t_min == t_max) {
        t_min = es.physical_minimum;
        t_max = es.physical_maximum;
    }

#ifdef DEBUG_EFFICIENCY
    QElapsedTimer time;
    time.start();
#endif

    double rate = (duration / recs); // milliseconds per record
    double tt = edf.startdate;

    EventStoreType c=0, last;

    int startpos = 0;

    if ((code == CPAP_Pressure) || (code == CPAP_IPAP) || (code == CPAP_EPAP)) {
        startpos = 20; // Shave the first 20 seconds of pressure data
        tt += rate * startpos;
    }

    qint16 *sptr = es.data;
    qint16 *eptr = sptr + recs;
    sptr += startpos;

    EventDataType min = t_max, max = t_min, tmp;

    EventList *el = nullptr;

    if (recs > startpos + 1) {

        // Prime last with a good starting value
        do {
            last = *sptr++;
            tmp = EventDataType(last) * es.gain;

            if ((tmp >= t_min) && (tmp <= t_max)) {
                min = tmp;
                max = tmp;
                el = sess->AddEventList(code, EVL_Event, es.gain, es.offset, 0, 0);

                el->AddEvent(tt, last);
                tt += rate;

                break;
            }

            tt += rate;

        } while (sptr < eptr);

        if (!el) {
            return;
        }

        for (; sptr < eptr; sptr++) {
            c = *sptr;

            if (last != c) {
                if (square) {
                    tmp = EventDataType(last) * es.gain;

                    if ((tmp >= t_min) && (tmp <= t_max)) {
                        if (tmp < min) {
                            min = tmp;
                        }

                        if (tmp > max) {
                            max = tmp;
                        }

                        el->AddEvent(tt, last);
                    } else {
                        // Out of bounds value, start a new eventlist
                        if (el->count() > 1) {
                            // that should be in session, not the eventlist.. handy for debugging though
                            el->setDimension(es.physical_dimension);

                            el = sess->AddEventList(code, EVL_Event, es.gain, es.offset, 0, 0);
                        } else {
                            el->clear(); // reuse the object
                        }
                    }
                }

                tmp = EventDataType(c) * es.gain;

                if ((tmp >= t_min) && (tmp <= t_max)) {
                    if (tmp < min) {
                        min = tmp;
                    }

                    if (tmp > max) {
                        max = tmp;
                    }

                    el->AddEvent(tt, c);
                } else {
                    if (el->count() > 1) {
                        el->setDimension(es.physical_dimension);

                        // Create and attach new EventList
                        el = sess->AddEventList(code, EVL_Event, es.gain, es.offset, 0, 0);
                    } else { el->clear(); }
                }
            }

            tt += rate;

            last = c;
        }

        tmp = EventDataType(c) * es.gain;

        if ((tmp >= t_min) && (tmp <= t_max)) {
            el->AddEvent(tt, c);
        }

        sess->setMin(code, min);
        sess->setMax(code, max);
        sess->setPhysMin(code, es.physical_minimum);
        sess->setPhysMax(code, es.physical_maximum);
        sess->updateLast(tt);
    }

#ifdef DEBUG_EFFICIENCY
    if (el != nullptr) {
        qint64 t = time.nsecsElapsed();
        int cnt = el->count();
        int bytes = cnt * (sizeof(EventStoreType) + sizeof(quint32));
        int wvbytes = recs * (sizeof(EventStoreType));
        QHash<ChannelID, qint64>::iterator it = channel_efficiency.find(code);

        if (it == channel_efficiency.end()) {
            channel_efficiency[code] = wvbytes - bytes;
            channel_time[code] = t;
        } else {
            it.value() += wvbytes - bytes;
            channel_time[code] += t;
        }
    }
#endif
}

// Load SAD Oximetry Signals
bool ResmedLoader::LoadSAD(Session *sess, const QString & path)
{
    ResMedEDFParser edf(path);
    if (!edf.Parse())
        return false;

    sess->updateFirst(edf.startdate);
    qint64 duration = edf.GetNumDataRecords() * edf.GetDuration();
    sess->updateLast(edf.startdate + duration);

    for (int s = 0; s < edf.GetNumSignals(); s++) {
        EDFSignal &es = edf.edfsignals[s];
        //qDebug() << "SAD:" << es.label << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum;
        long recs = es.nr * edf.GetNumDataRecords();
        ChannelID code;

        bool hasdata = false;

        for (int i = 0; i < recs; ++i) {
            if (es.data[i] != -1) {
                hasdata = true;
                break;
            }
        }
        if (!hasdata) {
            continue;
        }

        if (matchSignal(OXI_Pulse, es.label)) {
            code = OXI_Pulse;
            ToTimeDelta(sess, edf, es, code, recs, duration);
            sess->setPhysMax(code, 180);
            sess->setPhysMin(code, 18);
        } else if (matchSignal(OXI_SPO2, es.label)) {
            code = OXI_SPO2;
            es.physical_minimum = 60;
            ToTimeDelta(sess, edf, es, code, recs, duration);
            sess->setPhysMax(code, 100);
            sess->setPhysMin(code, 60);
        } else if (es.label != "Crc16") {
            qDebug() << "Unobserved ResMed SAD Signal " << es.label;
        }
    }

    return true;
}


bool ResmedLoader::LoadPLD(Session *sess, const QString & path)
{
    ResMedEDFParser edf(path);
    if (!edf.Parse())
        return false;

    // Is it save to assume the order does not change here?
    enum PLDType { MaskPres = 0, TherapyPres, ExpPress, Leak, RR, Vt, Mv, SnoreIndex, FFLIndex, U1, U2 };

    qint64 duration = edf.GetNumDataRecords() * edf.GetDuration();
    sess->updateFirst(edf.startdate);
    sess->updateLast(edf.startdate + duration);
    QString t;
    int emptycnt = 0;
    EventList *a = nullptr;
    double rate;
    long recs;
    ChannelID code;

    for (int s = 0; s < edf.GetNumSignals(); s++) {
        a = nullptr;
        EDFSignal &es = edf.edfsignals[s];
        recs = es.nr * edf.GetNumDataRecords();

        if (recs <= 0) { continue; }

        rate = double(duration) / double(recs);

        //qDebug() << "EVE:" << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum << es.gain;
        if (matchSignal(CPAP_Snore, es.label)) {
            code = CPAP_Snore;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (matchSignal(CPAP_Pressure, es.label)) {
            code = CPAP_Pressure;
            es.physical_maximum = 25;
            es.physical_minimum = 4;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (matchSignal(CPAP_IPAP, es.label)) {
            code = CPAP_IPAP;
            es.physical_maximum = 25;
            es.physical_minimum = 4;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (matchSignal(CPAP_EPAP, es.label)) { // Expiratory Pressure
            code = CPAP_EPAP;
            es.physical_maximum = 25;
            es.physical_minimum = 4;

            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        }  else if (matchSignal(CPAP_MinuteVent,es.label)) {
            code = CPAP_MinuteVent;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (matchSignal(CPAP_RespRate, es.label)) {
            code = CPAP_RespRate;
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
        } else if (matchSignal(CPAP_TidalVolume, es.label)) {
            code = CPAP_TidalVolume;
            es.gain *= 1000.0;
            es.physical_maximum *= 1000.0;
            es.physical_minimum *= 1000.0;
            //            es.digital_maximum*=1000.0;
            //            es.digital_minimum*=1000.0;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (matchSignal(CPAP_Leak, es.label)) {
            code = CPAP_Leak;
            es.gain *= 60.0;
            es.physical_maximum *= 60.0;
            es.physical_minimum *= 60.0;
            //            es.digital_maximum*=60.0;
            //            es.digital_minimum*=60.0;
            es.physical_dimension = "L/M";
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0, true);
            sess->setPhysMax(code, 120.0);
            sess->setPhysMin(code, 0);
        } else if (matchSignal(CPAP_FLG, es.label)) {
            code = CPAP_FLG;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (matchSignal(CPAP_MaskPressure, es.label)) {
            code = CPAP_MaskPressure;
            es.physical_maximum = 25;
            es.physical_minimum = 4;

            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (matchSignal(CPAP_IE, es.label)) { //I:E ratio
            code = CPAP_IE;
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (matchSignal(CPAP_Ti, es.label)) {
            code = CPAP_Ti;
            // There are TWO of these with the same label on my VPAP Adapt 36037

            if (sess->eventlist.contains(code)) {
                continue;
            }
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (matchSignal(CPAP_Te, es.label)) {
            code = CPAP_Te;
            // There are TWO of these with the same label on my VPAP Adapt 36037
            if (sess->eventlist.contains(code)) {
                continue;
            }
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (matchSignal(CPAP_TgMV, es.label)) {
            code = CPAP_TgMV;
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label == "") { // What the hell resmed??
            if (emptycnt == 0) {
                code = RMS9_E01;
//                ToTimeDelta(sess, edf, es, code, recs, duration);
            } else if (emptycnt == 1) {
                code = RMS9_E02;
//                ToTimeDelta(sess, edf, es, code, recs, duration);
            } else {
                qDebug() << "Unobserved Empty Signal " << es.label;
            }

            emptycnt++;
        }  else if (es.label != "Crc16") {
            qDebug() << "Unobserved ResMed PLD Signal " << es.label;
            a = nullptr;
        }

        if (a) {
            sess->setMin(code, a->Min());
            sess->setMax(code, a->Max());
            sess->setPhysMin(code, es.physical_minimum);
            sess->setPhysMax(code, es.physical_maximum);
            a->setDimension(es.physical_dimension);
        }

    }

    return true;
}

void ResInitModelMap()
{
    // don't really need this anymore
//    Resmed_Model_Map = {
//        { "S9 Escape",      { 36001, 36011, 36021, 36141, 36201, 36221, 36261, 36301, 36361 } },
//        { "S9 Escape Auto", { 36002, 36012, 36022, 36302, 36362 } },
//        { "S9 Elite",       { 36003, 36013, 36023, 36103, 36113, 36123, 36143, 36203, 36223, 36243, 36263, 36303, 36343, 36363 } },
//        { "S9 Autoset",     { 36005, 36015, 36025, 36105, 36115, 36125, 36145, 36205, 36225, 36245, 36265, 36305, 36325, 36345, 36365 } },
//        { "S9 AutoSet CS",  { 36100, 36110, 36120, 36140, 36200, 36220, 36360 } },
//        { "S9 AutoSet 25",  { 36106, 36116, 36126, 36146, 36206, 36226, 36366 } },
//        { "S9 AutoSet for Her", { 36065 } },
//        { "S9 VPAP S",      { 36004, 36014, 36024, 36114, 36124, 36144, 36204, 36224, 36284, 36304 } },
//        { "S9 VPAP Auto",   { 36006, 36016, 36026 } },
//        { "S9 VPAP Adapt",  { 36037, 36007, 36017, 36027, 36367 } },
//        { "S9 VPAP ST",     { 36008, 36018, 36028, 36108, 36148, 36208, 36228, 36368 } },
//        { "S9 VPAP ST 22",  { 36118, 36128 } },
//        { "S9 VPAP ST-A",   { 36039, 36159, 36169, 36379 } },
//      //S8 Series
//        { "S8 Escape",      { 33007 } },
//        { "S8 Elite II",    { 33039 } },
//        { "S8 Escape II",   { 33051 } },
//        { "S8 Escape II AutoSet", { 33064 } },
//        { "S8 AutoSet II",  { 33129 } },
//    };

    ////////////////////////////////////////////////////////////////////////////
    // Translation lookup table for non-english machines
    ////////////////////////////////////////////////////////////////////////////

    // Only put the first part, enough to be identifiable, because ResMed likes
    // to signal names crop short
    // Read this from a table?

    resmed_codes.clear();

    // BRP file
    resmed_codes[CPAP_FlowRate].push_back("Flow");
    resmed_codes[CPAP_FlowRate].push_back("Flow.40ms");
    resmed_codes[CPAP_MaskPressureHi].push_back("Mask Pres");
    resmed_codes[CPAP_MaskPressureHi].push_back("Press.40ms");


    // PLD File
    resmed_codes[CPAP_MaskPressure].push_back("Mask Pres");


    resmed_codes[CPAP_RespEvent].push_back("Resp Event");
    resmed_codes[CPAP_Pressure].push_back("Therapy Pres");
    resmed_codes[CPAP_IPAP].push_back("Insp Pres");
    resmed_codes[CPAP_IPAP].push_back("IPAP");
    resmed_codes[CPAP_IPAP].push_back("S.BL.IPAP");
    resmed_codes[CPAP_EPAP].push_back("Exp Pres");
    resmed_codes[CPAP_EPAP].push_back("EprPress.2s");
    resmed_codes[CPAP_EPAP].push_back("EPAP");
    resmed_codes[CPAP_EPAP].push_back("S.BL.EPAP");
    resmed_codes[CPAP_EPAPHi].push_back("Max EPAP");
    resmed_codes[CPAP_EPAPLo].push_back("Min EPAP");
    resmed_codes[CPAP_EPAPLo].push_back("S.VA.MinEPAP");
    resmed_codes[CPAP_IPAPHi].push_back("Max IPAP");
    resmed_codes[CPAP_IPAPHi].push_back("S.VA.MaxIPAP");
    resmed_codes[CPAP_IPAPLo].push_back("Min IPAP");

    resmed_codes[CPAP_PS].push_back("PS");
    resmed_codes[CPAP_PS].push_back("S.VA.PS");
    resmed_codes[CPAP_PSMin].push_back("Min PS");
    resmed_codes[CPAP_PSMax].push_back("Max PS");

    resmed_codes[CPAP_Leak].push_back("Leak"); // Leak Leck Lekk Läck Fuites
    resmed_codes[CPAP_Leak].push_back("Leck");
    resmed_codes[CPAP_Leak].push_back("Fuites");
    resmed_codes[CPAP_Leak].push_back("Fuite");
    resmed_codes[CPAP_Leak].push_back("Fuga");

    resmed_codes[CPAP_Leak].push_back("\xE6\xBC\x8F\xE6\xB0\x94");
    resmed_codes[CPAP_Leak].push_back("Lekk");
    resmed_codes[CPAP_Leak].push_back("Läck");
    resmed_codes[CPAP_Leak].push_back("LÃ¤ck");
    resmed_codes[CPAP_RespRate].push_back("RR");
    resmed_codes[CPAP_RespRate].push_back("AF");
    resmed_codes[CPAP_RespRate].push_back("FR");
    resmed_codes[CPAP_MinuteVent].push_back("MV");
    resmed_codes[CPAP_MinuteVent].push_back("VM");
    resmed_codes[CPAP_TidalVolume].push_back("Vt");
    resmed_codes[CPAP_TidalVolume].push_back("VC");
    resmed_codes[CPAP_IE].push_back("I:E");
    resmed_codes[CPAP_IE].push_back("IERatio.2s");
    resmed_codes[CPAP_Snore].push_back("Snore");
    resmed_codes[CPAP_FLG].push_back("FFL Index");
    resmed_codes[CPAP_Ti].push_back("Ti");
    resmed_codes[CPAP_Ti].push_back("B5ITime.2s");
    resmed_codes[CPAP_Te].push_back("Te");
    resmed_codes[CPAP_Te].push_back("B5ETime.2s");

    resmed_codes[CPAP_TgMV].push_back("TgMV");
    resmed_codes[CPAP_TgMV].push_back("TgtVent.2s");
    resmed_codes[OXI_Pulse].push_back("Pulse");
    resmed_codes[OXI_Pulse].push_back("Puls");
    resmed_codes[OXI_Pulse].push_back("Pouls");
    resmed_codes[OXI_Pulse].push_back("Pols");
    resmed_codes[OXI_SPO2].push_back("SpO2");
    resmed_codes[CPAP_Obstructive].push_back("Obstructive apnea");
    resmed_codes[CPAP_Hypopnea].push_back("Hypopnea");
    resmed_codes[CPAP_Apnea].push_back("Apnea");
    resmed_codes[CPAP_RERA].push_back("Arousal");
    resmed_codes[CPAP_ClearAirway].push_back("Central apnea");
    resmed_codes[CPAP_Mode].push_back("Mode");
    resmed_codes[CPAP_Mode].push_back("Modus");
    resmed_codes[CPAP_Mode].push_back("Funktion");
    resmed_codes[CPAP_Mode].push_back("\xE6\xA8\xA1\xE5\xBC\x8F");  // Chinese

    resmed_codes[RMS9_SetPressure].push_back("Set Pressure");
    resmed_codes[RMS9_SetPressure].push_back("Eingest. Druck");
    resmed_codes[RMS9_SetPressure].push_back("Ingestelde druk");
    resmed_codes[RMS9_SetPressure].push_back("\xE8\xAE\xBE\xE5\xAE\x9A\xE5\x8E\x8B\xE5\x8A\x9B"); // Chinese
    resmed_codes[RMS9_SetPressure].push_back("Pres. prescrite");
    resmed_codes[RMS9_SetPressure].push_back("Inställt tryck");
    resmed_codes[RMS9_SetPressure].push_back("InstÃ¤llt tryck");

    resmed_codes[RMS9_EPR].push_back("EPR");
    //resmed_codes[RMS9_EPR].push_back("S.EPR.EPRType");

    resmed_codes[RMS9_EPR].push_back("\xE5\x91\xBC\xE6\xB0\x94\xE9\x87\x8A\xE5\x8E\x8B\x28\x45\x50"); // Chinese
    resmed_codes[RMS9_EPRLevel].push_back("EPR Level");
    resmed_codes[RMS9_EPRLevel].push_back("EPR-Stufe");
    resmed_codes[RMS9_EPRLevel].push_back("EPR-niveau");
    resmed_codes[RMS9_EPRLevel].push_back("\x45\x50\x52\x20\xE6\xB0\xB4\xE5\xB9\xB3"); // Chinese

    resmed_codes[RMS9_EPRLevel].push_back("Niveau EPR");
    resmed_codes[RMS9_EPRLevel].push_back("EPR-nivå");
    resmed_codes[RMS9_EPRLevel].push_back("EPR-nivÃ¥");
    resmed_codes[CPAP_PressureMax].push_back("Max Pressure");
    resmed_codes[CPAP_PressureMax].push_back("Max. Druck");
    resmed_codes[CPAP_PressureMax].push_back("Max druk");

    resmed_codes[CPAP_PressureMax].push_back("\xE6\x9C\x80\xE5\xA4\xA7\xE5\x8E\x8B\xE5\x8A\x9B"); // Chinese
    resmed_codes[CPAP_PressureMax].push_back("Pression max.");
    resmed_codes[CPAP_PressureMax].push_back("Max tryck");
    resmed_codes[CPAP_PressureMin].push_back("Min Pressure");
    resmed_codes[CPAP_PressureMin].push_back("Min. Druck");
    resmed_codes[CPAP_PressureMin].push_back("Min druk");
    resmed_codes[CPAP_PressureMin].push_back("\xE6\x9C\x80\xE5\xB0\x8F\xE5\x8E\x8B\xE5\x8A\x9B"); // Chinese
    resmed_codes[CPAP_PressureMin].push_back("Pression min.");
    resmed_codes[CPAP_PressureMin].push_back("Min tryck");

    // SAD file
    resmed_codes[OXI_Pulse].push_back("Pulse.1s");
    resmed_codes[OXI_SPO2].push_back("SpO2.1s");

    // PLD file
    resmed_codes[CPAP_MaskPressure].push_back("MaskPress.2s");
    resmed_codes[CPAP_Pressure].push_back("Press.2s");   // Un problemo... IPAP also uses this.. :/
    resmed_codes[CPAP_EPAP].push_back("EPRPress.2s");
    resmed_codes[CPAP_Leak].push_back("Leak.2s");
    resmed_codes[CPAP_RespRate].push_back("RespRate.2s");
    resmed_codes[CPAP_TidalVolume].push_back("TidVol.2s");
    resmed_codes[CPAP_MinuteVent].push_back("MinVent.2s");
    resmed_codes[CPAP_Snore].push_back("Snore.2s");
    resmed_codes[CPAP_FLG].push_back("FlowLim.2s");

    //S.AS.StartPress
    resmed_codes[CPAP_PressureMin].push_back("S.AS.MinPress");
    resmed_codes[CPAP_PressureMax].push_back("S.AS.MaxPress");

    resmed_codes[RMS9_SetPressure].push_back("S.C.Press");

    resmed_codes[RMS9_EPRLevel].push_back("S.EPR.Level");
}

ChannelID ResmedLoader::CPAPModeChannel() { return RMS9_Mode; }
ChannelID ResmedLoader::PresReliefMode() { return RMS9_EPR; }
ChannelID ResmedLoader::PresReliefLevel() { return RMS9_EPRLevel; }

void ResmedLoader::initChannels()
{
    using namespace schema;
    Channel * chan = nullptr;
    channel.add(GRP_CPAP, chan = new Channel(RMS9_Mode = 0xe203, SETTING, MT_CPAP,   SESSION,
        "RMS9_Mode",
        QObject::tr("Mode"),
        QObject::tr("CPAP Mode"),
        QObject::tr("Mode"),
        "", LOOKUP, Qt::green));

    chan->addOption(0, QObject::tr("CPAP"));
    chan->addOption(1, QObject::tr("APAP"));
    chan->addOption(2, QObject::tr("VPAP-T"));
    chan->addOption(3, QObject::tr("VPAP-S"));
    chan->addOption(4, QObject::tr("VPAP-S/T"));
    chan->addOption(5, QObject::tr("??"));
    chan->addOption(6, QObject::tr("VPAPauto"));
    chan->addOption(7, QObject::tr("ASV"));
    chan->addOption(8, QObject::tr("ASVAuto"));
    chan->addOption(9, QObject::tr("???"));
    chan->addOption(10, QObject::tr("???"));
    chan->addOption(11, QObject::tr("Auto for Her"));

    channel.add(GRP_CPAP, chan = new Channel(RMS9_EPR = 0xe201, SETTING, MT_CPAP,   SESSION,
        "EPR", QObject::tr("EPR"),
        QObject::tr("ResMed Exhale Pressure Relief"),
        QObject::tr("EPR"),
        "", LOOKUP, Qt::green));


    chan->addOption(0, STR_TR_Off);
    chan->addOption(1, QObject::tr("Ramp Only"));
    chan->addOption(2, QObject::tr("Full Time"));
    chan->addOption(3, QObject::tr("Patient???"));

    channel.add(GRP_CPAP, chan = new Channel(RMS9_EPRLevel = 0xe202, SETTING,  MT_CPAP,  SESSION,
        "EPRLevel", QObject::tr("EPR Level"),
        QObject::tr("Exhale Pressure Relief Level"),
        QObject::tr("EPR Level"),
        "", LOOKUP, Qt::blue));

    chan->addOption(0, QObject::tr("0cmH2O"));
    chan->addOption(1, QObject::tr("1cmH2O"));
    chan->addOption(2, QObject::tr("2cmH2O"));
    chan->addOption(3, QObject::tr("3cmH2O"));

//    RMS9_SmartStart, RMS9_HumidStatus, RMS9_HumidLevel,
//             RMS9_PtAccess, RMS9_Mask, RMS9_ABFilter, RMS9_ClimateControl, RMS9_TubeType,
//             RMS9_Temp, RMS9_TempEnable;

    channel.add(GRP_CPAP, chan = new Channel(RMS9_SmartStart = 0xe204, SETTING, MT_CPAP, SESSION,
        "RMS9_SmartStart", QObject::tr("SmartStart"),
        QObject::tr("Machine auto starts by breathing"),
        QObject::tr("Smart Start"),
        "", LOOKUP, Qt::black));

    chan->addOption(0, STR_TR_Off);
    chan->addOption(1, STR_TR_On);

    channel.add(GRP_CPAP, chan = new Channel(RMS9_HumidStatus = 0xe205, SETTING, MT_CPAP, SESSION,
        "RMS9_HumidStat", QObject::tr("Humid. Status"),
        QObject::tr("Humidifier Enabled Status"),
        QObject::tr("Humidifier Status"),
        "", LOOKUP, Qt::black));

    chan->addOption(0, STR_TR_Off);
    chan->addOption(1, STR_TR_On);

    channel.add(GRP_CPAP, chan = new Channel(RMS9_HumidLevel = 0xe206, SETTING, MT_CPAP, SESSION,
        "RMS9_HumidLevel", QObject::tr("Humid. Level"),
        QObject::tr("Humidity Level"),
        QObject::tr("Humidity Level"),
        "", LOOKUP, Qt::black));

    chan->addOption(0, STR_TR_Off);
    chan->addOption(1, "1");
    chan->addOption(2, "2");
    chan->addOption(3, "3");
    chan->addOption(4, "4");
    chan->addOption(5, "5");
    chan->addOption(6, "6");
    chan->addOption(7, "7");
    chan->addOption(8, "8");

    channel.add(GRP_CPAP, chan = new Channel(RMS9_Temp = 0xe207, SETTING, MT_CPAP, SESSION,
        "RMS9_Temp", QObject::tr("Temperature"),
        QObject::tr("ClimateLine Temperature"),
        QObject::tr("Temperature"),
        "ºC", INTEGER, Qt::black));


    channel.add(GRP_CPAP, chan = new Channel(RMS9_TempEnable = 0xe208, SETTING, MT_CPAP, SESSION,
        "RMS9_TempEnable", QObject::tr("Temp. Enable"),
        QObject::tr("ClimateLine Temperature Enable"),
        QObject::tr("Temperature Enable"),
        "", LOOKUP, Qt::black));

    chan->addOption(0, STR_TR_Off);
    chan->addOption(1, "1");
    chan->addOption(2, "2");
    chan->addOption(3, "3");

    channel.add(GRP_CPAP, chan = new Channel(RMS9_ABFilter= 0xe209, SETTING, MT_CPAP, SESSION,
        "RMS9_ABFilter", QObject::tr("AB Filter"),
        QObject::tr("Antibacterial Filter"),
        QObject::tr("Antibacterial Filter"),
        "", LOOKUP, Qt::black));

    chan->addOption(0, STR_TR_No);
    chan->addOption(1, STR_TR_Yes);

    channel.add(GRP_CPAP, chan = new Channel(RMS9_PtAccess= 0xe20A, SETTING, MT_CPAP, SESSION,
        "RMS9_PTAccess", QObject::tr("Pt. Access"),
        QObject::tr("Patient Access"),
        QObject::tr("Patient Access"),
        "", LOOKUP, Qt::black));

    chan->addOption(0, STR_TR_No);
    chan->addOption(1, STR_TR_Yes);

    channel.add(GRP_CPAP, chan = new Channel(RMS9_ClimateControl= 0xe20B, SETTING, MT_CPAP, SESSION,
        "RMS9_ClimateControl", QObject::tr("Climate Control"),
        QObject::tr("Climate Control"),
        QObject::tr("Climate Control"),
        "", LOOKUP, Qt::black));

    chan->addOption(0, QObject::tr("Manual"));
    chan->addOption(1, QObject::tr("Auto"));

    channel.add(GRP_CPAP, chan = new Channel(RMS9_Mask= 0xe20C, SETTING, MT_CPAP, SESSION,
        "RMS9_Mask", QObject::tr("Mask"),
        QObject::tr("ResMed Mask Setting"),
        QObject::tr("Mask"),
        "", LOOKUP, Qt::black));

    chan->addOption(0, QObject::tr("Pillows"));
    chan->addOption(1, QObject::tr("Full Face"));
    chan->addOption(2, QObject::tr("Nasal"));

    channel.add(GRP_CPAP, chan = new Channel(RMS9_RampEnable = 0xe20D, SETTING, MT_CPAP, SESSION,
        "RMS9_RampEnable", QObject::tr("Ramp"),
        QObject::tr("Ramp Enable"),
        QObject::tr("Ramp"),
        "", LOOKUP, Qt::black));

    chan->addOption(0, STR_TR_Off);
    chan->addOption(1, STR_TR_On);


    // Modelmap needs channels initalized above!!!
    ResInitModelMap();

}

bool resmed_initialized = false;
void ResmedLoader::Register()
{
    if (resmed_initialized) { return; }

    qDebug() << "Registering ResmedLoader";
    RegisterLoader(new ResmedLoader());

    resmed_initialized = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Model number information
//    36003, 36013, 36023, 36103, 36113, 36123, 36143, 36203,
//    36223, 36243, 36263, 36303, 36343, 36363 S9 Elite Series
//    36005, 36015, 36025, 36105, 36115, 36125, 36145, 36205,
//    36225, 36245, 36265, 36305, 36325, 36345, 36365 S9 AutoSet Series
//    36065 S9 AutoSet for Her
//    36001, 36011, 36021, 36141, 36201, 36221, 36261, 36301,
//    36361 S9 Escape
//    36002, 36012, 36022, 36302, 36362 S9 Escape Auto
//    36004, 36014, 36024, 36114, 36124, 36144, 36204, 36224,
//    36284, 36304 S9 VPAP S (+ H5i, + Climate Control)
//    36006, 36016, 36026 S9 VPAP AUTO (+ H5i, + Climate Control)

//    36007, 36017, 36027, 36367
//    S9 VPAP ADAPT (+ H5i, + Climate
//    Control)
//    36008, 36018, 36028, 36108, 36148, 36208, 36228, 36368 S9 VPAP ST (+ H5i, + Climate Control)
//    36100, 36110, 36120, 36140, 36200, 36220, 36360 S9 AUTOSET CS
//    36106, 36116, 36126, 36146, 36206, 36226, 36366 S9 AUTOSET 25
//    36118, 36128 S9 VPAP ST 22
//    36039, 36159, 36169, 36379 S9 VPAP ST-A
//    24921, 24923, 24925, 24926, 24927 ResMed Power Station II (RPSII)
//    33030 S8 Compact
//    33001, 33007, 33013, 33036, 33060 S8 Escape
//    33032 S8 Lightweight
//    33033 S8 AutoScore
//    33048, 33051, 33052, 33053, 33054, 33061 S8 Escape II
//    33055 S8 Lightweight II
//    33021 S8 Elite
//    33039, 33045, 33062, 33072, 33073, 33074, 33075 S8 Elite II
//    33044 S8 AutoScore II
//    33105, 33112, 33126 S8 AutoSet (including Spirit & Vantage)
//    33128, 33137 S8 Respond
//    33129, 33141, 33150 S8 AutoSet II
//    33136, 33143, 33144, 33145, 33146, 33147, 33148 S8 AutoSet Spirit II
//    33138 S8 AutoSet C
//    26101, 26121 VPAP Auto 25
//    26119, 26120 VPAP S
//    26110, 26122 VPAP ST
//    26104, 26105, 26125, 26126 S8 Auto 25
//    26102, 26103, 26106, 26107, 26108, 26109, 26123, 26127 VPAP IV
//    26112, 26113, 26114, 26115, 26116, 26117, 26118, 26124 VPAP IV ST

