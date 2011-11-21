/*

SleepLib PRS1 Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
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

PRS1::PRS1(Profile *p,MachineID id):CPAP(p,id)
{
    m_class=prs1_class_name;
    properties["Brand"]="Philips Respironics";
    properties["Model"]="System One";

}
PRS1::~PRS1()
{

}


PRS1Loader::PRS1Loader()
{
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

    QVector<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    QVector<Machine *>::iterator i;
    for (i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()=="PRS1") && ((*i)->properties["Serial"]==serial)) {
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

    m->properties["Serial"]=serial;
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
            m->properties["SubModel"]=ModelMap[i];
        }
    }
    if (prop["SerialNumber"]!=m->properties["Serial"]) {
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
        //if (qprogress) qprogress->Pulse();
    }

    SessionID session;
    long ext;
    typedef QVector<QString> StringList;
    QMap<SessionID,StringList> sessfiles;
    int size=paths.size();
    int cnt=0;
    bool ok;
    for (QList<QString>::iterator p=paths.begin(); p!=paths.end(); p++) {
        dir.setPath(*p);
        if (!dir.exists() || !dir.isReadable()) continue;
        flist=dir.entryInfoList();

        for (int i=0;i<flist.size();i++) {
            QFileInfo fi=flist.at(i);
            QString ext_s=fi.fileName().section(".",-1);
            QString session_s=fi.fileName().section(".",0,-2);

            ext=ext_s.toLong(&ok);
            if (!ok) continue;
            session=session_s.toLong(&ok);
            if (!ok) continue;

            if (sessfiles[session].capacity()==0) sessfiles[session].resize(3);

            if (ext==1) {
                 sessfiles[session][0]=fi.canonicalFilePath();
            } else if (ext==2) {
                 sessfiles[session][1]=fi.canonicalFilePath();
            } else if (ext==5) {
                 sessfiles[session][2]=fi.canonicalFilePath();
            }
            cnt++;

            if (qprogress) qprogress->setValue((float(cnt)/float(size)*33.0));

            QApplication::processEvents();

        }
    }
    size=sessfiles.size();
    if (size==0)
        return 0;

    cnt=0;
    for (QMap<SessionID,StringList>::iterator s=sessfiles.begin(); s!=sessfiles.end(); s++) {
        session=s.key();
        cnt++;
        if (qprogress) qprogress->setValue(33.0+(float(cnt)/float(size)*33.0));
        QApplication::processEvents();

        if (m->SessionExists(session)) continue;
        if (s.value()[0].isEmpty()) continue;

        Session *sess=new Session(m,session);
        if (!OpenSummary(sess,s.value()[0])) {
            //qWarning() << "PRS1Loader: Dodgy summary file " << s.value()[0];

            delete sess;
            continue;
        }
        //sess->SetSessionID(sess->start().GetTicks());
        if (!s.value()[1].isEmpty()) {
            if (!OpenEvents(sess,s.value()[1])) {
                qWarning() << "PRS1Loader: Couldn't open event file " << s.value()[1];
            }
        }
        if (!s.value()[2].isEmpty()) {
            if (!OpenWaveforms(sess,s.value()[2])) {
                qWarning() << "PRS1Loader: Couldn't open event file " << s.value()[2];
            }
        }
        if ((sess->last() - sess->first()) == 0) {
        //const double ignore_thresh=300.0/3600.0;// Ignore useless sessions under 5 minute
        //if (sess->hours()<=ignore_thresh) {
            qDebug() << "Ignoring empty session" << session;//  << "which is only" << (sess->hours()*60.0) << "minute(s) long";
            delete sess;
            continue;
        }


        if (sess->count(CPAP_IPAP)>0) {
            //sess->summaryCPAP_Mode]!=MODE_ASV)
            sess->settings[CPAP_Mode]=MODE_BIPAP;

            if (sess->settings[PRS1_FlexMode].toInt()!=PR_NONE) {
                sess->settings[PRS1_FlexMode]=PR_BIFLEX;
            }


            sess->setAvg(CPAP_Pressure,(sess->avg(CPAP_EPAP)+sess->avg(CPAP_IPAP))/2.0);
            sess->setWavg(CPAP_Pressure,(sess->wavg(CPAP_EPAP)+sess->wavg(CPAP_IPAP))/2.0);
            sess->setMin(CPAP_Pressure,sess->min(CPAP_EPAP));
            sess->setMax(CPAP_Pressure,sess->max(CPAP_IPAP));
            sess->set90p(CPAP_Pressure,(sess->p90(CPAP_IPAP)+sess->p90(CPAP_EPAP))/2.0);
            //sess->p90(CPAP_EPAP);
            //sess->p90(CPAP_IPAP);
        } else {
            //sess->avg(CPAP_Pressure);
            //sess->wavg(CPAP_Pressure);
            //sess->p90(CPAP_Pressure);
            //sess->min(CPAP_Pressure);
            //sess->max(CPAP_Pressure);
            //sess->cph(CPAP_Pressure);

            if (!sess->settings.contains(CPAP_PressureMin)) {
                sess->settings[CPAP_BrokenSummary]=true;
                //sess->set_last(sess->first());
                if (sess->min(CPAP_Pressure)==sess->max(CPAP_Pressure)) {
                    sess->settings[CPAP_Mode]=MODE_CPAP; // no ramp
                } else {
                    sess->settings[CPAP_Mode]=MODE_UNKNOWN;
                }
                //sess->Set("FlexMode",PR_UNKNOWN);
            }

        }
        if (sess->settings[CPAP_Mode]==MODE_CPAP) {
            sess->settings[CPAP_PressureMax]=sess->settings[CPAP_PressureMin];
        }

        //Printf(sess->start().Format()+wxT(" avgsummary=%.3f avgmine=%.3f\n"),sess->summary[CPAP_PressureAverage].GetDouble(),sess->weighted_avg_event_field(CPAP_Pressure,0));
        sess->SetChanged(true);
        m->AddSession(sess,profile);

    }
    QString s;
    s.sprintf("%i",prs1_data_version);
    m->properties["DataVersion"]=s;
    m->Save(); // Save any new sessions to disk in our format */
    if (qprogress) qprogress->setValue(100);

    //qDebug() << "OpenMachine Done";
    return true;
}
//bool PRS1Loader::OpenMachine()
//{
//}

bool PRS1Loader::OpenSummary(Session *session,QString filename)
{
    int size,seconds,br,htype,version,sequence;
    qint64 timestamp;
    unsigned char header[24];
    unsigned char ext,sum;

    //qDebug() << "Opening PRS1 Summary " << filename;
    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly))
          return false;

    if (!f.exists())
        return false;

    int hl=16;

    br=f.read((char *)header,hl);

    if (header[0]!=PRS1_MAGIC_NUMBER)
        return false;

    sequence=size=timestamp=seconds=ext=0;
    sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
    timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
    size=(header[2] << 8) | header[1];
    ext=header[6];
    htype=header[3]; // 00 = normal // 01=waveform // could be a bool?
    version=header[4];
    sequence=sequence;
    version=version; // don't need it here?

    htype=htype; // shut the warning up.. this is useless.

    if (ext!=PRS1_SUMMARY_FILE)
        return false;

    size-=(hl+2);

    // Calculate header checksum and compare to verify header
    sum=0;
    for (int i=0; i<hl-1; i++) sum+=header[i];
    if (sum!=header[hl-1])
        return false;

    if (size<=19) {
      //  qDebug() << "Ignoring short session file " << filename;
        return false;
    }

    qint64 date=timestamp*1000;


    //memset(m_buffer,0,size);
    unsigned char * buffer=m_buffer;
    br=f.read((char *)buffer,size);
    if (br<size) {
        return false;
    }
    if (size<0x30)
        return true;

    session->set_first(date);

    double max;
    session->settings[CPAP_PressureMin]=(EventDataType)buffer[0x03]/10.0;
    session->settings[CPAP_PressureMax]=max=(EventDataType)buffer[0x04]/10.0;
    int offset=0;
    if (buffer[0x05]!=0) { // This is a time value for ASV stuff
        // non zero adds extra fields..
        offset=4;
    }

    session->settings[CPAP_RampTime]=(int)buffer[offset+0x06]; // Minutes. Convert to seconds/hours here?
    session->settings[CPAP_RampPressure]=(EventDataType)buffer[offset+0x07]/10.0;

    if (max>0) { // Ignoring bipap until I see some more data.
        session->settings[CPAP_Mode]=(int)MODE_APAP;
    } else session->settings[CPAP_Mode]=(int)MODE_CPAP;

    // This is incorrect..
    if (buffer[offset+0x08] & 0x80) { // Flex Setting
        if (buffer[offset+0x08] & 0x08) {
            if (max>0) session->settings[PRS1_FlexMode]=(int)PR_AFLEX;
            else session->settings[PRS1_FlexMode]=(int)PR_CFLEXPLUS;
        } else session->settings[PRS1_FlexMode]=(int)PR_CFLEX;
    } else session->settings[PRS1_FlexMode]=(int)PR_NONE;

    session->settings["FlexSet"]=(int)buffer[offset+0x08] & 3;
    session->settings["HumidSet"]=(int)buffer[offset+0x09]&0x0f;
    session->settings["HumidStat"]=(buffer[offset+0x09]&0x80)==0x80;
    session->settings["SysLock"]=(buffer[offset+0x0a]&0x80)==0x80;
    session->settings["SysOneResistStat"]=(buffer[offset+0x0a]&0x40)==0x40;
    session->settings["SysOneResistSet"]=(int)buffer[offset+0x0a]&7;
    session->settings["HoseDiam"]=((buffer[offset+0x0a]&0x08)?"15mm":"22mm");
    session->settings["AutoOff"]=(buffer[offset+0x0c]&0x10)==0x10;
    session->settings["MaskAlert"]=(buffer[offset+0x0c]&0x08)==0x08;
    session->settings["ShowAHI"]=(buffer[offset+0x0c]&0x04)==0x04;

    unsigned duration=buffer[offset+0x14] | (buffer[0x15] << 8);
    //session->settings[CPAP_Duration]=(int)duration;
    //qDebug() << "ID: " << session->session() << " " << duration;
    //float hours=float(duration)/3600.0;
    //session->set_hours(hours);
    if (!duration)
        return false;

    session->set_last(date+qint64(duration)*1000L);
    //session->settings[PRS1_PressureMinAchieved]=buffer[offset+0x16]/10.0;
    //session->settings[PRS1_PressureMaxAchieved]=buffer[offset+0x17]/10.0;
    //session->settings[PRS1_PressureAvg]=buffer[offset+0x18]/10.0;
    //session->settings[PRS1_Pressure90]=buffer[offset+0x19]/10.0;

    if (max==0) {
      //  session->settings[PRS1_PressureAvg]=session->settings[PRS1_PressureMin];
    }

    // Not using these because sometimes this summary is broken.
   /*/if (size==0x4d) {

        session->settings[CPAP_Obstructive]=(int)buffer[offset+0x1C] | (buffer[offset+0x1D] << 8);
        session->settings[CPAP_ClearAirway]=(int)buffer[offset+0x20] | (buffer[offset+0x21] << 8);
        session->settings[CPAP_Hypopnea]=(int)buffer[offset+0x2A] | (buffer[offset+0x2B] << 8);
        session->settings[CPAP_RERA]=(int)buffer[offset+0x2E] | (buffer[offset+0x2F] << 8);
        session->settings[CPAP_FlowLimit]=(int)buffer[offset+0x30] | (buffer[offset+0x31] << 8);
    }*/
    return true;
}

// v2 event parser.
bool PRS1Loader::Parse002(Session *session,unsigned char *buffer,int size,qint64 timestamp)
{
    /*ChannelID Codes[]={
        PRS1_Unknown00, PRS1_Unknown01, CPAP_Pressure, CPAP_EPAP, CPAP_PressurePulse, CPAP_RERA, CPAP_Obstructive, CPAP_ClearAirway,
        PRS1_Unknown08, PRS1_Unknown09, CPAP_Hypopnea, PRS1_Unknown0B, CPAP_FlowLimit, CPAP_VSnore, PRS1_Unknown0E, CPAP_CSR, PRS1_Unknown10,
        CPAP_Leak, PRS1_Unknown12
    };
    int ncodes=sizeof(Codes)/sizeof(ChannelID); */

    //QHash<ChannelID,EventList *> Code;
    EventList * Code[0x20]={NULL};

    EventDataType data[10];

    session->updateFirst(timestamp);
    //qint64 start=timestamp;
    qint64 t=timestamp;
    qint64 tt;
    int pos=0;
    int cnt=0;
    short delta;
    while (pos<size) {
        unsigned char code=buffer[pos++];
        if (code>0x12) {
            qDebug() << "Illegal PRS1 code " << hex << int(code) << " appeared at " << hex << pos+16;
            return false;
        }
        delta=0;
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
            if (!Code[2]) {
                Code[2]=session->AddEventList(CPAP_Pressure,EVL_Event,0.1);
                if (!Code[2]) return false;
            }
            Code[2]->AddEvent(t,buffer[pos++]);
            break;
        case 0x03: // BIPAP Pressure
            if (!Code[3]) {
                if (!(Code[3]=session->AddEventList(CPAP_EPAP,EVL_Event,0.1))) return false;
                if (!(Code[4]=session->AddEventList(CPAP_IPAP,EVL_Event,0.1))) return false;
                if (!(Code[5]=session->AddEventList(CPAP_PS,EVL_Event,0.1))) return false;
            }
            Code[3]->AddEvent(t,data[0]=buffer[pos++]);
            Code[4]->AddEvent(t,data[1]=buffer[pos++]);
            Code[5]->AddEvent(t,data[1]-data[0]);
            break;
        case 0x04: // Pressure Pulse
            if (!Code[6]) {
                if (!(Code[6]=session->AddEventList(CPAP_PressurePulse,EVL_Event))) return false;
            }
            Code[6]->AddEvent(t,buffer[pos++]);
            //qDebug() << hex << data[0];
            break;
        case 0x05: // RERA
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            if (!Code[7]) {
                if (!(Code[7]=session->AddEventList(CPAP_RERA,EVL_Event))) return false;
            }
            Code[7]->AddEvent(tt,data[0]);
            break;

        case 0x06: // Obstructive Apoanea
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            if (!Code[8]) {
                if (!(Code[8]=session->AddEventList(CPAP_Obstructive,EVL_Event))) return false;
            }
            Code[8]->AddEvent(tt,data[0]);
            break;
        case 0x07: // Clear Airway
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            if (!Code[9]) {
                if (!(Code[9]=session->AddEventList(CPAP_ClearAirway,EVL_Event))) return false;
            }
            Code[9]->AddEvent(tt,data[0]);
            break;
        case 0x0a: // Hypopnea
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            if (!Code[10]) {
                if (!(Code[10]=session->AddEventList(CPAP_Hypopnea,EVL_Event))) return false;
            }
            Code[10]->AddEvent(tt,data[0]);
            break;
        case 0x0c: // Flow Limitation
            data[0]=buffer[pos++];
            tt=t-(qint64(data[0])*1000L);
            if (!Code[11]) {
                if (!(Code[11]=session->AddEventList(CPAP_FlowLimit,EVL_Event))) return false;
            }
            Code[11]->AddEvent(tt,data[0]);
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
            if (!Code[13]) {
                if (!(Code[13]=session->AddEventList(CPAP_VSnore,EVL_Event))) return false;
            }
            Code[13]->AddEvent(t,0);
            break;
        case 0x11: // Leak Rate & Snore Graphs
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            if (!Code[14]) {
                if (!(Code[14]=session->AddEventList(CPAP_Leak,EVL_Event))) return false;
                if (!(Code[15]=session->AddEventList(CPAP_Snore,EVL_Event))) return false;
            }
            Code[14]->AddEvent(t,data[0]);
            Code[15]->AddEvent(t,data[1]);
            if (data[1]>0) {
                if (!Code[16]) {
                    if (!(Code[16]=session->AddEventList(CPAP_VSnore2,EVL_Event))) return false;
                }
                Code[16]->AddEvent(t,data[1]);
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
            Code[17]->AddEvent(t,data[0]);
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
            if (!Code[23]) {
                if (!(Code[23]=session->AddEventList(CPAP_CSR,EVL_Event))) return false;
            }
            Code[23]->AddEvent(tt,data[0]);
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
            qWarning() << "Some new fandangled PRS1 code detected " << hex << int(code) << " at " << pos+15;
            return false;
        }
    }
    session->updateLast(t);

    return true;
}

bool PRS1Loader::Parse002ASV(Session *session,unsigned char *buffer,int size,qint64 timestamp)
{
    QString Codes[]={
        PRS1_00, PRS1_01, CPAP_Pressure, CPAP_EPAP, CPAP_PressurePulse, CPAP_Obstructive,
        CPAP_ClearAirway, CPAP_Hypopnea, PRS1_08,  CPAP_FlowLimit, PRS1_0A, CPAP_CSR,
        PRS1_0C, CPAP_VSnore, PRS1_0E, PRS1_0F, PRS1_10,
        CPAP_Leak, PRS1_12
    };

    int ncodes=sizeof(Codes)/sizeof(QString);

    EventList * Code[0x20]={NULL};
    //for (int i=0;i<0x20;i++) Code[i]=NULL;

    session->updateFirst(timestamp);
    EventDataType data[10];

    //qint64 start=timestamp;
    qint64 t=timestamp;
    qint64 tt;
    int pos=0;
    int cnt=0;
    short delta;//,duration;
    QDateTime d;
    while (pos<size) {
        unsigned char code=buffer[pos++];
        if (code>=ncodes) {
            qDebug() << "Illegal PRS1 code " << hex << int(code) << " appeared at " << hex << pos+16;
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
        QString cpapcode=Codes[(int)code];
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

        case 0x02: // Pressure
            data[0]=buffer[pos++];
            if (!Code[2]) {
                if (!(Code[2]=session->AddEventList(cpapcode,EVL_Event,0.1))) return false;
            }
            Code[2]->AddEvent(t,data[0]);
            break;
        case 0x04: // Pressure Pulse
            data[0]=buffer[pos++];
            if (!Code[3]) {
                if (!(Code[3]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[3]->AddEvent(t,data[0]);
            break;

        case 0x05:
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!Code[4]) {
                if (!(Code[4]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[4]->AddEvent(tt,data[0]);
            break;

        case 0x06:
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!Code[5]) {
                if (!(Code[5]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[5]->AddEvent(tt,data[0]);
            break;
        case 0x07:
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!Code[6]) {
                if (!(Code[6]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[6]->AddEvent(tt,data[0]);
            break;
        case 0x08: // ASV Codes
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!Code[10]) {
                if (!(Code[10]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[10]->AddEvent(tt,data[0]);
            break;
        case 0x09: // ASV Codes
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!Code[11]) {
                if (!(Code[11]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[11]->AddEvent(tt,data[0]);

            break;

        case 0x0a:
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!Code[7]) {
                if (!(Code[7]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[7]->AddEvent(tt,data[0]);
            break;

        case 0x0c:
            data[0]=buffer[pos++];
            tt-=qint64(data[0])*1000L; // Subtract Time Offset
            if (!Code[8]) {
                if (!(Code[8]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[8]->AddEvent(tt,data[0]);
            break;


        case 0x0b: // Cheyne Stokes
            data[0]=((unsigned char *)buffer)[pos+1]<<8 | ((unsigned char *)buffer)[pos];
            //data[0]*=2;
            pos+=2;
            data[1]=((unsigned char *)buffer)[pos]; //|buffer[pos+1] << 8
            pos+=1;
            //tt-=delta;
            tt-=qint64(data[1])*1000L;
            if (!Code[9]) {
                if (!(Code[9]=session->AddEventList(cpapcode,EVL_Event))) return false;
            }
            Code[9]->AddEvent(tt,data[0]);
            //session->AddEvent(new Event(tt,cpapcode, data[0], data, 2));
            break;
        case 0x0d: // All the other ASV graph stuff.
            if (!Code[12]) {
                if (!(Code[12]=session->AddEventList(CPAP_IPAP,EVL_Event))) return false;
                if (!(Code[13]=session->AddEventList(CPAP_IPAPLo,EVL_Event))) return false;
                if (!(Code[14]=session->AddEventList(CPAP_IPAPHi,EVL_Event))) return false;
                if (!(Code[15]=session->AddEventList(CPAP_Leak,EVL_Event))) return false;
                if (!(Code[16]=session->AddEventList(CPAP_RespRate,EVL_Event))) return false;
                if (!(Code[17]=session->AddEventList(CPAP_PTB,EVL_Event))) return false;

                if (!(Code[18]=session->AddEventList(CPAP_MinuteVent,EVL_Event))) return false;
                if (!(Code[19]=session->AddEventList(CPAP_TidalVolume,EVL_Event))) return false;
                if (!(Code[20]=session->AddEventList(CPAP_Snore,EVL_Event))) return false;
                if (!(Code[22]=session->AddEventList(CPAP_EPAP,EVL_Event))) return false;
                if (!(Code[23]=session->AddEventList(CPAP_PS,EVL_Event))) return false;
            }
            Code[12]->AddEvent(t,data[0]=buffer[pos++]); // IAP
            Code[13]->AddEvent(t,buffer[pos++]); // IAP Low
            Code[14]->AddEvent(t,buffer[pos++]); // IAP High
            Code[15]->AddEvent(t,buffer[pos++]); // LEAK
            Code[16]->AddEvent(t,buffer[pos++]); // Breaths Per Minute
            Code[17]->AddEvent(t,buffer[pos++]); // Patient Triggered Breaths
            Code[18]->AddEvent(t,buffer[pos++]); // Minute Ventilation
            Code[19]->AddEvent(t,buffer[pos++]); // Tidal Volume
            Code[20]->AddEvent(t,data[2]=buffer[pos++]); // Snore
            if (data[2]>0) {
                if (!Code[21]) {
                    if (!(Code[21]=session->AddEventList("VSnore",EVL_Event))) return false;
                }
                Code[21]->AddEvent(t,0); //data[2]); // VSnore
            }
            Code[22]->AddEvent(t,data[1]=buffer[pos++]); // EPAP
            Code[23]->AddEvent(t,data[0]-data[1]);      // Pressure Support
            break;
        case 0x03: // BIPAP Pressure
            qDebug() << "0x03 Observed in ASV data!!????";

            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            /*data[0]/=10.0;
            data[1]/=10.0;
            session->AddEvent(new Event(t,CPAP_EAP, 0, data, 1));
            session->AddEvent(new Event(t,CPAP_IAP, 0, &data[1], 1)); */
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
        default:
            // ERROR!!!
            qWarning() << "Some new fandangled PRS1 code detected " << hex << int(code) << " at " << pos+15;
            return false;
        }
    }
    session->updateLast(t);
    return true;
}


bool PRS1Loader::OpenEvents(Session *session,QString filename)
{
    int size,sequence,seconds,br,version;
    qint64 timestamp;
    unsigned char header[24]; // use m_buffer?
    unsigned char ext,htype;

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    int hl=16;

    br=f.read((char *)header,hl);

    if (header[0]!=PRS1_MAGIC_NUMBER)
        return false;

    sequence=size=timestamp=seconds=ext=0;
    sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
    timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
    size=(header[2] << 8) | header[1];
    ext=header[6];
    htype=header[3]; // 00 = normal // 01=waveform // could be a bool?
    version=header[4];// | header[4];

    htype=htype;
    sequence=sequence;
    if (ext!=PRS1_EVENT_FILE)  // 2 == Event file
        return false;

    //size|=(header[3]<<16) | (header[4]<<24); // the jury is still out on the 32bitness of one. doesn't matter here anyway.

    size-=(hl+2);

    unsigned char sum=0;
    for (int i=0; i<hl-1; i++) sum+=header[i];
    if (sum!=header[hl-1]) return false;

    unsigned char *buffer=(unsigned char *)m_buffer;
    br=f.read((char *)buffer,size);
    if (br<size) {
        return false;
    }
    if (version==0) {
        if (!Parse002(session,buffer,size,timestamp*1000L)) {
            qDebug() << "Couldn't Parse PRS1 Event File " << filename;
            return false;
        }
    } else if (version==5) {
        if (!Parse002ASV(session,buffer,size,timestamp*1000L)) {
            qDebug() << "Couldn't Parse PRS1 (ASV) Event File " << filename;
            return false;
        }
    }

    return true;
}

struct WaveHeaderList {
    quint16 interleave;
    quint8  sample_format;
    WaveHeaderList(quint16 i,quint8 f){ interleave=i; sample_format=f; }
};

bool PRS1Loader::OpenWaveforms(Session *session,QString filename)
{
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
    char waveform[num_signals][500000];
    int wlength[num_signals];
    qint64 wdur[num_signals];
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

    QString FlowRate="FlowRate";
    QString MaskPressure="MaskPressure";
    QString wc[2]={FlowRate,MaskPressure};
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
                //diff=abs(diff);
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
                    if (wc[i]==FlowRate) {
                        a->setMax(120);
                        a->setMin(-120);
                    } else if (wc[i]==MaskPressure) {
                    /*    int v=ceil(a->max()/5);
                        a->setMax(v*5);
                        v=floor(a->min()/5);
                        a->setMin(v*5); */
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
        if (wc[i]==FlowRate) {
            a->setMax(120);
            a->setMin(-120);
        } else if (wc[i]==MaskPressure) {
        }
        session->updateLast(start+qint64(wdur[i])*1000L);
    }
    return true;
}

void InitModelMap()
{
    ModelMap[0x34]="RemStar Pro with C-Flex+";
    ModelMap[0x35]="RemStar Auto with A-Flex";
    ModelMap[0x37]="RemStar BiPAP Auto with Bi-Flex";
    ModelMap[0x41]="BiPAP autoSV Advanced";
};


bool initialized=false;
void PRS1Loader::Register()
{
    if (initialized) return;
    qDebug() << "Registering PRS1Loader";
    RegisterLoader(new PRS1Loader());
    InitModelMap();
    initialized=true;
}

