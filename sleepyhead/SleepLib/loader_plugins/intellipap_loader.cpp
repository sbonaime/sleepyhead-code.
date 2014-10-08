/* SleepLib (DeVilbiss) Intellipap Loader Implementation
 *
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

ChannelID INTP_SmartFlexMode, INTP_SmartFlexLevel;

Intellipap::Intellipap(MachineID id)
    : CPAP(id)
{
}

Intellipap::~Intellipap()
{
}

IntellipapLoader::IntellipapLoader()
{
    const QString INTELLIPAP_ICON = ":/icons/intellipap.png";
    QString s = newInfo().series;
    m_pixmap_paths[s] = INTELLIPAP_ICON;
    m_pixmaps[s] = QPixmap(INTELLIPAP_ICON);

    m_buffer = nullptr;
    m_type = MT_CPAP;
}

IntellipapLoader::~IntellipapLoader()
{
}

bool IntellipapLoader::Detect(const QString & givenpath)
{
    QDir dir(givenpath);

    if (!dir.exists()) {
        return false;
    }

    // Intellipap has a folder called SL in the root directory
    if (!dir.cd("SL")) {
        return false;
    }

    // Check for the settings file inside the SL folder
    if (!dir.exists("SET1")) {
        return false;
    }

    return true;
}

int IntellipapLoader::Open(QString path)
{
    // Check for SL directory
    // Check for DV5MFirm.bin?
    QString newpath;

    path = path.replace("\\", "/");

    QString dirtag = "SL";

    if (path.endsWith("/" + dirtag)) {
        return -1;
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

    if (!f.exists()) { return -1; }

    f.open(QFile::ReadOnly);
    QTextStream tstream(&f);

    const QString INT_PROP_Serial = "Serial";
    const QString INT_PROP_Model = "Model";
    const QString INT_PROP_Mode = "Mode";
    const QString INT_PROP_MaxPressure = "Max Pressure";
    const QString INT_PROP_MinPressure = "Min Pressure";
    const QString INT_PROP_IPAP = "IPAP";
    const QString INT_PROP_EPAP = "EPAP";
    const QString INT_PROP_PS = "PS";
    const QString INT_PROP_RampPressure = "Ramp Pressure";
    const QString INT_PROP_RampTime = "Ramp Time";

    const QString INT_PROP_HourMeter = "Usage Hours";
    const QString INT_PROP_ComplianceMeter = "Compliance Hours";
    const QString INT_PROP_ErrorCode = "Error";
    const QString INT_PROP_LastErrorCode = "Long Error";
    const QString INT_PROP_LowUseThreshold = "Low Usage";
    const QString INT_PROP_SmartFlex = "SmartFlex";
    const QString INT_PROP_SmartFlexMode = "SmartFlexMode";


    QHash<QString, QString> lookup;
        lookup["Sn"] = INT_PROP_Serial;
        lookup["Mn"] = INT_PROP_Model;
        lookup["Mo"] = INT_PROP_Mode;     // 0 cpap, 1 auto
        //lookup["Pn"]="??";
        lookup["Pu"] = INT_PROP_MaxPressure;
        lookup["Pl"] = INT_PROP_MinPressure;
        lookup["Pi"] = INT_PROP_IPAP;
        lookup["Pe"] = INT_PROP_EPAP;  // == WF on Auto models
        lookup["Ps"] = INT_PROP_PS;             // == WF on Auto models, Pressure support
        //lookup["Ds"]="??";
        //lookup["Pc"]="??";
        lookup["Pd"] = INT_PROP_RampPressure;
        lookup["Dt"] = INT_PROP_RampTime;
        //lookup["Ld"]="??";
        //lookup["Lh"]="??";
        //lookup["FC"]="??";
        //lookup["FE"]="??";
        //lookup["FL"]="??";
        lookup["A%"]="ApneaThreshold";
        lookup["Ad"]="ApneaDuration";
        lookup["H%"]="HypopneaThreshold";
        lookup["Hd"]="HypopneaDuration";
        //lookup["Pi"]="??";
        //lookup["Pe"]="??";
        lookup["Ri"]="SmartFlexIRnd";  // Inhale Rounding (0-5)
        lookup["Re"]="SmartFlexERnd"; // Inhale Rounding (0-5)
        //lookup["Bu"]="??"; // WF
        //lookup["Ie"]="??"; // 20
        //lookup["Se"]="??"; // 05    //Inspiratory trigger?
        //lookup["Si"]="??"; // 05    // Expiratory Trigger?
        //lookup["Mi"]="??"; // 0
        lookup["Uh"]="HoursMeter"; // 0000.0
        lookup["Up"]="ComplianceMeter"; // 0000.00
        //lookup["Er"]="ErrorCode";, // E00
        //lookup["El"]="LongErrorCode"; // E00 00/00/0000
        //lookup["Hp"]="??";, // 1
        //lookup["Hs"]="??";, // 02
        //lookup["Lu"]="LowUseThreshold"; // defaults to 0 (4 hours)
        lookup["Sf"] = INT_PROP_SmartFlex;
        lookup["Sm"] = INT_PROP_SmartFlexMode;
        lookup["Ks=s"]="Ks_s";
        lookup["Ks=i"]="ks_i";

    QHash<QString, QString> set1;
    QHash<QString, QString>::iterator hi;

    Machine *mach = nullptr;

    MachineInfo info = newInfo();


    bool ok;

    //EventDataType min_pressure = 0, max_pressure = 0, set_ipap = 0, set_ps = 0,
    EventDataType ramp_pressure = 0, set_epap = 0, ramp_time = 0;

    int papmode = 0, smartflex = 0, smartflexmode = 0;
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

        if (key == INT_PROP_Mode) {
            papmode = value.toInt(&ok);
        } else if (key == INT_PROP_Serial) {
            info.serial = value;
        } else if (key == INT_PROP_Model) {
            info.model = value;
        } else if (key == INT_PROP_MinPressure) {
            //min_pressure = value.toFloat() / 10.0;
        } else if (key == INT_PROP_MaxPressure) {
            //max_pressure = value.toFloat() / 10.0;
        } else if (key == INT_PROP_IPAP) {
            //set_ipap = value.toFloat() / 10.0;
        } else if (key == INT_PROP_EPAP) {
            set_epap = value.toFloat() / 10.0;
        } else if (key == INT_PROP_PS) {
            //set_ps = value.toFloat() / 10.0;
        } else if (key == INT_PROP_RampPressure) {
            ramp_pressure = value.toFloat() / 10.0;
        } else if (key == INT_PROP_RampTime) {
            ramp_time = value.toFloat() / 10.0;
        } else if (key == INT_PROP_SmartFlex) {
            smartflex = value.toInt();
        } else if (key == INT_PROP_SmartFlexMode) {
            smartflexmode = value.toInt();
        } else {
            set1[key] = value;
        }
        qDebug() << key << "=" << value;
    }

    CPAPMode mode = MODE_UNKNOWN;

    switch (papmode) {
    case 0:
        mode = MODE_CPAP;
        break;
    case 1:
        mode = (set_epap > 0) ? MODE_BILEVEL_FIXED : MODE_APAP;
        break;
    default:
        qDebug() << "New machine mode";
    }

    if (!info.serial.isEmpty()) {
        mach = CreateMachine(info);
    }

    if (!mach) {
        qDebug() << "Couldn't get Intellipap machine record";
        return -1;
    }

    QString backupPath = mach->getBackupPath();
    QString copypath = path;

    if (QDir::cleanPath(path).compare(QDir::cleanPath(backupPath)) != 0) {
        copyPath(path, backupPath);
    }


    // Refresh properties data..
    for (QHash<QString, QString>::iterator i = set1.begin(); i != set1.end(); i++) {
        mach->properties[i.key()] = i.value();
    }

    f.close();

    ///////////////////////////////////////////////
    // Parse the Session Index (U File)
    ///////////////////////////////////////////////
    unsigned char buf[27];
    filename = newpath + "/U";
    f.setFileName(filename);

    if (!f.exists()) { return -1; }

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
        // big endian
        ts1 = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
        ts2 = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
        // buf[8] == ??? What is this byte? A Bit Field? A checksum?
        ts1 += ep;
        ts2 += ep;
        SessionStart.append(ts1);
        SessionEnd.append(ts2);
    } while (cnt > 0);

    qDebug() << "U file logs" << SessionStart.size() << "sessions.";
    f.close();

    ///////////////////////////////////////////////
    // Parse the Session Data (L File)
    ///////////////////////////////////////////////
    filename = newpath + "/L";
    f.setFileName(filename);

    if (!f.exists()) { return -1; }

    f.open(QFile::ReadOnly);
    long size = f.size();
    int recs = size / 26;
    m_buffer = new unsigned char [size];

    if (size != f.read((char *)m_buffer, size)) {
        qDebug()  << "Couldn't read 'L' data" << filename;
        return -1;
    }

    Session *sess;
    SessionID sid;
    QHash<SessionID,qint64> rampstart;
    QHash<SessionID,qint64> rampend;

    for (int i = 0; i < SessionStart.size(); i++) {
        sid = SessionStart[i];

        if (mach->SessionExists(sid)) {
            // knock out the already imported sessions..
            SessionStart[i] = 0;
            SessionEnd[i] = 0;
        } else if (!Sessions.contains(sid)) {
            sess = Sessions[sid] = new Session(mach, sid);

            sess->really_set_first(qint64(sid) * 1000L);
         //   sess->really_set_last(qint64(SessionEnd[i]) * 1000L);

            rampstart[sid] = 0;
            rampend[sid] = 0;
            sess->SetChanged(true);
            if (mode >= MODE_BILEVEL_FIXED) {
                sess->AddEventList(CPAP_IPAP, EVL_Event);
                sess->AddEventList(CPAP_EPAP, EVL_Event);
                sess->AddEventList(CPAP_PS, EVL_Event);
            } else {
                sess->AddEventList(CPAP_Pressure, EVL_Event);
            }

            sess->AddEventList(INTELLIPAP_Unknown1, EVL_Event);
            sess->AddEventList(INTELLIPAP_Unknown2, EVL_Event);

            sess->AddEventList(CPAP_LeakTotal, EVL_Event);
            sess->AddEventList(CPAP_MaxLeak, EVL_Event);
            sess->AddEventList(CPAP_TidalVolume, EVL_Event);
            sess->AddEventList(CPAP_MinuteVent, EVL_Event);
            sess->AddEventList(CPAP_RespRate, EVL_Event);
            sess->AddEventList(CPAP_Snore, EVL_Event);

            sess->AddEventList(CPAP_Obstructive, EVL_Event);
            sess->AddEventList(CPAP_VSnore, EVL_Event);
            sess->AddEventList(CPAP_Hypopnea, EVL_Event);
            sess->AddEventList(CPAP_NRI, EVL_Event);
            sess->AddEventList(CPAP_LeakFlag, EVL_Event);
            sess->AddEventList(CPAP_ExP, EVL_Event);


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
    int rampval = 0;
    sid = 0;
    //SessionID lastsid = 0;

    //int last_minp=0, last_maxp=0, last_ps=0, last_pres = 0;

    for (int i = 0; i < recs; i++) {
        // convert timestamp to real epoch
        ts1 = ((m_buffer[pos] << 24) | (m_buffer[pos + 1] << 16) | (m_buffer[pos + 2] << 8) | m_buffer[pos + 3]) + ep;

        for (int j = 0; j < SessionStart.size(); j++) {
            sid = SessionStart[j];

            if (!sid) { continue; }

            if ((ts1 >= (quint32)sid) && (ts1 <= SessionEnd[j])) {
                Session *sess = Sessions[sid];

                qint64 time = quint64(ts1) * 1000L;
                sess->really_set_last(time);
                sess->settings[CPAP_Mode] = mode;

                int minp = m_buffer[pos + 0x13];
                int maxp = m_buffer[pos + 0x14];
                int ps = m_buffer[pos + 0x15];
                int pres = m_buffer[pos + 0xd];

                if (mode >= MODE_BILEVEL_FIXED) {
                    rampval = maxp;
                } else {
                    rampval = minp;
                }

                qint64 rs = rampstart[sid];

                if (pres < rampval) {
                    if (!rs) {
                        // ramp started


//                        int rv = pres-rampval;
//                        double ramp =

                        rampstart[sid] = time;
                    }
                    rampend[sid] = time;
                } else {
                    if (rs > 0) {
                        if (!sess->eventlist.contains(CPAP_Ramp)) {
                            sess->AddEventList(CPAP_Ramp, EVL_Event);
                        }
                        int duration = (time - rs) / 1000L;
                        sess->eventlist[CPAP_Ramp][0]->AddEvent(time, duration);

                        rampstart.remove(sid);
                        rampend.remove(sid);
                    }
                }


                // Do this after ramp, because ramp calcs might need to insert interpolated pressure samples
                if (mode >= MODE_BILEVEL_FIXED) {

                    sess->settings[CPAP_EPAP] = float(minp) / 10.0;
                    sess->settings[CPAP_IPAP] = float(maxp) / 10.0;

                    sess->settings[CPAP_PS] = float(ps) / 10.0;


                    sess->eventlist[CPAP_IPAP][0]->AddEvent(time, float(pres) / 10.0);
                    sess->eventlist[CPAP_EPAP][0]->AddEvent(time, float(pres-ps) / 10.0);
//                   rampval = maxp;

                } else {
                    sess->eventlist[CPAP_Pressure][0]->AddEvent(time, float(pres) / 10.0); // current pressure
//                    rampval = minp;

                    if (mode == MODE_APAP) {
                        sess->settings[CPAP_PressureMin] = float(minp) / 10.0;
                        sess->settings[CPAP_PressureMax] = float(maxp) / 10.0;
                    } else if (mode == MODE_CPAP) {
                        sess->settings[CPAP_Pressure] = float(maxp) / 10.0;
                    }
                }


                sess->eventlist[CPAP_LeakTotal][0]->AddEvent(time, m_buffer[pos + 0x7]); // "Average Leak"
                sess->eventlist[CPAP_MaxLeak][0]->AddEvent(time, m_buffer[pos + 0x6]); // "Max Leak"

                int rr = m_buffer[pos + 0xa];
                sess->eventlist[CPAP_RespRate][0]->AddEvent(time, rr); // Respiratory Rate
               // sess->eventlist[INTELLIPAP_Unknown1][0]->AddEvent(time, m_buffer[pos + 0xf]); //
                sess->eventlist[INTELLIPAP_Unknown1][0]->AddEvent(time, m_buffer[pos + 0xc]);

                sess->eventlist[CPAP_Snore][0]->AddEvent(time, m_buffer[pos + 0x4]); //4/5??

                if (m_buffer[pos+0x4] > 0) {
                    sess->eventlist[CPAP_VSnore][0]->AddEvent(time, m_buffer[pos + 0x5]);
                }

                // 0x0f == Leak Event
                // 0x04 == Snore?
                if (m_buffer[pos + 0xf] > 0) { // Leak Event
                    sess->eventlist[CPAP_LeakFlag][0]->AddEvent(time, m_buffer[pos + 0xf]);
                }

                if (m_buffer[pos + 0x5] > 4) { // This matches Exhale Puff.. not sure why 4
                    //MW: Are the lower 2 bits something else?

                    sess->eventlist[CPAP_ExP][0]->AddEvent(time, m_buffer[pos + 0x5]);
                }

                if (m_buffer[pos + 0x10] > 0) {
                    sess->eventlist[CPAP_Obstructive][0]->AddEvent(time, m_buffer[pos + 0x10]);
                }

                if (m_buffer[pos + 0x11] > 0) {
                    sess->eventlist[CPAP_Hypopnea][0]->AddEvent(time, m_buffer[pos + 0x11]);
                }

                if (m_buffer[pos + 0x12] > 0) { // NRI // is this == to RERA?? CA??
                    sess->eventlist[CPAP_NRI][0]->AddEvent(time, m_buffer[pos + 0x12]);
                }

                quint16 tv = (m_buffer[pos + 0x8] << 8) | m_buffer[pos + 0x9]; // correct

                sess->eventlist[CPAP_TidalVolume][0]->AddEvent(time, tv);

                EventDataType mv = tv * rr; // MinuteVent=TidalVolume * Respiratory Rate
                sess->eventlist[CPAP_MinuteVent][0]->AddEvent(time, mv / 1000.0);
                break;
            } else {
            }
            //lastsid = sid;
        }

        pos += 26;
    }

    // Close any open ramps and store the event.
    QHash<SessionID,qint64>::iterator rit;
    QHash<SessionID,qint64>::iterator rit_end = rampstart.end();

    for (rit = rampstart.begin(); rit != rit_end; ++rit) {
        qint64 rs = rit.value();
        SessionID sid = rit.key();
        if (rs > 0) {
            qint64 re = rampend[rit.key()];

            Session *sess = Sessions[sid];
            if (!sess->eventlist.contains(CPAP_Ramp)) {
                sess->AddEventList(CPAP_Ramp, EVL_Event);
            }

            int duration = (re - rs) / 1000L;
            sess->eventlist[CPAP_Ramp][0]->AddEvent(re, duration);
            rit.value() = 0;

        }
    }

    for (int i = 0; i < SessionStart.size(); i++) {
        SessionID sid = SessionStart[i];

        if (sid) {
            sess = Sessions[sid];

            if (!sess) continue;

//            quint64 first = qint64(sid) * 1000L;
            //quint64 last = qint64(SessionEnd[i]) * 1000L;

            if (sess->last() > 0) {
             //   sess->really_set_last(last);


                sess->settings[INTP_SmartFlexLevel] = smartflex;

                if (smartflexmode == 0) {
                    sess->settings[INTP_SmartFlexMode] = PM_FullTime;
                } else {
                    sess->settings[INTP_SmartFlexMode] = PM_RampOnly;
                }

                sess->settings[CPAP_RampPressure] = ramp_pressure;
                sess->settings[CPAP_RampTime] = ramp_time;

                sess->UpdateSummaries();

                addSession(sess);
            } else {
                delete sess;
            }

        }
    }

    finishAddingSessions();
    mach->Save();


    delete [] m_buffer;

    if (qprogress) { qprogress->setValue(100); }

    f.close();

    int c = Sessions.size();
    return c;
}

void IntellipapLoader::initChannels()
{
    using namespace schema;
    Channel * chan = nullptr;
    channel.add(GRP_CPAP, chan = new Channel(INTP_SmartFlexMode = 0x1165, SETTING,  MT_CPAP,  SESSION,
        "INTPSmartFlexMode", QObject::tr("SmartFlex Mode"),
        QObject::tr("Intellipap pressure relief mode."),
        QObject::tr("SmartFlex Mode"),
        "", DEFAULT, Qt::green));


    chan->addOption(0, STR_TR_Off);
    chan->addOption(1, QObject::tr("Ramp Only"));
    chan->addOption(2, QObject::tr("Full Time"));

    channel.add(GRP_CPAP, chan = new Channel(INTP_SmartFlexLevel = 0x1169, SETTING,  MT_CPAP,  SESSION,
        "INTPSmartFlexLevel", QObject::tr("SmartFlex Level"),
        QObject::tr("Intellipap pressure relief level."),
        QObject::tr("SmartFlex Level"),
        "", DEFAULT, Qt::green));
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
