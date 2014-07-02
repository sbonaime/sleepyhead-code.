/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib PRS1 Loader Implementation
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
#include <QDataStream>
#include <QMessageBox>
#include <QProgressBar>
#include <QDebug>
#include <cmath>
#include "SleepLib/schema.h"
#include "prs1_loader.h"
#include "SleepLib/session.h"
#include "SleepLib/calcs.h"


//const int PRS1_MAGIC_NUMBER = 2;
//const int PRS1_SUMMARY_FILE=1;
//const int PRS1_EVENT_FILE=2;
//const int PRS1_WAVEFORM_FILE=5;


//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the prs1_data_version in prs1_loader.h when making changes to this loader
// that change loader behaviour or modify channels.
//********************************************************************************************


extern QProgressBar *qprogress;

QHash<int, QString> ModelMap;

#define PRS1_CRC_CHECK

#ifdef PRS1_CRC_CHECK
typedef quint16 crc_t;

static const crc_t crc_table[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

crc_t CRC16(const unsigned char *data, size_t data_len)
{
    crc_t crc = 0;
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = (crc ^ *data) & 0xff;
        crc = (crc_table[tbl_idx] ^ (crc >> 8)) & 0xffff;

        data++;
    }

    return crc & 0xffff;
}
#endif

PRS1::PRS1(Profile *p, MachineID id): CPAP(p, id)
{
    m_class = prs1_class_name;
}
PRS1::~PRS1()
{

}


/*! \struct WaveHeaderList
    \brief Used in PRS1 Waveform Parsing */
struct WaveHeaderList {
    quint16 interleave;
    quint8  sample_format;
    WaveHeaderList(quint16 i, quint8 f) { interleave = i; sample_format = f; }
};


PRS1Loader::PRS1Loader()
{
    // Todo: Register PRS1 custom channels

    //genCRCTable();
    m_buffer = nullptr;
    m_type = MT_CPAP;
}

PRS1Loader::~PRS1Loader()
{
}
Machine *PRS1Loader::CreateMachine(QString serial, Profile *profile)
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
        if (((*i)->GetClass() == STR_MACH_PRS1) && ((*i)->properties[STR_PROP_Serial] == serial)) {
            PRS1List[serial] = *i; //static_cast<CPAP *>(*i);
            found = true;
            m = *i;
            break;
        }
    }

    if (!found) {
        m = new PRS1(profile, 0);
    }

    m->properties[STR_PROP_Brand] = "Philips Respironics";
    m->properties[STR_PROP_Series] = "System One";

    if (found) {
        return m;
    }

    PRS1List[serial] = m;
    profile->AddMachine(m);

    m->properties[STR_PROP_Serial] = serial;
    m->properties[STR_PROP_DataVersion] = QString::number(prs1_data_version);

    QString path = "{" + STR_GEN_DataFolder + "}/" + m->GetClass() + "_" + serial + "/";
    m->properties[STR_PROP_Path] = path;
    m->properties[STR_PROP_BackupPath] = path + "Backup/";

    return m;
}
bool isdigit(QChar c)
{
    if ((c >= '0') && (c <= '9')) { return true; }

    return false;
}

const QString PR_STR_PSeries = "P-Series";


// Tests path to see if it has (what looks like) a valid PRS1 folder structure
bool PRS1Loader::Detect(const QString & path)
{
    QString newpath = path;

    newpath.replace("\\", "/");

    if (!newpath.endsWith("/" + PR_STR_PSeries)) {
        newpath = path + "/" + PR_STR_PSeries;
    }

    QDir dir(newpath);

    if ((!dir.exists() || !dir.isReadable())) {
        return false;
    }
    qDebug() << "PRS1Loader::Detect path=" << newpath;

    QFile lastfile(newpath+"/last.txt");
    if (!lastfile.exists()) {
        return false;
    }

    if (!lastfile.open(QIODevice::ReadOnly)) {
        qDebug() << "PRS1Loader: last.txt exists but I couldn't open it!";
        return false;
    }

    QString last = lastfile.readLine(64);
    last = last.trimmed();
    lastfile.close();

    QFile f(newpath+"/"+last);
    if (!f.exists()) {
        qDebug() << "in PRS1Loader::Detect():" << last << "does not exist, despite last.txt saying it does";
        return false;
    }
    // newpath is a valid path
    return true;
}

int PRS1Loader::Open(QString path, Profile *profile)
{
    QString newpath;
    path = path.replace("\\", "/");

    if (path.endsWith("/" + PR_STR_PSeries)) {
        newpath = path;
    } else {
        newpath = path + "/" + PR_STR_PSeries;
    }

    qDebug() << "PRS1Loader::Open path=" << newpath;

    QDir dir(newpath);

    if ((!dir.exists() || !dir.isReadable())) {
        return 0;
    }

    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist = dir.entryInfoList();

    QList<QString> SerialNumbers;
    QList<QString>::iterator sn;

    for (int i = 0; i < flist.size(); i++) {
        QFileInfo fi = flist.at(i);
        QString filename = fi.fileName();

        if ((filename[0] == 'P') && (isdigit(filename[1])) && (isdigit(filename[2]))) {
            SerialNumbers.push_back(filename);
        } else if (filename.toLower() == "last.txt") { // last.txt points to the current serial number
            QString file = fi.canonicalFilePath();
            QFile f(file);

            if (!fi.isReadable()) {
                qDebug() << "PRS1Loader: last.txt exists but I couldn't read it!";
                continue;
            }

            if (!f.open(QIODevice::ReadOnly)) {
                qDebug() << "PRS1Loader: last.txt exists but I couldn't open it!";
                continue;
            }

            last = f.readLine(64);
            last = last.trimmed();
            f.close();
        }
    }

    if (SerialNumbers.empty()) { return 0; }

    m_buffer = new unsigned char [max_load_buffer_size]; //allocate once and reuse.
    Machine *m;

    for (sn = SerialNumbers.begin(); sn != SerialNumbers.end(); sn++) {
        QString s = *sn;
        m = CreateMachine(s, profile);

        try {
            if (m) {
                OpenMachine(m, newpath + "/" + (*sn), profile);
            }
        } catch (OneTypePerDay e) {
            Q_UNUSED(e)
            profile->DelMachine(m);
            PRS1List.erase(PRS1List.find(s));
            QMessageBox::warning(nullptr, QObject::tr("Import Error"),
                                 QObject::tr("This Machine Record cannot be imported in this profile.\nThe Day records overlap with already existing content."),
                                 QMessageBox::Ok);
            delete m;
        }
    }

    delete [] m_buffer;
    return PRS1List.size();
}

bool PRS1Loader::ParseProperties(Machine *m, QString filename)
{
    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }

    QString line;
    QHash<QString, QString> prop;

    QString s = f.readLine();
    QChar sep = '=';
    QString key, value;

    while (!f.atEnd()) {
        key = s.section(sep, 0, 0); //BeforeFirst(sep);

        if (key == s) { continue; }

        value = s.section(sep, 1).trimmed(); //AfterFirst(sep).Strip();

        if (value == s) { continue; }

        prop[key] = value;
        s = f.readLine();
    }

    bool ok;
    QString pt = prop["ProductType"];
    int i = pt.toInt(&ok, 16);

    if (ok) {
        if (ModelMap.find(i) != ModelMap.end()) {
            m->properties[STR_PROP_Model] = ModelMap[i];
        }
    }

    if (prop["SerialNumber"] != m->properties[STR_PROP_Serial]) {
        qDebug() << "Serial Number in PRS1 properties.txt doesn't match directory structure";
    } else { prop.erase(prop.find("SerialNumber")); } // already got it stored.

    for (QHash<QString, QString>::iterator i = prop.begin(); i != prop.end(); i++) {
        m->properties[i.key()] = i.value();
    }

    f.close();
    return true;
}

void copyPath(QString src, QString dst)
{
    QDir dir(src);
    if (!dir.exists())
        return;

    // Recursively handle directories
    foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString dst_path = dst + QDir::separator() + d;
        dir.mkpath(dst_path);
        copyPath(src + QDir::separator() + d, dst_path);
    }

    // Files
    foreach (QString f, dir.entryList(QDir::Files)) {
        QString srcFile = src + QDir::separator() + f;
        QString destFile = dst + QDir::separator() + f;

        if (!QFile::exists(destFile)) {
            QFile::copy(srcFile, destFile);
        }
    }
}


int PRS1Loader::OpenMachine(Machine *m, QString path, Profile *profile)
{
    Q_UNUSED(profile)

    qDebug() << "Opening PRS1 " << path;
    QDir dir(path);

    if (!dir.exists() || (!dir.isReadable())) {
        return 0;
    }
    QString backupPath = p_profile->Get(m->properties[STR_PROP_BackupPath]) + path.section("/", -2);
    copyPath(path, backupPath);

    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist = dir.entryInfoList();

    QString filename;

    if (qprogress) { qprogress->setValue(0); }

    QStringList paths;

    for (int i = 0; i < flist.size(); i++) {
        QFileInfo fi = flist.at(i);
        filename = fi.fileName();

        if ((filename[0].toLower() == 'p') && (isdigit(filename[1]))) {
            // p0, p1, p2.. etc.. folders contain the session data
            paths.push_back(fi.canonicalFilePath());
        } else if (filename.toLower() == "properties.txt") {
            ParseProperties(m, fi.canonicalFilePath());
        } else if (filename.toLower() == "e") {
            // Error files..
            // Reminder: I have been given some info about these. should check it over.
        }
    }

    QString modelstr = m->properties["ModelNumber"];

    if (modelstr.endsWith("P"))
        modelstr.chop(1);

    bool ok;
    int model = modelstr.toInt(&ok);
    if (ok) {
        // Assumption is made here all PRS1 machines less than 450P are not data capable.. this could be wrong one day.
        if ((model < 450)) {
            QMessageBox::information(NULL,
                                     QObject::tr("Non Data Capable Machine"),
                                     QString(QObject::tr("Your Philips Respironics CPAP machine (Model %1) is unfortunately not a data capable model.")+"\n\n"+
                                             QObject::tr("I'm sorry to report that SleepyHead can only track hours of use for this machine.")).
                                     arg(m->properties["ModelNumber"]),QMessageBox::Ok);

        }

        if ((model % 100) >= 60) {
            // 60 series machine warning??
        }

    } else {
        // model number didn't parse.. Meh...
    }

    SessionID sid;
    long ext;
    QHash<SessionID, QStringList> sessfiles;
    int size = paths.size();

    prs1sessions.clear();
    new_sessions.clear(); // this hash is used by OpenFile

    // for each p0/p1/p2/etc... folder
    for (int p=0; p < size; ++p) {
        dir.setPath(paths.at(p));

        if (!dir.exists() || !dir.isReadable()) { continue; }

        flist = dir.entryInfoList();

        // Scan for individual session files
        for (int i = 0; i < flist.size(); i++) {
            QFileInfo fi = flist.at(i);
            QString ext_s = fi.fileName().section(".", -1);
            QString session_s = fi.fileName().section(".", 0, -2);

            ext = ext_s.toLong(&ok);
            if (!ok) { // not a numerical extension
                continue;
            }

            sid = session_s.toLong(&ok);
            if (!ok) { // not a numerical session ID
                continue;
            }

            if (m->SessionExists(sid)) {
                // Skip already imported session
                continue;
            }

            PRS1FileGroup * group = nullptr;

            QHash<SessionID, PRS1FileGroup*>::iterator it = prs1sessions.find(sid);
            if (it != prs1sessions.end()) {
                group = it.value();
            } else {
                group = new PRS1FileGroup();
                prs1sessions[sid] = group;
                // save a loop an que this now
                queTask(new PRS1Import(this, sid, group, m));
           }


            switch (ext) {
            case 0:
                group->compliance = fi.canonicalFilePath();
                break;
            case 1:
                group->summary = fi.canonicalFilePath();
                break;
            case 2:
                group->event = fi.canonicalFilePath();
                break;
            case 5:
                group->waveform = fi.canonicalFilePath();
                break;
            default:
                break;
            }
        }
    }

    int tasks = countTasks();
    runTasks(p_profile->session->multithreading());
    return tasks;
}

bool PRS1SessionData::ParseF5Events()
{
    ChannelID Codes[] = {
        PRS1_00, PRS1_01, CPAP_Pressure, CPAP_EPAP, CPAP_PressurePulse, CPAP_Obstructive,
        CPAP_ClearAirway, CPAP_Hypopnea, PRS1_08,  CPAP_FlowLimit, PRS1_0A, CPAP_CSR,
        PRS1_0C, CPAP_VSnore, PRS1_0E, PRS1_0F,
        CPAP_LargeLeak, // Large leak apparently
        CPAP_LeakTotal, PRS1_12
    };

    int ncodes = sizeof(Codes) / sizeof(ChannelID);
    EventList *Code[0x20] = {nullptr};

    EventList *OA = session->AddEventList(CPAP_Obstructive, EVL_Event);
    EventList *HY = session->AddEventList(CPAP_Hypopnea, EVL_Event);
    EventList *CSR = session->AddEventList(CPAP_CSR, EVL_Event);
    EventList *LEAK = session->AddEventList(CPAP_LeakTotal, EVL_Event);
    EventList *SNORE = session->AddEventList(CPAP_Snore, EVL_Event);
    EventList *IPAP = session->AddEventList(CPAP_IPAP, EVL_Event, 0.1F);
    EventList *EPAP = session->AddEventList(CPAP_EPAP, EVL_Event, 0.1F);
    EventList *PS = session->AddEventList(CPAP_PS, EVL_Event, 0.1F);
    EventList *IPAPLo = session->AddEventList(CPAP_IPAPLo, EVL_Event, 0.1F);
    EventList *IPAPHi = session->AddEventList(CPAP_IPAPHi, EVL_Event, 0.1F);
    EventList *RR = session->AddEventList(CPAP_RespRate, EVL_Event);
    EventList *PTB = session->AddEventList(CPAP_PTB, EVL_Event);

    EventList *MV = session->AddEventList(CPAP_MinuteVent, EVL_Event);
    EventList *TV = session->AddEventList(CPAP_TidalVolume, EVL_Event, 10.0F);

    EventList *CA = nullptr; //session->AddEventList(CPAP_ClearAirway, EVL_Event);
    EventList *VS = nullptr, * FL = nullptr; //,* RE=nullptr,* VS2=nullptr;

    //EventList * PRESSURE=nullptr;
    //EventList * PP=nullptr;

    EventDataType data[10];//,tmp;


    //qint64 start=timestamp;
    qint64 t = qint64(event->timestamp) * 1000L;
    session->updateFirst(t);
    qint64 tt;
    int pos = 0;
    int cnt = 0;
    short delta;//,duration;
    QDateTime d;
    bool badcode = false;
    unsigned char lastcode3 = 0, lastcode2 = 0, lastcode = 0, code = 0;
    int lastpos = 0, startpos = 0, lastpos2 = 0, lastpos3 = 0;

    int size = event->m_data.size();
    unsigned char * buffer = (unsigned char *)event->m_data.data();

    while (pos < size) {
        lastcode3 = lastcode2;
        lastcode2 = lastcode;
        lastcode = code;
        lastpos3 = lastpos2;
        lastpos2 = lastpos;
        lastpos = startpos;
        startpos = pos;
        code = buffer[pos++];

        if (code >= ncodes) {
            qDebug() << "Illegal PRS1 code " << hex << int(code) << " appeared at " << hex << startpos;
            qDebug() << "1: (" << int(lastcode) << hex << lastpos << ")";
            qDebug() << "2: (" << int(lastcode2) << hex << lastpos2 << ")";
            qDebug() << "3: (" << int(lastcode3) << hex << lastpos3 << ")";
            return false;
        }

        if (code == 0) {
        } else if (code != 0x12) {
            delta = buffer[pos];
            //duration=buffer[pos+1];
            //delta=buffer[pos+1] << 8 | buffer[pos];
            pos += 2;
            t += qint64(delta) * 1000L;
        }

        ChannelID cpapcode = Codes[(int)code];
        //EventDataType PS;
        tt = t;
        cnt++;
        int fc = 0;

        switch (code) {
        case 0x00: // Unknown (ASV Pressure value)
            // offset?
            data[0] = buffer[pos++];
            fc++;

            if (!buffer[pos - 1]) { // WTH???
                data[1] = buffer[pos++];
                fc++;
            }

            if (!buffer[pos - 1]) {
                data[2] = buffer[pos++];
                fc++;
            }

            break;

        case 0x01: // Unknown
            if (!Code[1]) {
                if (!(Code[1] = session->AddEventList(cpapcode, EVL_Event, 0.1F))) { return false; }
            }

            Code[1]->AddEvent(t, 0);
            break;

        case 0x02: // Pressure ???
            data[0] = buffer[pos++];
            //            if (!Code[2]) {
            //                if (!(Code[2]=session->AddEventList(cpapcode,EVL_Event,0.1))) return false;
            //            }
            //            Code[2]->AddEvent(t,data[0]);
            break;

        case 0x04: // Pressure Pulse??
            data[0] = buffer[pos++];

            if (!Code[3]) {
                if (!(Code[3] = session->AddEventList(cpapcode, EVL_Event))) { return false; }
            }

            Code[3]->AddEvent(t, data[0]);
            break;

        case 0x05:
            //code=CPAP_Obstructive;
            data[0] = buffer[pos++];
            tt -= qint64(data[0]) * 1000L; // Subtract Time Offset
            OA->AddEvent(tt, data[0]);
            break;

        case 0x06:
            //code=CPAP_ClearAirway;
            data[0] = buffer[pos++];
            tt -= qint64(data[0]) * 1000L; // Subtract Time Offset

            if (!CA) {
                if (!(CA = session->AddEventList(cpapcode, EVL_Event))) { return false; }
            }

            CA->AddEvent(tt, data[0]);
            break;

        case 0x07:
            //code=CPAP_Hypopnea;
            data[0] = buffer[pos++];
            tt -= qint64(data[0]) * 1000L; // Subtract Time Offset
            HY->AddEvent(tt, data[0]);
            break;

        case 0x08: // ???
            data[0] = buffer[pos++];
            tt -= qint64(data[0]) * 1000L; // Subtract Time Offset
            qDebug() << "Code 8 found at " << hex << pos - 1 << " " << tt;

            if (!Code[10]) {
                if (!(Code[10] = session->AddEventList(cpapcode, EVL_Event))) { return false; }
            }

            //????
            //data[1]=buffer[pos++]; // ???
            Code[10]->AddEvent(tt, data[0]);
            pos++;
            break;

        case 0x09: // ASV Codes
            //code=CPAP_FlowLimit;
            data[0] = buffer[pos++];
            tt -= qint64(data[0]) * 1000L; // Subtract Time Offset

            if (!FL) {
                if (!(FL = session->AddEventList(cpapcode, EVL_Event))) { return false; }
            }

            FL->AddEvent(tt, data[0]);

            break;

        case 0x0a:
            data[0] = buffer[pos++];
            tt -= qint64(data[0]) * 1000L; // Subtract Time Offset

            if (!Code[7]) {
                if (!(Code[7] = session->AddEventList(cpapcode, EVL_Event))) { return false; }
            }

            Code[7]->AddEvent(tt, data[0]);
            break;


        case 0x0b: // Cheyne Stokes
            data[0] = ((unsigned char *)buffer)[pos + 1] << 8 | ((unsigned char *)buffer)[pos];
            //data[0]*=2;
            pos += 2;
            data[1] = ((unsigned char *)buffer)[pos]; //|buffer[pos+1] << 8
            pos += 1;
            //tt-=delta;
            tt -= qint64(data[1]) * 1000L;

            if (!CSR) {
                if (!(CSR = session->AddEventList(cpapcode, EVL_Event))) {
                    qDebug() << "!CSR addeventlist exit";
                    return false;
                }
            }

            CSR->AddEvent(tt, data[0]);
            break;

        case 0x0c:
            data[0] = buffer[pos++];
            tt -= qint64(data[0]) * 1000L; // Subtract Time Offset
            qDebug() << "Code 12 found at " << hex << pos - 1 << " " << tt;

            if (!Code[8]) {
                if (!(Code[8] = session->AddEventList(cpapcode, EVL_Event))) { return false; }
            }

            Code[8]->AddEvent(tt, data[0]);
            pos += 2;
            break;

        case 0x0d: // All the other ASV graph stuff.
            IPAP->AddEvent(t, data[0] = buffer[pos++]); // 00=IAP
            data[4] = buffer[pos++];
            IPAPLo->AddEvent(t, data[4]);               // 01=IAP Low
            data[5] = buffer[pos++];
            IPAPHi->AddEvent(t, data[5]);               // 02=IAP High
            LEAK->AddEvent(t, buffer[pos++]);           // 03=LEAK
            RR->AddEvent(t, buffer[pos++]);             // 04=Breaths Per Minute
            PTB->AddEvent(t, buffer[pos++]);            // 05=Patient Triggered Breaths
            MV->AddEvent(t, buffer[pos++]);             // 06=Minute Ventilation
            //tmp=buffer[pos++] * 10.0;
            TV->AddEvent(t, buffer[pos++]);             // 07=Tidal Volume
            SNORE->AddEvent(t, data[2] = buffer[pos++]); // 08=Snore

            if (data[2] > 0) {
                if (!VS) {
                    if (!(VS = session->AddEventList(CPAP_VSnore, EVL_Event))) {
                        qDebug() << "!VS eventlist exit";
                        return false;
                    }
                }

                VS->AddEvent(t, 0); //data[2]); // VSnore
            }

            EPAP->AddEvent(t, data[1] = buffer[pos++]); // 09=EPAP
            data[2] = data[0] - data[1];
            PS->AddEvent(t, data[2]);           // Pressure Support
            if (event->familyVersion >= 1) {
                data[0] = buffer[pos++];

            }
            break;

        case 0x03: // BIPAP Pressure
            qDebug() << "0x03 Observed in ASV data!!????";

            data[0] = buffer[pos++];
            data[1] = buffer[pos++];
            //            data[0]/=10.0;
            //            data[1]/=10.0;
            //            session->AddEvent(new Event(t,CPAP_EAP, 0, data, 1));
            //            session->AddEvent(new Event(t,CPAP_IAP, 0, &data[1], 1));
            break;

        case 0x11: // Not Leak Rate
            qDebug() << "0x11 Observed in ASV data!!????";
            //if (!Code[24]) {
            //   Code[24]=new EventList(cpapcode,EVL_Event);
            //}
            //Code[24]->AddEvent(t,buffer[pos++]);
            break;

        case 0x0e: // Unknown
            qDebug() << "0x0E Observed in ASV data!!????";
            data[0] = buffer[pos++]; // << 8) | buffer[pos];
            //session->AddEvent(new Event(t,cpapcode, 0, data, 1));
            break;

        case 0x10: // Unknown
            qDebug() << "0x10 Observed in ASV data!!????";
            data[0] = buffer[pos++]; // << 8) | buffer[pos];
            data[1] = buffer[pos++];
            data[2] = buffer[pos++];
            //session->AddEvent(new Event(t,cpapcode, 0, data, 3));
            break;

        case 0x0f:
            qDebug() << "0x0f Observed in ASV data!!????";

            data[0] = buffer[pos + 1] << 8 | buffer[pos];
            pos += 2;
            data[1] = buffer[pos]; //|buffer[pos+1] << 8
            pos += 1;
            tt -= qint64(data[1]) * 1000L;
            //session->AddEvent(new Event(tt,cpapcode, 0, data, 2));
            break;

        case 0x12: // Summary
            qDebug() << "0x12 Observed in ASV data!!????";
            data[0] = buffer[pos++];
            data[1] = buffer[pos++];
            data[2] = buffer[pos + 1] << 8 | buffer[pos];
            pos += 2;
            //session->AddEvent(new Event(t,cpapcode, 0, data,3));
            break;

        default:  // ERROR!!!
            qWarning() << "Some new fandangled PRS1 code detected " << hex << int(code) << " at " << pos - 1;
            badcode = true;
            break;
        }

        if (badcode) {
            break;
        }
    }

    session->updateLast(t);
    session->m_cnt.clear();
    session->m_cph.clear();
    session->settings[CPAP_IPAPLo] = session->Min(CPAP_IPAPLo);
    session->settings[CPAP_IPAPHi] = session->Max(CPAP_IPAPHi);
    session->settings[CPAP_PSMax] = session->Max(CPAP_IPAPHi) - session->Min(CPAP_EPAP);
    session->settings[CPAP_PSMin] = session->Min(CPAP_IPAPLo) - session->Min(CPAP_EPAP);

    session->m_valuesummary[CPAP_Pressure].clear();
    session->m_valuesummary.erase(session->m_valuesummary.find(CPAP_Pressure));

    return true;

}

bool PRS1SessionData::ParseF0Events()
{
    unsigned char code=0;
    EventList *Code[0x20] = {0};
    EventDataType data[10];
    int cnt = 0;
    short delta;
    int tdata;
    int pos;
    qint64 t = qint64(event->timestamp) * 1000L, tt;

    session->updateFirst(t);

    EventList *OA = session->AddEventList(CPAP_Obstructive, EVL_Event);
    EventList *HY = session->AddEventList(CPAP_Hypopnea, EVL_Event);
    EventList *CSR = session->AddEventList(CPAP_CSR, EVL_Event);
    EventList *LEAK = session->AddEventList(CPAP_LeakTotal, EVL_Event);
    EventList *SNORE = session->AddEventList(CPAP_Snore, EVL_Event);

    EventList *CA = nullptr; //session->AddEventList(CPAP_ClearAirway, EVL_Event);
    EventList *VS = nullptr;
    EventList *VS2 = nullptr;
    EventList *FL = nullptr;
    EventList *RE = nullptr;

    EventList *PRESSURE = nullptr;
    EventList *EPAP = nullptr;
    EventList *IPAP = nullptr;
    EventList *PS = nullptr;

    EventList *PP = nullptr;

    //session->AddEventList(CPAP_VSnore, EVL_Event);
    //EventList * VS=session->AddEventList(CPAP_Obstructive, EVL_Event);
    unsigned char lastcode3 = 0, lastcode2 = 0, lastcode = 0;
    int lastpos = 0, startpos = 0, lastpos2 = 0, lastpos3 = 0;

    int size = event->m_data.size();
    unsigned char * buffer = (unsigned char *)event->m_data.data();

    for (pos = 0; pos < size;) {
        lastcode3 = lastcode2;
        lastcode2 = lastcode;
        lastcode = code;
        lastpos3 = lastpos2;
        lastpos2 = lastpos;
        lastpos = startpos;
        startpos = pos;
        code = buffer[pos++];

        if (code > 0x12) {
            qDebug() << "Illegal PRS1 code " << hex << int(code) << " appeared at " << hex << startpos;
            qDebug() << "1: (" << hex << int(lastcode) << hex << lastpos << ")";
            qDebug() << "2: (" << hex << int(lastcode2) << hex << lastpos2 << ")";
            qDebug() << "3: (" << hex << int(lastcode3) << hex << lastpos3 << ")";
            return false;
        }

        if (code != 0x12) {
            delta = buffer[pos + 1] << 8 | buffer[pos];
            pos += 2;
            t += qint64(delta) * 1000L;
            tt = t;
        }

        cnt++;

        switch (code) {

        case 0x00: // Unknown 00
            if (!Code[0]) {
                if (!(Code[0] = session->AddEventList(PRS1_00, EVL_Event))) { return false; }
            }

            Code[0]->AddEvent(t, buffer[pos++]);

            if ((event->family == 0) && (event->familyVersion >= 4)) {
                pos++;
            }

            break;

        case 0x01: // Unknown
            if (!Code[1]) {
                if (!(Code[1] = session->AddEventList(PRS1_01, EVL_Event))) { return false; }
            }

            Code[1]->AddEvent(t, 0);

            if ((event->family == 0) && (event->familyVersion >= 4)) {
                if (!PRESSURE) {
                    PRESSURE = session->AddEventList(CPAP_Pressure, EVL_Event, 0.1F);

                    if (!PRESSURE) { return false; }
                }

                PRESSURE->AddEvent(t, buffer[pos++]);
            }

            break;

        case 0x02: // Pressure
            if ((event->family == 0) && (event->familyVersion >= 4)) {  // BiPAP Pressure
                if (!EPAP) {
                    if (!(EPAP = session->AddEventList(CPAP_EPAP, EVL_Event, 0.1F))) { return false; }

                    if (!(IPAP = session->AddEventList(CPAP_IPAP, EVL_Event, 0.1F))) { return false; }

                    if (!(PS = session->AddEventList(CPAP_PS, EVL_Event, 0.1F))) { return false; }
                }

                EPAP->AddEvent(t, data[0] = buffer[pos++]);
                IPAP->AddEvent(t, data[1] = buffer[pos++]);
                PS->AddEvent(t, data[1] - data[0]);
            } else {
                if (!PRESSURE) {
                    PRESSURE = session->AddEventList(CPAP_Pressure, EVL_Event, 0.1F);

                    if (!PRESSURE) { return false; }
                }

                PRESSURE->AddEvent(t, buffer[pos++]);
            }

            break;

        case 0x03: // BIPAP Pressure
            if (!EPAP) {
                if (!(EPAP = session->AddEventList(CPAP_EPAP, EVL_Event, 0.1F))) { return false; }

                if (!(IPAP = session->AddEventList(CPAP_IPAP, EVL_Event, 0.1F))) { return false; }

                if (!(PS = session->AddEventList(CPAP_PS, EVL_Event, 0.1F))) { return false; }
            }

            EPAP->AddEvent(t, data[0] = buffer[pos++]);
            IPAP->AddEvent(t, data[1] = buffer[pos++]);
            PS->AddEvent(t, data[1] - data[0]);
            break;

        case 0x04: // Pressure Pulse
            if (!PP) {
                if (!(PP = session->AddEventList(CPAP_PressurePulse, EVL_Event))) { return false; }
            }

            PP->AddEvent(t, buffer[pos++]);
            break;

        case 0x05: // RERA
            data[0] = buffer[pos++];
            tt = t - (qint64(data[0]) * 1000L);

            if (!RE) {
                if (!(RE = session->AddEventList(CPAP_RERA, EVL_Event))) { return false; }
            }

            RE->AddEvent(tt, data[0]);
            break;

        case 0x06: // Obstructive Apoanea
            data[0] = buffer[pos++];
            tt = t - (qint64(data[0]) * 1000L);
            OA->AddEvent(tt, data[0]);
            break;

        case 0x07: // Clear Airway
            data[0] = buffer[pos++];
            tt = t - (qint64(data[0]) * 1000L);

            if (!CA) {
                if (!(CA = session->AddEventList(CPAP_ClearAirway, EVL_Event))) { return false; }
            }

            CA->AddEvent(tt, data[0]);
            break;

        case 0x0a: // Hypopnea
            data[0] = buffer[pos++];
            tt = t - (qint64(data[0]) * 1000L);
            HY->AddEvent(tt, data[0]);
            break;

        case 0x0c: // Flow Limitation
            data[0] = buffer[pos++];
            tt = t - (qint64(data[0]) * 1000L);

            if (!FL) {
                if (!(FL = session->AddEventList(CPAP_FlowLimit, EVL_Event))) { return false; }
            }

            FL->AddEvent(tt, data[0]);
            break;

        case 0x0b: // Hypopnea related code
            data[0] = buffer[pos++];
            data[1] = buffer[pos++];

            if (!Code[12]) {
                if (!(Code[12] = session->AddEventList(PRS1_0B, EVL_Event))) { return false; }
            }

            // FIXME
            Code[12]->AddEvent(t, data[0]);
            break;

        case 0x0d: // Vibratory Snore
            if (!VS) {
                if (!(VS = session->AddEventList(CPAP_VSnore, EVL_Event))) { return false; }
            }

            VS->AddEvent(t, 0);
            break;

        case 0x11: // Leak Rate & Snore Graphs
            data[0] = buffer[pos++];
            data[1] = buffer[pos++];

            LEAK->AddEvent(t, data[0]);
            SNORE->AddEvent(t, data[1]);

            if (data[1] > 0) {
                if (!VS2) {
                    if (!(VS2 = session->AddEventList(CPAP_VSnore2, EVL_Event))) { return false; }
                }

                VS2->AddEvent(t, data[1]);
            }

            if ((event->family == 0) && (event->familyVersion >= 4)) {
                pos++;
            }

            break;

        case 0x0e: // Unknown
            data[0] = ((char *)buffer)[pos++];
            data[1] = buffer[pos++]; //(buffer[pos+1] << 8) | buffer[pos];
            //data[0]/=10.0;
            //pos+=2;
            data[2] = buffer[pos++];

            if (!Code[17]) {
                if (!(Code[17] = session->AddEventList(PRS1_0E, EVL_Event))) { return false; }
            }

            tdata = unsigned(data[1]) << 8 | unsigned(data[0]);
            Code[17]->AddEvent(t, tdata);
            //qDebug() << hex << data[0] << data[1] << data[2];
            //session->AddEvent(new Event(t,cpapcode, 0, data, 3));
            //tt-=data[1]*1000;
            //session->AddEvent(new Event(t,CPAP_CSR, data, 2));
            break;

        case 0x10: // Unknown, Large Leak
            data[0] = buffer[pos + 1] << 8 | buffer[pos];
            pos += 2;
            data[1] = buffer[pos++];

            if (!Code[20]) {
                if (!(Code[20] = session->AddEventList(CPAP_LargeLeak, EVL_Event))) { return false; }
            }

            tt = t - qint64(data[1]) * 1000L;
            Code[20]->AddEvent(tt, data[0]);
            break;

        case 0x0f: // Cheyne Stokes Respiration
            data[0] = buffer[pos + 1] << 8 | buffer[pos];
            pos += 2;
            data[1] = buffer[pos++];
            tt = t - qint64(data[1]) * 1000L;
            CSR->AddEvent(tt, data[0]);
            break;

        case 0x12: // Summary
            data[0] = buffer[pos++];
            data[1] = buffer[pos++];
            data[2] = buffer[pos + 1] << 8 | buffer[pos];
            pos += 2;

            if (!Code[24]) {
                if (!(Code[24] = session->AddEventList(PRS1_12, EVL_Event))) { return false; }
            }

            Code[24]->AddEvent(t, data[0]);
            break;

        default:
            // ERROR!!!
            qWarning() << "Some new fandangled PRS1 code detected in" << event->sessionid << hex
                       << int(code) << " at " << pos - 1;
            return false;
        }
    }

    session->updateLast(t);
    session->m_cnt.clear();
    session->m_cph.clear();
    session->m_lastchan.clear();
    session->m_firstchan.clear();
    session->m_valuesummary[CPAP_Pressure].clear();
    session->m_valuesummary.erase(session->m_valuesummary.find(CPAP_Pressure));

    return true;
}


bool PRS1SessionData::ParseCompliance()
{
    if (!compliance) return false;
    return true;
}

bool PRS1SessionData::ParseSummary()
{
    if (!summary) return false;
    if (summary->m_data.size() < 59) {
        return false;
    }
    const unsigned char * data = (unsigned char *)summary->m_data.constData();

    session->set_first(qint64(summary->timestamp) * 1000L);

    //////////////////////////////////////////////////////////////////////////////////////////
    // ASV Codes (Family 5) Recheck 17/10/2013
    // These are all confirmed off Encore reports

    //cpapmax=EventDataType(data[0x02])/10.0;   // Max Pressure in ASV machines
    //minepap=EventDataType(data[0x03])/10.0;   // Min EPAP
    //maxepap=EventDataType(data[0x04])/10.0;   // Max EPAP
    //minps=EventDataType(data[0x05])/10.0      // Min Pressure Support
    //maxps=EventDataType(data[0x06])/10.0      // Max Pressure Support

    //duration=data[0x1B] | data[0x1C] << 8)  // Session length in seconds

    //epap90=EventDataType(data[0x21])/10.0;    // EPAP 90%
    //epapavg=EventDataType(data[0x22])/10.0;   // EPAP Average
    //ps90=EventDataType(data[0x23])/10.0;      // Pressure Support 90%
    //psavg=EventDataType(data[0x24])/10.0;     // Pressure Support Average

    //TODO: minpb=data[0x] | data[0x] << 8;           // Minutes in PB
    //TODO: minleak=data[0x] | data[0x] << 8;         // Minutes in Large Leak
    //TODO: oa_cnt=data[0x] | data[0x] << 8;          // Obstructive events count

    //ca_cnt=data[0x2d] | data[0x2e] << 8;      // Clear Airway Events count
    //h_cnt=data[0x2f] | data[0x30] << 8;       // Hypopnea events count
    //fl_cnt=data[0x33] | data[0x34] << 8;      // Flow Limitation events count

    //avg_leak=EventDataType(data[0x35]);       // Average Leak
    //avgptb=EventDataType(data[0x36]);         // Average Patient Triggered Breaths %
    //avgbreathrate=EventDataType(data[0x37]);  // Average Breaths Per Minute
    //avgminvent=EventDataType(data[0x38]);     // Average Minute Ventilation
    //avg_tidalvol=EventDataType(data[0x39])*10.0;  // Average Tidal Volume
    //////////////////////////////////////////////////////////////////////////////////////////


    //quint8 rectype = data[0x00];



    EventDataType max, min;

    min = float(data[0x03]) / 10.0; // Min EPAP
    max = float(data[0x04]) / 10.0; // Max EPAP
    int offset = 0;
    int duration = 0;

    // This is a time value for ASV stuff
    if (summary->family == 5) {
        offset = 4; // non zero adds 4 extra fields..

        if (summary->familyVersion == 0) {
            duration = data[0x1B] | data[0x1C] << 8;
        }
    } else if (summary->family == 0) {

        if (summary->familyVersion == 2) {
            duration = data[0x14] | data[0x15] << 8;
        }
        if (summary->familyVersion >= 4) {
            offset = 2;
        }

    }

    if (duration > 0) {
        session->set_last(qint64(summary->timestamp + duration) * 1000L);
    }
    if (!event) {
        session->setSummaryOnly(true);
    }

    // Minutes. Convert to seconds/hours here?
    session->settings[CPAP_RampTime] = (int)data[offset + 0x06];
    session->settings[CPAP_RampPressure] = (EventDataType)data[offset + 0x07] / 10.0;

    if (max > 0) { // Ignoring bipap until we see some more data during import
        session->settings[CPAP_Mode] = (summary->family == 5) ? (int)MODE_ASV : (int)MODE_APAP;

        session->settings[CPAP_PressureMin] = (EventDataType)min;
        session->settings[CPAP_PressureMax] = (EventDataType)max;
    } else {
        session->settings[CPAP_Mode] = (int)MODE_CPAP;
        session->settings[CPAP_Pressure] = (EventDataType)min;
    }

    if (data[offset + 0x08] & 0x80) { // Flex Setting
        if (data[offset + 0x08] & 0x08) {
            if (max > 0) {
                if (summary->family == 5) {
                    session->settings[CPAP_PresReliefType] = (int)PR_BIFLEX;
                } else {
                    session->settings[CPAP_PresReliefType] = (int)PR_AFLEX;
                }
            } else { session->settings[CPAP_PresReliefType] = (int)PR_CFLEXPLUS; }
        } else { session->settings[CPAP_PresReliefType] = (int)PR_CFLEX; }
    } else { session->settings[CPAP_PresReliefType] = (int)PR_NONE; }

    session->settings[CPAP_PresReliefMode] = (int)PM_FullTime; // only has one mode


    session->settings[CPAP_PresReliefSet] = (int)(data[offset + 0x08] & 7);
    session->settings[PRS1_SysLock] = (data[offset + 0x0a] & 0x80) == 0x80;
    session->settings[PRS1_HoseDiam] = ((data[offset + 0x0a] & 0x08) ? "15mm" : "22mm");
    session->settings[PRS1_AutoOff] = (data[offset + 0x0c] & 0x10) == 0x10;
    session->settings[PRS1_MaskAlert] = (data[offset + 0x0c] & 0x08) == 0x08;
    session->settings[PRS1_ShowAHI] = (data[offset + 0x0c] & 0x04) == 0x04;

    if (summary->family == 0 && summary->familyVersion >= 4) {
        if ((data[offset + 0x0a] & 0x04) == 0x04) { // heated tubing off
            session->settings[CPAP_HumidSetting] = (int)data[offset + 0x09] & 0x0f;
        } else {
            session->settings[CPAP_HumidSetting] = (int)(data[offset + 0x09] & 0x30) >> 4;
        }

        session->settings[PRS1_SysOneResistSet] = (int)(data[offset + 0x0b] & 0x38) >> 3;
        /* These should be added to channels, if they are correct(?) */
        /* for now, leave commented out                              */
        /*
        session->settings[PRS1_HeatedTubing]=(data[offset+0x0a]&0x04)!=0x04;
        session->settings[PRS1_HeatedTubingConnected]=(data[offset+0x0b]&0x01)==0x01;
        session->settings[PRS1_HeatedTubingTemp]=(int)(data[offset+0x09]&0x80)>>5
            + (data[offset+0x0a]&0x03);
        */
    } else {
        session->settings[CPAP_HumidSetting] = (int)data[offset + 0x09] & 0x0f;
        session->settings[PRS1_HumidStatus] = (data[offset + 0x09] & 0x80) == 0x80;
        session->settings[PRS1_SysOneResistStat] = (data[offset + 0x0a] & 0x40) == 0x40;
        session->settings[PRS1_SysOneResistSet] = (int)data[offset + 0x0a] & 7;
    }



    // Set recommended Graph values..
    session->setPhysMax(CPAP_LeakTotal, 120);
    session->setPhysMin(CPAP_LeakTotal, 0);
    session->setPhysMax(CPAP_Pressure, 25);
    session->setPhysMin(CPAP_Pressure, 4);
    session->setPhysMax(CPAP_IPAP, 25);
    session->setPhysMin(CPAP_IPAP, 4);
    session->setPhysMax(CPAP_EPAP, 25);
    session->setPhysMin(CPAP_EPAP, 4);
    session->setPhysMax(CPAP_PS, 25);
    session->setPhysMin(CPAP_PS, 0);


    if (summary->family == 0 && summary->familyVersion == 0) {
    }

    return true;
}

bool PRS1SessionData::ParseEvents()
{
    bool res = false;
    if (!event) return false;
    switch (event->family) {
    case 0:
        res = ParseF0Events();
        break;
    case 5:
        res= ParseF5Events();
        break;
    default:
        qDebug() << "Unknown PRS1 familyVersion" << event->familyVersion;
        return false;
    }
    if (res) {
        if (session->count(CPAP_IPAP) > 0) {
            if (session->settings[CPAP_Mode].toInt() != (int)MODE_ASV) {
                session->settings[CPAP_Mode] = MODE_BIPAP;
            }

            if (session->settings[CPAP_PresReliefType].toInt() != PR_NONE) {
                session->settings[CPAP_PresReliefType] = PR_BIFLEX;
            }

            EventDataType min = session->settings[CPAP_PressureMin].toDouble();
            EventDataType max = session->settings[CPAP_PressureMax].toDouble();
            session->settings[CPAP_EPAP] = min;
            session->settings[CPAP_IPAP] = max;

            session->settings[CPAP_PS] = max - min;
            session->settings.erase(session->settings.find(CPAP_PressureMin));
            session->settings.erase(session->settings.find(CPAP_PressureMax));

            session->m_valuesummary.erase(session->m_valuesummary.find(CPAP_Pressure));
            session->m_wavg.erase(session->m_wavg.find(CPAP_Pressure));
            session->m_min.erase(session->m_min.find(CPAP_Pressure));
            session->m_max.erase(session->m_max.find(CPAP_Pressure));
            session->m_gain.erase(session->m_gain.find(CPAP_Pressure));

        } else {

            if (!session->settings.contains(CPAP_Pressure) && !session->settings.contains(CPAP_PressureMin)) {
                session->settings[CPAP_BrokenSummary] = true;

                //session->set_last(session->first());
                if (session->Min(CPAP_Pressure) == session->Max(CPAP_Pressure)) {
                    session->settings[CPAP_Mode] = MODE_CPAP; // no ramp
                    session->settings[CPAP_Pressure] = session->Min(CPAP_Pressure);
                } else {
                    session->settings[CPAP_Mode] = MODE_APAP;
                    session->settings[CPAP_PressureMin] = session->Min(CPAP_Pressure);
                    session->settings[CPAP_PressureMax] = 0; //session->Max(CPAP_Pressure);
                }

                //session->Set("FlexMode",PR_UNKNOWN);

            }
        }

    }
    return res;
}

bool PRS1SessionData::ParseWaveforms()
{
    int size = waveforms.size();

    for (int i=0; i < size; ++i) {
        PRS1DataChunk * waveform = waveforms.at(i);
        int num = waveform->waveformInfo.size();

        int size = waveform->m_data.size();
        if (size == 0) {
            continue;
        }
        quint64 ti = quint64(waveform->timestamp) * 1000L;

        if (num > 1) {
            // Process interleaved samples
            QVector<QByteArray> data;
            data.resize(num);

            int pos = 0;
            do {
                for (int n=0; n < num; n++) {
                    int interleave = waveform->waveformInfo.at(n).interleave;
                    data[n].append(waveform->m_data.mid(pos, interleave));
                    pos += interleave;
                }
            } while (pos < size);

            if (data[0].size() > 0) {
                EventList * flow = session->AddEventList(CPAP_FlowRate, EVL_Waveform, 1.0, 0.0, 0.0, 0.0, (waveform->duration * 1000.0) /  data[0].size());
                flow->AddWaveform(ti, (char *)data[0].data(), data[0].size(), waveform->duration);
            }

            if (data[1].size() > 0) {
                EventList * pres = session->AddEventList(CPAP_MaskPressureHi, EVL_Waveform, 1.0, 0.0, 0.0, 0.0, (waveform->duration * 1000.0) /  data[1].size());
                pres->AddWaveform(ti, (unsigned char *)data[1].data(), data[1].size(), waveform->duration);
            }

        } else {
            // Non interleaved, so can process it much faster
            EventList * flow = session->AddEventList(CPAP_FlowRate, EVL_Waveform, 1.0, 0.0, 0.0, 0.0, (waveform->duration * 1000.0) /  waveform->m_data.size());
            flow->AddWaveform(ti, (char *)waveform->m_data.data(), waveform->m_data.size(), waveform->duration);
        }
    }
    return true;
}

void PRS1Import::run()
{
    group->ParseChunks();

    QMap<SessionID, PRS1SessionData*>::iterator it;

    // Do session lists..
    for (it = group->sessions.begin(); it != group->sessions.end(); ++it) {
        PRS1SessionData * sg = it.value();
        sg->session = new Session(mach, it.key());

        if (!sg->ParseSummary()) {
//            delete sg->session;
//            continue;
        }
        if (!sg->ParseEvents()) {
           // delete sg->session;
           // continue;
        }
        sg->ParseWaveforms();

        if (sg->session->last() < sg->session->first()) {
            // if last isn't set, duration couldn't be gained from summary, parsing events or waveforms..
            // This session is dodgy, so kill it
            sg->session->really_set_last(sg->session->first());
        }
        sg->session->SetChanged(true);

        loader->sessionMutex.lock();
        mach->AddSession(sg->session, p_profile);
        loader->sessionMutex.unlock();

        // Update indexes, process waveform and perform flagging
        sg->session->UpdateSummaries();

        // Save is not threadsafe
        loader->saveMutex.lock();
        sg->session->Store(p_profile->Get(mach->properties[STR_PROP_Path]));
        loader->saveMutex.unlock();

        sg->session->TrashEvents();

        delete sg;
    }
    delete group;
}

void PRS1FileGroup::ParseChunks()
{
//    qDebug() << "Parsing chunks for session" <<  summary << event;
    if (ParseFile(compliance)) {
        // Compliance only piece of crap machine, nothing else to do.. :(
   //     return;
    }

    ParseFile(summary);
    ParseFile(event);
    ParseFile(waveform);
}

bool PRS1FileGroup::ParseFile(QString path)
{
    if (path.isEmpty())
        return false;

    QFile f(path);

    if (!f.exists()) {
        return false;
    }

    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }

    PRS1DataChunk *chunk = nullptr, *lastchunk = nullptr;

    quint16 blocksize;
    quint16 wvfm_signals;

    unsigned char * header;
    int cnt = 0;

    int lastheadersize = 0;
    int lastblocksize = 0;

    int cruft = 0;

    do {
        QByteArray headerBA = f.read(16);
        if (headerBA.size() != 16) {
            break;
        }
        header = (unsigned char *)headerBA.data();

        blocksize = (header[2] << 8) | header[1];
        if (blocksize == 0) break;

        chunk = new PRS1DataChunk();

        chunk->sessionid = (header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
        chunk->fileVersion = header[0];
        chunk->htype = header[3];  // 00 = normal ?? // 01=waveform ?? // could be a bool signifying extra header bytes?
        chunk->family = header[4];
        chunk->familyVersion = header[5];
        chunk->ext = header[6];
        chunk->timestamp = (header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];

        if (lastchunk != nullptr) {
            if ((lastchunk->fileVersion != chunk->fileVersion)
                && (lastchunk->ext != chunk->ext)
                && (lastchunk->family != chunk->family)
                && (lastchunk->familyVersion != chunk->familyVersion)
                && (lastchunk->htype != chunk->htype)) {
                QByteArray junk = f.read(lastblocksize - 16);


                if (lastchunk->ext == 5) {
                    // The data is random crap
//                    lastchunk->m_data.append(junk.mid(lastheadersize-16));
                }
                delete chunk;
                ++cruft;
                if (cruft > 3)
                    break;

                continue;
                // Corrupt header.. skip it.
            }
        }

        int diff = 0;

        //////////////////////////////////////////////////////////
        // Waveform Header
        //////////////////////////////////////////////////////////
        if (chunk->ext == 5) {
            // Get extra 8 bytes in waveform header.
            QByteArray extra = f.read(4);
            if (extra.size() != 4) {
                delete chunk;
                break;
            }
            headerBA.append(extra);
            // Get the header address again to be safe
            header = (unsigned char *)headerBA.data();

            chunk->duration = header[0x0f] | header[0x10] << 8;

            wvfm_signals = header[0x12] | header[0x13] << 8;

            int sbsize = wvfm_signals * 3 + 1;
            QByteArray sbextra = f.read(sbsize);
            if (sbextra.size() != sbsize) {
                delete chunk;
                break;
            }
            headerBA.append(sbextra);
            header = (unsigned char *)headerBA.data();

            // Read the waveform information in reverse.
            int pos = 0x14 + (wvfm_signals - 1) * 3;
            for (int i = 0; i < wvfm_signals; ++i) {
                quint16 interleave = header[pos] | header[pos + 1] << 8;
                quint8 sample_format = header[pos + 2];
                chunk->waveformInfo.push_back(PRS1Waveform(interleave, sample_format));
                pos -= 3;
            }
            if (lastchunk != nullptr) {
                diff = (chunk->timestamp - lastchunk->timestamp) - lastchunk->duration;

            }
        }

        int headersize = headerBA.size();

        lastblocksize = blocksize;
        lastheadersize = headersize;
        blocksize -= headersize;



        // Check header checksum
        quint8 csum = 0;
        for (int i=0; i < headersize-1; ++i) csum += header[i];
        if (csum != header[headersize-1]) {
            // header checksum error.
            delete chunk;
            return false;
        }
        // Read data block
        chunk->m_data = f.read(blocksize);

        if (chunk->m_data.size() < blocksize) {
            delete chunk;
            break;
        }

        // last two bytes contain crc16 checksum.
        int ds = chunk->m_data.size();
        quint16 crc16 = chunk->m_data.at(ds-2) | chunk->m_data.at(ds-1) << 8;
        chunk->m_data.chop(2);

#ifdef PRS1_CRC_CHECK
        quint16 calc16 = CRC16((unsigned char *)chunk->m_data.data(), chunk->m_data.size());
        if (calc16 != crc16) {
            // corrupt data block.. bleh..
        }
#endif

        if (chunk->ext == 5) {
            if (lastchunk != nullptr) {

                Q_ASSERT(lastchunk->sessionid == chunk->sessionid);

                if (diff == 0) {
                    // In sync, so append waveform data to previous chunk
                    lastchunk->m_data.append(chunk->m_data);
                    lastchunk->duration += chunk->duration;
                    delete chunk;
                    cnt++;
                    chunk = lastchunk;
                    continue;
                }
                // else start a new chunk to resync
            }
        }

        QMap<SessionID, PRS1SessionData*>::iterator it = sessions.find(chunk->sessionid);
        if (it == sessions.end()) {
            it = sessions.insert(chunk->sessionid, new PRS1SessionData());
        }

        switch (chunk->ext) {
        case 0:
            it.value()->compliance = chunk;
            break;
        case 1:
            it.value()->summary = chunk;
            break;
        case 2:
            it.value()->event = chunk;
            break;
        case 5:
            it.value()->waveforms.append(chunk);
            break;
        default:
            qDebug() << "Cruft file in PRS1FileGroup::ParseFile " << path;
            delete chunk;
            return false;
        }

        lastchunk = chunk;
        cnt++;
    } while (!f.atEnd());
    return true;
}

void InitModelMap()
{
    ModelMap[0x34] = "RemStar Pro with C-Flex+"; // 450/460P
    ModelMap[0x35] = "RemStar Auto with A-Flex"; // 550/560P
    ModelMap[0x36] = "RemStar BiPAP Pro with Bi-Flex";
    ModelMap[0x37] = "RemStar BiPAP Auto with Bi-Flex";
    ModelMap[0x38] = "RemStar Plus :(";          // 150/250P/260P
    ModelMap[0x41] = "BiPAP autoSV Advanced";
}

bool initialized = false;

using namespace schema;

Channel PRS1Channels;

void PRS1Loader::Register()
{
    if (initialized) { return; }

    qDebug() << "Registering PRS1Loader";
    RegisterLoader(new PRS1Loader());
    InitModelMap();
    initialized = true;

    channel.add(GRP_CPAP, new Channel(CPAP_PressurePulse = 0x1009, MINOR_FLAG,    SESSION,
        "PressurePulse",  QObject::tr("Pressure Pulse"),
        QObject::tr("A pulse of pressure 'pinged' to detect a closed airway."),
        QObject::tr("PP"),       STR_UNIT_EventsPerHour,    DEFAULT,    QColor("dark red")));


    QString unknowndesc=QObject::tr("Unknown PRS1 Code %1");
    QString unknownname=QObject::tr("PRS1_%1");
    QString unknownshort=QObject::tr("PRS1_%1");

    channel.add(GRP_CPAP, new Channel(PRS1_00 = 0x1150, MINOR_FLAG,    SESSION,
        "PRS1_00",
        QString(unknownname).arg(0,2,16,QChar('0')),
        QString(unknowndesc).arg(0,2,16,QChar('0')),
        QString(unknownshort).arg(0,2,16,QChar('0')),
        STR_UNIT_Unknown,
        DEFAULT,    QColor("black")));

    channel.add(GRP_CPAP, new Channel(PRS1_01 = 0x1151, MINOR_FLAG,    SESSION,
        "PRS1_01",
        QString(unknownname).arg(1,2,16,QChar('0')),
        QString(unknowndesc).arg(1,2,16,QChar('0')),
        QString(unknownshort).arg(1,2,16,QChar('0')),
        STR_UNIT_Unknown,
        DEFAULT,    QColor("black")));

    channel.add(GRP_CPAP, new Channel(PRS1_08 = 0x1152, MINOR_FLAG,    SESSION,
        "PRS1_08",
        QString(unknownname).arg(8,2,16,QChar('0')),
        QString(unknowndesc).arg(8,2,16,QChar('0')),
        QString(unknownshort).arg(8,2,16,QChar('0')),
        STR_UNIT_Unknown,
        DEFAULT,    QColor("black")));

    channel.add(GRP_CPAP, new Channel(PRS1_0A = 0x1154, MINOR_FLAG,    SESSION,
        "PRS1_0A",
        QString(unknownname).arg(0xa,2,16,QChar('0')),
        QString(unknowndesc).arg(0xa,2,16,QChar('0')),
        QString(unknownshort).arg(0xa,2,16,QChar('0')),
        STR_UNIT_Unknown,
        DEFAULT,    QColor("black")));
    channel.add(GRP_CPAP, new Channel(PRS1_0B = 0x1155, MINOR_FLAG,    SESSION,
        "PRS1_0B",
        QString(unknownname).arg(0xb,2,16,QChar('0')),
        QString(unknowndesc).arg(0xb,2,16,QChar('0')),
        QString(unknownshort).arg(0xb,2,16,QChar('0')),
        STR_UNIT_Unknown,
        DEFAULT,    QColor("black")));
    channel.add(GRP_CPAP, new Channel(PRS1_0C = 0x1156, MINOR_FLAG,    SESSION,
        "PRS1_0C",
        QString(unknownname).arg(0xc,2,16,QChar('0')),
        QString(unknowndesc).arg(0xc,2,16,QChar('0')),
        QString(unknownshort).arg(0xc,2,16,QChar('0')),
        STR_UNIT_Unknown,
        DEFAULT,    QColor("black")));
    channel.add(GRP_CPAP, new Channel(PRS1_0E = 0x1157, MINOR_FLAG,    SESSION,
        "PRS1_0E",
        QString(unknownname).arg(0xe,2,16,QChar('0')),
        QString(unknowndesc).arg(0xe,2,16,QChar('0')),
        QString(unknownshort).arg(0xe,2,16,QChar('0')),
        STR_UNIT_Unknown,
        DEFAULT,    QColor("black")));
    channel.add(GRP_CPAP, new Channel(PRS1_12 = 0x1159, MINOR_FLAG,    SESSION,
        "PRS1_12",
        QString(unknownname).arg(0x12,2,16,QChar('0')),
        QString(unknowndesc).arg(0x12,2,16,QChar('0')),
        QString(unknownshort).arg(0x12,2,16,QChar('0')),
        STR_UNIT_Unknown,
        DEFAULT,    QColor("black")));


}




