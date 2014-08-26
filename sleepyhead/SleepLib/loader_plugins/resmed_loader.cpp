/* SleepLib ResMed Loader Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
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
#include <cmath>

#include "resmed_loader.h"
#include "SleepLib/session.h"
#include "SleepLib/calcs.h"

#ifdef DEBUG_EFFICIENCY
#include <QElapsedTimer>  // only available in 4.8
#endif

extern QProgressBar *qprogress;

QHash<QString, QList<quint16> > Resmed_Model_Map;

const QString STR_UnknownModel = "Resmed S9 ???";

ChannelID RMS9_EPR, RMS9_EPRLevel;


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
const QString STR_ext_EDF = "edf";
const QString STR_ext_gz = ".gz";


// Looks up foreign language Signal names that match this channelID
EDFSignal *EDFParser::lookupSignal(ChannelID ch)
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

EDFSignal *EDFParser::lookupLabel(QString name, int index)
{
    QHash<QString, QList<EDFSignal *> >::iterator it = signalList.find(name);

    if (it == signalList.end()) return nullptr;

    if (index >= it.value().size()) return nullptr;

    return it.value()[index];
}

EDFParser::EDFParser(QString name)
{
    buffer = nullptr;
    Open(name);
}
EDFParser::~EDFParser()
{
    for (QVector<EDFSignal>::iterator s = edfsignals.begin(); s != edfsignals.end(); s++) {
        if ((*s).data) { delete [](*s).data; }
    }

    if (buffer) { delete [] buffer; }
}

void ResmedLoader::ParseSTR(Machine *mach, QStringList strfiles)
{
    QStringList::iterator strend = strfiles.end();
    for (QStringList::iterator it = strfiles.begin(); it != strend; ++it) {
        EDFParser str(*it);
        if (!str.Parse()) continue;
        if (mach->serial() != str.serialnumber) {
            qDebug() << "Trying to import a STR.edf from another machine, skipping" << mach->serial() << str.serialnumber;
            qDebug() << (*it);
            continue;
        }

        QDateTime start = QDateTime::fromMSecsSinceEpoch(str.startdate);
        QDate date = start.date();

        qDebug() << "Parsing" << *it << date << str.GetNumDataRecords() << str.GetNumSignals();

        EDFSignal *maskon = str.lookupLabel("Mask On");
        if (!maskon) {
            maskon = str.lookupLabel("MaskOn");
        }
        EDFSignal *maskoff = str.lookupLabel("Mask Off");
        if (!maskoff) {
             maskoff = str.lookupLabel("MaskOff");
        }
        EDFSignal *sig = nullptr;
        quint32 laston = 0;

        bool skipday;

        int size = str.GetNumDataRecords();
        int cnt=0;
        QDateTime dt = start;

        // For each data record, representing 1 day each
        for (int rec = 0; rec < str.GetNumDataRecords(); ++rec) {
            uint timestamp = dt.toTime_t();
            int recstart = rec * maskon->nr;

            skipday = false;
            if ((++cnt % 10) == 0) {
                // TODO: Change me to emit once MachineLoader is QObjectified...
                if (qprogress) { qprogress->setValue(10.0 + (float(cnt) / float(size) * 90.0)); }

                QApplication::processEvents();
            }

            // Scan the mask on/off events
            for (int s = 0; s < maskon->nr; ++s) {
                qint32 on = maskon->data[recstart+s];
                qint32 off = maskoff->data[recstart+s];
                quint32 ontime = timestamp + on * 60;
                quint32 offtime = timestamp + off * 60;

                // -1 marks empty record, but can start with mask off, if sleep crosses noon
                if (on < 0) {
                    if (off < 0) continue;  // Both are -1, skip the rest of this day
                    // laston stops on this record

                    QMap<quint32, STRRecord>::iterator si = strsess.find(laston);
                    if (si != strsess.end()) {
                        if (si.value().maskoff == 0) {
                            if (offtime > laston) {
                                si.value().maskoff = offtime;
                            }
                        } else {
                            if (si.value().maskoff != offtime) {
                                // not sure why this happens.
                                qDebug() << "WTF?? mask off's don't match"
                                        << QDateTime::fromTime_t(laston).toString()
                                        << QDateTime::fromTime_t(si.value().maskoff).toString()
                                        << "!=" << QDateTime::fromTime_t(offtime).toString();
                            }
                            //Q_ASSERT(si.value().maskoff == offtime);
                        }
                    }
                    continue;
                }

                QMap<quint32, STRRecord>::iterator sid = strsess.find(ontime);
                // Record already exists?
                if (sid != strsess.end()) {
                    // then skip
                    laston = ontime;
                    continue;
                }

                // For every mask on, there will be a session within 1 minute either way
                // We can use that for data matching
                STRRecord R;

                R.maskon = ontime;
                if (offtime > 0) {
                    R.maskoff = offtime;
                }
                CPAPMode mode = MODE_UNKNOWN;

                if ((sig = str.lookupSignal(CPAP_Mode))) {
                    int mod = EventDataType(sig->data[rec]) * sig->gain + sig->offset;

                    if (mod >= 8) {       // mod 8 == vpap adapt variable epap
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

                }


                if ((sig = str.lookupLabel("Mask Dur"))) {
                    R.maskdur = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }


                if ((sig = str.lookupLabel("Leak Med"))) {
                    float gain = sig->gain * 60.0;
                    R.leakgain = gain;
                    R.leakmed = EventDataType(sig->data[rec]) * gain + sig->offset;
                }
                if ((sig = str.lookupLabel("Leak Max"))) {
                    float gain = sig->gain * 60.0;
                    R.leakgain = gain;
                    R.leakmax = EventDataType(sig->data[rec]) * gain + sig->offset;
                }
                if ((sig = str.lookupLabel("Leak 95"))) {
                    float gain = sig->gain * 60.0;
                    R.leakgain = gain;
                    R.leak95 = EventDataType(sig->data[rec]) * gain + sig->offset;
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
                if ((sig = str.lookupSignal(CPAP_EPAP))) {
                    R.epap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if ((sig = str.lookupSignal(CPAP_EPAPHi))) {
                    R.max_epap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if ((sig = str.lookupSignal(CPAP_EPAPLo))) {
                    R.min_epap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                bool haveipap = false;
                if ((sig = str.lookupSignal(CPAP_IPAP))) {
                    R.ipap = EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                    haveipap = true;
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
                    R.ipap = R.min_ipap = R.max_ipap = R.max_epap + R.max_ps;
                }

//                if (mode == MODE_ASV_VARIABLE_EPAP) {
//                    // ResMed reuses this code on 36037.. the dummies :(
//                } else if (mode == MODE_ASV) {
//                    if (!haveipap) {
//                        R.ipap = R.min_ipap = R.max_ipap = R.max_epap + R.max_ps;
//                    }
//                }

                EventDataType epr = -1, epr_level = -1;
                if ((sig = str.lookupSignal(RMS9_EPR))) {
                    epr= EventDataType(sig->data[rec]) * sig->gain + sig->offset;
                }
                if ((sig = str.lookupSignal(RMS9_EPRLevel))) {
                    epr_level= EventDataType(sig->data[rec]) * sig->gain + sig->offset;
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

                laston = ontime;

                QDateTime dontime = QDateTime::fromTime_t(ontime);
                date = dontime.date();
                R.date = date;

                //CHECKME: Should I be taking noon day split time into account here?
                strdate[date].push_back(&strsess.insert(ontime, R).value());

                //QDateTime dofftime = QDateTime::fromTime_t(offtime);
                //qDebug() << "Mask on" << dontime << "Mask off" << dofftime;
            }
            dt = dt.addDays(1);
        }
    }
}

// Read a 16 bits integer
qint16 EDFParser::Read16()
{
    if ((pos + 2) > filesize) {
        return 0;
    }

#ifdef Q_LITTLE_ENDIAN
    // Intel, etc...
    qint16 res = *(qint16 *)&buffer[pos];
#else
    // ARM, PPC, etc..
    qint16 res = quint8(buffer[pos]) | (qint8(buffer[pos+1]) << 8);
#endif

    pos += 2;
    return res;
}

QString EDFParser::Read(unsigned n)
{
    if ((pos + long(n)) > filesize) {
        return "";
    }

    QByteArray buf(&buffer[pos], n);
    pos+=n;

    return buf.trimmed();
}
bool EDFParser::Parse()
{
    bool ok;
    QString temp, temp2;

    version = QString::fromLatin1(header.version, 8).toLong(&ok);

    if (!ok) {
        return false;
    }

    //patientident=QString::fromLatin1(header.patientident,80);
    recordingident = QString::fromLatin1(header.recordingident, 80); // Serial number is in here..
    int snp = recordingident.indexOf("SRN=");
    serialnumber.clear();
    /*char * idx=index(header.recordingident,'=');
    idx++;
    for (int i=0;i<16;++i) {
        if (*idx==0x20) break;
        serialnumber+=*idx;
        ++idx;
    } */

    for (int i = snp + 4; i < recordingident.length(); i++) {
        if (recordingident[i] == ' ') {
            break;
        }

        serialnumber += recordingident[i];
    }

    QDateTime startDate = QDateTime::fromString(QString::fromLatin1(header.datetime, 16),
                          "dd.MM.yyHH.mm.ss");
    //startDate.toTimeSpec(Qt::UTC);

    QDate d2 = startDate.date();

    if (d2.year() < 2000) {
        d2.setDate(d2.year() + 100, d2.month(), d2.day());
        startDate.setDate(d2);
    }

    if (!startDate.isValid()) {
        qDebug() << "Invalid date time retreieved parsing EDF File " << filename;
        return false;
    }

    startdate = qint64(startDate.toTime_t()) * 1000L;
    //startdate-=timezoneOffset();

    //qDebug() << startDate.toString("yyyy-MM-dd HH:mm:ss");

    num_header_bytes = QString::fromLatin1(header.num_header_bytes, 8).toLong(&ok);

    if (!ok) {
        return false;
    }

    //reserved44=QString::fromLatin1(header.reserved,44);
    num_data_records = QString::fromLatin1(header.num_data_records, 8).toLong(&ok);

    if (!ok) {
        return false;
    }

    dur_data_record = (QString::fromLatin1(header.dur_data_records, 8).toDouble(&ok) * 1000.0L);

    if (!ok) {
        return false;
    }

    num_signals = QString::fromLatin1(header.num_signals, 4).toLong(&ok);

    if (!ok) {
        return false;
    }

    enddate = startdate + dur_data_record * qint64(num_data_records);
    // if (dur_data_record==0)
    //   return false;

    // this could be loaded quicker by transducer_type[signal] etc..

    // Initialize fixed-size signal list.
    edfsignals.resize(num_signals);

    for (int i = 0; i < num_signals; i++) {
        EDFSignal &sig = edfsignals[i];
        sig.data = nullptr;
        sig.label = Read(16);

        signal_labels.push_back(sig.label);
        signalList[sig.label].push_back(&sig);
        signal.push_back(&sig);
    }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].transducer_type = Read(80); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].physical_dimension = Read(8); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].physical_minimum = Read(8).toDouble(&ok); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].physical_maximum = Read(8).toDouble(&ok); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].digital_minimum = Read(8).toDouble(&ok); }

    for (int i = 0; i < num_signals; i++) {
        EDFSignal &e = edfsignals[i];
        e.digital_maximum = Read(8).toDouble(&ok);
        e.gain = (e.physical_maximum - e.physical_minimum) / (e.digital_maximum - e.digital_minimum);
        e.offset = 0;
    }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].prefiltering = Read(80); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].nr = Read(8).toLong(&ok); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].reserved = Read(32); }

    // allocate the buffers
    for (int i = 0; i < num_signals; i++) {
        //qDebug//cout << "Reading signal " << signals[i]->label << endl;
        EDFSignal &sig = edfsignals[i];

        long recs = sig.nr * num_data_records;

        if (num_data_records < 0) {
            sig.data = nullptr;
            continue;
        }

        sig.data = new qint16 [recs];
        sig.pos = 0;
    }

    for (int x = 0; x < num_data_records; x++) {
        for (int i = 0; i < num_signals; i++) {
            EDFSignal &sig = edfsignals[i];
#ifdef Q_LITTLE_ENDIAN
            // Intel x86, etc..
            memcpy((char *)&sig.data[sig.pos], (char *)&buffer[pos], sig.nr * 2);
            sig.pos += sig.nr;
            pos += sig.nr * 2;
#else
            // Big endian safe
            for (int j=0;j<sig.nr;j++) {
                qint16 t=Read16();
                sig.data[sig.pos++]=t;
            }
#endif
        }
    }

    return true;
}
bool EDFParser::Open(QString name)
{
    Q_ASSERT(buffer == nullptr);

    if (name.endsWith(STR_ext_gz)) {
        // Open and decempress file to buffer

        filename = name.mid(0, -3);

        // Get file length from inside gzip file
        QFile fi(name);

        if (!fi.open(QFile::ReadOnly) || !fi.seek(fi.size() - 4)) {
            goto badfile;
        }

        unsigned char ch[4];
        fi.read((char *)ch, 4);
        filesize = ch[0] | (ch [1] << 8) | (ch[2] << 16) | (ch[3] << 24);

        datasize = filesize - EDFHeaderSize;
        if (datasize < 0) {
            goto badfile;
        }

        // Open gzip file for reading
        gzFile f = gzopen(name.toLatin1(), "rb");
        if (!f) {
            goto badfile;
        }

        // Decompressed header and data block
        gzread(f, (char *)&header, EDFHeaderSize);
        buffer = new char [datasize];
        gzread(f, buffer, datasize);
        gzclose(f);
    } else {

        // Open and read uncompressed file
        QFile f(name);

        if (!f.open(QIODevice::ReadOnly)) {
            goto badfile;
        }

        filename = name;
        filesize = f.size();
        datasize = filesize - EDFHeaderSize;

        if (datasize < 0) {
            goto badfile;
        }

        f.read((char *)&header, EDFHeaderSize);

        buffer = new char [datasize];
        f.read(buffer, datasize);
        f.close();
    }

    pos = 0;
    return true;

badfile:
    qDebug() << "EDFParser::Open() Couldn't open file" << name;
    return false;
}


void ResmedImport::run()
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

    if (!group.EVE.isEmpty()) {
        loader->LoadEVE(sess, group.EVE);
    }
    if (!group.BRP.isEmpty()) {
        loader->LoadBRP(sess, group.BRP);
    }
    if (!group.PLD.isEmpty()) {
        loader->LoadPLD(sess, group.PLD);
    }
    if (!group.SAD.isEmpty()) {
        loader->LoadSAD(sess, group.SAD);
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

        if (R.mode >= 0) {
            sess->settings[CPAP_Mode] = R.mode;
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

        // Ignore all the rest of the sumary data, because there is enough available to calculate it with higher accuracy.

        if (sess->length() > 0) {
            loader->addSession(sess);
        } else {
            delete sess;
            return;
        }
    }

    // Update indexes, process waveform and perform flagging
    sess->UpdateSummaries();

    // Save is not threadsafe
    loader->saveMutex.lock();
    sess->Store(mach->getDataPath());
    loader->saveMutex.unlock();

    // Free the memory used by this session
    sess->TrashEvents();
}

ResmedLoader::ResmedLoader()
{
    m_type = MT_CPAP;
}
ResmedLoader::~ResmedLoader()
{
}

void ResmedImportStage2::run()
{
    Session * sess = new Session(mach, R.maskon);

    sess->really_set_first(qint64(R.maskon) * 1000L);
    sess->really_set_last(qint64(R.maskoff) * 1000L);

    // Claim this record for future imports
    sess->settings[RMS9_MaskOnTime] = R.maskon;
    sess->setSummaryOnly(true);

    sess->SetChanged(true);

    // First take the settings

    if (R.mode >= 0) {
        sess->settings[CPAP_Mode] = R.mode;
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
    Q_ASSERT(dtit != loader->strdate.end());

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
    loader->saveMutex.lock();
    sess->Store(mach->getDataPath());
    loader->saveMutex.unlock();
}


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

                if (value.contains("S9")) {
                    value.replace("S9", "");
                    info.series = value;
                } else if (value.contains("AirSense 10")) {
                    value.replace("AirSense 10", "");
                    info.series = "AirSense 10";
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



struct EDFduration {
    EDFduration() { start = end = 0; type = EDF_UNKNOWN; }
    EDFduration(const EDFduration & copy) {
        path = copy.path;
        start = copy.start;
        end = copy.end;
        type = copy.type;
        filename = copy.filename;
    }
    EDFduration(quint32 start, quint32 end, QString path) :
        start(start), end(end), path(path) {}
    quint32 start;
    quint32 end;
    QString path;
    QString filename;
    EDFType type;
};

EDFType lookupEDFType(QString text)
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

// Looks inside an EDF or EDF.gz and grabs the start and duration
EDFduration getEDFDuration(QString filename)
{
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
    QString ext = filename.section("_", -1).section(".",0,0).toUpper();

    QDateTime dt2 = QDateTime::fromString(filedate, "yyyyMMdd_hhmmss");
    quint32 st2 = dt2.toTime_t();

    start = qMin(st2, start);

    if (end < start) end = qMax(st2, start); // This alone should really cover the EVE.EDF condition

//    if (ext == "EVE") {
//        // This is an unavoidable kludge, because there genuinely is no duration given for EVE files.
//        // It could partially be avoided by parsing the EDF annotations completely, but on days with no events, this would be pointless.

//        // Add some seconds to make sure some overlap happens with related sessions.

//        // ************** Be cautious with this value **************

//        // A Firmware bug causes (perhaps with failing SD card) sessions to sometimes take a long time to write, and it can screw this up
//        // I've really got no way of detecting the other condition.. I can either have one or the other.

//        // Wait... EVE and BRP start at the same time.. that should be enough to counter overlaps!
//        end += 1;
//    }

    if ((end - start) < 10) end = start + 10;

    EDFduration dur(start, end, filename);

    dur.type = lookupEDFType(ext.toUpper());

    return dur;
}

int ResmedLoader::scanFiles(Machine * mach, QString datalog_path)
{
    QHash<QString, SessionID> skipfiles;

    bool create_backups = true; //p_profile->session->backupCardData();

    QString backup_path = mach->getBackupPath();

    QString dlog = datalog_path;

    if (datalog_path == backup_path + RMS9_STR_datalog + "/") {
        // Don't create backups if importing from backup folder
        create_backups = false;
    }

    // Read the "already imported" file list
    QFile impfile(mach->getDataPath()+"/imported_files.csv");
    if (impfile.open(QFile::ReadOnly)) {
        QTextStream impstream(&impfile);
        QString serial;
        impstream >> serial;
        if (mach->serial() == serial) {
            QString line, file, str;
            SessionID sid;
            bool ok;
            do {
                line = impstream.readLine();
                file = line.section(',',0,0);
                str = line.section(',',1);
                sid = str.toInt(&ok);

                skipfiles[file] = sid;
            } while (!impstream.atEnd());
        }
    }
    impfile.close();

    QStringList dirs;
    dirs.push_back(datalog_path);

    QDir dir(datalog_path);

    dir.setFilter(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
    QFileInfoList flist = dir.entryInfoList();
    QString filename;
    bool ok, gz;


    // Scan for any sub folders
    for (int i = 0; i < flist.size(); i++) {
        QFileInfo fi = flist.at(i);
        filename = fi.fileName();

        if (filename.length() == 4) {
            // year folder (used in backups)
            filename.toInt(&ok);

            if (ok) {
                dirs.push_back(fi.canonicalFilePath());
            }
        } else if (filename.length() == 8) {
            // S10 stores sessions per day folders
            filename.toInt(&ok);

            if (ok) {
                dirs.push_back(fi.canonicalFilePath());
            }
        }
    }

    QStringList newSkipFiles;

    QMap<QString, EDFduration> newfiles; // used for duplicate checking, and session overlap testing to group sessions
    QHash<EDFType, QList<EDFduration *> > filesbytype;

    // Scan through all folders looking for EDF files, skip any already imported and peek inside to get durations
    for (int d=0; d < dirs.size(); ++d) {
        dir.setPath(dirs.at(d));
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);

        flist = dir.entryInfoList();

        // get number of files in current directory being processed
        int size = flist.size();

        // For each file in flist...
        for (int i = 0; i < size; i++) {
            QFileInfo fi = flist.at(i);
            filename = fi.fileName();

            // Forget about it if it can't be read.
            if (!fi.isReadable()) {
                continue;
            }

            // Chop off the .gz component if it exists
            if (filename.endsWith(STR_ext_gz)) {
                filename.chop(3);
                gz = true;
            } else { gz = false; }

            // Skip if this file is in the already imported list
            if (skipfiles.contains(filename)) continue;

            if (newfiles.contains(filename)) {
                // Not sure what to do with it.. delete it? check compress status and delete the other one?
                qDebug() << "Duplicate EDF file detected" << filename;
                continue;
            }

            // Peek inside file and get duration in seconds..

            // Accept only .edf and .edf.gz files
            if (filename.right(4).toLower() != "." + STR_ext_EDF) {
                continue;
            }


            QString fullname = fi.canonicalFilePath();

            // Peek inside the EDF file and get the EDFDuration record for the session matching that follows

            QMap<QString, EDFduration>::iterator it = newfiles.insert(filename, getEDFDuration(fullname));
            EDFduration *dur = &it.value();
            dur->filename = filename;

            filesbytype[dur->type].append(dur);
        }
    }

    QList<EDFType> EDForder;
    EDForder.push_back(EDF_PLD);
    EDForder.push_back(EDF_BRP);
    EDForder.push_back(EDF_SAD);
    EDForder.push_back(EDF_EVE);
    EDForder.push_back(EDF_CSL);

    for (int i=0; i<3; i++) {
        EDFType basetype = EDForder.takeFirst();

        // Process PLD files
        QList<EDFduration *> & LIST = filesbytype[basetype];
        int pld_size = LIST.size();
        for (int f=0; f < pld_size; ++f) {
            const EDFduration * dur = LIST.at(f);

            quint32 start = dur->start;
            if (start == 0) continue;

            quint32 end = dur->end;
            QHash<EDFType, QString> grp;
            grp[EDF_PLD] = create_backups ? backup(dur->path, backup_path) : dur->path;;


            QStringList files;
            files.append(dur->filename);


            for (int o=0; o<EDForder.size(); ++o) {
                EDFType type = EDForder.at(o);

                QList<EDFduration *> & EDF_list = filesbytype[type];
                QList<EDFduration *>::iterator item;
                QList<EDFduration *>::iterator list_end = EDF_list.end();
                for (item = EDF_list.begin(); item != list_end; ++item) {
                    const EDFduration * dur2 = *item;

                    // Do the sessions Overlap?
                    if ((start < dur2->end) && ( dur2->start < end)) {
                        start = qMin(start, dur2->start);
                        end = qMax(end, dur2->end);

                        files.append(dur2->filename);

                        grp[type] = create_backups ? backup(dur2->path, backup_path) : dur2->path;

                        filesbytype[type].erase(item);
                        break;
                    }
                }

            }
            if (mach->SessionExists(start) == nullptr) {
                EDFGroup group(grp[EDF_BRP], grp[EDF_EVE], grp[EDF_PLD], grp[EDF_SAD], grp[EDF_CSL]);
                queTask(new ResmedImport(this, start, group, mach));
                for (int i=0; i<files.size(); i++) skipfiles[files.at(i)] = start;
            }
        }
    }


    // No PLD files

/*    QMap<QString, EDFduration>::iterator it;
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
    int c = countTasks();
    runTasks(p_profile->session->multithreading());

    newSkipFiles.append(skipfiles.keys());
    impfile.remove();

    if (impfile.open(QFile::WriteOnly)) {
        QTextStream out(&impfile);
        out << mach->serial();
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
}

int ResmedLoader::Open(QString path)
{

    QString key, value;
    QString line;
    QString newpath;
    QString filename;

    QHash<QString, QString> idmap;  // Temporary properties hash

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
                if (value.contains("S9")) {
                    value.replace("S9", "");
                    info.series = "S9";
                } else if (value.contains("AirSense 10")) {
                    value.replace("AirSense 10", "");
                    info.series = "AirSense 10";
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
    Machine *m = CreateMachine(info);

    bool create_backups = p_profile->session->backupCardData();
    bool compress_backups = p_profile->session->compressBackupData();

    QString backup_path = m->getBackupPath();

    if (path == backup_path) {
        // Don't create backups if importing from backup folder
        create_backups = false;
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Parse the idmap into machine objects properties, (overwriting any old values)
    ///////////////////////////////////////////////////////////////////////////////////
    for (QHash<QString, QString>::iterator i = idmap.begin(); i != idmap.end(); i++) {
        m->properties[i.key()] = i.value();
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Open and Parse STR.edf file
    ///////////////////////////////////////////////////////////////////////////////////
    QStringList strfiles;
    strfiles.push_back(strpath);
    QDir dir(path + "STR_Backup");
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::Readable);
    QFileInfoList flist = dir.entryInfoList();

    {
    int size = flist.size();
    for (int i = 0; i < size; i++) {
        QFileInfo fi = flist.at(i);
        filename = fi.fileName();
        if (filename.startsWith("STR", Qt::CaseInsensitive)) {
            strfiles.push_back(fi.filePath());
        }
    }
    }

    strsess.clear();
    ParseSTR(m, strfiles);

    EDFParser stredf(strpath);
    if (!stredf.Parse()) {
        qDebug() << "Faulty file" << RMS9_STR_strfile;
        return 0;
    }

    if (stredf.serialnumber != info.serial) {
        qDebug() << "Identification.tgt Serial number doesn't match STR.edf!";
    }


    // Creating early as we need the object
    dir.setPath(newpath);


    ///////////////////////////////////////////////////////////////////////////////////
    // Create the backup folder for storing a copy of everything in..
    // (Unless we are importing from this backup folder)
    ///////////////////////////////////////////////////////////////////////////////////
    if (create_backups) {
        if (!dir.exists(backup_path)) {
            if (!dir.mkpath(backup_path + RMS9_STR_datalog)) {
                qDebug() << "Could not create S9 backup directory :-/";
            }
        }

        // Copy Identification files to backup folder
        QFile::copy(path + RMS9_STR_idfile + STR_ext_TGT, backup_path + RMS9_STR_idfile + STR_ext_TGT);
        QFile::copy(path + RMS9_STR_idfile + STR_ext_CRC, backup_path + RMS9_STR_idfile + STR_ext_CRC);

        QDateTime dts = QDateTime::fromMSecsSinceEpoch(stredf.startdate);
        dir.mkpath(backup_path + "STR_Backup");
        QString strmonthly = backup_path + "STR_Backup/STR-" + dts.toString("yyyyMM") + "." + STR_ext_EDF;

        //copy STR files to backup folder
        if (strpath.endsWith(STR_ext_gz)) { // Already compressed. Don't bother decompressing..
            QFile::copy(strpath, backup_path + RMS9_STR_strfile + STR_ext_EDF + STR_ext_gz);
        } else { // Compress STR file to backup folder
            QString strf = backup_path + RMS9_STR_strfile + STR_ext_EDF;

            // Copy most recent to STR.edf
            if (QFile::exists(strf)) {
                QFile::remove(strf);
            }

            if (QFile::exists(strf + STR_ext_gz)) {
                QFile::remove(strf + STR_ext_gz);
            }

            compress_backups ?
            compressFile(strpath, strf)
            :
            QFile::copy(strpath, strf);

        }

        // Keep one STR.edf backup every month
        if (!QFile::exists(strmonthly) && !QFile::exists(strmonthly + ".gz")) {
            compress_backups ?
            compressFile(strpath, strmonthly)
            :
            QFile::copy(strpath, strmonthly);
        }

        // Meh.. these can be calculated if ever needed for ResScan SDcard export
        QFile::copy(path + "STR.crc", backup_path + "STR.crc");
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Process the actual STR.edf data
    ///////////////////////////////////////////////////////////////////////////////////

    qint64 numrecs = stredf.GetNumDataRecords();
    qint64 duration = numrecs * stredf.GetDuration();

    int days = duration / 86400000L; // GetNumDataRecords = this.. Duh!

    // Process STR.edf and find first and last time for each day

    QVector<qint8> dayused;
    dayused.resize(days);

    time_t time = stredf.startdate / 1000L; // == 12pm on first day

    // reset time to first day
    time = stredf.startdate / 1000;

    ///////////////////////////////////////////////////////////////////////////////////
    // Scan DATALOG files, sort, and import any new sessions
    ///////////////////////////////////////////////////////////////////////////////////

    int new_sessions = scanFiles(m, newpath);

    ////////////////////////////////////////////////////////////////////////////////////
    // Now look for any new summary data that can be extracted from STR.edf records
    ////////////////////////////////////////////////////////////////////////////////////
    QMap<quint32, STRRecord>::iterator it;
    QMap<quint32, STRRecord>::iterator end = strsess.end();

    QHash<SessionID, Session *>::iterator sessit;
    QHash<SessionID, Session *>::iterator sessend = m->sessionlist.end();;

    int size = m->sessionlist.size();
    int cnt=0;
    Session * sess;

    // Scan through all sessions, and remove any strsess records that have a matching session already
    for (sessit = m->sessionlist.begin(); sessit != sessend; ++sessit) {
        sess = *sessit;
        quint32 key = sess->settings[RMS9_MaskOnTime].toUInt();

        QMap<quint32, STRRecord>::iterator e = strsess.find(key);
        if (e != end) {
            strsess.erase(e);
        }
    }


    size = strsess.size();
    cnt=0;
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
            m->skipSaveTask();
            continue;
        }

        //Q_ASSERT(R.sessionid == 0);

        // the following should not happen
        if (R.sessionid > 0) {
            m->skipSaveTask();
            continue;
        }

        queTask(new ResmedImportStage2(this, R, m));
    }

    new_sessions += countTasks();
    runTasks();

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
    return new_sessions;
}


QString ResmedLoader::backup(QString fullname, QString backup_path)
{
    bool compress = p_profile->session->compressBackupData();

    QString filename, yearstr, newname, oldname;
    bool ok, gz = (fullname.right(3).toLower() == STR_ext_gz);

    filename = fullname.section("/", -1);

    if (gz) {
        filename.chop(3);
    }

    yearstr = filename.left(4);
    yearstr.toInt(&ok, 10);


    if (!ok) {
        qDebug() << "Invalid EDF filename given to ResMedLoader::backup()";
        return "";
    }

    newname = backup_path + RMS9_STR_datalog + "/" + yearstr;
    QDir dir;
    dir.mkpath(newname);
    newname += "/" + filename;

    QString tmpname = newname;

    if (compress) {
        newname += STR_ext_gz;
    }

    // First make sure the correct backup exists.
    if (!QFile::exists(newname)) {
        if (compress) {
            gz ?
            QFile::copy(fullname, newname)      // Already compressed.. copy it to the right location
            :
            compressFile(fullname, newname);
        } else {
            // dont really care if it's compressed and not meant to be, leave it that way
            QFile::copy(fullname, newname);
        }
    } // else backup already exists...

    // Now the correct backup is in place, we can trash any
    if (compress) {
        // Remove any uncompressed duplicate
        if (QFile::exists(tmpname)) {
            QFile::remove(tmpname);
        }
    } else {
        // Delete the non compressed copy and choose it instead.
        if (QFile::exists(tmpname + STR_ext_gz)) {
            QFile::remove(tmpname);
            newname = tmpname + STR_ext_gz;
        }

    }

    // Remove any traces from old backup directory structure
    oldname = backup_path + RMS9_STR_datalog + "/" + filename;

    if (QFile::exists(oldname)) {
        QFile::remove(oldname);
    }

    if (QFile::exists(oldname + STR_ext_gz)) {
        QFile::remove(oldname + STR_ext_gz);
    }

    return newname;
}


bool ResmedLoader::LoadEVE(Session *sess, const QString & path)
{
    EDFParser edf(path);
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

    EventList *OA = nullptr, *HY = nullptr, *CA = nullptr, *UA = nullptr;

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
        sess->updateFirst(tt);
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
                        OA->AddEvent(tt, duration);
                    } else if (matchSignal(CPAP_Hypopnea, t)) {
                        HY->AddEvent(tt, duration + 10); // Only Hyponea's Need the extra duration???
                    } else if (matchSignal(CPAP_Apnea, t)) {
                        UA->AddEvent(tt, duration);
                    } else if (matchSignal(CPAP_ClearAirway, t)) {
                        // Not all machines have it, so only create it when necessary..
                        if (!CA) {
                            if (!(CA = sess->AddEventList(CPAP_ClearAirway, EVL_Event))) { return false; }
                        }

                        CA->AddEvent(tt, duration);
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

        sess->updateLast(tt);
    }

    return true;
}

bool ResmedLoader::LoadBRP(Session *sess, const QString & path)
{
    EDFParser edf(path);
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

        } else {
            qDebug() << "Unobserved ResMed BRP Signal " << es.label;
            continue;
        }

        if (code) {
            double rate = double(duration) / double(recs);
            EventList *a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->setDimension(es.physical_dimension);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            sess->setMin(code, a->Min());
            sess->setMax(code, a->Max());
            sess->setPhysMin(code, es.physical_minimum);
            sess->setPhysMax(code, es.physical_maximum);
        }
    }

    return true;
}

// Convert EDFSignal data to sleepyheads Time-Delta Event format
void ResmedLoader::ToTimeDelta(Session *sess, EDFParser &edf, EDFSignal &es, ChannelID code,
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

    EventStoreType c, last;

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

#endif
}

// Load SAD Oximetry Signals
bool ResmedLoader::LoadSAD(Session *sess, const QString & path)
{
    EDFParser edf(path);
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
        } else {
            qDebug() << "Unobserved ResMed SAD Signal " << es.label;
        }
    }

    return true;
}


bool ResmedLoader::LoadPLD(Session *sess, const QString & path)
{
    EDFParser edf(path);
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
            sess->settings[CPAP_Mode] = MODE_BILEVEL_FIXED;
            es.physical_maximum = 25;
            es.physical_minimum = 4;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (matchSignal(CPAP_MinuteVent,es.label)) {
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
        } else if (matchSignal(CPAP_EPAP, es.label)) { // Expiratory Pressure
            code = CPAP_EPAP;
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
                ToTimeDelta(sess, edf, es, code, recs, duration);
            } else if (emptycnt == 1) {
                code = RMS9_E02;
                ToTimeDelta(sess, edf, es, code, recs, duration);
            } else {
                qDebug() << "Unobserved Empty Signal " << es.label;
            }

            emptycnt++;
        } else {
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

    resmed_codes[CPAP_FlowRate].push_back("Flow");
    resmed_codes[CPAP_MaskPressureHi].push_back("Mask Pres");
    resmed_codes[CPAP_MaskPressure].push_back("Mask Pres");
    resmed_codes[CPAP_RespEvent].push_back("Resp Event");
    resmed_codes[CPAP_Pressure].push_back("Therapy Pres");
    resmed_codes[CPAP_IPAP].push_back("Insp Pres");
    resmed_codes[CPAP_IPAP].push_back("IPAP");
    resmed_codes[CPAP_EPAP].push_back("Exp Pres");
    resmed_codes[CPAP_EPAP].push_back("EPAP");
    resmed_codes[CPAP_EPAPHi].push_back("Max EPAP");
    resmed_codes[CPAP_EPAPLo].push_back("Min EPAP");
    resmed_codes[CPAP_IPAPHi].push_back("Max IPAP");
    resmed_codes[CPAP_IPAPLo].push_back("Min IPAP");

    resmed_codes[CPAP_PS].push_back("PS");
    resmed_codes[CPAP_PSMin].push_back("Min PS");
    resmed_codes[CPAP_PSMax].push_back("Max PS");

    resmed_codes[CPAP_Leak].push_back("Leak");
    resmed_codes[CPAP_Leak].push_back("Leck");

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
    resmed_codes[CPAP_Snore].push_back("Snore");
    resmed_codes[CPAP_FLG].push_back("FFL Index");
    resmed_codes[CPAP_Ti].push_back("Ti");
    resmed_codes[CPAP_Te].push_back("Te");
    resmed_codes[CPAP_TgMV].push_back("TgMV");
    resmed_codes[OXI_Pulse].push_back("Pulse");
    resmed_codes[OXI_Pulse].push_back("Puls");
    resmed_codes[OXI_Pulse].push_back("Pols");
    resmed_codes[OXI_SPO2].push_back("SpO2");
    resmed_codes[CPAP_Obstructive].push_back("Obstructive apnea");
    resmed_codes[CPAP_Hypopnea].push_back("Hypopnea");
    resmed_codes[CPAP_Apnea].push_back("Apnea");
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

    // BRP file
    resmed_codes[CPAP_FlowRate].push_back("Flow.40ms");
    resmed_codes[CPAP_MaskPressureHi].push_back("Pres.40ms");

    // SAD file
    resmed_codes[OXI_Pulse].push_back("Pulse.1s");
    resmed_codes[OXI_SPO2].push_back("SpO2.1s");

    // PLD file
    resmed_codes[CPAP_MaskPressure].push_back("MaskPress.2s");
    resmed_codes[CPAP_Pressure].push_back("Press.2s");
    //resmed_codes[RMS9_EPRPressure].push_back("EPRPress.2s");
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

ChannelID ResmedLoader::PresReliefMode() { return RMS9_EPR; }
ChannelID ResmedLoader::PresReliefLevel() { return RMS9_EPRLevel; }

void ResmedLoader::initChannels()
{
    using namespace schema;
    Channel * chan = nullptr;
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

