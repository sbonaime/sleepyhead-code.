/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Fisher & Paykel Icon Loader Implementation
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QDir>
#include <QProgressBar>
#include <QMessageBox>
#include <QDataStream>
#include <QTextStream>
#include <cmath>

#include "icon_loader.h"

extern QProgressBar *qprogress;

FPIcon::FPIcon(Profile *p, MachineID id)
    : CPAP(p, id)
{
    m_class = fpicon_class_name;
}

FPIcon::~FPIcon()
{
}

FPIconLoader::FPIconLoader()
{
    m_buffer = nullptr;
}

FPIconLoader::~FPIconLoader()
{
}

int FPIconLoader::Open(QString &path, Profile *profile)
{
    QString newpath;

    path = path.replace("\\", "/");

    if (path.endsWith("/")) {
        path.chop(1);
    }

    QString dirtag = "FPHCARE";

    if (path.endsWith("/" + dirtag)) {
        newpath = path;
    } else {
        newpath = path + "/" + dirtag;
    }

    newpath += "/ICON/";

    QString filename;

    QDir dir(newpath);

    if ((!dir.exists() || !dir.isReadable())) {
        return 0;
    }

    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist = dir.entryInfoList();

    QStringList SerialNumbers;

    bool ok;

    for (int i = 0; i < flist.size(); i++) {
        QFileInfo fi = flist.at(i);
        QString filename = fi.fileName();

        filename.toInt(&ok);

        if (ok) {
            SerialNumbers.push_back(filename);
        }
    }

    Machine *m;

    QString npath;

    for (int i = 0; i < SerialNumbers.size(); i++) {
        QString &sn = SerialNumbers[i];
        m = CreateMachine(sn, profile);

        npath = newpath + "/" + sn;

        try {
            if (m) { OpenMachine(m, npath, profile); }
        } catch (OneTypePerDay e) {
            profile->DelMachine(m);
            MachList.erase(MachList.find(sn));
            QMessageBox::warning(nullptr, "Import Error",
                                 "This Machine Record cannot be imported in this profile.\nThe Day records overlap with already existing content.",
                                 QMessageBox::Ok);
            delete m;
        }
    }

    return MachList.size();
}

struct FPWaveChunk {
    FPWaveChunk() {
        st = 0;
        duration = 0;
        flow = nullptr;
        pressure = nullptr;
        leak = nullptr;
        file = 0;
    }
    FPWaveChunk(qint64 start, qint64 dur, int f) { st = start; duration = dur; file = f, flow = nullptr; leak = nullptr; pressure = nullptr; }
    FPWaveChunk(const FPWaveChunk &copy) {
        st = copy.st;
        duration = copy.duration;
        flow = copy.flow;
        leak = copy.leak;
        pressure = copy.pressure;
        file = copy.file;
    }
    qint64 st;
    qint64 duration;
    int file;
    EventList *flow;
    EventList *leak;
    EventList *pressure;
};

bool operator<(const FPWaveChunk &a, const FPWaveChunk &b)
{
    return (a.st < b.st);
}

int FPIconLoader::OpenMachine(Machine *mach, QString &path, Profile *profile)
{
    qDebug() << "Opening FPIcon " << path;
    QDir dir(path);

    if (!dir.exists() || (!dir.isReadable())) {
        return false;
    }

    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist = dir.entryInfoList();

    QString filename, fpath;

    if (qprogress) { qprogress->setValue(0); }

    QStringList summary, log, flw, det;

    for (int i = 0; i < flist.size(); i++) {
        QFileInfo fi = flist.at(i);
        filename = fi.fileName();
        fpath = path + "/" + filename;

        if (filename.left(3).toUpper() == "SUM") {
            summary.push_back(fpath);
            OpenSummary(mach, fpath, profile);
        } else if (filename.left(3).toUpper() == "DET") {
            det.push_back(fpath);
        } else if (filename.left(3).toUpper() == "FLW") {
            flw.push_back(fpath);
        } else if (filename.left(3).toUpper() == "LOG") {
            log.push_back(fpath);
        }
    }

    for (int i = 0; i < det.size(); i++) {
        OpenDetail(mach, det[i], profile);
    }

    for (int i = 0; i < flw.size(); i++) {
        OpenFLW(mach, flw[i], profile);
    }

    SessionID sid;//,st;
    float hours, mins;

    qDebug() << "Last 20 Sessions";

    int cnt = 0;
    QDateTime dt;
    QString a;
    QMap<SessionID, Session *>::iterator it = Sessions.end();
    it--;
    dt = QDateTime::fromTime_t(qint64(it.value()->first()) / 1000L);
    QDate date = dt.date().addDays(-7);
    it++;

    do {
        it--;
        Session *sess = it.value();
        sid = sess->session();
        hours = sess->hours();
        mins = hours * 60;
        dt = QDateTime::fromTime_t(sid);

        if (sess->channelDataExists(CPAP_FlowRate)) { a = "(flow)"; }
        else { a = ""; }

        qDebug() << cnt << ":" << dt << "session" << sid << "," << mins << "minutes" << a;

        if (dt.date() < date) { break; }

        ++cnt;

    } while (it != Sessions.begin());


    //    qDebug() << "Unmatched Sessions";
    //    QList<FPWaveChunk> chunks;
    //    for (QMap<int,QDate>::iterator dit=FLWDate.begin();dit!=FLWDate.end();dit++) {
    //        int k=dit.key();
    //        //QDate date=dit.value();
    ////        QList<Session *> values = SessDate.values(date);
    //        for (int j=0;j<FLWTS[k].size();j++) {

    //            FPWaveChunk chunk(FLWTS[k].at(j),FLWDuration[k].at(j),k);
    //            chunk.flow=FLWMapFlow[k].at(j);
    //            chunk.leak=FLWMapLeak[k].at(j);
    //            chunk.pressure=FLWMapPres[k].at(j);

    //            chunks.push_back(chunk);

    //            zz=FLWTS[k].at(j)/1000;
    //            dur=double(FLWDuration[k].at(j))/60000.0;
    //            bool b,c=false;
    //            if (Sessions.contains(zz)) b=true; else b=false;
    //            if (b) {
    //                if (Sessions[zz]->channelDataExists(CPAP_FlowRate)) c=true;
    //            }
    //            qDebug() << k << "-" <<j << ":" << zz << qRound(dur) << "minutes" << (b ? "*" : "") << (c ? QDateTime::fromTime_t(zz).toString() : "");
    //        }
    //    }
    //    qSort(chunks);
    //    bool b,c;
    //    for (int i=0;i<chunks.size();i++) {
    //        const FPWaveChunk & chunk=chunks.at(i);
    //        zz=chunk.st/1000;
    //        dur=double(chunk.duration)/60000.0;
    //        if (Sessions.contains(zz)) b=true; else b=false;
    //        if (b) {
    //            if (Sessions[zz]->channelDataExists(CPAP_FlowRate)) c=true;
    //        }
    //        qDebug() << chunk.file << ":" << i << zz << dur << "minutes" << (b ? "*" : "") << (c ? QDateTime::fromTime_t(zz).toString() : "");
    //    }

    mach->Save();

    return true;
}

QDateTime FPIconLoader::readFPDateTime(quint8 *data)
{
    quint32 ts = (data[3] << 24) | (data[2] << 16) | ((data[1] << 8) | data[0]); // ^ 0xc00000;
    // 0x20a41b18

    quint8 day = ts & 0x1f; // 0X18      24
    ts >>= 5;               // 10520D8
    quint8 month = ts & 0x0f; // 0X08      8
    ts >>= 4;               // 10520D
    quint8 year = ts & 0x3f; // 0X0D      13
    ts >>= 6;               // 4148
    quint8 second = ts & 0x3f; // 0X08      8
    ts >>= 6;               // 20A
    quint8 minute = ts & 0x3f; // 0A        10
    ts >>= 6;               // 10
    quint8 hour = ts & 0x1f; // 10        16
    QDate date = QDate(2000 + year, month, day);
    QTime time = QTime(hour, minute, second);
    QDateTime datetime = QDateTime(date, time);
    return datetime;
}

/*
 *in >> a1;
in >> a2;
t1=a2 << 8 | a1;

if (t1==0xfafe)
    break;

day=t1 & 0x1f;
month=(t1 >> 5) & 0x0f;
year=2000+((t1 >> 9) & 0x3f);

in >> a1;
in >> a2;

ts=((a2 << 8) | a1) << 1;
ts|=(t1 >> 15) & 1;

second=(ts & 0x3f);
minute=(ts >> 6) & 0x3f;
hour=(ts >> 12) & 0x1f; */



// FLW Header Structure
// 0x0000-0x01fe
// newline (0x0d) seperated list of machine information strings.
// magic?         0201
// version        1.5.0
// serial number  12 digits
// Machine Series "ICON"
// Machine Model  "Auto"
// Remainder of header is 0 filled...
// 0x01ff 8 bit additive sum checksum byte of previous header bytes

// 0x0200-0x0203 32bit timestamp in
bool FPIconLoader::OpenFLW(Machine *mach, QString filename, Profile *profile)
{
    Q_UNUSED(mach);
    Q_UNUSED(profile);
    QByteArray data;
    quint16 t1;
    quint32 ts;
    double ti;

    EventList *flow = nullptr, * pressure = nullptr, *leak = nullptr;
    QDateTime datetime;

    unsigned char *buf, *endbuf;


    qDebug() << filename;
    QByteArray header;
    QFile file(filename);

    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Couldn't open" << filename;
        return false;
    }

    header = file.read(0x200);

    if (header.size() != 0x200) {
        qDebug() << "Short file" << filename;
        return false;
    }

    unsigned char hsum = 0xff;

    for (int i = 0; i < 0x1ff; i++) {
        hsum += header[i];
    }

    if (hsum != header[0x1ff]) {
        qDebug() << "Header checksum mismatch" << filename;
    }

    QTextStream htxt(&header);
    QString h1, version, fname, serial, model, type;
    htxt >> h1;
    htxt >> version;
    htxt >> fname;
    htxt >> serial;
    htxt >> model;
    htxt >> type;

    if (mach->properties[STR_PROP_Model].isEmpty()) {
        mach->properties[STR_PROP_Model] = model + " " + type;
    }

    fname.chop(4);
    QString num = fname.right(4);
    int filenum = num.toInt();

    data = file.readAll();

    buf = (unsigned char *)data.data();
    endbuf = buf + data.size();

    t1 = buf[1] << 8 | buf[0];

    if (t1 == 0xfafe) { // End of file marker..
        qDebug() << "FaFE observed in" << filename;
        return false;
    }

    datetime = readFPDateTime(buf);
    buf += 4;


    QDate date;
    QTime time;

    if (!datetime.isValid()) {
        qDebug() << "DateTime invalid in OpenFLW:" << filename;
        return false;
    }  else {
        date = datetime.date();
        time = datetime.time();
        ts = datetime.toTime_t();
    }

    ti = qint64(ts) * 1000L;

    EventStoreType pbuf[256];

    QMap<SessionID, Session *>::iterator sit = Sessions.find(ts);

    Session *sess;

    bool newsess = false;

    if (sit != Sessions.end()) {
        sess = sit.value();
        qDebug() << filenum << ":" << date << sess->session() << ":" << sess->hours() * 60.0;
    } else {
        qint64 k = -1;
        Session *s1 = nullptr;
        sess = nullptr;

        for (sit = Sessions.begin(); sit != Sessions.end(); sit++) {
            s1 = sit.value();
            qint64 z = qAbs(s1->first() - ti);

            if (z < 3600000) {
                if ((k < 0) || (k > z)) {
                    k = z;
                    sess = s1;
                }
            }
        }

        if (sess) {
            sess->set_first(ti);
            sess->setFirst(CPAP_FlowRate, ti);
            sess->setFirst(CPAP_MaskPressure, ti);
        } else {
            sess = new Session(mach, ts);
            sess->set_first(ti);
            sess->setFirst(CPAP_FlowRate, ti);
            sess->setFirst(CPAP_MaskPressure, ti);
            newsess = true;
            qDebug() << filenum << ":" << date << "couldn't find matching session for" << ts;
        }
    }

    const int samples_per_block = 50;
    const double rate = 1000.0 / double(samples_per_block);

    // F&P Overwrites this file, not appends to it.
    flow = new EventList(EVL_Waveform, 1.0, 0, 0, 0, rate);
    //leak=new EventList(EVL_Event,1.0,0,0,0,rate*double(samples_per_block)); // 1 per second
    pressure = new EventList(EVL_Event, 0.01, 0, 0, 0,
                             rate * double(samples_per_block)); // 1 per second

    flow->setFirst(ti);
    //leak->setFirst(ti);
    pressure->setFirst(ti);

    qint16 pr;
    quint16 lkaj;

    EventDataType val;
    qint16 tmp;

    do {
        quint8 *p = buf;

        // Scan ahead looking for end of block, marked by ff ff
        do {
            p++;

            if (p >= endbuf) {
                delete flow;
                delete leak;
                delete pressure;
                return false;
            }
        } while (!((p[0] == 0xff) && (p[1] == 0xff)));

        // The Pressure and lkaj codes are before the end of block marker
        p -= 3;
        pr = p[1] << 8 | p[0];
        lkaj = p[2];
        int i = 0;

        pressure->AddEvent(ti, pr);
        //leak->AddEvent(ti,lkaj);

        do {
            tmp = buf[1] << 8 | buf[0];
            val = (EventDataType(tmp) / 100.0) - lkaj;

            if (val < -128) { val = -128; }
            else if (val > 128) { val = 128; }

            buf += 2;

            pbuf[i++] = val;
        } while (buf < p);

        flow->AddWaveform(ti, pbuf, i, rate);
        ti += i * rate;

        buf = p + 5;

        if (buf >= endbuf) {
            break;
        }
    } while (!((buf[0] == 0xff) && (buf[1] == 0x7f)));


    if (sess) {
        sess->setLast(CPAP_FlowRate, ti);
        sess->setLast(CPAP_MaskPressure, ti);
        sess->eventlist[CPAP_FlowRate].push_back(flow);
        // sess->eventlist[CPAP_Leak].push_back(leak);
        sess->eventlist[CPAP_MaskPressure].push_back(pressure);
    }

    if (newsess) {
        mach->AddSession(sess, profile);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Open Summary file
////////////////////////////////////////////////////////////////////////////////////////////
bool FPIconLoader::OpenSummary(Machine *mach, QString filename, Profile *profile)
{
    qDebug() << filename;
    QByteArray header;
    QFile file(filename);

    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Couldn't open" << filename;
        return false;
    }

    header = file.read(0x200);

    if (header.size() != 0x200) {
        qDebug() << "Short file" << filename;
        return false;
    }

    unsigned char hsum = 0xff;

    for (int i = 0; i < 0x1ff; i++) {
        hsum += header[i];
    }

    if (hsum != header[0x1ff]) {
        qDebug() << "Header checksum mismatch" << filename;
    }

    QTextStream htxt(&header);
    QString h1, version, fname, serial, model, type;
    htxt >> h1;
    htxt >> version;
    htxt >> fname;
    htxt >> serial;
    htxt >> model;
    htxt >> type;
    mach->properties[STR_PROP_Model] = model + " " + type;


    QByteArray data;
    data = file.readAll();
    //long size=data.size(),pos=0;
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 t1;//,t2;
    quint32 ts;
    //QByteArray line;
    unsigned char a1, a2, a3, a4, a5, p1, p2,  p3, p4, p5, j1, j2, j3 , j4, j5, j6, j7, x1, x2;

    quint16 d1, d2, d3;


    QDateTime datetime;

    int runtime, usage;

    int day, month, year, hour, minute, second;
    QDate date;

    do {
        in >> a1;
        in >> a2;
        t1 = a2 << 8 | a1;

        if (t1 == 0xfafe) {
            break;
        }

        day = t1 & 0x1f;
        month = (t1 >> 5) & 0x0f;
        year = 2000 + ((t1 >> 9) & 0x3f);

        in >> a1;
        in >> a2;

        ts = ((a2 << 8) | a1) << 1;
        ts |= (t1 >> 15) & 1;

        second = (ts & 0x3f);
        minute = (ts >> 6) & 0x3f;
        hour = (ts >> 12) & 0x1f;

        datetime = QDateTime(QDate(year, month, day), QTime(hour, minute, second));

        date = datetime.date();
        ts = datetime.toTime_t();

        // the following two quite often match in value
        in >> a1;  // 0x04 Run Time
        in >> a2;  // 0x05 Usage Time
        runtime = a1 * 360; // durations are in tenth of an hour intervals
        usage = a2 * 360;

        in >> a3;  // 0x06  // Ramps???
        in >> a4;  // 0x07  // a pressure value?
        in >> a5;  // 0x08  // ?? varies.. always less than 90% leak..

        in >> d1;  // 0x09
        in >> d2;  // 0x0b
        in >> d3;  // 0x0d   // 90% Leak value..

        in >> p1;  // 0x0f
        in >> p2;  // 0x10

        in >> j1;  // 0x11
        in >> j2;  // 0x12  // Apnea Events
        in >> j3;  // 0x13  // Hypopnea events
        in >> j4;  // 0x14  // Flow Limitation events
        in >> j5;  // 0x15
        in >> j6;  // 0x16
        in >> j7;  // 0x17

        in >> p3;  // 0x18
        in >> p4;  // 0x19
        in >> p5;  // 0x1a

        in >> x1;  // 0x1b
        in >> x2;  // 0x1c    // humidifier setting

        if (!mach->SessionExists(ts)) {
            Session *sess = new Session(mach, ts);
            sess->really_set_first(qint64(ts) * 1000L);
            sess->really_set_last(qint64(ts + usage) * 1000L);
            sess->SetChanged(true);
            sess->setCount(CPAP_Obstructive, j2);
            sess->setCount(CPAP_Hypopnea, j3);
            SessDate.insert(date, sess);

            //            sess->setCount(CPAP_Obstructive,j1);
            //            sess->setCount(CPAP_Hypopnea,j2);
            //            sess->setCount(CPAP_ClearAirway,j3);
            //            sess->setCount(CPAP_Apnea,j4);
            //sess->setCount(CPAP_,j5);
            if (p1 != p2) {
                sess->settings[CPAP_Mode] = (int)MODE_APAP;
                sess->settings[CPAP_PressureMin] = p3 / 10.0;
                sess->settings[CPAP_PressureMax] = p4 / 10.0;
            } else {
                sess->settings[CPAP_Mode] = (int)MODE_CPAP;
                sess->settings[CPAP_Pressure] = p1 / 10.0;
            }

            sess->settings[CPAP_HumidSetting] = x2;
            //sess->settings[CPAP_PresReliefType]=PR_SENSAWAKE;
            Sessions[ts] = sess;
            mach->AddSession(sess, profile);
        }
    } while (!in.atEnd());

    return true;
}

bool FPIconLoader::OpenDetail(Machine *mach, QString filename, Profile *profile)
{
    Q_UNUSED(mach);
    Q_UNUSED(profile);

    qDebug() << filename;
    QByteArray header;
    QFile file(filename);

    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Couldn't open" << filename;
        return false;
    }

    header = file.read(0x200);

    if (header.size() != 0x200) {
        qDebug() << "Short file" << filename;
        return false;
    }

    unsigned char hsum = 0;

    for (int i = 0; i < 0x1ff; i++) {
        hsum += header[i];
    }

    if (hsum != header[0x1ff]) {
        qDebug() << "Header checksum mismatch" << filename;
    }

    QByteArray index;
    index = file.read(0x800);
    //long size=index.size(),pos=0;
    QDataStream in(index);

    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);
    quint32 ts;
    QDateTime datetime;
    QDate date;
    QTime time;
    //FPDetIdx *idx=(FPDetIdx *)index.data();


    QVector<quint32> times;
    QVector<quint16> start;
    QVector<quint8> records;

    quint16 t1;
    quint16 strt;
    quint8 recs, z1, z2;

    int day, month, year, hour, minute, second;

    int totalrecs = 0;

    do {
        in >> z1;
        in >> z2;
        t1 = z2 << 8 | z1;

        if (t1 == 0xfafe) {
            break;
        }

        day = t1 & 0x1f;
        month = (t1 >> 5) & 0x0f;
        year = 2000 + ((t1 >> 9) & 0x3f);

        in >> z1;
        in >> z2;
        //

        ts = ((z2 << 8) | z1) << 1;
        ts |= (t1 >> 15) & 1;

        //
        second = (ts & 0x3f);
        minute = (ts >> 6) & 0x3f;
        hour = (ts >> 12) & 0x1f;

        datetime = QDateTime(QDate(year, month, day), QTime(hour, minute, second));
        //datetime=datetime.toTimeSpec(Qt::UTC);

        ts = datetime.toTime_t();

        date = datetime.date();
        time = datetime.time();
        in >> strt;
        in >> recs;
        totalrecs += recs;

        if (Sessions.contains(ts)) {
            times.push_back(ts);
            start.push_back(strt);
            records.push_back(recs);
        }
    } while (!in.atEnd());

    QByteArray databytes = file.readAll();

    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::BigEndian);
    // 5 byte repeating patterns

    quint8 *data = (quint8 *)databytes.data();

    qint64 ti;
    quint8 pressure, leak, a1, a2, a3;
    SessionID sessid;
    Session *sess;
    int idx;

    for (int r = 0; r < start.size(); r++) {
        sessid = times[r];
        sess = Sessions[sessid];
        ti = qint64(sessid) * 1000L;
        sess->really_set_first(ti);
        EventList *LK = sess->AddEventList(CPAP_LeakTotal, EVL_Event, 1);
        EventList *PR = sess->AddEventList(CPAP_Pressure, EVL_Event, 0.1);
        EventList *FLG = sess->AddEventList(CPAP_FLG, EVL_Event);
        EventList *OA = sess->AddEventList(CPAP_Obstructive, EVL_Event);
        EventList *H = sess->AddEventList(CPAP_Hypopnea, EVL_Event);
        EventList *FL = sess->AddEventList(CPAP_FlowLimit, EVL_Event);

        unsigned stidx = start[r];
        int rec = records[r];

        idx = stidx * 15;

        for (int i = 0; i < rec; i++) {
            for (int j = 0; j < 3; j++) {
                pressure = data[idx];
                leak = data[idx + 1];
                a1 = data[idx + 2];
                a2 = data[idx + 3];
                a3 = data[idx + 4];
                PR->AddEvent(ti, pressure);
                LK->AddEvent(ti, leak);

                if (a1 > 0) { OA->AddEvent(ti, a1); }

                if (a2 > 0) { H->AddEvent(ti, a2); }

                if (a3 > 0) { FL->AddEvent(ti, a3); }

                FLG->AddEvent(ti, a3);
                ti += 120000L;
                idx += 5;
            }
        }

        //  sess->really_set_last(ti-360000L);
        //        sess->SetChanged(true);
        //       mach->AddSession(sess,profile);
    }

    return 1;
}


Machine *FPIconLoader::CreateMachine(QString serial, Profile *profile)
{
    if (!profile) {
        return nullptr;
    }

    qDebug() << "Create Machine " << serial;

    QList<Machine *> ml = profile->GetMachines(MT_CPAP);
    bool found = false;
    QList<Machine *>::iterator i;
    Machine *m;

    for (i = ml.begin(); i != ml.end(); i++) {
        if (((*i)->GetClass() == fpicon_class_name) && ((*i)->properties[STR_PROP_Serial] == serial)) {
            MachList[serial] = *i;
            found = true;
            m = *i;
            break;
        }
    }

    if (!found) {
        m = new FPIcon(profile, 0);
    }

    m->properties[STR_PROP_Brand] = "Fisher & Paykel";
    m->properties[STR_PROP_Series] = STR_MACH_FPIcon;
    m->properties[STR_PROP_Model] = STR_MACH_FPIcon;

    if (found) {
        return m;
    }


    MachList[serial] = m;
    profile->AddMachine(m);

    m->properties[STR_PROP_Serial] = serial;
    m->properties[STR_PROP_DataVersion] = QString::number(fpicon_data_version);

    QString path = "{" + STR_GEN_DataFolder + "}/" + m->GetClass() + "_" + serial + "/";
    m->properties[STR_PROP_Path] = path;
    m->properties[STR_PROP_BackupPath] = path + "Backup/";

    return m;
}

bool FPIconLoader::Detect(const QString & path)
{
    return false;
}


bool fpicon_initialized = false;
void FPIconLoader::Register()
{
    if (fpicon_initialized) { return; }

    qDebug() << "Registering F&P Icon Loader";
    RegisterLoader(new FPIconLoader());
    //InitModelMap();
    fpicon_initialized = true;
}
