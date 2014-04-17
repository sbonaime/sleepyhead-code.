/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib CMS50X Loader Implementation
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the cms50_data_version in cms50_loader.h when making changes to this loader
// that change loader behaviour or modify channels.
//********************************************************************************************

#include <QProgressBar>
#include <QApplication>
#include <QDir>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include <QList>

using namespace std;

#include "cms50_loader.h"
#include "SleepLib/machine.h"
#include "SleepLib/session.h"

extern QProgressBar *qprogress;

// Possibly need to replan this to include oximetry

CMS50Loader::CMS50Loader()
{
}

CMS50Loader::~CMS50Loader()
{
}
int CMS50Loader::Open(QString &path, Profile *profile)
{
    // CMS50 folder structure detection stuff here.

    // Not sure whether to both supporting SpO2 files here, as the timestamps are utterly useless for overlay purposes.
    // May just ignore the crap and support my CMS50 logger

    // Contains three files
    // Data Folder
    // SpO2 Review.ini
    // SpO2.ini
    if (!profile) {
        qWarning() << "Empty Profile in CMS50Loader::Open()";
        return 0;
    }

    // This bit needs modifying for the SPO2 folder detection.
    QDir dir(path);
    QString tmp = path + "/Data"; // The directory path containing the .spor/.spo2 files

    if ((dir.exists("SpO2 Review.ini") || dir.exists("SpO2.ini"))
            && dir.exists("Data")) {
        // SPO2Review/etc software

        return OpenCMS50(tmp, profile);
    }

    return 0;
}
int CMS50Loader::OpenCMS50(QString &path, Profile *profile)
{
    QString filename, pathname;
    QList<QString> files;
    QDir dir(path);

    if (!dir.exists()) {
        return 0;
    }

    if (qprogress) { qprogress->setValue(0); }

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist = dir.entryInfoList();

    QString fn;

    for (int i = 0; i < flist.size(); i++) {
        QFileInfo fi = flist.at(i);
        fn = fi.fileName().toLower();

        if (fn.endsWith(".spor") || fn.endsWith(".spo2")) {
            files.push_back(fi.canonicalFilePath());
        }

        //if (loader_progress) loader_progress->Pulse();

    }

    int size = files.size();

    if (size == 0) { return 0; }

    Machine *mach = CreateMachine(profile);
    int cnt = 0;

    for (QList<QString>::iterator n = files.begin(); n != files.end(); n++, ++cnt) {
        if (qprogress) { qprogress->setValue((float(cnt) / float(size) * 50.0)); }

        QApplication::processEvents();
        OpenSPORFile((*n), mach, profile);
    }

    mach->Save();

    if (qprogress) { qprogress->setValue(100); }

    return 1;
}

bool CMS50Loader::OpenSPORFile(QString path, Machine *mach, Profile *profile)
{
    if (!mach || !profile) {
        return false;
    }

    QFile f(path);
    unsigned char tmp[256];

    qint16 data_starts;
    qint16 some_code;
    qint16 some_more_code;
    int seconds = 0, num_records;
    int br;

    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }

    // Find everything after the last _

    QString str = path.section("/", -1);
    str = str.section("_", -1);
    str = str.section(".", 0, 0);

    QDateTime dt;

    if (str.length() == 14) {
        dt = QDateTime::fromString(str, "yyyyMMddHHmmss");
    } else if (str.length() == 12) {
        dt = QDateTime::fromString(str, "yyyyMMddHHmm");
    } else {
        qDebug() << "CMS50::Spo[r2] Dodgy date field";
        return false;
    }

    if (!dt.isValid()) {
        return false;
    }

    SessionID sessid = dt.toTime_t(); // Import date becomes session id

    if (mach->SessionExists(sessid)) {
        return false;    // Already imported
    }

    br = f.read((char *)tmp, 2);

    if (br != 2) { return false; }

    data_starts = tmp[0] | (tmp[1] << 8);

    br = f.read((char *)tmp, 2);

    if (br != 2) { return false; }

    some_code = tmp[0] | (tmp[1] << 8); // 512 or 256 observed
    Q_UNUSED(some_code);

    br = f.read((char *)tmp, 2);

    if (br != 2) { return false; }

    seconds = tmp[0] | (tmp[1] << 8);

    if (!seconds) {
        num_records = (f.size() - data_starts);
        seconds = num_records / 2;
    } else {
        num_records = seconds << 1;
    }

    if (seconds < 60) {
        // Don't bother importing short sessions
        return false;
    }

    br = f.read((char *)tmp, 2);

    if (br != 2) { return false; }

    some_more_code = tmp[0] | (tmp[1] << 8); // == 0
    Q_UNUSED(some_more_code);

    br = f.read((char *)tmp, 34); // Read widechar date record

    if (br != 34) { return false; }

    for (int i = 0; i < 17; i++) { // Convert to 8bit
        tmp[i] = tmp[i << 1];
    }

    tmp[17] = 0;
    QString datestr = (char *)tmp;
    QDateTime date;
    qint64 starttime;

    if (datestr.isEmpty()) { // Has Internal date record, so use it
        date = QDateTime::fromString(datestr, "MM/dd/yy HH:mm:ss");
        QDate d2 = date.date();

        if (d2.year() < 2000) { // Nice to see CMS50 is Y2K friendly..
            d2.setDate(d2.year() + 100, d2.month(), d2.day());
            date.setDate(d2);
        }

        if (!date.isValid()) {
            qDebug() << "Invalid date time retreieved in CMS50::OpenSPO[R2]File";
            return false;
        }

        starttime = qint64(date.toTime_t()) * 1000L;
    } else if (dt.isValid()) { // Else take the filenames date
        date = dt;
        starttime = qint64(dt.toTime_t()) * 1000L;
    } else {  // Has nothing, so add it up to current time
        qDebug() << "CMS50: Couldn't get any start date indication";
        date = QDateTime::currentDateTime();
        date = date.addSecs(-seconds);
        starttime = qint64(date.toTime_t()) * 1000L;
    }

    f.seek(data_starts);

    buffer = new char [num_records];
    br = f.read(buffer, num_records);

    if (br != num_records) {
        qDebug() << "Short .spo[R2] File: " << path;
        delete [] buffer;
        return false;
    }

    //QDateTime last_pulse_time=date;
    //QDateTime last_spo2_time=date;

    EventDataType last_pulse = buffer[0];
    EventDataType last_spo2 = buffer[1];
    EventDataType cp = 0, cs = 0;

    Session *sess = new Session(mach, sessid);
    sess->updateFirst(starttime);
    EventList *oxip = sess->AddEventList(OXI_Pulse, EVL_Event);
    EventList *oxis = sess->AddEventList(OXI_SPO2, EVL_Event);

    oxip->AddEvent(starttime, last_pulse);
    oxis->AddEvent(starttime, last_spo2);

    EventDataType PMin = 0, PMax = 0, SMin = 0, SMax = 0, PAvg = 0, SAvg = 0;
    int PCnt = 0, SCnt = 0;
    qint64 tt = starttime;
    //fixme: Need two lasttime values here..
    qint64 lasttime = starttime;

    bool first_p = true, first_s = true;

    for (int i = 2; i < num_records; i += 2) {
        cp = buffer[i];
        cs = buffer[i + 1];

        if (last_pulse != cp) {
            oxip->AddEvent(tt, cp);

            if (tt > lasttime) { lasttime = tt; }

            if (cp > 0) {
                if (first_p) {
                    PMin = cp;
                    first_p = false;
                } else {
                    if (PMin > cp) { PMin = cp; }
                }

                PAvg += cp;
                PCnt++;
            }
        }

        if (last_spo2 != cs) {
            oxis->AddEvent(tt, cs);

            if (tt > lasttime) { lasttime = tt; }

            if (cs > 0) {
                if (first_s) {
                    SMin = cs;
                    first_s = false;
                } else {
                    if (SMin > cs) { SMin = cs; }
                }

                SAvg += cs;
                SCnt++;
            }
        }

        last_pulse = cp;
        last_spo2 = cs;

        if (PMax < cp) { PMax = cp; }

        if (SMax < cs) { SMax = cs; }

        tt += 1000; // An educated guess of 1 second. Verified by gcz@cpaptalk
    }

    if (cp) { oxip->AddEvent(tt, cp); }

    if (cs) { oxis->AddEvent(tt, cs); }

    sess->updateLast(tt);

    EventDataType pa = 0, sa = 0;

    if (PCnt > 0) { pa = PAvg / double(PCnt); }

    if (SCnt > 0) { sa = SAvg / double(SCnt); }

    sess->setMin(OXI_Pulse, PMin);
    sess->setMax(OXI_Pulse, PMax);
    sess->setAvg(OXI_Pulse, pa);
    sess->setMin(OXI_SPO2, SMin);
    sess->setMax(OXI_SPO2, SMax);
    sess->setAvg(OXI_SPO2, sa);

    mach->AddSession(sess, profile);
    sess->SetChanged(true);
    delete [] buffer;

    return true;
}
Machine *CMS50Loader::CreateMachine(Profile *profile)
{
    if (!profile) {
        return NULL;
    }

    // NOTE: This only allows for one CMS50 machine per profile..
    // Upgrading their oximeter will use this same record..

    QList<Machine *> ml = profile->GetMachines(MT_OXIMETER);

    for (QList<Machine *>::iterator i = ml.begin(); i != ml.end(); i++) {
        if ((*i)->GetClass() == cms50_class_name)  {
            return (*i);
            break;
        }
    }

    qDebug() << "Create CMS50 Machine Record";

    Machine *m = new Oximeter(profile, 0);
    m->SetClass(cms50_class_name);
    m->properties[STR_PROP_Brand] = "Contec";
    m->properties[STR_PROP_Model] = "CMS50X";
    m->properties[STR_PROP_DataVersion] = QString::number(cms50_data_version);

    profile->AddMachine(m);
    QString path = "{" + STR_GEN_DataFolder + "}/" + m->GetClass() + "_" + m->hexid() + "/";
    m->properties[STR_PROP_Path] = path;

    return m;
}



static bool cms50_initialized = false;

void CMS50Loader::Register()
{
    if (cms50_initialized) { return; }

    qDebug() << "Registering CMS50Loader";
    RegisterLoader(new CMS50Loader());
    //InitModelMap();
    cms50_initialized = true;
}

