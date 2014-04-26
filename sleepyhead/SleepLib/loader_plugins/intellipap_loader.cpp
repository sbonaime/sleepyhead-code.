/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib (DeVilbiss) Intellipap Loader Implementation
 * Notes: Intellipap requires the SmartLink attachment to access this data.
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QDir>
#include <QProgressBar>

#include "intellipap_loader.h"

extern QProgressBar *qprogress;

Intellipap::Intellipap(Profile *p, MachineID id)
    : CPAP(p, id)
{
    m_class = intellipap_class_name;
}

Intellipap::~Intellipap()
{
}

IntellipapLoader::IntellipapLoader()
{
    m_buffer = nullptr;
}

IntellipapLoader::~IntellipapLoader()
{
}

bool IntellipapLoader::Detect(const QString & path)
{
    return false;
}

int IntellipapLoader::Open(QString &path, Profile *profile)
{
    // Check for SL directory
    // Check for DV5MFirm.bin?
    QString newpath;

    path = path.replace("\\", "/");

    QString dirtag = "SL";

    if (path.endsWith("/" + dirtag)) {
        return 0;
        //newpath=path;
    } else {
        newpath = path + "/" + dirtag;
    }

    QString filename;

    //////////////////////////
    // Parse the Settings File
    //////////////////////////
    filename = newpath + "/SET1";
    QFile f(filename);

    if (!f.exists()) { return 0; }

    f.open(QFile::ReadOnly);
    QTextStream tstream(&f);

    QHash<QString, QString> lookup = {
        {"Sn", STR_PROP_Serial },
        {"Mn", STR_PROP_ModelNumber },
        {"Mo", "PAPMode" },     // 0 cpap, 1 auto
        //{"Pn", "??" },
        {"Pu", "MaxPressure" },
        {"Pl", "MaxPressure" },
        //{"Ds", "??" },
        //{"Pc", "??" },
        {"Pd", "RampPressure" }, // Pressure Delay
        {"Dt", "RampTime" },
        //{"Ld", "??" },
        //{"Lh", "??" },
        //{"FC", "??" },
        //{"FE", "??" },
        //{"FL", "??" },
        {"A%", "ApneaThreshold" },
        {"Ad", "ApneaDuration" },
        {"H%", "HypopneaThreshold" },
        {"Hd", "HypopneaDuration" },
        //{"Pi", "??" },
        //{"Pe", "??" },
        {"Ri", "SmartFlexIRnd" },  // Inhale Rounding (0-5)
        {"Re", "SmartFlexERnd" }, // Inhale Rounding (0-5)
        //{"Bu", "??" }, // WF
        //{"Ie", "??" }, // 20
        //{"Se", "??" }, // 05
        //{"Si", "??" }, // 05
        //{"Mi", "??" }, // 0
        {"Uh", "HoursMeter"}, // 0000.0
        {"Up", "ComplianceMeter"}, // 0000.00
        //{"Er", "ErrorCode"}, // E00
        //{"El", "LastErrorCode"}, // E00 00/00/0000
        //{"Hp", "??"}, // 1
        //{"Hs", "??"}, // 02
        //{"Lu", "LowUseThreshold"}, // defaults to 0 (4 hours)
        {"Sf", "SmartFlex"},
        {"Sm", "SmartFlexMode"},
        {"Ks=s", "Ks_s"},
        {"Ks=i", "ks_i"}

    };

    QHash<QString, QString> set1;
    QHash<QString, QString>::iterator hi;

    while (1) {
        QString line = tstream.readLine();

        if ((line.length() <= 2) ||
                (line.isNull())) { break; }

        QString key = line.section("\t", 0, 0).trimmed();
        hi = lookup.find(key);

        if (hi != lookup.end()) {
            key = hi.value();
        }

        QString value = line.section("\t", 1).trimmed();
        set1[key] = value;
        qDebug() << key << "=" << value;
    }

    Machine *mach = nullptr;

    if (set1.contains(STR_PROP_Serial)) {
        mach = CreateMachine(set1[STR_PROP_Serial], profile);
    }

    if (!mach) {
        qDebug() << "Couldn't get Intellipap machine record";
        return 0;
    }

    // Refresh properties data..
    for (QHash<QString, QString>::iterator i = set1.begin(); i != set1.end(); i++) {
        mach->properties[i.key()] = i.value();
    }

    mach->properties[STR_PROP_Model] = STR_MACH_Intellipap + " " +
                                       mach->properties[STR_PROP_ModelNumber];

    f.close();

    //////////////////////////
    // Parse the Session Index
    //////////////////////////
    unsigned char buf[27];
    filename = newpath + "/U";
    f.setFileName(filename);

    if (!f.exists()) { return 0; }

    QVector<quint32> SessionStart;
    QVector<quint32> SessionEnd;
    QHash<SessionID, Session *> Sessions;

    quint32 ts1, ts2;//, length;
    //unsigned char cs;
    f.open(QFile::ReadOnly);
    int cnt = 0;
    QDateTime epoch(QDate(2002, 1, 1), QTime(0, 0, 0), Qt::UTC); // Intellipap Epoch
    int ep = epoch.toTime_t();

    do {
        cnt = f.read((char *)buf, 9);
        ts1 = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
        ts2 = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
        ts1 += ep;
        ts2 += ep;
        SessionStart.append(ts1);
        SessionEnd.append(ts2);
        //cs=buf[8];
    } while (cnt > 0);

    qDebug() << "U file logs" << SessionStart.size() << "sessions.";
    f.close();

    //////////////////////////
    // Parse the Session Data
    //////////////////////////
    filename = newpath + "/L";
    f.setFileName(filename);

    if (!f.exists()) { return 0; }

    f.open(QFile::ReadOnly);
    long size = f.size();
    int recs = size / 26;
    m_buffer = new unsigned char [size];

    if (size != f.read((char *)m_buffer, size)) {
        qDebug()  << "Couldn't read 'L' data" << filename;
        return 0;
    }

    Session *sess;
    SessionID sid;

    for (int i = 0; i < SessionStart.size(); i++) {
        sid = SessionStart[i];

        if (mach->SessionExists(sid)) {
            // knock out the already imported sessions..
            SessionStart[i] = 0;
            SessionEnd[i] = 0;
        } else if (!Sessions.contains(sid)) {
            sess = Sessions[sid] = new Session(mach, sid);
            sess->SetChanged(true);
            sess->AddEventList(CPAP_IPAP, EVL_Event);
            sess->AddEventList(CPAP_EPAP, EVL_Event);
            sess->AddEventList(CPAP_Pressure, EVL_Event);

            sess->AddEventList(INTELLIPAP_Unknown1, EVL_Event);
            sess->AddEventList(INTELLIPAP_Unknown2, EVL_Event);

            sess->AddEventList(CPAP_LeakTotal, EVL_Event);
            sess->AddEventList(CPAP_MaxLeak, EVL_Event);
            sess->AddEventList(CPAP_TidalVolume, EVL_Event);
            sess->AddEventList(CPAP_MinuteVent, EVL_Event);
            sess->AddEventList(CPAP_RespRate, EVL_Event);
            sess->AddEventList(CPAP_Snore, EVL_Event);
        } else {
            // If there is a double up, null out the earlier session
            // otherwise there will be a crash on shutdown.
            for (int z = 0; z < SessionStart.size(); z++) {
                if (SessionStart[z] == (quint32)sid) {
                    SessionStart[z] = 0;
                    SessionEnd[z] = 0;
                    break;
                }
            }

            QDateTime d = QDateTime::fromTime_t(sid);
            qDebug() << sid << "has double ups" << d;
            /*Session *sess=Sessions[sid];
            Sessions.erase(Sessions.find(sid));
            delete sess;
            SessionStart[i]=0;
            SessionEnd[i]=0; */
        }
    }

    long pos = 0;

    for (int i = 0; i < recs; i++) {
        // convert timestamp to real epoch
        ts1 = ((m_buffer[pos] << 24) | (m_buffer[pos + 1] << 16) | (m_buffer[pos + 2] << 8) | m_buffer[pos
                + 3]) + ep;

        for (int j = 0; j < SessionStart.size(); j++) {
            sid = SessionStart[j];

            if (!sid) { continue; }

            if ((ts1 >= (quint32)sid) && (ts1 <= SessionEnd[j])) {
                Session *sess = Sessions[sid];
                qint64 time = quint64(ts1) * 1000L;
                sess->eventlist[CPAP_Pressure][0]->AddEvent(time, m_buffer[pos + 0xd] / 10.0); // current pressure
                sess->eventlist[CPAP_EPAP][0]->AddEvent(time, m_buffer[pos + 0x13] / 10.0); // epap / low
                sess->eventlist[CPAP_IPAP][0]->AddEvent(time, m_buffer[pos + 0x14] / 10.0); // ipap / high

                sess->eventlist[CPAP_LeakTotal][0]->AddEvent(time, m_buffer[pos + 0x7]); // "Average Leak"
                sess->eventlist[CPAP_MaxLeak][0]->AddEvent(time, m_buffer[pos + 0x6]); // "Max Leak"

                int rr = m_buffer[pos + 0xa];
                sess->eventlist[CPAP_RespRate][0]->AddEvent(time, rr); // Respiratory Rate
                sess->eventlist[INTELLIPAP_Unknown1][0]->AddEvent(time, m_buffer[pos + 0xf]); //
                sess->eventlist[INTELLIPAP_Unknown1][0]->AddEvent(time, m_buffer[pos + 0xc]);

                sess->eventlist[CPAP_Snore][0]->AddEvent(time, m_buffer[pos + 0x4]); //4/5??

                // 0x0f == Leak Event
                // 0x04 == Snore?
                if (m_buffer[pos + 0xf] > 0) { // Leak Event
                    if (!sess->eventlist.contains(CPAP_LeakFlag)) {
                        sess->AddEventList(CPAP_LeakFlag, EVL_Event);
                    }

                    sess->eventlist[CPAP_LeakFlag][0]->AddEvent(time, m_buffer[pos + 0xf]);
                }

                if (m_buffer[pos + 0x5] > 4) { // This matches Exhale Puff.. not sure why 4
                    if (!sess->eventlist.contains(CPAP_ExP)) {
                        sess->AddEventList(CPAP_ExP, EVL_Event);
                    }

                    for (int q = 0; q < m_buffer[pos + 0x5]; q++) {
                        sess->eventlist[CPAP_ExP][0]->AddEvent(time, m_buffer[pos + 0x5]);
                    }
                }

                if (m_buffer[pos + 0x10] > 0) {
                    if (!sess->eventlist.contains(CPAP_Obstructive)) {
                        sess->AddEventList(CPAP_Obstructive, EVL_Event);
                    }

                    for (int q = 0; q < m_buffer[pos + 0x10]; q++) {
                        sess->eventlist[CPAP_Obstructive][0]->AddEvent(time, m_buffer[pos + 0x10]);
                    }
                }

                if (m_buffer[pos + 0x11] > 0) {
                    if (!sess->eventlist.contains(CPAP_Hypopnea)) {
                        sess->AddEventList(CPAP_Hypopnea, EVL_Event);
                    }

                    for (int q = 0; q < m_buffer[pos + 0x11]; q++) {
                        sess->eventlist[CPAP_Hypopnea][0]->AddEvent(time, m_buffer[pos + 0x11]);
                    }
                }

                if (m_buffer[pos + 0x12] > 0) { // NRI // is this == to RERA?? CA??
                    if (!sess->eventlist.contains(CPAP_NRI)) {
                        sess->AddEventList(CPAP_NRI, EVL_Event);
                    }

                    for (int q = 0; q < m_buffer[pos + 0x12]; q++) {
                        sess->eventlist[CPAP_NRI][0]->AddEvent(time, m_buffer[pos + 0x12]);
                    }
                }

                quint16 tv = (m_buffer[pos + 0x8] << 8) | m_buffer[pos + 0x9]; // correct

                sess->eventlist[CPAP_TidalVolume][0]->AddEvent(time, tv);

                EventDataType mv = tv * rr; // MinuteVent=TidalVolume * Respiratory Rate
                sess->eventlist[CPAP_MinuteVent][0]->AddEvent(time, mv / 1000.0);
                break;
            }

        }

        pos += 26;
    }

    for (int i = 0; i < SessionStart.size(); i++) {
        SessionID sid = SessionStart[i];

        if (sid) {
            sess = Sessions[sid];
            //if (sess->eventlist.size()==0) {
            //   delete sess;
            //   continue;
            //}
            quint64 first = qint64(sid) * 1000L;
            quint64 last = qint64(SessionEnd[i]) * 1000L;

            sess->settings[CPAP_PresReliefType] = (PRTypes)PR_SMARTFLEX;
            int i = set1["SmartFlex"].toInt();
            sess->settings[CPAP_PresReliefSet] = i;
            int sfm = set1["SmartFlexMode"].toInt();

            if (sfm == 0) {
                sess->settings[CPAP_PresReliefMode] = PM_FullTime;
            } else {
                sess->settings[CPAP_PresReliefMode] = PM_RampOnly;
            }

            EventDataType max = sess->Max(CPAP_IPAP);
            EventDataType min = sess->Min(CPAP_EPAP);
            EventDataType pres = sess->Min(CPAP_Pressure);

            if (max == min) {
                sess->settings[CPAP_Mode] = (int)MODE_CPAP;
                sess->settings[CPAP_Pressure] = min;
            } else {
                sess->settings[CPAP_Mode] = (int)MODE_APAP;
                sess->settings[CPAP_PressureMin] = min;
                sess->settings[CPAP_PressureMax] = max;
            }

            sess->eventlist.erase(sess->eventlist.find(CPAP_IPAP));
            sess->eventlist.erase(sess->eventlist.find(CPAP_EPAP));
            sess->m_min.erase(sess->m_min.find(CPAP_EPAP));
            sess->m_max.erase(sess->m_max.find(CPAP_EPAP));

            if (pres < min) {
                sess->settings[CPAP_RampPressure] = pres;
            }

            //quint64 len=last-first;
            //if (len>0) {
            //if (!sess->first()) {
            sess->set_first(first);
            sess->set_last(last);
            // }
            sess->UpdateSummaries();
            mach->AddSession(sess, profile);
            /*} else {
                delete sess;
            }*/
        }
    }

    mach->properties[STR_PROP_DataVersion] = QString().sprintf("%i", intellipap_data_version);

    mach->Save();

    delete [] m_buffer;

    if (qprogress) { qprogress->setValue(100); }

    f.close();

    return 1;
}

Machine *IntellipapLoader::CreateMachine(QString serial, Profile *profile)
{
    if (!profile) {
        return nullptr;
    }

    qDebug() << "Create Machine " << serial;

    QList<Machine *> ml = profile->GetMachines(MT_CPAP);
    bool found = false;
    QList<Machine *>::iterator i;
    Machine *m = nullptr;

    for (i = ml.begin(); i != ml.end(); i++) {
        if (((*i)->GetClass() == intellipap_class_name) && ((*i)->properties[STR_PROP_Serial] == serial)) {
            MachList[serial] = *i; //static_cast<CPAP *>(*i);
            found = true;
            m = *i;
            break;
        }
    }

    if (!found) {
        m = new Intellipap(profile, 0);
    }

    m->properties[STR_PROP_Brand] = "DeVilbiss";
    m->properties[STR_PROP_Series] = STR_MACH_Intellipap;

    if (found) {
        return m;
    }


    MachList[serial] = m;
    profile->AddMachine(m);

    m->properties[STR_PROP_Serial] = serial;
    m->properties[STR_PROP_DataVersion] = QString::number(intellipap_data_version);

    QString path = "{" + STR_GEN_DataFolder + "}/" + m->GetClass() + "_" + serial + "/";
    m->properties[STR_PROP_Path] = path;
    m->properties[STR_PROP_BackupPath] = path + "Backup/";

    return m;
}


bool intellipap_initialized = false;
void IntellipapLoader::Register()
{
    if (intellipap_initialized) { return; }

    qDebug() << "Registering IntellipapLoader";
    RegisterLoader(new IntellipapLoader());
    //InitModelMap();
    intellipap_initialized = true;
}
