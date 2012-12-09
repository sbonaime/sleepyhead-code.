/*

SleepLib PRS1 Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
Copyright: (c)2011 Mark Watkins

*/

#include <QApplication>
#include <QString>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProgressBar>
#include <QDebug>
#include <cmath>
#include "SleepLib/schema.h"
#include "prs1_loader.h"
#include "SleepLib/session.h"
#include "SleepLib/calcs.h"


const int PRS1_MAGIC_NUMBER=2;
const int PRS1_SUMMARY_FILE=1;
const int PRS1_EVENT_FILE=2;
const int PRS1_WAVEFORM_FILE=5;


//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the prs1_data_version in prs1_loader.h when making changes to this loader
// that change loader behaviour or modify channels.
//********************************************************************************************


extern QProgressBar *qprogress;

QHash<int,QString> ModelMap;

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
    crc_t crc=0;
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = (crc ^ *data) & 0xff;
        crc = (crc_table[tbl_idx] ^ (crc >> 8)) & 0xffff;

        data++;
    }
    return crc & 0xffff;
}
#endif

PRS1::PRS1(Profile *p,MachineID id):CPAP(p,id)
{
    m_class=prs1_class_name;
    properties[STR_PROP_Brand]="Philips Respironics";
    properties[STR_PROP_Model]="System One";

}
PRS1::~PRS1()
{

}


/*! \struct WaveHeaderList
    \brief Used in PRS1 Waveform Parsing */
struct WaveHeaderList {
    quint16 interleave;
    quint8  sample_format;
    WaveHeaderList(quint16 i,quint8 f){ interleave=i; sample_format=f; }
};


PRS1Loader::PRS1Loader()
{
    //genCRCTable();
    m_buffer=NULL;
}

PRS1Loader::~PRS1Loader()
{
    for (QHash<QString,Machine *>::iterator i=PRS1List.begin(); i!=PRS1List.end(); i++) {
        delete i.value();
    }
}
Machine *PRS1Loader::CreateMachine(QString serial,Profile *profile)
{
    if (!profile)
        return NULL;
    qDebug() << "Create Machine " << serial;

    QList<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    QList<Machine *>::iterator i;
    for (i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()==STR_MACH_PRS1) && ((*i)->properties[STR_PROP_Serial]==serial)) {
            PRS1List[serial]=*i; //static_cast<CPAP *>(*i);
            found=true;
            break;
        }
    }
    if (found) return *i;

    //assert(PRS1List.find(serial)==PRS1List.end())
    Machine *m=new PRS1(profile,0);

    PRS1List[serial]=m;
    profile->AddMachine(m);

    m->properties[STR_PROP_Serial]=serial;
    m->properties[STR_PROP_DataVersion]=QString::number(prs1_data_version);

    QString path="{"+STR_GEN_DataFolder+"}/"+m->GetClass()+"_"+serial+"/";
    m->properties[STR_PROP_Path]=path;
    m->properties[STR_PROP_BackupPath]=path+"Backup/";

    return m;
}
bool isdigit(QChar c)
{
    if ((c>='0') && (c<='9')) return true;
    return false;
}
int PRS1Loader::Open(QString & path,Profile *profile)
{
    QString newpath,pseries="P-Series";
    qDebug() << "PRS1Loader::Open path=" << newpath;
    if (path.endsWith(QDir::separator()+pseries)) {
        newpath=path;
    } else {
        newpath=path+QDir::separator()+pseries;
    }

    QDir dir(newpath);

    if ((!dir.exists() || !dir.isReadable()))
        return 0;

    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist=dir.entryInfoList();

    QList<QString> SerialNumbers;
    QList<QString>::iterator sn;

    for (int i=0;i<flist.size();i++) {
        QFileInfo fi=flist.at(i);
        QString filename=fi.fileName();

        if ((filename[0]=='P') && (isdigit(filename[1])) && (isdigit(filename[2]))) {
            SerialNumbers.push_back(filename);
        } else if (filename.toLower()=="last.txt") { // last.txt points to the current serial number
            QString file=fi.canonicalFilePath();
            QFile f(file);
            if (!fi.isReadable()) {
                qDebug() << "PRS1Loader: last.txt exists but I couldn't read it!";
                continue;
            }
            if (!f.open(QIODevice::ReadOnly)) {
                qDebug() << "PRS1Loader: last.txt exists but I couldn't open it!";
                continue;
            }
            last=f.readLine(64);
            last=last.trimmed();
            f.close();
        }
    }

    if (SerialNumbers.empty()) return 0;

    m_buffer=new unsigned char [max_load_buffer_size]; //allocate once and reuse.
    Machine *m;
    for (sn=SerialNumbers.begin(); sn!=SerialNumbers.end(); sn++) {
        QString s=*sn;
        m=CreateMachine(s,profile);
        try {
            if (m) OpenMachine(m,newpath+QDir::separator()+(*sn),profile);
        } catch(OneTypePerDay e) {
            profile->DelMachine(m);
            PRS1List.erase(PRS1List.find(s));
            QMessageBox::warning(NULL,"Import Error","This Machine Record cannot be imported in this profile.\nThe Day records overlap with already existing content.",QMessageBox::Ok);
            delete m;
        }
    }
    delete [] m_buffer;
    return PRS1List.size();
}

bool PRS1Loader::ParseProperties(Machine *m,QString filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QString line;
    QHash<QString,QString> prop;

    QString s=f.readLine();
    QChar sep='=';
    QString key,value;

    while (!f.atEnd()) {
        key=s.section(sep,0,0); //BeforeFirst(sep);
        if (key==s) continue;
        value=s.section(sep,1).trimmed(); //AfterFirst(sep).Strip();
        if (value==s) continue;
        prop[key]=value;
        s=f.readLine();
    }
    bool ok;
    QString pt=prop["ProductType"];
    int i=pt.toInt(&ok,16);
    if (ok) {
        if (ModelMap.find(i)!=ModelMap.end()) {
            m->properties[STR_PROP_SubModel]=ModelMap[i];
        }
    }
    if (prop["SerialNumber"]!=m->properties[STR_PROP_Serial]) {
        qDebug() << "Serial Number in PRS1 properties.txt doesn't match directory structure";
    } else prop.erase(prop.find("SerialNumber")); // already got it stored.

    for (QHash<QString,QString>::iterator i=prop.begin(); i!=prop.end(); i++) {
        m->properties[i.key()]=i.value();
    }

    f.close();
    return true;
}

int PRS1Loader::OpenMachine(Machine *m,QString path,Profile *profile)
{

    qDebug() << "Opening PRS1 " << path;
    QDir dir(path);
    if (!dir.exists() || (!dir.isReadable()))
         return false;


    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist=dir.entryInfoList();

    QString filename;
    if (qprogress) qprogress->setValue(0);

    QList<QString> paths;
    for (int i=0;i<flist.size();i++) {
        QFileInfo fi=flist.at(i);
        filename=fi.fileName();

         if ((filename[0].toLower()=='p') && (isdigit(filename[1]))) {
            paths.push_back(fi.canonicalFilePath());
        } else if (filename.toLower()=="properties.txt") {
            ParseProperties(m,fi.canonicalFilePath());
        } else if (filename.toLower()=="e") {
            // don't really give a crap about .004 files yet.
        }
    }

    SessionID sid;
    long ext;
    QHash<SessionID, QStringList> sessfiles;
    int size=paths.size();
    int cnt=0;
    bool ok;

    new_sessions.clear();
    for (QList<QString>::iterator p=paths.begin(); p!=paths.end(); p++) {
        dir.setPath(*p);
        if (!dir.exists() || !dir.isReadable()) continue;
        flist=dir.entryInfoList();

        for (int i=0;i<flist.size();i++) {
            QFileInfo fi=flist.at(i);
            QString ext_s=fi.fileName().section(".",-1);
            QString session_s=fi.fileName().section(".",0,-2);

            ext=ext_s.toLong(&ok);
            if (!ok)
                continue;

            sid=session_s.toLong(&ok);
            if (!ok)
                continue;

            if (m->SessionExists(sid))
                continue; // could skip this and error check data by reloading summary.

            if ((ext==1) || (ext==0)) {
                OpenFile(m,fi.canonicalFilePath()); // Open just the summary files first round
            } else {
                sessfiles[sid].push_back(fi.canonicalFilePath()); // and keep the rest of the names
            }
        }
    }
    cnt=0;
    size=new_sessions.size();
    for (QHash<SessionID, Session *>::iterator it=new_sessions.begin();it!=new_sessions.end();it++) {
        sid=it.key();

        if (sessfiles.contains(sid)) {
            for (int j=0;j<sessfiles[sid].size();j++) {
                QString name=sessfiles[sid][j];
                if (name.endsWith(".002")) {
                    OpenFile(m,name);
                } else if (name.endsWith(".005")) {
                    OpenWaveforms(sid,name);
                }
            }
        }
        // sessions are fully loaded here..

        if ((++cnt % 10)==0) {
            if (qprogress) qprogress->setValue(0.0+(float(cnt)/float(size)*100.0));
            QApplication::processEvents();
        }
    }
    // strictly can do this in the above loop, but this is cautionary
    cnt=0;
    //QVector<SessionID> KillList;
    for (QHash<SessionID, Session *>::iterator it=new_sessions.begin();it!=new_sessions.end();it++) {
        Session *sess=it.value();

        if ((sess->length()) == 0) {
        //const double ignore_thresh=300.0/3600.0;// Ignore useless sessions under 5 minute
        //if (sess->hours()<=ignore_thresh) {
            qDebug() << "Ignoring empty session" << sess->session();
            SessionID id=sess->session();
            delete sess;
            new_sessions[id]=NULL;
            //KillList.push_back(id);
            continue;
        }

        if (sess->count(CPAP_IPAP)>0) {
            //sess->summaryCPAP_Mode]!=MODE_ASV)
            if (sess->settings[CPAP_Mode].toInt()!=(int)MODE_ASV) {
                sess->settings[CPAP_Mode]=MODE_BIPAP;
            }

            if (sess->settings[CPAP_PresReliefType].toInt()!=PR_NONE) {
                sess->settings[CPAP_PresReliefType]=PR_BIFLEX;
            }

            EventDataType min=sess->settings[CPAP_PressureMin].toDouble();
            EventDataType max=sess->settings[CPAP_PressureMax].toDouble();
            sess->settings[CPAP_EPAP]=min;
            sess->settings[CPAP_IPAP]=max;

            sess->settings[CPAP_PS]=max-min;
            sess->settings.erase(sess->settings.find(CPAP_PressureMin));
            sess->settings.erase(sess->settings.find(CPAP_PressureMax));

            sess->m_valuesummary.erase(sess->m_valuesummary.find(CPAP_Pressure));
            sess->m_wavg.erase(sess->m_wavg.find(CPAP_Pressure));
            sess->m_min.erase(sess->m_min.find(CPAP_Pressure));
            sess->m_max.erase(sess->m_max.find(CPAP_Pressure));
            sess->m_gain.erase(sess->m_gain.find(CPAP_Pressure));


        } else {

            if (!sess->settings.contains(CPAP_Pressure) && !sess->settings.contains(CPAP_PressureMin)) {
                sess->settings[CPAP_BrokenSummary]=true;
                //sess->set_last(sess->first());
                if (sess->Min(CPAP_Pressure)==sess->Max(CPAP_Pressure)) {
                    sess->settings[CPAP_Mode]=MODE_CPAP; // no ramp
                } else {
                    sess->settings[CPAP_Mode]=MODE_UNKNOWN;
                }
                //sess->Set("FlexMode",PR_UNKNOWN);
            }

        }
        if (sess->settings[CPAP_Mode].toInt()==(int)MODE_CPAP) {
            //sess->settings[CPAP_PressureMax]=sess->settings[CPAP_PressureMin];
        }

        sess->SetChanged(true);
        m->AddSession(sess,profile);
        if ((++cnt % 10) ==0) {
            //if (qprogress) qprogress->setValue(33.0+(float(cnt)/float(size)*33.0));
            QApplication::processEvents();
        }

    }

    m->properties[STR_PROP_DataVersion]=QString::number(prs1_data_version);
    m->properties[STR_PROP_LastImported]=QDateTime::currentDateTime().toString(Qt::ISODate);
    m->Save(); // Save any new sessions to disk in our format
    if (qprogress) qprogress->setValue(100);

    return cnt;
}

//struct PRS1SummaryR5 {
//    quint8 unknown1;
//    quint8 unknown2;
//    quint8 unknown3;
//    quint8 pressure_min;
//    quint8 pressure_max;
//    quint8 unknown;

//    quint32 flags;


//};// __attribute__((packed));

bool PRS1Loader::ParseSummary(Machine *mach, qint32 sequence, quint32 timestamp, unsigned char *data, quint16 size, char version)
{
    if (mach->SessionExists(sequence)) {
    	qDebug() << "sessionexists exit";
        return false;
    }

    if (size<44-16) {
	qDebug() << "size exit";
        return false;
    }

    Session *session=new Session(mach,sequence);
    session->really_set_first(qint64(timestamp)*1000L);
    EventDataType max,min;

    min=float(data[0x03])/10.0;
    max=float(data[0x04])/10.0;
    int offset=0;

    if (version==5) { //data[0x05]!=0) { // This is a time value for ASV stuff
        offset=4;   // non zero adds 4 extra fields..
    }

    session->settings[CPAP_RampTime]=(int)data[offset+0x06]; // Minutes. Convert to seconds/hours here?
    session->settings[CPAP_RampPressure]=(EventDataType)data[offset+0x07]/10.0;

    if (max>0) { // Ignoring bipap until we see some more data during import
        session->settings[CPAP_Mode]=(version==5) ? (int)MODE_ASV : (int)MODE_APAP;

        session->settings[CPAP_PressureMin]=(EventDataType)min;
        session->settings[CPAP_PressureMax]=(EventDataType)max;
    } else {
        session->settings[CPAP_Mode]=(int)MODE_CPAP;
        session->settings[CPAP_Pressure]=(EventDataType)min;
    }

    // This is incorrect..
    if (data[offset+0x08] & 0x80) { // Flex Setting
        if (data[offset+0x08] & 0x08) {
            if (max>0) {
                if (version==5) {
                    session->settings[CPAP_PresReliefType]=(int)PR_BIFLEX;
                } else {
                    session->settings[CPAP_PresReliefType]=(int)PR_AFLEX;
                }
            }
            else session->settings[CPAP_PresReliefType]=(int)PR_CFLEXPLUS;
        } else session->settings[CPAP_PresReliefType]=(int)PR_CFLEX;
    } else session->settings[CPAP_PresReliefType]=(int)PR_NONE;

    session->settings[CPAP_PresReliefMode]=(int)PM_FullTime; // only has one mode


    session->settings[CPAP_PresReliefSet]=(int)(data[offset+0x08] & 7);
    session->settings[CPAP_HumidSetting]=(int)data[offset+0x09]&0x0f;
    session->settings[PRS1_HumidStatus]=(data[offset+0x09]&0x80)==0x80;
    session->settings[PRS1_SysLock]=(data[offset+0x0a]&0x80)==0x80;
    session->settings[PRS1_SysOneResistStat]=(data[offset+0x0a]&0x40)==0x40;
    session->settings[PRS1_SysOneResistSet]=(int)data[offset+0x0a]&7;
    session->settings[PRS1_HoseDiam]=((data[offset+0x0a]&0x08)?"15mm":"22mm");
    session->settings[PRS1_AutoOff]=(data[offset+0x0c]&0x10)==0x10;
    session->settings[PRS1_MaskAlert]=(data[offset+0x0c]&0x08)==0x08;
    session->settings[PRS1_ShowAHI]=(data[offset+0x0c]&0x04)==0x04;


    unsigned duration;

    // up to this point appears to becorrect for 0x01 & 0x00
    if (size<59) {
        duration=data[offset+0x12] | (data[0x13] << 8);
        duration*=2;
        session->really_set_last(qint64(timestamp+duration)*1000L);
        if (max>0) {
            session->setMin(CPAP_Pressure,min);
            session->setMax(CPAP_Pressure,max);
            //session->setWavg(CPAP_Pressure,min);
        }
    } else {
        // 0X28 & 0X29 is length on r5

        duration=data[offset+0x14] | (data[0x15] << 8);
        if (!duration) {
	    qDebug() << "!duration exit";
            delete session;
            return false;
        }

        session->really_set_last(qint64(timestamp+duration)*1000L);
        float hours=float(duration)/3600.0;

        // Not using these because sometimes this summary is broken.
        //EventDataType minp,maxp,avgp,p90p;

        //minp=float(data[offset+0x16])/10.0;
        //maxp=float(data[offset+0x17])/10.0;
        //p90p=float(data[offset+0x18])/10.0;
        //avgp=float(data[offset+0x19])/10.0;

        short minp=data[offset+0x16];
        short maxp=data[offset+0x17];
        short medp=data[offset+0x19];
        short p90p=data[offset+0x18];

        if (version<5) {
            if (minp>0) session->setMin(CPAP_Pressure,EventDataType(minp)*0.10);
            if (maxp>0) session->setMax(CPAP_Pressure,EventDataType(maxp)*0.10);
            if (medp>0) session->setWavg(CPAP_Pressure,EventDataType(medp)*0.10); // ??

            session->m_gain[CPAP_Pressure]=0.1;
            session->m_valuesummary[CPAP_Pressure][minp]=5;
            session->m_valuesummary[CPAP_Pressure][medp]=46;
            session->m_valuesummary[CPAP_Pressure][p90p]=44;
            session->m_valuesummary[CPAP_Pressure][maxp]=5;
        }

//        if (p90p>0) {
//            session->set90p(CPAP_Pressure,p90p);
//        }

        int oc, cc, hc, rc, fc;
        session->setCount(CPAP_Obstructive,oc=(int)data[offset+0x1C] | (data[offset+0x1D] << 8));
        session->setCount(CPAP_ClearAirway,cc=(int)data[offset+0x20] | (data[offset+0x21] << 8));
        session->setCount(CPAP_Hypopnea,hc=(int)data[offset+0x2A] | (data[offset+0x2B] << 8));
        session->setCount(CPAP_RERA,rc=(int)data[offset+0x2E] | (data[offset+0x2F] << 8));
        session->setCount(CPAP_FlowLimit,fc=(int)data[offset+0x30] | (data[offset+0x31] << 8));

        session->setCph(CPAP_Obstructive,float(oc/hours));
        session->setCph(CPAP_ClearAirway,float(cc/hours));
        session->setCph(CPAP_Hypopnea,float(hc/hours));
        session->setCph(CPAP_RERA,float(rc/hours));
        session->setCph(CPAP_FlowLimit,float(fc/hours));
    }
    new_sessions[sequence]=session;
    return true;
}
bool PRS1Loader::Parse002v5(qint32 sequence, quint32 timestamp, unsigned char *buffer, quint16 size)
{
    if (!new_sessions.contains(sequence)) {
        qDebug() << "contains squence exit";
	return false;
    }
    Session *session=new_sessions[sequence];

    ChannelID Codes[]={
        PRS1_00, PRS1_01, CPAP_Pressure, CPAP_EPAP, CPAP_PressurePulse, CPAP_Obstructive,
        CPAP_ClearAirway, CPAP_Hypopnea, PRS1_08,  CPAP_FlowLimit, PRS1_0A, CPAP_CSR,
        PRS1_0C, CPAP_VSnore, PRS1_0E, PRS1_0F, PRS1_10,
        CPAP_LeakTotal, PRS1_12
    };

    int ncodes=sizeof(Codes)/sizeof(ChannelID);
    EventList * Code[0x20]={NULL};

    EventList * OA=session->AddEventList(CPAP_Obstructive, EVL_Event);
    EventList * HY=session->AddEventList(CPAP_Hypopnea, EVL_Event);
    EventList * CSR=session->AddEventList(CPAP_CSR, EVL_Event);
    EventList * LEAK=session->AddEventList(CPAP_LeakTotal,EVL_Event);
    EventList * SNORE=session->AddEventList(CPAP_Snore,EVL_Event);
    EventList * IPAP=session->AddEventList(CPAP_IPAP,EVL_Event,0.1);
    EventList * EPAP=session->AddEventList(CPAP_EPAP,EVL_Event,0.1);
    EventList * PS=session->AddEventList(CPAP_PS,EVL_Event,0.1);
    EventList * IPAPLo=session->AddEventList(CPAP_IPAPLo,EVL_Event,0.1);
    EventList * IPAPHi=session->AddEventList(CPAP_IPAPHi,EVL_Event,0.1);
    EventList * RR=session->AddEventList(CPAP_RespRate,EVL_Event);
    EventList * PTB=session->AddEventList(CPAP_PTB,EVL_Event);

    EventList * MV=session->AddEventList(CPAP_MinuteVent,EVL_Event);
    EventList * TV=session->AddEventList(CPAP_TidalVolume,EVL_Event,10);

    EventList * CA=NULL; //session->AddEventList(CPAP_ClearAirway, EVL_Event);
    EventList * VS=NULL, * FL=NULL;//,* RE=NULL,* VS2=NULL;

    //EventList * PRESSURE=NULL;
    //EventList * PP=NULL;

    EventDataType data[10];//,tmp;


    //qint64 start=timestamp;
    qint64 t=qint64(timestamp)*1000L;
    session->updateFirst(t);
    qint64 tt;
    int pos=0;
    int cnt=0;
    short delta;//,duration;
    QDateTime d;
    bool badcode=false;
    unsigned char lastcode3=0,lastcode2=0,lastcode=0,code=0;
    int lastpos=0,startpos=0,lastpos2=0,lastpos3=0;

    while (pos<size) {
        lastcode3=lastcode2;
        lastcode2=lastcode;
        lastcode=code;
        lastpos3=lastpos2;
        lastpos2=lastpos;
        lastpos=startpos;
        startpos=pos;
        code=buffer[pos++];
        if (code>=ncodes) {
            qDebug() << "Illegal PRS1 code " << hex << int(code) << " appeared at " << hex << startpos;
            qDebug() << "1: (" << int(lastcode ) << hex << lastpos << ")";
            qDebug() << "2: (" << int(lastcode2) << hex << lastpos2 <<")";
            qDebug() << "3: (" << int(lastcode3) << hex << lastpos3 <<")";
            return false;
        }
        if (code==0) {
        } else
        if (code!=0x12) {
            delta=buffer[pos];
            //duration=buffer[pos+1];
            //delta=buffer[pos+1] << 8 | buffer[pos];
            pos+=2;
            t+=qint64(delta)*1000L;
        }
        ChannelID cpapcode=Codes[(int)code];
        //EventDataType PS;
        tt=t;
        cnt++;
        int fc=0;

        switch (code) {
        case 0x00: // Unknown (ASV Pressure value)
            // offset?
            data[0]=buffer[pos++];
            fc++;
            if (!buffer[pos-1]) {   // WTH???
                data[1]=buffer[pos++];
                fc++;
            }
            if (!buffer[pos-1]) {
                data[2]=buffer[pos++];
                fc++;
            }
            break;
        case 0x01: // Unknown
            if (!Code[1]) {
                if (!(Code[1]=session->AddEventList(cpapcode,EVL_Event,0.1))) return false;
            }
            Code[1]->AddEvent(t,0);
            break;

        case 0x02: // Pressure ???
            data[0]=buffer[pos++];
//            if (!Code[2]) {
//                if (!(Code[2]=session->AddEventList(cpapcode,EVL_Event,0.1))) return false;
//            }
//            Code[2]->AddEvent(t,data[0]);
            break;
        case 0x04: // Pressure Pulse??
            data[0]=buffer[pos++];
            if (!Code[3]) {
                if (!(Code[3]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[3]->AddEvent(t,data[0]);
            break;

        case 0x05:
            //code=CPAP_Obstructive;
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            OA->AddEvent(tt,data[0]);
            break;

        case 0x06:
            //code=CPAP_ClearAirway;
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!CA) {
                if (!(CA=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            CA->AddEvent(tt,data[0]);
            break;
        case 0x07:
            //code=CPAP_Hypopnea;
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            HY->AddEvent(tt,data[0]);
            break;
        case 0x08: // ???
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            qDebug() << "Code 8 found at " << hex << pos-1 << " " << tt;
	    if (!Code[10]) {
                if (!(Code[10]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }

            //????
            //data[1]=buffer[pos++]; // ???
            Code[10]->AddEvent(tt,data[0]);
	    pos++;
            break;
        case 0x09: // ASV Codes
            //code=CPAP_FlowLimit;
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!FL) {
                if (!(FL=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            FL->AddEvent(tt,data[0]);

            break;

        case 0x0a:
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!Code[7]) {
                if (!(Code[7]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[7]->AddEvent(tt,data[0]);
            break;


        case 0x0b: // Cheyne Stokes
            data[0]=((unsigned char *)buffer)[pos+1]<<8 | ((unsigned char *)buffer)[pos];
            //data[0]*=2;
            pos+=2;
            data[1]=((unsigned char *)buffer)[pos]; //|buffer[pos+1] << 8
            pos+=1;
            //tt-=delta;
            tt-=qint64(data[1])*1000L;
            if (!CSR) {
                if (!(CSR=session->AddEventList(cpapcode,EVL_Event))) {
		    qDebug() << "!CSR addeventlist exit";
                    return false;
		}
            }
            CSR->AddEvent(tt,data[0]);
            break;

        case 0x0c:
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
	    qDebug() << "Code 12 found at " << hex << pos-1 << " " << tt;
            if (!Code[8]) {
                if (!(Code[8]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[8]->AddEvent(tt,data[0]);
	    pos+=2;
            break;

        case 0x0d: // All the other ASV graph stuff.
            IPAP->AddEvent(t,data[0]=buffer[pos++]);    // 00=IAP
            data[4]=buffer[pos++];
            IPAPLo->AddEvent(t,data[4]);                // 01=IAP Low
            data[5]=buffer[pos++];
            IPAPHi->AddEvent(t,data[5]);                // 02=IAP High
            LEAK->AddEvent(t,buffer[pos++]);            // 03=LEAK
            RR->AddEvent(t,buffer[pos++]);              // 04=Breaths Per Minute
            PTB->AddEvent(t,buffer[pos++]);             // 05=Patient Triggered Breaths
            MV->AddEvent(t,buffer[pos++]);              // 06=Minute Ventilation
            //tmp=buffer[pos++] * 10.0;
            TV->AddEvent(t,buffer[pos++]);              // 07=Tidal Volume
            SNORE->AddEvent(t,data[2]=buffer[pos++]);   // 08=Snore
            if (data[2]>0) {
                if (!VS) {
                    if (!(VS=session->AddEventList(CPAP_VSnore,EVL_Event))) {
			qDebug() << "!VS eventlist exit";
                        return false;
		    }
                }
                VS->AddEvent(t,0); //data[2]); // VSnore
            }
            EPAP->AddEvent(t,data[1]=buffer[pos++]);    // 09=EPAP
            data[2]=data[0]-data[1];
            PS->AddEvent(t,data[2]);            // Pressure Support
            break;
        case 0x03: // BIPAP Pressure
            qDebug() << "0x03 Observed in ASV data!!????";

            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
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
            data[0]=buffer[pos++]; // << 8) | buffer[pos];
            //session->AddEvent(new Event(t,cpapcode, 0, data, 1));
            break;

        case 0x10: // Unknown
            qDebug() << "0x10 Observed in ASV data!!????";
            data[0]=buffer[pos++]; // << 8) | buffer[pos];
            data[1]=buffer[pos++];
            data[2]=buffer[pos++];
            //session->AddEvent(new Event(t,cpapcode, 0, data, 3));
            break;
        case 0x0f:
            qDebug() << "0x0f Observed in ASV data!!????";

            data[0]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            data[1]=buffer[pos]; //|buffer[pos+1] << 8
            pos+=1;
            tt-=qint64(data[1])*1000L;
            //session->AddEvent(new Event(tt,cpapcode, 0, data, 2));
            break;
        case 0x12: // Summary
            qDebug() << "0x12 Observed in ASV data!!????";
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            data[2]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            //session->AddEvent(new Event(t,cpapcode, 0, data,3));
            break;
        default:  // ERROR!!!
            qWarning() << "Some new fandangled PRS1 code detected " << hex << int(code) << " at " << pos-1;
            badcode=true;
            break;
        }
        if (badcode) {
            break;
        }
    }
    session->updateLast(t);
    session->m_cnt.clear();
    session->m_cph.clear();
    session->settings[CPAP_IPAPLo]=session->Min(CPAP_IPAPLo);
    session->settings[CPAP_IPAPHi]=session->Max(CPAP_IPAPHi);
    session->settings[CPAP_PSMax]=session->Max(CPAP_IPAPHi) - session->Min(CPAP_EPAP);
    session->settings[CPAP_PSMin]=session->Min(CPAP_IPAPLo) - session->Min(CPAP_EPAP);

    session->m_valuesummary[CPAP_Pressure].clear();
    session->m_valuesummary.erase(session->m_valuesummary.find(CPAP_Pressure));
    return true;

}

bool PRS1Loader::Parse002(qint32 sequence, quint32 timestamp, unsigned char *buffer, quint16 size)
{
    if (!new_sessions.contains(sequence))
        return false;

    unsigned char code;
    EventList * Code[0x20]={0};
    EventDataType data[10];
    int cnt=0;
    short delta;
    int tdata;
    quint64 pos;
    qint64 t=qint64(timestamp)*1000L,tt;

    Session *session=new_sessions[sequence];
    session->updateFirst(t);


    EventList * OA=session->AddEventList(CPAP_Obstructive, EVL_Event);
    EventList * HY=session->AddEventList(CPAP_Hypopnea, EVL_Event);
    EventList * CSR=session->AddEventList(CPAP_CSR, EVL_Event);
    EventList * LEAK=session->AddEventList(CPAP_LeakTotal,EVL_Event);
    EventList * SNORE=session->AddEventList(CPAP_Snore,EVL_Event);

    EventList * CA=NULL; //session->AddEventList(CPAP_ClearAirway, EVL_Event);
    EventList * VS=NULL, * VS2=NULL, * FL=NULL,* RE=NULL;

    EventList * PRESSURE=NULL;
    EventList * EPAP=NULL;
    EventList * IPAP=NULL;
    EventList * PS=NULL;

    EventList * PP=NULL;

    //session->AddEventList(CPAP_VSnore, EVL_Event);
    //EventList * VS=session->AddEventList(CPAP_Obstructive, EVL_Event);

    for (pos=0;pos<size;) {
        code=buffer[pos++];
        if (code>0x12) {
            qDebug() << "Illegal PRS1 code in" << sequence << hex << int(code) << " appeared at " << hex << pos;
            return false;
        }
        if (code!=0x12) {
            delta=buffer[pos+1] << 8 | buffer[pos];
            pos+=2;
            t+=qint64(delta)*1000L;
            tt=t;
        }
        cnt++;
        switch (code) {

        case 0x00: // Unknown 00
            if (!Code[0]) {
                if (!(Code[0]=session->AddEventList(PRS1_00,EVL_Event))) return false;
            }
            Code[0]->AddEvent(t,buffer[pos++]);
            break;
        case 0x01: // Unknown
            if (!Code[1]) {
                if (!(Code[1]=session->AddEventList(PRS1_01,EVL_Event))) return false;
            }
            Code[1]->AddEvent(t,0);
            break;
        case 0x02: // Pressure
            if (!PRESSURE) {
                PRESSURE=session->AddEventList(CPAP_Pressure,EVL_Event,0.1);
                if (!PRESSURE) return false;
            }
            PRESSURE->AddEvent(t,buffer[pos++]);
            break;
        case 0x03: // BIPAP Pressure
            if (!EPAP) {
                if (!(EPAP=session->AddEventList(CPAP_EPAP,EVL_Event,0.1))) return false;
                if (!(IPAP=session->AddEventList(CPAP_IPAP,EVL_Event,0.1))) return false;
                if (!(PS=session->AddEventList(CPAP_PS,EVL_Event,0.1))) return false;
            }
            EPAP->AddEvent(t,data[0]=buffer[pos++]);
            IPAP->AddEvent(t,data[1]=buffer[pos++]);
            PS->AddEvent(t,data[1]-data[0]);
            break;
        case 0x04: // Pressure Pulse
            if (!PP) {
                if (!(PP=session->AddEventList(CPAP_PressurePulse,EVL_Event))) return false;
            }
            PP->AddEvent(t,buffer[pos++]);
            break;
        case 0x05: // RERA
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            if (!RE) {
                if (!(RE=session->AddEventList(CPAP_RERA,EVL_Event))) return false;
            }
            RE->AddEvent(tt,data[0]);
            break;

        case 0x06: // Obstructive Apoanea
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            OA->AddEvent(tt,data[0]);
            break;
        case 0x07: // Clear Airway
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            if (!CA) {
                if (!(CA=session->AddEventList(CPAP_ClearAirway,EVL_Event))) return false;
            }
            CA->AddEvent(tt,data[0]);
            break;

        case 0x0a: // Hypopnea
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            HY->AddEvent(tt,data[0]);
            break;

        case 0x0c: // Flow Limitation
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            if (!FL) {
                if (!(FL=session->AddEventList(CPAP_FlowLimit,EVL_Event))) return false;
            }
            FL->AddEvent(tt,data[0]);
            break;

        case 0x0b: // Hypopnea related code
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            if (!Code[12]) {
                if (!(Code[12]=session->AddEventList(PRS1_0B,EVL_Event))) return false;
            }
            // FIXME
            Code[12]->AddEvent(t,data[0]);
            break;

        case 0x0d: // Vibratory Snore
            if (!VS) {
                if (!(VS=session->AddEventList(CPAP_VSnore,EVL_Event))) return false;
            }
            VS->AddEvent(t,0);
            break;
        case 0x11: // Leak Rate & Snore Graphs
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];

            LEAK->AddEvent(t,data[0]);
            SNORE->AddEvent(t,data[1]);

            if (data[1]>0) {
                if (!VS2) {
                    if (!(VS2=session->AddEventList(CPAP_VSnore2,EVL_Event))) return false;
                }
                VS2->AddEvent(t,data[1]);
            }
            break;
        case 0x0e: // Unknown
            data[0]=((char *)buffer)[pos++];
            data[1]=buffer[pos++]; //(buffer[pos+1] << 8) | buffer[pos];
            //data[0]/=10.0;
            //pos+=2;
            data[2]=buffer[pos++];

            if (!Code[17]) {
                if (!(Code[17]=session->AddEventList(PRS1_0E,EVL_Event))) return false;
            }
            tdata=unsigned(data[1]) << 8 | unsigned(data[0]);
            Code[17]->AddEvent(t,tdata);
            //qDebug() << hex << data[0] << data[1] << data[2];
            //session->AddEvent(new Event(t,cpapcode, 0, data, 3));
            //tt-=data[1]*1000;
            //session->AddEvent(new Event(t,CPAP_CSR, data, 2));
            break;
        case 0x10: // Unknown
            data[0]=buffer[pos++]; // << 8) | buffer[pos];
            data[1]=buffer[pos++];
            data[2]=buffer[pos++];
            if (!Code[20]) {
                if (!(Code[20]=session->AddEventList(PRS1_10,EVL_Event))) return false;
            }
            Code[20]->AddEvent(t,data[0]);
            break;
        case 0x0f: // Cheyne Stokes Respiration
            data[0]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            data[1]=buffer[pos++];
            tt=t-qint64(data[1])*1000L;
            CSR->AddEvent(tt,data[0]);
            break;
        case 0x12: // Summary
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            data[2]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            if (!Code[24]) {
                if (!(Code[24]=session->AddEventList(PRS1_12,EVL_Event))) return false;
            }
            Code[24]->AddEvent(t,data[0]);
            break;
        default:
            // ERROR!!!
            qWarning() << "Some new fandangled PRS1 code detected in" << sequence << hex << int(code) << " at " << pos-1;
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


//bool PRS1Loader::ParseWaveform(qint32 sequence, quint32 timestamp, unsigned char *data, quint16 size, quint16 duration, quint16 num_signals, quint16 interleave, quint8 sample_format)
//{
//    Q_UNUSED(interleave)
//    Q_UNUSED(sample_format)
//    // this whole function is currently unused..

//    if (!new_sessions.contains(sequence))
//        return false;

//    qint64 t=qint64(timestamp)*1000L;

//    Session *session=new_sessions[sequence];

//    if (num_signals==1) {
//        session->updateFirst(t);
//        double d=duration*1000;
//        EventDataType rate=d / EventDataType(size);
//        EventList *ev=session->AddEventList(CPAP_FlowRate,EVL_Waveform,1.0,0.00,0,0,rate);
//        ev->AddWaveform(t,(char *)data,size,qint64(duration)*1000L);
//        session->updateLast(t+qint64(duration)*1000L);

//    }

//    return true;
//}

bool PRS1Loader::OpenFile(Machine *mach, QString filename)
{
    int sequence,version;
    quint32 timestamp;
    qint64 pos;
    unsigned char ext,sum, htype;
    unsigned char *header,*data;
    int chunk,hl;
    quint16 size,datasize,c16,crc;

    // waveform stuff
    vector<WaveHeaderList> whl;
    quint16 interleave,duration,num_signals;
    quint8  sample_format;

    QFile f(filename);
    if (!f.exists())
        return false;

    if (!f.open(QIODevice::ReadOnly))
        return false;

    qint64 filesize=f.size();
    if (f.read((char *)m_buffer,filesize)<filesize) {
        qDebug() << "Couldn't read full file" << filename;
        return false;
    }
    pos=0;
    bool wasfaulty=false, faulty=false;
    for (chunk=0;pos<filesize;++chunk, pos+=size) {
        header=&m_buffer[pos];
        if (header[0]!=PRS1_MAGIC_NUMBER) {
            if (chunk==0)
	    	qDebug() << "Invalid magic number" << filename;
                return false;
            break;
        }

        size=(header[2] << 8) | header[1];
        htype=header[3]; // 00 = normal // 01=waveform // could be a bool?
        Q_UNUSED(htype);
        version=header[4]; // == 5
        ext=header[6];
        sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
        timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];

        if (ext==5) {
            duration=header[0xf] | header[0x10] << 8;    // block duration in seconds
            Q_UNUSED(duration);
            num_signals=header[0x12] | header[0x13] << 8;
            if (num_signals>2) {
                qWarning() << "More than 2 Waveforms in " << filename;
                return false;
            }
            long pos2=0x14+(num_signals-1)*3;
            // add the in reverse...
            for (int i=0;i<num_signals;i++) {
                interleave=header[pos2] | header[pos2+1] << 8;
                sample_format=header[pos2+2];
                whl.push_back(WaveHeaderList(interleave,sample_format));
                pos2-=3;
            }
            hl=15+6+(num_signals*3);

        } else hl=16;

        wasfaulty=faulty;
        sum=0;
        for (int i=0; i<hl-1; i++) sum+=header[i];
        unsigned char c8=header[hl-1];
        if (sum!=c8) {
            if (chunk==0) {
                qDebug() << "File" << filename << "has faulty header at" << pos;
                if ((size>0) && (pos+size<filesize) && (header[size]==2)) {
                    faulty=true;
                    continue;
                }

                return (chunk!=0);
            }
            break;
        }
        datasize=size-hl-2;

        data=&header[hl];

#ifdef PRS1_CRC_CHECK
        c16=CRC16(data,datasize);
        crc=(data[datasize+1] << 8) | data[datasize];
        if (crc!=c16) {
            qDebug() << "File" << filename << "failed CRC check at chunk" << chunk << "wanted" << crc << "got" << c16;
            faulty=true;
            continue;
        }
#endif

        if (wasfaulty) {
            qDebug() << "Managed to continue with next block";
        }


        qDebug() << "Loading" << filename << sequence << timestamp << size;
        //if (ext==0) ParseCompliance(data,size);
        if (ext<=1) {
	    qDebug() << "Call Parse Summary";
            ParseSummary(mach,sequence,timestamp,data,datasize,version);
        } else if (ext==2) {
            if (version==5) {
	       qDebug() << "Call parse002v5 hl=" << hl;
               if (!Parse002v5(sequence,timestamp,data,datasize)) {
                   qDebug() << "in file: " << filename;
               }
            } else {
	       qDebug() << "Call parse002";
               Parse002(sequence,timestamp,data,datasize);
            }
        } else if (ext==5) {
            //ParseWaveform(mach,sequence,timestamp,data,datasize,duration,num_signals,interleave,sample_format);
        }
    }
    return true;
}


bool PRS1Loader::OpenWaveforms(SessionID sid, QString filename)
{
    Session * session=new_sessions[sid];
    //int sequence,seconds,br,htype,version,numsignals;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open waveform file" << filename;
        return false;
    }

    int pos,br,size=file.size();
    if (size>max_load_buffer_size) {
        qWarning() << "Waveform too big, increase max_load_buffer_size in PRS1Loader sourcecode" << filename;
        return false;
    }

    br=file.read((char *)m_buffer,size);
    if (br<size) {
        qWarning() << "Couldn't read waveform data.. Disk error?" << filename;
        return false;
    }

    // Look at the initial header and assume this header size for the lot.
    if ((m_buffer[0]!=2) || (m_buffer[6]!=0x05)) {
        qWarning() << "Not correct waveform format" << filename;
        return false;
    }
    //quint8 version=m_buffer[4];
    quint32 start=m_buffer[0xb] | m_buffer[0xc] << 8 | m_buffer[0xd] << 16 | m_buffer[0x0e] << 24;

    session->updateFirst(qint64(start)*1000L);

    quint16 num_signals=m_buffer[0x12] | m_buffer[0x13] << 8;
    if (num_signals>2) {
        qWarning() << "More than 2 Waveforms in " << filename;
        return false;
    }
    pos=0x14+(num_signals-1)*3;
    vector<WaveHeaderList> whl;
    // add the in reverse...
    for (int i=0;i<num_signals;i++) {
        quint16 interleave=m_buffer[pos] | m_buffer[pos+1] << 8;
        quint8  sample_format=m_buffer[pos+2];
        whl.push_back(WaveHeaderList(interleave,sample_format));
        pos-=3;
    }
    int hl=15+5+(num_signals*3);
    quint16 duration,length=0;
    quint32 timestamp,lasttimestamp;


    pos=0;
    int block=0;
    lasttimestamp=start;
    duration=0;
    int corrupt=0;
    char waveform[2][500000];
    int wlength[2];
    qint64 wdur[2];
    //EventList *evl[num_signals];
    for (int i=0;i<num_signals;i++) {
        wlength[i]=0;
        wdur[i]=0;
        //evl[i]=NULL;

        //waveform[i].resize(500000);
    }
    //SampleFormat *wf[num_signals];
    //EventList *FlowData=EventList(CPAP_FlowRate, EVL_Waveform,1,0,-128,128);
    //EventList *MaskData=EventList(CPAP_MaskPressure, EVL_Waveform,1,0,-128,128);

    //QString FlowRate=CPAP_FlowRate;
    //QString MaskPressure="MaskPressure";
    ChannelID wc[2]={CPAP_FlowRate,CPAP_MaskPressure};
    do {
        timestamp=m_buffer[pos+0xb] | m_buffer[pos+0xc] << 8 | m_buffer[pos+0xd] << 16 | m_buffer[pos+0x0e] << 24;
        register unsigned char sum8=0;
        for (int i=0;i<hl;i++) sum8+=m_buffer[pos+i];
        if (m_buffer[pos+hl]!=sum8) {
            if (block==0) {
                qDebug() << "Faulty Header Checksum, aborting" << filename;
                return false;
            }
            if (++corrupt>3) {
                qDebug() << "3 Faulty Waveform Headers in a row.. Aborting" << filename;
                return false;
            }
            qDebug() << "Faulty Header Checksum, skipping block" << block;
            pos+=length;
            continue;
        } else {
            int diff=timestamp-(lasttimestamp+duration);
            if (block==1) {
                if (diff>0)
                    start-=diff;
            }
            length=m_buffer[pos+0x1] | m_buffer[pos+0x2] << 8;      // block length in bytes
            duration=m_buffer[pos+0xf] | m_buffer[pos+0x10] << 8;    // block duration in seconds


            if (diff<0) {
                qDebug() << "Padding waveform to keep sync" << block;
                //diff=qAbs(diff);
                for (int i=0;i<num_signals;i++) {
                    for (int j=0;j<diff;j++) {
                        for (int k=0;k<whl[i].interleave;k++)
                            waveform[i][wlength[i]++]=0;
                    }
                    wdur[i]+=diff;
                }

            } else
            if (diff>0 && wlength[0]>0)  {
                qDebug() << "Timestamp resync" << block << diff << corrupt << duration << timestamp-lasttimestamp << filename;


                for (int i=0;i<num_signals;i++) {
                    double rate=(double(wdur[i])*1000.0)/double(wlength[i]);
                    double gain;
                    if (i==1) gain=0.1; else gain=1;
                    EventList *a=session->AddEventList(wc[i],EVL_Waveform,gain,0,0,0,rate);
                    //EventList *a=new EventList(wc[i],EVL_Waveform,gain,0,0,0,rate);
                    //session->machine()->registerChannel(wc[i]);
                    if (whl[i].sample_format)
                        a->AddWaveform(qint64(start)*1000L,(unsigned char *)waveform[i],wlength[i],qint64(wdur[i])*1000L);
                    else {
                        a->AddWaveform(qint64(start)*1000L,(char *)waveform[i],wlength[i],qint64(wdur[i])*1000L);
                    }
                    if (wc[i]==CPAP_FlowRate) {
                        a->setMax(120);
                        a->setMin(-120);
                    } else if (wc[i]==CPAP_MaskPressure) {
//                       int v=ceil(a->Max()/5);
//                        a->setMax(v*5);
//                        v=floor(a->Min()/5);
//                        a->setMin(v*5);
                    }

                    session->updateLast(start+(qint64(wdur[i])*1000L));
                    wlength[i]=0;
                    wdur[i]=0;
                }
                start=timestamp;
                corrupt=0;
            }
        }

        pos+=hl+1;
        //qDebug() <<  (duration*num_signals*whl[0].interleave) << duration;
        if (num_signals==1) { // no interleave.. this is much quicker.
            int bs=duration*whl[0].interleave;
            memcpy((char *)&(waveform[0])[wlength[0]],(char *)&m_buffer[pos],bs);

            wlength[0]+=bs;

            pos+=bs;
        } else {
            for (int i=0;i<duration;i++) {
                for (int s=0;s<num_signals;s++) {
                    memcpy((char *)&(waveform[s])[wlength[s]],(char *)&m_buffer[pos],whl[s].interleave);
                    wlength[s]+=whl[s].interleave;
                    pos+=whl[s].interleave;
                }
            }
        }
        for (int i=0;i<num_signals;i++) wdur[i]+=duration;
        pos+=2;
        block++;
        lasttimestamp=timestamp;
    } while (pos<size);

    for (int i=0;i<num_signals;i++) {
        double rate=(double(wdur[i])*1000.0)/double(wlength[i]);
        double gain;
        if (i==1) gain=0.1; else gain=1;
        EventList *a=session->AddEventList(wc[i],EVL_Waveform,gain,0,0,0,rate);

        if (whl[i].sample_format)
            a->AddWaveform(qint64(start)*1000L,(unsigned char *)waveform[i],wlength[i],qint64(wdur[i])*1000L);
        else {
            a->AddWaveform(qint64(start)*1000L,(char *)waveform[i],wlength[i],qint64(wdur[i])*1000L);
        }
        if (wc[i]==CPAP_FlowRate) {
            a->setMax(120);
            a->setMin(-120);
        } else if (wc[i]==CPAP_MaskPressure) {
        }
        session->updateLast(start+qint64(wdur[i])*1000L);
    }
    // lousy family 5 check to see if already has RespRate
    return true;
}

void InitModelMap()
{
    ModelMap[0x34]="RemStar Pro with C-Flex+";
    ModelMap[0x35]="RemStar Auto with A-Flex";
    ModelMap[0x36]="RemStar BiPAP Pro with Bi-Flex";
    ModelMap[0x37]="RemStar BiPAP Auto with Bi-Flex";
    ModelMap[0x38]="RemStar Plus :(";
    ModelMap[0x41]="BiPAP autoSV Advanced";
}

bool initialized=false;
void PRS1Loader::Register()
{
    if (initialized) return;
    qDebug() << "Registering PRS1Loader";
    RegisterLoader(new PRS1Loader());
    InitModelMap();
    initialized=true;
}




