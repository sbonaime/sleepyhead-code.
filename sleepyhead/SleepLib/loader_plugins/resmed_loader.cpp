/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib ResMed Loader Implementation
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
#include <QDebug>
#include <cmath>

#include "resmed_loader.h"
#include "SleepLib/session.h"
#include "SleepLib/calcs.h"

#ifdef DEBUG_EFFICIENCY
#include <QElapsedTimer>  // only available in 4.8
#endif

extern QProgressBar *qprogress;
QHash<int, QString> RMS9ModelMap;
QHash<ChannelID, QVector<QString> > resmed_codes;

const QString STR_ext_TGT = "tgt";
const QString STR_ext_CRC = "crc";
const QString STR_ext_EDF = "edf";
const QString STR_ext_gz = ".gz";


// Looks up foreign language Signal names that match this channelID
EDFSignal *EDFParser::lookupSignal(ChannelID ch)
{
    QHash<ChannelID, QVector<QString> >::iterator ci;
    QHash<QString, EDFSignal *>::iterator jj;
    ci = resmed_codes.find(ch);

    if (ci == resmed_codes.end()) {
        return NULL;
    }

    for (int i = 0; i < ci.value().size(); i++) {
        jj = lookup.find(ci.value()[i]);

        if (jj == lookup.end()) {
            continue;
        }

        return jj.value();
    }

    return NULL;
}

EDFSignal *EDFParser::lookupName(QString name)
{
    QHash<QString, EDFSignal *>::iterator i = lookup.find(name);

    if (i != lookup.end()) { return i.value(); }

    return NULL;
}

EDFParser::EDFParser(QString name)
{
    buffer = NULL;
    Open(name);
}
EDFParser::~EDFParser()
{
    QVector<EDFSignal>::iterator s;

    for (s = edfsignals.begin(); s != edfsignals.end(); s++) {
        if ((*s).data) { delete [](*s).data; }
    }

    if (buffer) { delete [] buffer; }
}
qint16 EDFParser::Read16()
{
    if ((pos + long(sizeof(qint16))) > filesize) {
        return 0;
    }

    qint16 res = *(qint16 *)&buffer[pos];
    pos += sizeof(qint16);
    return res;
}
QString EDFParser::Read(unsigned n)
{
    if ((pos + n) > filesize) {
        return "";
    }

    QString str;

    for (unsigned i = 0; i < n; i++) {
        str += buffer[pos++];
    }

    return str.trimmed();
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

    dur_data_record = QString::fromLatin1(header.dur_data_records, 8).toDouble(&ok) * 1000.0;

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
        EDFSignal &signal = edfsignals[i];
        signal.data = NULL;
        signal.label = Read(16);
        lookup[signal.label] = &signal; // Safe: edfsignals won't move.
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
            return false;
        }

        sig.data = new qint16 [recs];
        sig.pos = 0;
    }

    for (int x = 0; x < num_data_records; x++) {
        for (int i = 0; i < num_signals; i++) {
            EDFSignal &sig = edfsignals[i];
            memcpy((char *)&sig.data[sig.pos], (char *)&buffer[pos], sig.nr * 2);
            sig.pos += sig.nr;
            pos += sig.nr * 2;
            // big endian will probably screw up without this..
            /*for (int j=0;j<sig.nr;j++) {
                qint16 t=Read16();
                sig.data[sig.pos++]=t;
            } */
        }
    }

    return true;
}
bool EDFParser::Open(QString name)
{

    //Urk.. This needs fixing for VC++, as it doesn't have packed attribute type..

    if (name.endsWith(STR_ext_gz)) {
        filename = name.mid(0, -3);
        QFile fi(name);
        fi.open(QFile::ReadOnly);
        fi.seek(fi.size() - 4);
        unsigned char ch[4];
        fi.read((char *)ch, 4);
        filesize = ch[0] | (ch [1] << 8) | (ch[2] << 16) | (ch[3] << 24);
        datasize = filesize - EDFHeaderSize;

        if (datasize < 0) { return false; }

        qDebug() << "Size of" << name << "uncompressed=" << filesize;
        gzFile f = gzopen(name.toLatin1(), "rb");

        if (!f) {
            qDebug() << "EDFParser::Open() Couldn't open file" << name;
            return false;
        }

        gzread(f, (char *)&header, EDFHeaderSize);
        buffer = new char [datasize];
        //gzbuffer(f,65536*2);
        gzread(f, buffer, datasize);
        gzclose(f);
    } else {
        QFile f(name);

        if (!f.open(QIODevice::ReadOnly)) {
            return false;
        }

        filename = name;
        filesize = f.size();
        datasize = filesize - EDFHeaderSize;

        if (datasize < 0) { return false; }

        f.read((char *)&header, EDFHeaderSize);
        //qDebug() << "Opening " << name;
        buffer = new char [datasize];
        f.read(buffer, datasize);
        f.close();
    }

    pos = 0;
    return true;
}

ResmedLoader::ResmedLoader()
{
}
ResmedLoader::~ResmedLoader()
{
}

Machine *ResmedLoader::CreateMachine(QString serial, Profile *profile)
{
    if (!profile) { return NULL; }

    QList<Machine *> ml = profile->GetMachines(MT_CPAP);
    bool found = false;
    QList<Machine *>::iterator i;
    Machine *m = NULL;

    for (i = ml.begin(); i != ml.end(); i++) {
        if (((*i)->GetClass() == resmed_class_name) && ((*i)->properties[STR_PROP_Serial] == serial)) {
            ResmedList[serial] = *i; //static_cast<CPAP *>(*i);
            found = true;
            m = *i;
            break;
        }
    }

    if (!found) {
        m = new CPAP(profile, 0);
    }

    m->properties[STR_PROP_Brand] = STR_MACH_ResMed;
    m->properties[STR_PROP_Series] = "S9";

    if (found) {
        return m;
    }

    qDebug() << "Create ResMed Machine" << serial;
    m->SetClass(resmed_class_name);

    ResmedList[serial] = m;
    profile->AddMachine(m);

    m->properties[STR_PROP_Serial] = serial;
    m->properties[STR_PROP_DataVersion] = QString::number(resmed_data_version);

    QString path = "{" + STR_GEN_DataFolder + "}/" + m->GetClass() + "_" + serial + "/";
    m->properties[STR_PROP_Path] = path;
    m->properties[STR_PROP_BackupPath] = path + "Backup/";

    return m;
}

long event_cnt = 0;

const QString RMS9_STR_datalog = "DATALOG";
const QString RMS9_STR_idfile = "Identification.";
const QString RMS9_STR_strfile = "STR.";


int ResmedLoader::Open(QString &path, Profile *profile)
{

    QString serial;                 // Serial number
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
        return 0;
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Parse Identification.tgt file (containing serial number and machine information)
    ///////////////////////////////////////////////////////////////////////////////////
    filename = path + RMS9_STR_idfile + STR_ext_TGT;
    QFile f(filename);

    // Abort if this file is dodgy..
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        return 0;
    }

    // Parse # entries into idmap.
    while (!f.atEnd()) {
        line = f.readLine().trimmed();

        if (!line.isEmpty()) {
            key = line.section(" ", 0, 0);
            value = line.section(" ", 1);
            key = key.section("#", 1);

            if (key == "SRN") {
                key = STR_PROP_Serial;
                serial = value;
            }

            idmap[key] = value;
        }
    }

    f.close();

    // Abort if no serial number
    if (serial.isEmpty()) {
        qDebug() << "S9 Data card has no valid serial number in Indentification.tgt";
        return 0;
    }

    // Early check for STR.edf file, so we can early exit before creating faulty machine record.
    QString strpath = path + RMS9_STR_strfile + STR_ext_EDF; // STR.edf file
    f.setFileName(strpath);

    if (!f.exists()) { // No STR.edf.. Do we have a STR.edf.gz?
        strpath += STR_ext_gz;
        f.setFileName(strpath);

        if (!f.exists()) {
            qDebug() << "Missing STR.edf file";
            return 0;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Create machine object (unless it's already registered)
    ///////////////////////////////////////////////////////////////////////////////////
    Machine *m = CreateMachine(serial, profile);

    bool create_backups = PROFILE.session->backupCardData();
    bool compress_backups = PROFILE.session->compressBackupData();

    QString backup_path = PROFILE.Get(m->properties[STR_PROP_BackupPath]);

    if (backup_path.isEmpty()) {
        backup_path = PROFILE.Get(m->properties[STR_PROP_Path]) + "Backup/";
    }

    if (path == backup_path) {
        create_backups = false;
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Parse the idmap into machine objects properties, (overwriting any old values)
    ///////////////////////////////////////////////////////////////////////////////////
    for (QHash<QString, QString>::iterator i = idmap.begin(); i != idmap.end(); i++) {
        m->properties[i.key()] = i.value();

        if (i.key() == "PCD") { // Lookup Product Code for real model string
            bool ok;
            int j = i.value().toInt(&ok);

            if (ok) {
                m->properties[STR_PROP_Model] = RMS9ModelMap[j];
            }

            m->properties[STR_PROP_ModelNumber] = i.value();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Open and Parse STR.edf file
    ///////////////////////////////////////////////////////////////////////////////////
    EDFParser stredf(strpath);

    if (!stredf.Parse()) {
        qDebug() << "Faulty file" << RMS9_STR_strfile;
        return 0;
    }

    if (stredf.serialnumber != serial) {
        qDebug() << "Identification.tgt Serial number doesn't match STR.edf!";
    }


    // Creating early as we need the object
    QDir dir(newpath);


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

    //QDateTime dt1=QDateTime::fromTime_t(stredf.startdate/1000L);
    //QDateTime dt2=QDateTime::fromTime_t(stredf.enddate/1000L);
    //QDate dd1=dt1.date();
    //QDate dd2=dt2.date();

    //    for (int s=0;s<stredf.GetNumSignals();s++) {
    //        EDFSignal &es = stredf.edfsignals[s];
    //        long recs=es.nr*stredf.GetNumDataRecords();
    //        //qDebug() << "STREDF:" << es.label << recs;
    //    }

    // Process STR.edf and find first and last time for each day

    QVector<qint8> dayused;
    dayused.resize(days);
    QList<SessionID> strfirst;
    QList<SessionID> strlast;
    QList<int> strday;
    QList<bool> dayfoo;

    QHash<qint16, QList<time_t> > daystarttimes;
    QHash<qint16, QList<time_t> > dayendtimes;
    //qint16 on,off;
    //qint16 o1[10],o2[10];
    //time_t st,et;
    time_t time = stredf.startdate / 1000L; // == 12pm on first day
    //    for (int i=0;i<days;i++) {
    //        EDFSignal *maskon=stredf.lookup["Mask On"];
    //        EDFSignal *maskoff=stredf.lookup["Mask Off"];
    //        int j=i*10;

    //        // Counts for on and off don't line up, and I'm not sure why
    //        // The extra 'off' always seems to start with a 1 at the beginning
    //        // A signal it's carried over from the day before perhaps? (noon boundary)
    //        int ckon=0,ckoff=0;
    //        for (int k=0;k<10;k++) {
    //            on=maskon->data[j+k];
    //            off=maskoff->data[j+k];
    //            o1[k]=on;
    //            o2[k]=off;
    //            if (on >= 0) ckon++;
    //            if (off >= 0) ckoff++;
    //        }

    //        // set to true if day starts with machine running
    //        int offset=ckoff-ckon;
    //        dayfoo.push_back(offset>0);

    //        st=0,et=0;
    //        time_t l,f;

    //        // Find the Min & Max times for this day
    //        for (int k=0;k<ckon;k++) {
    //            on=o1[k];
    //            off=o2[k+offset];
    //            f=time+on*60;
    //            l=time+off*60;
    //            daystarttimes[i].push_back(f);
    //            dayendtimes[i].push_back(l);

    //            if (!st || (st > f)) st=f;
    //            if (!et || (et < l)) et=l;
    //        }
    //        strfirst.push_back(st);
    //        strlast.push_back(et);
    //        strday.push_back(i);
    //        dayused[i]=ckon;
    //        time+=86400;
    //    }

    // reset time to first day
    time = stredf.startdate / 1000;

    ///////////////////////////////////////////////////////////////////////////////////
    // Open DATALOG file and build list of session files
    ///////////////////////////////////////////////////////////////////////////////////

    QStringList dirs;
    dirs.push_back(newpath);
    dir.setFilter(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
    QFileInfoList flist = dir.entryInfoList();
    bool ok;

    for (int i = 0; i < flist.size(); i++) {
        QFileInfo fi = flist.at(i);
        filename = fi.fileName();

        if (filename.length() == 4) {
            filename.toInt(&ok);

            if (ok) {
                dirs.push_back(fi.canonicalFilePath());
            }
        }
    }

    QString datestr;
    SessionID sessionid;
    QDateTime date;
    QString fullname;
    bool gz;
    int size;
    QMap<SessionID, QStringList>::iterator si;

    sessfiles.clear();

    for (int dc = 0; dc < dirs.size(); dc++) {

        dir.setPath(dirs.at(dc));
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);
        flist = dir.entryInfoList();

        size = flist.size();


        // For each file in flist...
        for (int i = 0; i < size; i++) {
            QFileInfo fi = flist.at(i);
            filename = fi.fileName();

            // Forget about it if it can't be read.
            if (!fi.isReadable()) {
                continue;
            }

            if (filename.endsWith(STR_ext_gz)) {
                filename.chop(3);
                gz = true;
            } else { gz = false; }

            // Accept only .edf and .edf.gz files
            if (filename.right(4).toLower() != "." + STR_ext_EDF) {
                continue;
            }

            fullname = fi.canonicalFilePath();

            // Extract the session date out of the filename
            datestr = filename.section("_", 0, 1);

            // Take the filename's date, and
            date = QDateTime::fromString(datestr, "yyyyMMdd_HHmmss");
            date = date.toUTC();

            // Skip file if dates invalid, the filename is clearly wrong..
            if (!date.isValid()) {
                continue;
            }

            // convert this date to UNIX epoch to form the sessionID
            sessionid = date.toTime_t();

            ////////////////////////////////////////////////////////////////////////////////////////////
            // Resmed bugs up on the session filenames.. More than these 3 seconds

            // Moral of the story, when writing firmware and saving in batches, use the same datetimes,
            // and provide firmware updates for free to your customers.
            ////////////////////////////////////////////////////////////////////////////////////////////
            si = sessfiles.find(sessionid);

            if (si == sessfiles.end()) {
                // Scan 3 seconds either way for sessions..
                for (int j = 1; j < 3; j++) {

                    if ((si = sessfiles.find(sessionid + j)) != sessfiles.end()) {
                        sessionid += j;
                        break;
                    } else if ((si = sessfiles.find(sessionid - j)) != sessfiles.end()) {
                        sessionid -= j;
                        break;
                    }
                }
            }

            fullname = backup(fullname, backup_path);

            // Push current filename to ordered-by-sessionid list
            if (si != sessfiles.end()) {
                // Ignore if already compressed version of the same file exists.. (just in case)

                bool skip = false;

                // check for any other versions of the same file.
                for (int i = 0; i < si.value().size(); i++) {
                    QString st = si.value().at(i).section("/", -1);

                    if (st.endsWith(STR_ext_gz)) {
                        st.chop(3);
                    }

                    if (st == filename) {
                        skip = true;
                    }
                }

                if (!skip) {
                    si.value().push_back(fullname);
                }
            } else {
                sessfiles[sessionid].push_back(fullname);
            }

            //  if ((i % 10)==0) {
            // Update the progress bar
            if (qprogress) { qprogress->setValue((float(i + 1) / float(size) * 10.0)); }

            QApplication::processEvents();
            //  }
        }
    }

    QString fn;
    Session *sess;
    int cnt = 0;
    size = sessfiles.size();

    QHash<SessionID, int> sessday;


    /////////////////////////////////////////////////////////////////////////////
    // Scan over file list and knock out of dayused list
    /////////////////////////////////////////////////////////////////////////////
    //int dn;
    //    for (QMap<SessionID,QStringList>::iterator si=sessfiles.begin();si!=sessfiles.end();si++) {
    //        sessionid=si.key();

    //        // Earliest possible day number
    //        int edn=((sessionid-time)/86400)-1;
    //        if (edn<0) edn=0;

    //        // Find real day number from str.edf mask on/off data.
    //        dn=-1;
    //        for (int j=edn;j<strfirst.size();j++){
    //            st=strfirst.at(j);
    //            if (sessionid>=st) {
    //                et=strlast.at(j);
    //                if (sessionid<(et+300)) {
    //                    dn=j;
    //                    break;
    //                }
    //            }
    //        }
    //        // If found, mark day off so STR.edf summary data isn't used instead of the real thing.
    //        if (dn>=0) {
    //            dayused[dn]=0;
    //        }
    //    }

    EDFSignal *sig;

    /////////////////////////////////////////////////////////////////////////////
    // For all days not in session lists, (to get at days without data records)
    /////////////////////////////////////////////////////////////////////////////
    //    for (dn=0;dn<days;dn++) {
    //        // Skip days with loadable data.
    //        if (!dayused[dn])
    //            continue;

    //        if (!daystarttimes.contains(dn))
    //            continue;

    //        sess=NULL;

    //        int scnt=daystarttimes[dn].size(); // count of sessions for this day

    //        // Create a new session for each mask on/off segment in a day
    //        // But only populate the last one with summary data.
    //        for (int j=0;j<scnt;j++) {
    //            st=daystarttimes[dn].at(j);

    //            // Skip if session already exists
    //            if (m->SessionExists(st))
    //                continue;

    //            et=dayendtimes[dn].at(j);

    //            // Create the session
    //            sess=new Session(m,st);
    //            sess->really_set_first(qint64(st)*1000L);
    //            sess->really_set_last(qint64(et)*1000L);
    //            sess->SetChanged(true);
    //            m->AddSession(sess,profile);
    //        }
    //        // Add the actual data to the last session
    //        EventDataType tmp,dur;
    //        if (sess) {
    //            /////////////////////////////////////////////////////////////////////
    //            // CPAP Mode
    //            /////////////////////////////////////////////////////////////////////
    //            int mode;
    //            sig=stredf.lookupSignal(CPAP_Mode);
    //            if (sig) {
    //                mode=sig->data[dn];
    //            } else mode=0;


    //            /////////////////////////////////////////////////////////////////////
    //            // EPR Settings
    //            /////////////////////////////////////////////////////////////////////
    //            sess->settings[CPAP_PresReliefType]=PR_EPR;

    //            // Note: AutoSV machines don't have both fields
    //            sig=stredf.lookupSignal(RMS9_EPR);
    //            if (sig) {
    //                sess->settings[CPAP_PresReliefMode]=EventDataType(sig->data[dn])*sig->gain;
    //            }
    //            sig=stredf.lookupSignal(RMS9_EPRSet);
    //            if (sig)  {
    //                sess->settings[CPAP_PresReliefSet]=EventDataType(sig->data[dn])*sig->gain;
    //            }


    //            /////////////////////////////////////////////////////////////////////
    //            // Set Min & Max pressures depending on CPAP mode
    //            /////////////////////////////////////////////////////////////////////
    //            if (mode==0) {
    //                sess->settings[CPAP_Mode]=MODE_CPAP;
    //                sig=stredf.lookupSignal(RMS9_SetPressure); // ?? What's meant by Set Pressure?
    //                if (sig) {
    //                    EventDataType pressure=sig->data[dn]*sig->gain;
    //                    sess->settings[CPAP_Pressure]=pressure;
    //                }
    //            } else { // VPAP or Auto
    //                if (mode>5) {
    //                    if (mode>=7)
    //                        sess->settings[CPAP_Mode]=MODE_ASV;
    //                    else
    //                        sess->settings[CPAP_Mode]=MODE_BIPAP;

    //                    EventDataType tmp,epap=0,ipap=0;
    //                    if ((sig=stredf.lookupName("EPAP"))) {
    //                        epap=sig->data[dn]*sig->gain;
    //                        sess->settings[CPAP_EPAP]=epap;
    //                        sess->setMin(CPAP_EPAP,epap);
    //                    }
    //                    if ((sig=stredf.lookupName("IPAP"))) {
    //                        ipap=sig->data[dn]*sig->gain;
    //                        sess->settings[CPAP_IPAP]=ipap;
    //                    }
    //                    if ((sig=stredf.lookupName("PS"))) {
    //                        tmp=sig->data[dn]*sig->gain;
    //                        sess->settings[CPAP_PS]=tmp; // technically this is IPAP-EPAP
    //                        if (!ipap) {
    //                            // not really possible. but anyway, just in case..
    //                            sess->settings[CPAP_IPAP]=epap+tmp;
    //                        }
    //                    }
    //                    if ((sig=stredf.lookupName("Min PS"))) {
    //                        tmp=sig->data[dn]*sig->gain;
    //                        sess->settings[CPAP_PSMin]=tmp;
    //                        sess->settings[CPAP_IPAPLo]=epap+tmp;
    //                        sess->setMin(CPAP_IPAP,epap+tmp);
    //                    }
    //                    if ((sig=stredf.lookupName("Max PS"))) {
    //                        tmp=sig->data[dn]*sig->gain;
    //                        sess->settings[CPAP_PSMax]=tmp;
    //                        sess->settings[CPAP_IPAPHi]=epap+tmp;
    //                    }
    //                    if ((sig=stredf.lookupName("RR"))) { // Is this a setting to force respiratory rate on S/T machines?
    //                        tmp=sig->data[dn];
    //                        sess->settings[CPAP_RespRate]=tmp*sig->gain;
    //                    }

    //                    if ((sig=stredf.lookupName("Easy-Breathe"))) {
    //                        tmp=sig->data[dn]*sig->gain;

    //                        sess->settings[CPAP_PresReliefSet]=tmp;
    //                        sess->settings[CPAP_PresReliefType]=(int)PR_EASYBREATHE;
    //                        sess->settings[CPAP_PresReliefMode]=(int)PM_FullTime;
    //                    }

    //                } else {
    //                    sess->settings[CPAP_Mode]=MODE_APAP;
    //                    sig=stredf.lookupSignal(CPAP_PressureMin);
    //                    if (sig) {
    //                        EventDataType pressure=sig->data[dn]*sig->gain;
    //                        sess->settings[CPAP_PressureMin]=pressure;
    //                        //sess->setMin(CPAP_Pressure,pressure);
    //                    }
    //                    sig=stredf.lookupSignal(CPAP_PressureMax);
    //                    if (sig) {
    //                        EventDataType pressure=sig->data[dn]*sig->gain;
    //                        sess->settings[CPAP_PressureMax]=pressure;
    //                        //sess->setMax(CPAP_Pressure,pressure);
    //                    }
    //                }
    //            }

    //            EventDataType valmed=0,valmax=0,val95=0;

    //            /////////////////////////////////////////////////////////////////////
    //            // Leak Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("Leak Med"))) {
    //                valmed=sig->data[dn];
    //                if (valmed>=0) {
    //                    sess->m_gain[CPAP_Leak]=sig->gain*60.0;

    //                    sess->m_valuesummary[CPAP_Leak][valmed]=51;
    //                }
    //            }
    //            if ((sig=stredf.lookupName("Leak 95"))) {
    //                val95=sig->data[dn];
    //                if (val95>=0)
    //                    sess->m_valuesummary[CPAP_Leak][val95]=45;
    //            }
    //            if ((sig=stredf.lookupName("Leak Max"))) {
    //                valmax=sig->data[dn];
    //                if (valmax>=0) {
    //                    sess->setMax(CPAP_Leak,valmax*sig->gain*60.0);
    //                    sess->m_valuesummary[CPAP_Leak][valmax]=4;
    //                }
    //            }

    //            /////////////////////////////////////////////////////////////////////
    //            // Minute Ventilation Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("Min Vent Med"))) {
    //                valmed=sig->data[dn];
    //                sess->m_gain[CPAP_MinuteVent]=sig->gain;
    //                sess->m_valuesummary[CPAP_MinuteVent][valmed]=51;
    //            }
    //            if ((sig=stredf.lookupName("Min Vent 95"))) {
    //                val95=sig->data[dn];
    //                sess->m_valuesummary[CPAP_MinuteVent][val95]=45;
    //            }
    //            if ((sig=stredf.lookupName("Min Vent Max"))) {
    //                valmax=sig->data[dn];
    //                sess->setMax(CPAP_MinuteVent,valmax*sig->gain);
    //                sess->m_valuesummary[CPAP_MinuteVent][valmax]=4;
    //            }
    //            /////////////////////////////////////////////////////////////////////
    //            // Respiratory Rate Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("RR Med"))) {
    //                valmed=sig->data[dn];
    //                sess->m_gain[CPAP_RespRate]=sig->gain;
    //                sess->m_valuesummary[CPAP_RespRate][valmed]=51;
    //            }
    //            if ((sig=stredf.lookupName("RR 95"))) {
    //                val95=sig->data[dn];
    //                sess->m_valuesummary[CPAP_RespRate][val95]=45;
    //            }
    //            if ((sig=stredf.lookupName("RR Max"))) {
    //                valmax=sig->data[dn];
    //                sess->setMax(CPAP_RespRate,valmax*sig->gain);
    //                sess->m_valuesummary[CPAP_RespRate][valmax]=4;
    //            }

    //            /////////////////////////////////////////////////////////////////////
    //            // Tidal Volume Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("Tid Vol Med"))) {
    //                valmed=sig->data[dn];
    //                sess->m_gain[CPAP_TidalVolume]=sig->gain*1000.0;
    //                sess->m_valuesummary[CPAP_TidalVolume][valmed]=51;
    //            }
    //            if ((sig=stredf.lookupName("Tid Vol 95"))) {
    //                val95=sig->data[dn];
    //                sess->m_valuesummary[CPAP_TidalVolume][val95]=45;
    //            }
    //            if ((sig=stredf.lookupName("Tid Vol Max"))) {
    //                valmax=sig->data[dn];
    //                sess->setMax(CPAP_TidalVolume,valmax*sig->gain*1000.0);
    //                sess->m_valuesummary[CPAP_TidalVolume][valmax]=4;
    //            }

    //            /////////////////////////////////////////////////////////////////////
    //            // Target Minute Ventilation Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("Targ Vent Med"))) {
    //                valmed=sig->data[dn];
    //                sess->m_gain[CPAP_TgMV]=sig->gain;
    //                sess->m_valuesummary[CPAP_TgMV][valmed]=51;
    //            }
    //            if ((sig=stredf.lookupName("Targ Vent 95"))) {
    //                val95=sig->data[dn];
    //                sess->m_valuesummary[CPAP_TgMV][val95]=45;
    //            }
    //            if ((sig=stredf.lookupName("Targ Vent Max"))) {
    //                valmax=sig->data[dn];
    //                sess->setMax(CPAP_TgMV,valmax*sig->gain);
    //                sess->m_valuesummary[CPAP_TgMV][valmax]=4;
    //            }


    //            /////////////////////////////////////////////////////////////////////
    //            // I:E Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("I:E Med"))) {
    //                valmed=sig->data[dn];
    //                sess->m_gain[CPAP_IE]=sig->gain;
    //                sess->m_valuesummary[CPAP_IE][valmed]=51;
    //            }
    //            if ((sig=stredf.lookupName("I:E 95"))) {
    //                val95=sig->data[dn];
    //                sess->m_valuesummary[CPAP_IE][val95]=45;
    //            }
    //            if ((sig=stredf.lookupName("I:E Max"))) {
    //                valmax=sig->data[dn];
    //                sess->setMax(CPAP_IE,valmax*sig->gain);
    //                sess->m_valuesummary[CPAP_IE][valmax]=4;
    //            }

    //            /////////////////////////////////////////////////////////////////////
    //            // Mask Pressure Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("Mask Pres Med"))) {
    //                valmed=sig->data[dn];
    //                if (valmed >= 0) {
    //                    sess->m_gain[CPAP_Pressure]=sig->gain;
    //                    sess->m_valuesummary[CPAP_Pressure][valmed]=51;
    //                }
    //            }
    //            if ((sig=stredf.lookupName("Mask Pres 95"))) {
    //                val95=sig->data[dn];
    //                if (val95 >= 0) {
    //                    sess->m_valuesummary[CPAP_Pressure][val95]=45;
    //                }
    //            }
    //            if ((sig=stredf.lookupName("Mask Pres Max"))) {
    //                valmax=sig->data[dn];
    //                if (valmax >= 0) {
    //                    sess->setMax(CPAP_Pressure,valmax*sig->gain);
    //                    sess->m_valuesummary[CPAP_Pressure][valmax]=4;
    //                }
    //            }
    //            /////////////////////////////////////////////////////////////////////
    //            // Therapy Pressure Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("Therapy Pres Me"))) {
    //                valmed=sig->data[dn];
    //                if (valmed >= 0) {
    //                    //sess->m_gain[CPAP_Pressure]=sig->gain;
    //                    //sess->m_valuesummary[CPAP_Pressure][valmed]=51;
    //                }
    //            }
    //            if ((sig=stredf.lookupName("Therapy Pres 95"))) {
    //                val95=sig->data[dn];
    //                if (val95 >= 0) {
    ////                    sess->m_valuesummary[CPAP_Pressure][val95]=45;
    //                }
    //            }
    //            if ((sig=stredf.lookupName("Therapy Pres Ma"))) {
    //                valmax=sig->data[dn];
    //                if (valmax >= 0) {
    ////                    sess->setMax(CPAP_Pressure,valmax*sig->gain);
    ////                    sess->m_valuesummary[CPAP_Pressure][valmax]=4;
    //                }
    //            }

    //            /////////////////////////////////////////////////////////////////////
    //            // Inspiratory Pressure (IPAP) Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("Insp Pres Med"))) {
    //                valmed=sig->data[dn];
    //                sess->m_gain[CPAP_IPAP]=sig->gain;
    //                sess->m_valuesummary[CPAP_IPAP][valmed]=51;
    //            }
    //            if ((sig=stredf.lookupName("Insp Pres 95"))) {
    //                val95=sig->data[dn];
    //                sess->m_valuesummary[CPAP_IPAP][val95]=45;
    //            }
    //            if ((sig=stredf.lookupName("Insp Pres Max"))) {
    //                valmax=sig->data[dn];
    //                sess->setMax(CPAP_IPAP,valmax*sig->gain);
    //                sess->m_valuesummary[CPAP_IPAP][valmax]=4;
    //            }
    //            /////////////////////////////////////////////////////////////////////
    //            // Expiratory Pressure (EPAP) Summary
    //            /////////////////////////////////////////////////////////////////////
    //            if ((sig=stredf.lookupName("Exp Pres Med"))) {
    //                valmed=sig->data[dn];
    //                if (valmed>=0) {
    //                    sess->m_gain[CPAP_EPAP]=sig->gain;
    //                    sess->m_valuesummary[CPAP_EPAP][valmed]=51;
    //                }
    //            }
    //            if ((sig=stredf.lookupName("Exp Pres 95"))) {
    //                if (val95>=0) {
    //                    val95=sig->data[dn];
    //                    sess->m_valuesummary[CPAP_EPAP][val95]=45;
    //                }
    //            }
    //            if ((sig=stredf.lookupName("Exp Pres Max"))) {
    //                valmax=sig->data[dn];
    //                if (valmax>=0) {
    //                    sess->setMax(CPAP_EPAP,valmax*sig->gain);
    //                    sess->m_valuesummary[CPAP_EPAP][valmax]=4;
    //                }
    //            }

    //            /////////////////////////////////////////////////////////////////////
    //            // Duration and Event Indices
    //            /////////////////////////////////////////////////////////////////////
    //            dur=0;
    //            if ((sig=stredf.lookupName("Mask Dur"))) {
    //                dur=sig->data[dn]*sig->gain;
    //                dur/=60.0f; // convert to hours.
    //            }
    //            if ((sig=stredf.lookupName("OAI"))) { // Obstructive Apnea Index
    //                tmp=sig->data[dn]*sig->gain;
    //                if (tmp>=0) {
    //                    sess->setCph(CPAP_Obstructive,tmp);
    //                    sess->setCount(CPAP_Obstructive,tmp*dur); // Converting from indice to counts..
    //                }
    //            }
    //            if ((sig=stredf.lookupName("HI"))) { // Hypopnea Index
    //                tmp=sig->data[dn]*sig->gain;
    //                if (tmp>=0) {
    //                    sess->setCph(CPAP_Hypopnea,tmp);
    //                    sess->setCount(CPAP_Hypopnea,tmp*dur);
    //                }
    //            }
    //            if ((sig=stredf.lookupName("UAI"))) { // Unspecified Apnea Index
    //                tmp=sig->data[dn]*sig->gain;
    //                if (tmp>=0) {
    //                    sess->setCph(CPAP_Apnea,tmp);
    //                    sess->setCount(CPAP_Apnea,tmp*dur);
    //                }
    //            }
    //            if ((sig=stredf.lookupName("CAI"))) { // "Central" Apnea Index
    //                tmp=sig->data[dn]*sig->gain;
    //                if (tmp>=0) {
    //                    sess->setCph(CPAP_ClearAirway,tmp);
    //                    sess->setCount(CPAP_ClearAirway,tmp*dur);
    //                }
    //            }

    //        }

    //    }
    backup_path += RMS9_STR_datalog + "/";

    QString backupfile, backfile, crcfile, yearstr, bkuppath;

    /////////////////////////////////////////////////////////////////////////////
    // Scan through new file list and import sessions
    /////////////////////////////////////////////////////////////////////////////
    for (QMap<SessionID, QStringList>::iterator si = sessfiles.begin(); si != sessfiles.end(); si++) {
        sessionid = si.key();

        // Skip file if already imported
        if (m->SessionExists(sessionid)) {
            continue;
        }

        // Create the session
        sess = new Session(m, sessionid);

        QString oldbkfile;

        // Process EDF File List
        for (int i = 0; i < si.value().size(); ++i) {
            fullname = si.value()[i];
            filename = fullname.section("/", -1);
            gz = (filename.right(3).toLower() == STR_ext_gz);


            //            yearstr=filename.left(4);
            //            bkuppath=backup_path;
            //            int year=yearstr.toInt(&ok,10);
            //            if (ok) {
            //                bkuppath+=yearstr+"/";
            //                dir.mkpath(bkuppath);
            //            }

            //            // Copy the EDF file to the backup folder
            //            if (create_backups) {
            //                oldbkfile=backup_path+filename;
            //                backupfile=bkuppath+filename;

            //                bool dobackup=true;

            //                if (QFile::exists(oldbkfile+STR_ext_gz))
            //                    QFile::remove(oldbkfile+STR_ext_gz);
            //                if (QFile::exists(oldbkfile))
            //                    QFile::remove(oldbkfile);

            //                if (!gz && QFile::exists(backupfile+STR_ext_gz)) {
            //                    dobackup=false; // gzipped edf.. assume it's already a backup
            //                } else if (QFile::exists(backupfile)) {
            //                    if (gz) {
            //                        // don't bother, it's already there and compressed.
            //                        dobackup=false;
            //                    } else {
            //                        // non compressed file is there..
            //                        if (compress_backups) {
            //                            // remove old edf file, as we are writing a compressed one
            //                            QFile::remove(backupfile);
            //                        } else { // don't bother copying it.
            //                            dobackup=false;
            //                        }
            //                    }
            //                }
            //                if (dobackup) {
            //                    if (!gz) {
            //                        compress_backups ?
            //                            compressFile(fullname, backupfile)
            //                        :
            //                            QFile::copy(fullname, backupfile);
            //                    } else {
            //                        // already compressed, just copy it.
            //                        QFile::copy(fullname, backupfile);
            //                    }
            //                }

            //                if (!gz) {
            //                    backfile=filename.replace(".edf",".crc",Qt::CaseInsensitive);
            //                } else {
            //                    backfile=filename.replace(".edf.gz",".crc",Qt::CaseInsensitive);
            //                }

            //                backupfile=bkuppath+backfile;
            //                crcfile=newpath+backfile;
            //                QFile::copy(crcfile, backupfile);
            //            }

            EDFParser edf(fullname);

            // Parse the actual file
            if (!edf.Parse()) {
                continue;
            }

            // Give a warning if doesn't match the machine serial number in Identification.tgt
            if (edf.serialnumber != serial) {
                qDebug() << "edf Serial number doesn't match Identification.tgt";
            }

            fn = filename.section("_", -1).section(".", 0, 0).toLower();

            if (fn == "eve") { LoadEVE(sess, edf); }
            else if (fn == "pld") { LoadPLD(sess, edf); }
            else if (fn == "brp") { LoadBRP(sess, edf); }
            else if (fn == "sad") { LoadSAD(sess, edf); }
        }

        if ((++cnt % 10) == 0) {
            if (qprogress) { qprogress->setValue(10.0 + (float(cnt) / float(size) * 90.0)); }

            QApplication::processEvents();
        }

        int mode = 0;
        EventDataType prset = 0, prmode = 0;
        qint64 dif;
        int dn;

        if (!sess) { continue; }

        if (!sess->first()) {
            delete sess;
            continue;
        } else {

            sess->SetChanged(true);

            dif = sess->first() - stredf.startdate;

            dn = dif / 86400000L;

            if ((dn >= 0) && (dn < days)) {
                sig = stredf.lookupSignal(CPAP_Mode);

                if (sig) {
                    mode = sig->data[dn];
                } else { mode = 0; }




                // Ramp, Fulltime
                // AutoSV machines don't have both fields
                sig = stredf.lookupSignal(RMS9_EPR);

                if (sig) {
                    sess->settings[CPAP_PresReliefType] = PR_EPR;
                    prmode = EventDataType(sig->data[dn]) * sig->gain;

                    // Off,
                    if (prmode < 0) {
                        // Kaart's data has -1 here.. Not sure what it means.
                        prmode = 0;
                    } else if (prmode > sig->physical_maximum) {
                        prmode = sig->physical_maximum;
                    }

                    // My VPAP (using EasyBreathe) and JM's Elite (using none) have 0
                    sess->settings[CPAP_PresReliefMode] = prmode;
                }

                sig = stredf.lookupSignal(RMS9_EPRSet);

                if (sig)  {
                    prset = EventDataType(sig->data[dn]) * sig->gain;

                    if (prset > sig->physical_maximum) {
                        prset = sig->physical_maximum;
                    } else if (prset < sig->physical_minimum) {
                        prset = sig->physical_minimum;
                    }

                    sess->settings[CPAP_PresReliefSet] = prset;
                }


                if (mode == 0) {
                    sess->settings[CPAP_Mode] = MODE_CPAP;
                    sig = stredf.lookupSignal(RMS9_SetPressure); // ?? What's meant by Set Pressure?

                    if (sig) {
                        EventDataType pressure = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_Pressure] = pressure;
                    }
                } else if (mode > 5) {
                    if (mode >= 7) {
                        sess->settings[CPAP_Mode] =
                            MODE_ASV;    // interestingly, last digit of model number matches these when in full mode.
                    } else {
                        sess->settings[CPAP_Mode] = MODE_BIPAP;
                    }

                    EventDataType tmp, epap = 0, ipap = 0;


                    // All S9 machines have Set Pressure
                    // Elite has Min Pressure and Max Pressure
                    // VPAP Auto has EPAP, Min EPAP, IPAP and Max IPAP, and PS
                    // VPAP Adapt 36007 has just EPAP and PSLo/Hi,
                    // VPAP Adapt 36037 has EPAPLo, EPAPHi and PSLo/Hi


                    if (stredf.lookup.contains("EPAP")) {
                        sig = stredf.lookup["EPAP"];
                        epap = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_EPAP] = epap;
                    }

                    if (stredf.lookup.contains("IPAP")) {
                        sig = stredf.lookup["IPAP"];
                        ipap = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_IPAP] = ipap;
                    }

                    if (stredf.lookup.contains("Min EPAP")) {
                        sig = stredf.lookup["Min EPAP"];
                        epap = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_EPAPLo] = epap;
                    }

                    if (stredf.lookup.contains("Max EPAP")) {
                        sig = stredf.lookup["Max EPAP"];
                        epap = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_EPAPHi] = epap;
                    }

                    if (stredf.lookup.contains("Min IPAP")) {
                        sig = stredf.lookup["Min IPAP"];
                        ipap = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_IPAPLo] = ipap;
                    }

                    if (stredf.lookup.contains("Max IPAP")) {
                        sig = stredf.lookup["Max IPAP"];
                        ipap = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_IPAPHi] = ipap;
                    }

                    if (stredf.lookup.contains("PS")) {
                        sig = stredf.lookup["PS"];
                        tmp = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_PS] = tmp; // plain VPAP Pressure support
                    }

                    if (stredf.lookup.contains("Min PS")) {
                        sig = stredf.lookup["Min PS"];
                        tmp = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_PSMin] = tmp;
                    }

                    if (stredf.lookup.contains("Max PS")) {
                        sig = stredf.lookup["Max PS"];
                        tmp = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_PSMax] = tmp;
                    }

                    if (stredf.lookup.contains("RR")) { // Is this a setting to force respiratory rate on S/T machines?
                        sig = stredf.lookup["RR"];
                        tmp = sig->data[dn];
                        sess->settings[CPAP_RespRate] = tmp * sig->gain;
                    }

                    // this is not a setting on any machine I've played with, I think it's just an indication of the type of motor
                    if (stredf.lookup.contains("Easy-Breathe")) {
                        sig = stredf.lookup["Easy-Breathe"];
                        tmp = sig->data[dn] * sig->gain;

                        sess->settings[CPAP_PresReliefSet] = tmp;
                        sess->settings[CPAP_PresReliefType] = (int)PR_EASYBREATHE;
                        sess->settings[CPAP_PresReliefMode] = (int)PM_FullTime;
                    }

                } else {
                    sess->settings[CPAP_Mode] = MODE_APAP;
                    sig = stredf.lookupSignal(CPAP_PressureMin);

                    if (sig) {
                        EventDataType pressure = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_PressureMin] = pressure;
                        //sess->setMin(CPAP_Pressure,pressure);
                    }

                    sig = stredf.lookupSignal(CPAP_PressureMax);

                    if (sig) {
                        EventDataType pressure = sig->data[dn] * sig->gain;
                        sess->settings[CPAP_PressureMax] = pressure;
                        //sess->setMax(CPAP_Pressure,pressure);
                    }

                }
            }


            // The following only happens when the STR.edf file is not up to date..
            // This will only happen when the user fails to back up their SDcard properly.
            // Basically takes a guess.
            //            bool dodgy=false;
            //            if (!sess->settings.contains(CPAP_Mode)) {
            //                //The following is a lame assumption if 50th percentile == max, then it's CPAP
            //                EventDataType max=sess->Max(CPAP_Pressure);
            //                EventDataType p50=sess->percentile(CPAP_Pressure,0.60);
            //                EventDataType p502=sess->percentile(CPAP_MaskPressure,0.60);
            //                p50=qMax(p50, p502);
            //                if (max==0) {
            //                    dodgy=true;
            //                } else if (qAbs(max-p50)<1.8) {
            //                    max=round(max*10.0)/10.0;
            //                    sess->settings[CPAP_Mode]=MODE_CPAP;
            //                    if (max<1) {
            //                        int i=5;
            //                    }
            //                    sess->settings[CPAP_PressureMin]=max;
            //                    EventDataType epr=round(sess->Max(CPAP_EPAP)*10.0)/10.0;
            //                    int i=max-epr;
            //                    sess->settings[CPAP_PresReliefType]=PR_EPR;
            //                    prmode=(i>0) ? 0 : 1;
            //                    sess->settings[CPAP_PresReliefMode]=prmode;
            //                    sess->settings[CPAP_PresReliefSet]=i;


            //                } else {
            //                    // It's not cpap, so just take the highest setting for this machines history.
            //                    // This may fail if the missing str data is at the beginning of a fresh import.
            //                    CPAPMode mode=(CPAPMode)(int)PROFILE.calcSettingsMax(CPAP_Mode,MT_CPAP,sess->machine()->FirstDay(),sess->machine()->LastDay());
            //                    if (mode<MODE_APAP) mode=MODE_APAP;
            //                    sess->settings[CPAP_Mode]=mode;
            //                    // Assuming 10th percentile should cover for ramp/warmup
            //                    sess->settings[CPAP_PressureMin]=sess->percentile(CPAP_Pressure,0.10);
            //                    sess->settings[CPAP_PressureMax]=sess->Max(CPAP_Pressure);
            //                }
            //            }
            //Rather than take a dodgy guess, EPR settings can take a hit, and this data can simply be missed..

            // Add the session to the machine & profile objects
            //if (!dodgy)
            m->AddSession(sess, profile);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////
    // Process STR.edf now all valid Session data is imported
    /////////////////////////////////////////////////////////////////////////////////
    /*
        qint64 tt=stredf.startdate;
        QDateTime dt=QDateTime::fromMSecsSinceEpoch(tt);
        QDateTime mt;
        QDate d;

        EDFSignal *maskon=stredf.lookup["Mask On"];
        EDFSignal *maskoff=stredf.lookup["Mask Off"];
        int nr1=maskon->nr;
        int nr2=maskoff->nr;

        qint64 mon, moff;

        int mode;
        EventDataType prset, prmode;

        SessionID sid;
        for (int dn=0; dn < days; dn++, tt+=86400000L) {
            dt=QDateTime::fromMSecsSinceEpoch(tt);
            d=dt.date();
            Day * day=PROFILE.GetDay(d, MT_CPAP);
            if (day) {
                continue;
            }
            QString a;
            // Todo: check session start times.
            // mask time is in minutes per day, assuming starting from 12 noon
            // Also to think about: users who are too lazy to set their clocks, or who have flat clock batteries.

            int nr=maskon->nr;
            int j=dn * nr;
            qint16 m_on=-1, m_off=-1, m_off2=0;
            for (int i=0;i<10;i++) {
                m_on=maskon->data[j+i];
                if ((i>0) && (m_on >=0) && (m_on < m_off)) {
                    qDebug() << "Mask on before previous off";
                }
                m_off=maskoff->data[j+i];
                m_off2=m_off;


                if ((m_on >= 0) && (m_off < 0)) {
                    // valid session.. but machine turned off the next day
                    // look ahead and pinch the end time from tomorrows record
                    if ((dn+1) > days) {
                        qDebug() << "Last record should have contained a mask off event :(";
                        continue;
                    }
                    m_off=maskoff->data[j + nr];

                    if (maskon->data[j + nr] < 0) {
                        qDebug() << dn << "Looking ahead maskon should be < 0";
                        continue;
                    }
                    if (m_off < 0) {
                        qDebug() << dn << "Looking ahead maskoff should be > 0";
                        continue;
                    }

                    // It's in the next day, so add one day in minutes..
                    m_off+=1440;

                    // Valid

                } else if ((m_off >= 0) && (m_on < 0)) {

                    if (i>0) {
                        qDebug() << "WTH!??? Mask off but no on";
                    }
                    // first record of day.. might already be on (crossing noon)
                    // Safely ignore because they are picked up on the other day.
                    continue;
                } else if ((m_off < 0) && (m_on < 0))
                    continue;

                mon=tt + m_on * 60000L;
                moff=tt + m_off * 60000L;

                sid=mon/1000L;
                QDateTime on=QDateTime::fromMSecsSinceEpoch(mon);
                QDateTime off=QDateTime::fromMSecsSinceEpoch(moff);

                sess=new Session(m,sid);
                sess->set_first(mon);
                sess->set_last(moff);

                sig=stredf.lookupSignal(CPAP_Mode);
                if (sig) {
                    mode=sig->data[dn];
                } else mode=0;

                sess->settings[CPAP_PresReliefType]=PR_EPR;

                // AutoSV machines don't have both fields
                sig=stredf.lookupSignal(RMS9_EPR);
                if (sig) {
                    prmode=EventDataType(sig->data[dn])*sig->gain;
                    if (prmode>sig->physical_maximum) {
                        int i=5;
                    }
                    sess->settings[CPAP_PresReliefMode]=prmode;
                }

                sig=stredf.lookupSignal(RMS9_EPRSet);
                if (sig)  {
                    prset=EventDataType(sig->data[dn])*sig->gain;
                    if (prset>sig->physical_maximum) {
                        int i=5;
                    }
                    sess->settings[CPAP_PresReliefSet]=prset;
                }


                if (mode==0) {
                    sess->settings[CPAP_Mode]=MODE_CPAP;
                    sig=stredf.lookupSignal(RMS9_SetPressure); // ?? What's meant by Set Pressure?
                    if (sig) {
                        EventDataType pressure=sig->data[dn]*sig->gain;
                        sess->settings[CPAP_Pressure]=pressure;
                    }
                } else if (mode>5) {
                    if (mode>=7)
                        sess->settings[CPAP_Mode]=MODE_ASV;
                    else
                        sess->settings[CPAP_Mode]=MODE_BIPAP;

                    EventDataType tmp,epap=0,ipap=0;
                    if (stredf.lookup.contains("EPAP")) {
                        sig=stredf.lookup["EPAP"];
                        epap=sig->data[dn]*sig->gain;
                        sess->settings[CPAP_EPAP]=epap;
                    }
                    if (stredf.lookup.contains("IPAP")) {
                        sig=stredf.lookup["IPAP"];
                        ipap=sig->data[dn]*sig->gain;
                        sess->settings[CPAP_IPAP]=ipap;
                    }
                    if (stredf.lookup.contains("PS")) {
                        sig=stredf.lookup["PS"];
                        tmp=sig->data[dn]*sig->gain;
                        sess->settings[CPAP_PS]=tmp; // technically this is IPAP-EPAP
                        if (!ipap) {
                            // not really possible. but anyway, just in case..
                            sess->settings[CPAP_IPAP]=epap+tmp;
                        }
                    }
                    if (stredf.lookup.contains("Min PS")) {
                        sig=stredf.lookup["Min PS"];
                        tmp=sig->data[dn]*sig->gain;
                        sess->settings[CPAP_PSMin]=tmp;
                        sess->settings[CPAP_IPAPLo]=epap+tmp;
                        sess->setMin(CPAP_IPAP,epap+tmp);
                    }
                    if (stredf.lookup.contains("Max PS")) {
                        sig=stredf.lookup["Max PS"];
                        tmp=sig->data[dn]*sig->gain;
                        sess->settings[CPAP_PSMax]=tmp;
                        sess->settings[CPAP_IPAPHi]=epap+tmp;
                    }
                    if (stredf.lookup.contains("RR")) { // Is this a setting to force respiratory rate on S/T machines?
                        sig=stredf.lookup["RR"];
                        tmp=sig->data[dn];
                        sess->settings[CPAP_RespRate]=tmp*sig->gain;
                    }

                    if (stredf.lookup.contains("Easy-Breathe")) {
                        sig=stredf.lookup["Easy-Breathe"];
                        tmp=sig->data[dn]*sig->gain;

                        sess->settings[CPAP_PresReliefSet]=tmp;
                        sess->settings[CPAP_PresReliefType]=(int)PR_EASYBREATHE;
                        sess->settings[CPAP_PresReliefMode]=(int)PM_FullTime;
                    }

                } else {
                    sess->settings[CPAP_Mode]=MODE_APAP;
                    sig=stredf.lookupSignal(CPAP_PressureMin);
                    if (sig) {
                        EventDataType pressure=sig->data[dn]*sig->gain;
                        sess->settings[CPAP_PressureMin]=pressure;
                        //sess->setMin(CPAP_Pressure,pressure);
                    }
                    sig=stredf.lookupSignal(CPAP_PressureMax);
                    if (sig) {
                        EventDataType pressure=sig->data[dn]*sig->gain;
                        sess->settings[CPAP_PressureMax]=pressure;
                        //sess->setMax(CPAP_Pressure,pressure);
                    }

                }

                m->AddSession(sess,profile);

                a=QString("[%3] %1:%2, ").arg(on.toString()).arg(off.toString()).arg(sid);
                qDebug() << a.toStdString().data();
            }


        }
    */

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

    if (m) {
        m->Save();
    }

    if (qprogress) { qprogress->setValue(100); }

    qDebug() << "Total Events " << event_cnt;
    return 1;
}

QString ResmedLoader::backup(QString fullname, QString backup_path, bool compress)
{
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


bool ResmedLoader::LoadEVE(Session *sess, EDFParser &edf)
{
    // EVEnt records have useless duration record.

    QString t;
    long recs;
    double duration;
    char *data;
    char c;
    long pos;
    bool sign, ok;
    double d;
    double tt;
    //ChannelID code;
    //Event *e;
    //totaldur=edf.GetNumDataRecords()*edf.GetDuration();

    //    EventList *EL[4]={NULL};
    sess->updateFirst(edf.startdate);
    //if (edf.enddate>edf.startdate) sess->set_last(edf.enddate);

    EventList *OA = NULL, *HY = NULL, *CA = NULL, *UA = NULL;

    // Allow for empty sessions..
    OA = sess->AddEventList(CPAP_Obstructive, EVL_Event);
    HY = sess->AddEventList(CPAP_Hypopnea, EVL_Event);
    UA = sess->AddEventList(CPAP_Apnea, EVL_Event);

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
                    if (t == "obstructive apnea") {
                        OA->AddEvent(tt, duration);
                    } else if (t == "hypopnea") {
                        HY->AddEvent(tt, duration + 10); // Only Hyponea's Need the extra duration???
                    } else if (t == "apnea") {
                        UA->AddEvent(tt, duration);
                    } else if (t == "central apnea") {
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

            while ((data[pos] == 0) && pos < recs) { pos++; }

            if (pos >= recs) { break; }
        }

        sess->updateLast(tt);
        // qDebug(data);
    }

    return true;
}
bool ResmedLoader::LoadBRP(Session *sess, EDFParser &edf)
{
    QString t;
    sess->updateFirst(edf.startdate);
    qint64 duration = edf.GetNumDataRecords() * edf.GetDuration();
    sess->updateLast(edf.startdate + duration);

    for (int s = 0; s < edf.GetNumSignals(); s++) {
        EDFSignal &es = edf.edfsignals[s];
        //qDebug() << "BRP:" << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum;
        long recs = es.nr * edf.GetNumDataRecords();
        ChannelID code;

        if (es.label == "Flow") {
            es.gain *= 60.0;
            es.physical_minimum *= 60.0;
            es.physical_maximum *= 60.0;
            es.physical_dimension = "L/M";
            code = CPAP_FlowRate;
        } else if (es.label.startsWith("Mask Pres")) {
            code = CPAP_MaskPressureHi;
        } else if (es.label.startsWith("Resp Event")) {
            code = CPAP_RespEvent;
        } else {
            qDebug() << "Unobserved ResMed BRP Signal " << es.label;
            continue;
        }

        double rate = double(duration) / double(recs);
        EventList *a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
        a->setDimension(es.physical_dimension);
        a->AddWaveform(edf.startdate, es.data, recs, duration);
        sess->setMin(code, a->Min());
        sess->setMax(code, a->Max());
        sess->setPhysMin(code, es.physical_minimum);
        sess->setPhysMax(code, es.physical_maximum);
    }

    return true;
}
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
    //sess->UpdateFirst(tt);
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

    EventList *el = NULL;

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

        for (; sptr < eptr; sptr++) { //int i=startpos;i<recs;i++) {
            c = *sptr; //es.data[i];

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

    //return el;
}
bool ResmedLoader::LoadSAD(Session *sess, EDFParser &edf)
{
    QString t;
    sess->updateFirst(edf.startdate);
    qint64 duration = edf.GetNumDataRecords() * edf.GetDuration();
    sess->updateLast(edf.startdate + duration);

    for (int s = 0; s < edf.GetNumSignals(); s++) {
        EDFSignal &es = edf.edfsignals[s];
        //qDebug() << "SAD:" << es.label << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum;
        long recs = es.nr * edf.GetNumDataRecords();
        ChannelID code;

        if (es.label.startsWith("Puls")) {
            code = OXI_Pulse;
        } else if (es.label == "SpO2") {
            code = OXI_SPO2;
        } else {
            qDebug() << "Unobserved ResMed SAD Signal " << es.label;
            continue;
        }

        bool hasdata = false;

        for (int i = 0; i < recs; i++) {
            if (es.data[i] != -1) {
                hasdata = true;
                break;
            }
        }

        if (hasdata) {
            if (code == OXI_Pulse) {
                ToTimeDelta(sess, edf, es, code, recs, duration);
                sess->setPhysMax(code, 180);
                sess->setPhysMin(code, 18);
            } else if (code == OXI_SPO2) {
                es.physical_minimum = 60;
                ToTimeDelta(sess, edf, es, code, recs, duration);
                sess->setPhysMax(code, 100);
                sess->setPhysMin(code, 60);
            }
        }

    }

    return true;
}


bool ResmedLoader::LoadPLD(Session *sess, EDFParser &edf)
{
    // Is it save to assume the order does not change here?
    enum PLDType { MaskPres = 0, TherapyPres, ExpPress, Leak, RR, Vt, Mv, SnoreIndex, FFLIndex, U1, U2 };

    qint64 duration = edf.GetNumDataRecords() * edf.GetDuration();
    sess->updateFirst(edf.startdate);
    sess->updateLast(edf.startdate + duration);
    QString t;
    int emptycnt = 0;
    EventList *a = NULL;
    double rate;
    long recs;
    ChannelID code;

    for (int s = 0; s < edf.GetNumSignals(); s++) {
        a = NULL;
        EDFSignal &es = edf.edfsignals[s];
        recs = es.nr * edf.GetNumDataRecords();

        if (recs <= 0) { continue; }

        rate = double(duration) / double(recs);

        //qDebug() << "EVE:" << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum << es.gain;
        if (es.label == "Snore Index") {
            code = CPAP_Snore;
            ToTimeDelta(sess, edf, es, code, recs, duration, es.digital_maximum);
        } else if (es.label.startsWith("Therapy Pres")) {
            code = CPAP_Pressure; //TherapyPressure;
            es.physical_maximum = 25;
            es.physical_minimum = 4;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (es.label == "Insp Pressure") {
            code = CPAP_IPAP;
            sess->settings[CPAP_Mode] = MODE_BIPAP;
            es.physical_maximum = 25;
            es.physical_minimum = 4;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if ((es.label == "MV") || (es.label == "VM")) {
            code = CPAP_MinuteVent;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if ((es.label == "RR") || (es.label == "AF") || (es.label == "FR")) {
            code = CPAP_RespRate;
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
        } else if ((es.label == "Vt") || (es.label == "VC")) {
            code = CPAP_TidalVolume;
            es.gain *= 1000.0;
            es.physical_maximum *= 1000.0;
            es.physical_minimum *= 1000.0;
            //            es.digital_maximum*=1000.0;
            //            es.digital_minimum*=1000.0;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if ((es.label == "Leak") || (es.label.startsWith("Leck")) || (es.label.startsWith("Lekk"))
                   || (es.label.startsWith("Läck")) || es.label.startsWith("LÃ¤ck")) {
            code = CPAP_Leak;
            es.gain *= 60;
            es.physical_maximum *= 60;
            es.physical_minimum *= 60;
            //            es.digital_maximum*=60.0;
            //            es.digital_minimum*=60.0;
            es.physical_dimension = "L/M";
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0, true);
            sess->setPhysMax(code, 120.0);
            sess->setPhysMin(code, 0);
        } else if (es.label == "FFL Index") {
            code = CPAP_FLG;
            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (es.label.startsWith("Mask Pres")) {
            code = CPAP_MaskPressure;
            es.physical_maximum = 25;
            es.physical_minimum = 4;

            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (es.label.startsWith("Exp Press")) {
            code = CPAP_EPAP; //ExpiratoryPressure
            es.physical_maximum = 25;
            es.physical_minimum = 4;

            ToTimeDelta(sess, edf, es, code, recs, duration, 0, 0);
        } else if (es.label.startsWith("I:E")) {
            code = CPAP_IE; //I:E ratio?
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label.startsWith("Ti")) {
            code = CPAP_Ti;
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label.startsWith("Te")) {
            code = CPAP_Te;
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label.startsWith("TgMV")) {
            code = CPAP_TgMV;
            a = sess->AddEventList(code, EVL_Waveform, es.gain, es.offset, 0, 0, rate);
            a->AddWaveform(edf.startdate, es.data, recs, duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label == "") {
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
            a = NULL;
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

const QString RMS9_STR_Escape = "S9 Escape";
const QString RMS9_STR_EscapeAuto = "S9 Escape Auto";
const QString RMS9_STR_Elite = "S9 Elite";
const QString RMS9_STR_AutoSet = "S9 AutoSet";
const QString RMS9_STR_AutoSetForHer = "S9 AutoSet for Her";
const QString RMS9_STR_AutoSetCS = "S9 AutoSet CS";
const QString RMS9_STR_AutoSet25 = "S9 AutoSet 25";
const QString RMS9_STR_VPAP_S = "S9 VPAP S";
const QString RMS9_STR_VPAP_Auto = "S9 VPAP Auto";
const QString RMS9_STR_VPAP_Adapt = "S9 VPAP Adapt";
const QString RMS9_STR_VPAP_ST = "S9 VPAP ST";
const QString RMS9_STR_VPAP_STA = "S9 VPAP ST-A";
const QString RMS9_STR_VPAP_ST22 = "S9 VPAP ST 22";

void ResInitModelMap()
{
    // Escape Series
    RMS9ModelMap[36001] = RMS9ModelMap[36011] = RMS9ModelMap[36021] = RMS9ModelMap[36141] =
                              RMS9ModelMap[36201] = RMS9ModelMap[36221] = RMS9ModelMap[36261] = RMS9ModelMap[36301] =
                                          RMS9ModelMap[36361] = RMS9_STR_Escape;

    // Escape Auto Series
    RMS9ModelMap[36002] = RMS9ModelMap[36012] = RMS9ModelMap[36022] = RMS9ModelMap[36302] =
                              RMS9ModelMap[36362] = RMS9_STR_EscapeAuto;

    // Elite Series
    RMS9ModelMap[36003] = RMS9ModelMap[36013] = RMS9ModelMap[36023] = RMS9ModelMap[36103] =
                              RMS9ModelMap[36113] = RMS9ModelMap[36123] = RMS9ModelMap[36143] = RMS9ModelMap[36203] =
                                          RMS9ModelMap[36223] = RMS9ModelMap[36243] = RMS9ModelMap[36263] = RMS9ModelMap[36303] =
                                                  RMS9ModelMap[36343] = RMS9ModelMap[36363] = RMS9_STR_Elite;

    // AutoSet Series
    RMS9ModelMap[36005] = RMS9ModelMap[36015] = RMS9ModelMap[36025] = RMS9ModelMap[36105] =
                              RMS9ModelMap[36115] = RMS9ModelMap[36125] = RMS9ModelMap[36145] = RMS9ModelMap[36205] =
                                          RMS9ModelMap[36225] = RMS9ModelMap[36245] = RMS9ModelMap[36265] = RMS9ModelMap[36305] =
                                                  RMS9ModelMap[36325] = RMS9ModelMap[36345] = RMS9ModelMap[36365] = RMS9_STR_AutoSet;

    // AutoSet CS Series
    RMS9ModelMap[36100] = RMS9ModelMap[36110] = RMS9ModelMap[36120] = RMS9ModelMap[36140] =
                              RMS9ModelMap[36200] = RMS9ModelMap[36220] = RMS9ModelMap[36360] = RMS9_STR_AutoSetCS;

    // AutoSet 25 Series
    RMS9ModelMap[36106] = RMS9ModelMap[36116] = RMS9ModelMap[36126] = RMS9ModelMap[36146] =
                              RMS9ModelMap[36206] = RMS9ModelMap[36226] = RMS9ModelMap[36366] = RMS9_STR_AutoSet25;

    // Girly "For Her" AutoSet Series
    RMS9ModelMap[36065] = RMS9_STR_AutoSetForHer;

    // VPAP S Series (+H5i +Climate Control)
    RMS9ModelMap[36004] = RMS9ModelMap[36014] = RMS9ModelMap[36024] = RMS9ModelMap[36114] =
                              RMS9ModelMap[36124] = RMS9ModelMap[36144] = RMS9ModelMap[36204] = RMS9ModelMap[36224] =
                                          RMS9ModelMap[36284] = RMS9ModelMap[36304] = RMS9_STR_VPAP_S;

    // VPAP Auto Series (+H5i +Climate Control)
    RMS9ModelMap[36006] = RMS9ModelMap[36016] = RMS9ModelMap[36026] = RMS9_STR_VPAP_Auto;


    // VPAP Adapt Series (+H5i +Climate Control)
    // Trev's 36037 supports variable EPAP...
    RMS9ModelMap[36037] = RMS9ModelMap[36007] = RMS9ModelMap[36017] = RMS9ModelMap[36027] =
                              RMS9ModelMap[36367] = RMS9_STR_VPAP_Adapt;

    // VPAP ST Series (+H5i +Climate Control)
    RMS9ModelMap[36008] = RMS9ModelMap[36018] = RMS9ModelMap[36028] = RMS9ModelMap[36108] =
                              RMS9ModelMap[36148] = RMS9ModelMap[36208] = RMS9ModelMap[36228] = RMS9ModelMap[36368] =
                                          RMS9_STR_VPAP_ST;

    // VPAP ST 22 Series
    RMS9ModelMap[36118] = RMS9ModelMap[36128] = RMS9_STR_VPAP_ST22;

    // VPAP ST-A Series
    RMS9ModelMap[36039] = RMS9ModelMap[36159] = RMS9ModelMap[36169] = RMS9ModelMap[36379] =
                              RMS9_STR_VPAP_STA;


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


    /* S8 Series
    RMS9ModelMap[33007]="S8 Escape";
    RMS9ModelMap[33039]="S8 Elite II";
    RMS9ModelMap[33051]="S8 Escape II";
    RMS9ModelMap[33064]="S8 Escape II AutoSet";
    RMS9ModelMap[33064]="S8 Escape II AutoSet";
    RMS9ModelMap[33129]="S8 AutoSet II";
    */


    // Translation lookup table for non-english machines
    resmed_codes[CPAP_FlowRate].push_back("Flow");
    resmed_codes[CPAP_MaskPressureHi].push_back("Mask Pres");
    resmed_codes[CPAP_MaskPressureHi].push_back("Mask Pressure"); // vpap
    resmed_codes[CPAP_RespEvent].push_back("Resp Event");

    resmed_codes[CPAP_MaskPressure].push_back("Mask Pres");
    resmed_codes[CPAP_MaskPressure].push_back("Mask Pressure"); // vpap

    resmed_codes[CPAP_Pressure].push_back("Therapy Pres"); // not on vpap
    resmed_codes[CPAP_IPAP].push_back("Insp Pressure"); // on vpap

    resmed_codes[CPAP_EPAP].push_back("Exp Press");
    resmed_codes[CPAP_EPAP].push_back("Exp Pressure"); // vpap

    resmed_codes[CPAP_Leak].push_back("Leak");
    resmed_codes[CPAP_Leak].push_back("Leck.");
    resmed_codes[CPAP_Leak].push_back("Läcka");

    resmed_codes[CPAP_RespRate].push_back("RR");
    resmed_codes[CPAP_RespRate].push_back("AF");
    resmed_codes[CPAP_RespRate].push_back("FR");

    resmed_codes[CPAP_TidalVolume].push_back("Vt");
    resmed_codes[CPAP_TidalVolume].push_back("VC");

    resmed_codes[CPAP_MinuteVent].push_back("MV");
    resmed_codes[CPAP_MinuteVent].push_back("VM");

    resmed_codes[CPAP_IE].push_back("I:E"); // vpap
    resmed_codes[CPAP_Snore].push_back("Snore Index");
    resmed_codes[CPAP_FLG].push_back("FFL Index");

    resmed_codes[CPAP_RespEvent].push_back("RE");
    resmed_codes[CPAP_Ti].push_back("Ti");
    resmed_codes[CPAP_Te].push_back("Te");

    // Sad (oximetry)
    resmed_codes[OXI_Pulse].push_back("Pulse");
    resmed_codes[OXI_Pulse].push_back("Puls");                  // German & Swedish
    resmed_codes[OXI_Pulse].push_back("Pols");                  // Dutch
    resmed_codes[OXI_SPO2].push_back("SpO2");

    // Event annotations
    resmed_codes[CPAP_Obstructive].push_back("Obstructive apnea");
    resmed_codes[CPAP_Hypopnea].push_back("Hypopnea");
    resmed_codes[CPAP_Apnea].push_back("Apnea");
    resmed_codes[CPAP_ClearAirway].push_back("Central apnea");

    resmed_codes[CPAP_Mode].push_back("Mode");
    resmed_codes[CPAP_Mode].push_back("Modus"); // Dutch & German
    resmed_codes[CPAP_Mode].push_back("Funktion"); // Swedish

    resmed_codes[RMS9_SetPressure].push_back("Eingest. Druck");    // German
    resmed_codes[RMS9_SetPressure].push_back("Ingestelde druk");   // Dutch
    resmed_codes[RMS9_SetPressure].push_back("Set Pressure");      // English - Prescription
    resmed_codes[RMS9_SetPressure].push_back("Pres. prescrite");   // French
    resmed_codes[RMS9_SetPressure].push_back("Inställt tryck");    // Swedish
    resmed_codes[RMS9_SetPressure].push_back("InstÃ¤llt tryck");   // Swedish, QT5.2

    resmed_codes[RMS9_EPR].push_back("EPR");

    resmed_codes[RMS9_EPRSet].push_back("EPR Level");
    resmed_codes[RMS9_EPRSet].push_back("EPR-Stufe");             // French
    resmed_codes[RMS9_EPRSet].push_back("EPR-niveau");            // Dutch
    resmed_codes[RMS9_EPRSet].push_back("Niveau EPR");            // German
    resmed_codes[RMS9_EPRSet].push_back("EPR-nivå");              // Swedish
    resmed_codes[RMS9_EPRSet].push_back("EPR-nivÃ¥");             // Swedish, QT5.2

    resmed_codes[CPAP_PressureMax].push_back("Max Pressure");
    resmed_codes[CPAP_PressureMax].push_back("Max. Druck");       // German
    resmed_codes[CPAP_PressureMax].push_back("Max druk");         // Dutch
    resmed_codes[CPAP_PressureMax].push_back("Pression max.");    // French
    resmed_codes[CPAP_PressureMax].push_back("Max tryck");        // Swedish

    resmed_codes[CPAP_PressureMin].push_back("Min Pressure");
    resmed_codes[CPAP_PressureMin].push_back("Min. Druck");       // German
    resmed_codes[CPAP_PressureMin].push_back("Min druk");         // Dutch
    resmed_codes[CPAP_PressureMin].push_back("Pression min.");    // French
    resmed_codes[CPAP_PressureMin].push_back("Min tryck");        // Swedish

    // STR.edf
}


bool resmed_initialized = false;
void ResmedLoader::Register()
{
    if (resmed_initialized) { return; }

    qDebug() << "Registering ResmedLoader";
    RegisterLoader(new ResmedLoader());
    ResInitModelMap();
    resmed_initialized = true;
}

