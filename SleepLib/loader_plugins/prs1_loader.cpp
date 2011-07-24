/*

SleepLib PRS1 Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#include <QString>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProgressBar>
#include <QDebug>

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

map<int,QString> ModelMap;

PRS1::PRS1(Profile *p,MachineID id):CPAP(p,id)
{
    m_class=prs1_class_name;
    properties["Brand"]="Philips Respironics";
    properties["Model"]="System One";

    //SleepFlags= { CPAP_RERA, PRS1_VSnore2, CPAP_FlowLimit, CPAP_Hypopnea, CPAP_Obstructive, CPAP_ClearAirway, CPAP_CSR };
}
PRS1::~PRS1()
{

}


PRS1Loader::PRS1Loader()
{
    m_buffer=new unsigned char [max_load_buffer_size]; //allocate once and reuse.
}

PRS1Loader::~PRS1Loader()
{
    for (map<QString,Machine *>::iterator i=PRS1List.begin(); i!=PRS1List.end(); i++) {
        delete i->second;
    }
    delete [] m_buffer;
}
Machine *PRS1Loader::CreateMachine(QString serial,Profile *profile)
{
    if (!profile)
        return NULL;
    qDebug() << "Create Machine " << serial;

    vector<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    for (vector<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()=="PRS1") && ((*i)->properties["Serial"]==serial)) {
            PRS1List[serial]=*i; //static_cast<CPAP *>(*i);
            found=true;
            break;
        }
    }
    if (found) return PRS1List[serial];

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

    QString newpath;

    QString pseries="P-Series";
    if (path.endsWith("/"+pseries)) {
        newpath=path;
    } else {
        newpath=path+"/"+pseries;
    }

    QDir dir(newpath);

    if ((!dir.exists() || !dir.isReadable()))
        return 0;

    //qDebug() << "PRS1Loader::Open newpath=" << newpath;
    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist=dir.entryInfoList();


    list<QString> SerialNumbers;
    list<QString>::iterator sn;

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

    Machine *m;
    for (sn=SerialNumbers.begin(); sn!=SerialNumbers.end(); sn++) {
        QString s=*sn;
        m=CreateMachine(s,profile);
        try {
            if (m) OpenMachine(m,newpath+"/"+(*sn),profile);
        } catch(OneTypePerDay e) {
            profile->DelMachine(m);
            PRS1List.erase(s);
            QMessageBox::warning(NULL,"Import Error","This Machine Record cannot be imported in this profile.\nThe Day records overlap with already existing content.",QMessageBox::Ok);
            delete m;
        }
    }
    return PRS1List.size();
}

bool PRS1Loader::ParseProperties(Machine *m,QString filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QString line;
    map<QString,QString> prop;

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
    int i=pt.toInt(&ok,0);
    if (ok) {
        if (ModelMap.find(i)!=ModelMap.end()) {
            m->properties["SubModel"]=ModelMap[i];
        }
    }
    if (prop["SerialNumber"]!=m->properties["Serial"]) {
        qDebug() << "Serial Number in PRS1 properties.txt doesn't match directory structure";
    } else prop.erase("SerialNumber"); // already got it stored.

    for (map<QString,QString>::iterator i=prop.begin(); i!=prop.end(); i++) {
        m->properties[i->first]=i->second;
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

    list<QString> paths;
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
    typedef vector<QString> StringList;
    map<SessionID,StringList> sessfiles;
    int size=paths.size();
    int cnt=0;
    bool ok;
    for (list<QString>::iterator p=paths.begin(); p!=paths.end(); p++) {
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
            //if (qprogress) qprogress->Pulse(); //Update((float(cnt)/float(size)*25));

            if (qprogress) qprogress->setValue((float(cnt)/float(size)*33.0));
        }
    }
    size=sessfiles.size();
    if (size==0)
        return 0;

    cnt=0;
    for (map<SessionID,StringList>::iterator s=sessfiles.begin(); s!=sessfiles.end(); s++) {
        session=s->first;
        cnt++;
        if (qprogress) qprogress->setValue(33.0+(float(cnt)/float(size)*33.0));

        if (m->SessionExists(session)) continue;
        if (s->second[0].isEmpty()) continue;

        Session *sess=new Session(m,session);
        if (!OpenSummary(sess,s->second[0])) {
            //qWarning() << "PRS1Loader: Dodgy summary file " << s->second[0];

            delete sess;
            continue;
        }
        //sess->SetSessionID(sess->start().GetTicks());
        if (!s->second[1].isEmpty()) {
            if (!OpenEvents(sess,s->second[1])) {
                qWarning() << "PRS1Loader: Couldn't open event file " << s->second[1];
            }
        }
        if (!s->second[2].isEmpty()) {
            if (!OpenWaveforms(sess,s->second[2])) {
                qWarning() << "PRS1Loader: Couldn't open event file " << s->second[2];
            }
        }
        const double ignore_thresh=300.0/3600.0;// Ignore useless sessions under 5 minute
        if (sess->hours()<=ignore_thresh) {
            //qDebug() << "Ignoring short session" << session << "which is only" << (sess->hours()*60.0) << "minute(s) long";
            delete sess;
            continue;
        }
        sess->summary[CPAP_Obstructive]=sess->count_events(CPAP_Obstructive);
        sess->summary[CPAP_Hypopnea]=sess->count_events(CPAP_Hypopnea);
        sess->summary[CPAP_ClearAirway]=sess->count_events(CPAP_ClearAirway);
        sess->summary[CPAP_RERA]=sess->count_events(CPAP_RERA);
        sess->summary[CPAP_FlowLimit]=sess->count_events(CPAP_FlowLimit);
        sess->summary[CPAP_VSnore]=sess->count_events(CPAP_VSnore);

        sess->summary[CPAP_CSR]=sess->sum_event_field(CPAP_CSR,0);
        sess->summary[CPAP_Snore]=sess->sum_event_field(CPAP_Snore,0);


        if (sess->count_events(CPAP_IAP)>0) {
            //sess->summary[CPAP_Mode]!=MODE_ASV)
            sess->summary[CPAP_Mode]=MODE_BIPAP;

            sess->summary[BIPAP_PSAverage]=sess->weighted_avg_event_field(CPAP_PS,0);
            sess->summary[BIPAP_PSMin]=sess->min_event_field(CPAP_PS,0);
            sess->summary[BIPAP_PSMax]=sess->max_event_field(CPAP_PS,0);

            if (sess->summary[CPAP_PressureReliefType].toInt()!=PR_NONE) {
                sess->summary[CPAP_PressureReliefType]=PR_BIFLEX;
            }
            sess->summary[CPAP_PressureMedian]=(sess->avg_event_field(CPAP_EAP,0)+sess->avg_event_field(CPAP_IAP,0))/2.0;
            sess->summary[CPAP_PressureAverage]=(sess->weighted_avg_event_field(CPAP_IAP,0)+sess->weighted_avg_event_field(CPAP_EAP,0))/2.0;
            sess->summary[CPAP_PressureMinAchieved]=sess->min_event_field(CPAP_IAP,0);
            sess->summary[CPAP_PressureMaxAchieved]=sess->max_event_field(CPAP_EAP,0);

            sess->summary[BIPAP_EAPAverage]=sess->weighted_avg_event_field(CPAP_EAP,0);
            sess->summary[BIPAP_EAPMin]=sess->min_event_field(CPAP_EAP,0);
            sess->summary[BIPAP_EAPMax]=sess->max_event_field(CPAP_EAP,0);

            sess->summary[BIPAP_IAPAverage]=sess->weighted_avg_event_field(CPAP_IAP,0);
            sess->summary[BIPAP_IAPMin]=sess->min_event_field(CPAP_IAP,0);
            sess->summary[BIPAP_IAPMax]=sess->max_event_field(CPAP_IAP,0);



        } else {
            sess->summary[CPAP_PressureMedian]=sess->weighted_avg_event_field(CPAP_Pressure,0);
            sess->summary[CPAP_PressureAverage]=sess->weighted_avg_event_field(CPAP_Pressure,0);
            sess->summary[CPAP_PressureMinAchieved]=sess->min_event_field(CPAP_Pressure,0);
            sess->summary[CPAP_PressureMaxAchieved]=sess->max_event_field(CPAP_Pressure,0);
            if (sess->summary.find(CPAP_PressureMin)==sess->summary.end()) {
                sess->summary[CPAP_BrokenSummary]=true;
                //sess->set_last(sess->first());
                if (sess->summary[CPAP_PressureMinAchieved]==sess->summary[CPAP_PressureMaxAchieved]) {
                    sess->summary[CPAP_Mode]=MODE_CPAP;
                } else {
                    sess->summary[CPAP_Mode]=MODE_UNKNOWN;
                }
                sess->summary[CPAP_PressureReliefType]=PR_UNKNOWN;
            }

        }
        if (sess->summary[CPAP_Mode]==MODE_CPAP) {
            sess->summary[CPAP_PressureAverage]=sess->summary[CPAP_PressureMin];
            sess->summary[CPAP_PressureMax]=sess->summary[CPAP_PressureMin];
        }

        sess->summary[CPAP_LeakMinimum]=sess->min_event_field(CPAP_Leak,0);
        sess->summary[CPAP_LeakMaximum]=sess->max_event_field(CPAP_Leak,0); // should be merged..
        sess->summary[CPAP_LeakMedian]=sess->avg_event_field(CPAP_Leak,0);
        sess->summary[CPAP_LeakAverage]=sess->weighted_avg_event_field(CPAP_Leak,0);

        sess->summary[CPAP_SnoreMinimum]=sess->min_event_field(CPAP_Snore,0);
        sess->summary[CPAP_SnoreMaximum]=sess->max_event_field(CPAP_Snore,0);
        sess->summary[CPAP_SnoreMedian]=sess->avg_event_field(CPAP_Snore,0);
        sess->summary[CPAP_SnoreAverage]=sess->weighted_avg_event_field(CPAP_Snore,0);

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
    session->summary[CPAP_PressureMin]=(double)buffer[0x03]/10.0;
    session->summary[CPAP_PressureMax]=max=(double)buffer[0x04]/10.0;
    int offset=0;
    if (buffer[0x05]!=0) { // This is a time value for ASV stuff
        // non zero adds extra fields..
        offset=4;
    }

    session->summary[CPAP_RampTime]=(int)buffer[offset+0x06]; // Minutes. Convert to seconds/hours here?
    session->summary[CPAP_RampStartingPressure]=(double)buffer[offset+0x07]/10.0;

    if (max>0) { // Ignoring bipap until I see some more data.
        session->summary[CPAP_Mode]=(int)MODE_APAP;
    } else session->summary[CPAP_Mode]=(int)MODE_CPAP;

    // This is incorrect..
    if (buffer[offset+0x08] & 0x80) { // Flex Setting
        if (buffer[offset+0x08] & 0x08) {
            if (max>0) session->summary[CPAP_PressureReliefType]=(int)PR_AFLEX;
            else session->summary[CPAP_PressureReliefType]=(int)PR_CFLEXPLUS;
        } else session->summary[CPAP_PressureReliefType]=(int)PR_CFLEX;
    } else session->summary[CPAP_PressureReliefType]=(int)PR_NONE;

    session->summary[CPAP_PressureReliefSetting]=(int)buffer[offset+0x08] & 3;
    session->summary[CPAP_HumidifierSetting]=(int)buffer[offset+0x09]&0x0f;
    session->summary[CPAP_HumidifierStatus]=(buffer[offset+0x09]&0x80)==0x80;
    session->summary[PRS1_SystemLockStatus]=(buffer[offset+0x0a]&0x80)==0x80;
    session->summary[PRS1_SystemResistanceStatus]=(buffer[offset+0x0a]&0x40)==0x40;
    session->summary[PRS1_SystemResistanceSetting]=(int)buffer[offset+0x0a]&7;
    session->summary[PRS1_HoseDiameter]=(int)((buffer[offset+0x0a]&0x08)?15:22);
    session->summary[PRS1_AutoOff]=(buffer[offset+0x0c]&0x10)==0x10;
    session->summary[PRS1_MaskAlert]=(buffer[offset+0x0c]&0x08)==0x08;
    session->summary[PRS1_ShowAHI]=(buffer[offset+0x0c]&0x04)==0x04;

    unsigned duration=buffer[offset+0x14] | (buffer[0x15] << 8);
    session->summary[CPAP_Duration]=(int)duration;
    //qDebug() << "ID: " << session->session() << " " << duration;
    //float hours=float(duration)/3600.0;
    //session->set_hours(hours);
    if (!duration)
        return false;

    session->set_last(date+qint64(duration)*1000L);

    session->summary[CPAP_PressureMinAchieved]=buffer[offset+0x16]/10.0;
    session->summary[CPAP_PressureMaxAchieved]=buffer[offset+0x17]/10.0;
    session->summary[CPAP_PressureAverage]=buffer[offset+0x18]/10.0;
    session->summary[CPAP_PressurePercentValue]=buffer[offset+0x19]/10.0;
    session->summary[CPAP_PressurePercentName]=90.0;

    if (max==0) {
        session->summary[CPAP_PressureAverage]=session->summary[CPAP_PressureMin];
    }
   /*/if (size==0x4d) {

        session->summary[CPAP_Obstructive]=(int)buffer[offset+0x1C] | (buffer[offset+0x1D] << 8);
        session->summary[CPAP_ClearAirway]=(int)buffer[offset+0x20] | (buffer[offset+0x21] << 8);
        session->summary[CPAP_Hypopnea]=(int)buffer[offset+0x2A] | (buffer[offset+0x2B] << 8);
        session->summary[CPAP_RERA]=(int)buffer[offset+0x2E] | (buffer[offset+0x2F] << 8);
        session->summary[CPAP_FlowLimit]=(int)buffer[offset+0x30] | (buffer[offset+0x31] << 8);
    }*/
    return true;
}

// v2 event parser.
bool PRS1Loader::Parse002(Session *session,unsigned char *buffer,int size,qint64 timestamp)
{
    MachineCode Codes[]={
        PRS1_Unknown00, PRS1_Unknown01, CPAP_Pressure, CPAP_EAP, PRS1_PressurePulse, CPAP_RERA, CPAP_Obstructive, CPAP_ClearAirway,
        PRS1_Unknown08, PRS1_Unknown09, CPAP_Hypopnea, PRS1_Unknown0B, CPAP_FlowLimit, CPAP_VSnore, PRS1_Unknown0E, CPAP_CSR, PRS1_Unknown10,
        CPAP_Leak, PRS1_Unknown12
    };
    int ncodes=sizeof(Codes)/sizeof(MachineCode);

    EventDataType data[10];

    //qint64 start=timestamp;
    qint64 t=timestamp;
    qint64 tt;
    int pos=0;
    int cnt=0;
    short delta;//,duration;
    while (pos<size) {
        unsigned char code=buffer[pos++];
        if (code>=ncodes) {
            qDebug() << "Illegal PRS1 code " << hex << int(code) << " appeared at " << hex << pos+16;
            return false;
        }
        //if (code==0xe) {
        //    pos+=2;
        //} else
        if (code!=0x12) {
            //delta=buffer[pos];
            //duration=buffer[pos+1];
            delta=buffer[pos+1] << 8 | buffer[pos];
            pos+=2;
            //QDateTime d=QDateTime::fromMSecsSinceEpoch(t);
            //qDebug()<< d.toString("yyyy-MM-dd HH:mm:ss") << ": " << hex << pos+15 << " " << hex << int(code) << int(delta);
            t+=delta*1000;
        }
        MachineCode cpapcode=Codes[(int)code];
        tt=t;
        cnt++;
        //int fc=0;
        switch (code) {
        case 0x01: // Unknown            
            session->AddEvent(new Event(t,cpapcode, data,0));
            break;
        case 0x00: // Unknown (ASV Pressure value) // could this be RLE?
            // offset?
            data[0]=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, data,1));
            break;
        case 0x02: // Pressure
            data[0]=buffer[pos++]/10.0;
            session->AddEvent(new Event(t,cpapcode, data,1));
            break;
        case 0x03: // BIPAP Pressure
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            data[0]/=10.0;
            data[1]/=10.0;
            session->AddEvent(new Event(t,CPAP_EAP, data, 1));
            session->AddEvent(new Event(t,CPAP_IAP, &data[1], 1));
            data[1]-=data[0];
            session->AddEvent(new Event(t,CPAP_PS, &data[1], 1));
            break;
        case 0x04: // Pressure Pulse
            data[0]=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, data,1));
            //qDebug() << hex << data[0];
            break;
        case 0x05: // RERA
        case 0x06: // Obstructive Apoanea
        case 0x07: // Clear Airway
        case 0x0a: // Hypopnea
        case 0x0c: // Flow Limitation
            data[0]=buffer[pos++];
            tt-=data[0]*1000; // Subtract Time Offset
            session->AddEvent(new Event(tt,cpapcode,data,1));
            break;
        case 0x0b: // ASV Codes
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            session->AddEvent(new Event(tt,cpapcode,data,2));
            break;
        case 0x0d: // Vibratory Snore
            session->AddEvent(new Event(t,cpapcode, data,0));
            break;
        case 0x11: // Leak Rate
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, data,1));
            session->AddEvent(new Event(t,CPAP_Snore,&data[1],1));
            if (data[1]>0) {
                session->AddEvent(new Event(t,PRS1_VSnore2, &data[1],1));
            }
            break;
        case 0x0e: // Unknown
            data[0]=((char *)buffer)[pos++];
            data[1]=buffer[pos++]; //(buffer[pos+1] << 8) | buffer[pos];
            //data[0]/=10.0;
            //pos+=2;
            data[2]=buffer[pos++];
            //qDebug() << hex << data[0] << data[1] << data[2];
            session->AddEvent(new Event(t,cpapcode, data, 3));
            //tt-=data[1]*1000;
            //session->AddEvent(new Event(t,CPAP_CSR, data, 2));
            break;
        case 0x10: // Unknown
            data[0]=buffer[pos++]; // << 8) | buffer[pos];
            data[1]=buffer[pos++];
            data[2]=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, data, 3));
            break;
        case 0x0f: // Cheyne Stokes Respiration
            data[0]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            data[1]=buffer[pos++];
            tt-=data[1]*1000;
            session->AddEvent(new Event(tt,cpapcode, data, 2));
            break;
        case 0x12: // Summary
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            data[2]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            session->AddEvent(new Event(t,cpapcode, data,3));
            break;
        default:
            // ERROR!!!
            qWarning() << "Some new fandangled PRS1 code detected " << hex << int(code) << " at " << pos+15;
            return false;
        }
    }
    return true;
}

bool PRS1Loader::Parse002ASV(Session *session,unsigned char *buffer,int size,qint64 timestamp)
{
    MachineCode Codes[]={
        PRS1_Unknown00, PRS1_Unknown01, CPAP_Pressure, CPAP_EAP, PRS1_PressurePulse, CPAP_Obstructive, CPAP_ClearAirway, CPAP_Hypopnea,
        PRS1_Unknown08,  CPAP_FlowLimit, PRS1_Unknown0A, CPAP_CSR, PRS1_Unknown0C, CPAP_VSnore, PRS1_Unknown0E, PRS1_Unknown0F, PRS1_Unknown10,
        CPAP_Leak, PRS1_Unknown12
    };

    int ncodes=sizeof(Codes)/sizeof(MachineCode);

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
        //QDateTime d=QDateTime::fromMSecsSinceEpoch(t);
        //qDebug()<< d.toString("yyyy-MM-dd HH:mm:ss") << ": " << hex << pos+15 << " " << hex << int(code) ;
        if (code==0) {
        } else
        if (code!=0x12) {
            delta=buffer[pos];
            //duration=buffer[pos+1];
            //delta=buffer[pos+1] << 8 | buffer[pos];
            pos+=2;
            t+=delta*1000;
        }
        MachineCode cpapcode=Codes[(int)code];
        //EventDataType PS;
        tt=t;
        cnt++;
        int fc=0;
        switch (code) {
        case 0x01: // Unknown
            session->AddEvent(new Event(t,cpapcode, data,0));
            break;
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
            session->AddEvent(new Event(t,cpapcode, data,2));
            break;
        case 0x02: // Pressure
            data[0]=buffer[pos++]/10.0; // crappy EPAP pressure value.
            //session->AddEvent(new Event(t,cpapcode, data,1));
            break;
        case 0x04: // Pressure Pulse
            data[0]=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, data,1));
            break;

        case 0x0a:
            code=0x0a;
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x0c:
            data[0]=buffer[pos++];
            tt-=data[0]*1000; // Subtract Time Offset
            session->AddEvent(new Event(tt,cpapcode,data,1));
            break;
        case 0x0b: // Cheyne Stokes
            data[0]=((unsigned char *)buffer)[pos+1]<<8 | ((unsigned char *)buffer)[pos];
            data[0]*=2;
            pos+=2;
            data[1]=((unsigned char *)buffer)[pos]; //|buffer[pos+1] << 8
            pos+=1;
            //tt-=delta;
            tt-=data[1]*1000;
            session->AddEvent(new Event(tt,cpapcode, data, 2));
            //tt+=delta;
            break;
        case 0x08: // ASV Codes
        case 0x09: // ASV Codes
            data[0]=buffer[pos];
            tt-=buffer[pos++]*1000; // Subtract Time Offset
            session->AddEvent(new Event(tt,cpapcode,data,1));
            break;
        case 0x0d: // All the other ASV graph stuff.
            d=QDateTime::fromMSecsSinceEpoch(t);
            data[0]=buffer[pos++]/10.0;
            session->AddEvent(new Event(t,CPAP_IAP,&data[0],1)); //correct
            data[1]=buffer[pos++]/10.0; // Low IPAP
            session->AddEvent(new Event(t,CPAP_IAPLO,&data[1],1)); //correct
            data[2]=buffer[pos++]/10.0; // Hi IPAP
            session->AddEvent(new Event(t,CPAP_IAPHI,&data[2],1)); //correct
            data[3]=buffer[pos++];//Leak
            session->AddEvent(new Event(t,CPAP_Leak,&data[3],1)); // correct
            data[4]=buffer[pos++];//Breaths Per Minute
            session->AddEvent(new Event(t,CPAP_RespiratoryRate,&data[4],1)); //correct
            data[5]=buffer[pos++];//Patient Triggered Breaths
            session->AddEvent(new Event(t,CPAP_PatientTriggeredBreaths,&data[5],1)); //correct
            data[6]=buffer[pos++];//Minute Ventilation
            session->AddEvent(new Event(t,CPAP_MinuteVentilation,&data[6],1)); //correct
            data[7]=buffer[pos++]*10.0; // Tidal Volume
            session->AddEvent(new Event(t,CPAP_TidalVolume,&data[7],1)); //correct
            data[8]=buffer[pos++];
            session->AddEvent(new Event(t,CPAP_Snore,&data[8],1)); //correct
            if (data[8]>0) {
                session->AddEvent(new Event(t,CPAP_VSnore,&data[8],1)); //correct
            }
            data[9]=buffer[pos++]/10.0; // EPAP
            session->AddEvent(new Event(t,CPAP_EAP,&data[9],1)); //correct

            data[2]-=data[9]; // Pressure Support
            session->AddEvent(new Event(t,CPAP_PS,&data[2],1)); //correct
            //qDebug()<< d.toString("yyyy-MM-dd HH:mm:ss") << hex << session->session() << pos+15 << hex << int(code) << ": " << hex << int(data[0]) << " " << int(data[1]) << " " << int(data[2])  << " " << int(data[3]) << " " << int(data[4]) << " " << int(data[5])<< " " << int(data[6]) << " " << int(data[7]) << " " << int(data[8]) << " " << int(data[9]);
            break;
        case 0x03: // BIPAP Pressure
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            data[0]/=10.0;
            data[1]/=10.0;
            session->AddEvent(new Event(t,CPAP_EAP, data, 1));
            session->AddEvent(new Event(t,CPAP_IAP, &data[1], 1));
            break;
        case 0x11: // Leak Rate
            data[0]=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, data,1));
            break;
        case 0x0e: // Unknown
            data[0]=buffer[pos++]; // << 8) | buffer[pos];
            session->AddEvent(new Event(t,cpapcode, data, 1));
            break;

        case 0x10: // Unknown
            data[0]=buffer[pos++]; // << 8) | buffer[pos];
            data[1]=buffer[pos++];
            data[2]=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, data, 3));
            break;
        case 0x0f:
            data[0]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            data[1]=buffer[pos]; //|buffer[pos+1] << 8
            pos+=1;
            tt-=data[1]*1000;
            session->AddEvent(new Event(tt,cpapcode, data, 2));
            break;
        case 0x12: // Summary
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            data[2]=buffer[pos+1]<<8 | buffer[pos];
            pos+=2;
            session->AddEvent(new Event(t,cpapcode, data,3));
            break;
        default:
            // ERROR!!!
            qWarning() << "Some new fandangled PRS1 code detected " << hex << int(code) << " at " << pos+15;
            return false;
        }
    }
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
    quint8 version=m_buffer[4];
    quint32 start=m_buffer[0xb] | m_buffer[0xc] << 8 | m_buffer[0xd] << 16 | m_buffer[0x0e] << 24;
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
    quint16 duration,length;
    quint32 timestamp,lasttimestamp;


    pos=0;
    int block=0;
    lasttimestamp=start;
    duration=0;
    int corrupt=0;
    char waveform[num_signals][500000];
    int wlength[num_signals];
    int wdur[num_signals];
    for (int i=0;i<num_signals;i++) {
        wlength[i]=0;
        wdur[i]=0;
        //waveform[i].resize(500000);
    }
    SampleFormat *wf[num_signals];
    MachineCode wc[2]={CPAP_FlowRate,CPAP_MaskPressure};
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
                qDebug() << "Timestamp restarts" << block << diff << corrupt << duration << timestamp-lasttimestamp << filename;


                for (int i=0;i<num_signals;i++) {
                    wf[i]=new SampleFormat [wlength[i]];
                    for (int j=0;j<wlength[i];j++) {
                        if (whl[i].sample_format)
                            wf[i][j]=((unsigned char*)waveform[i])[j];
                        else
                            wf[i][j]=(waveform[i])[j];
                    }
                    Waveform *w=new Waveform(qint64(start)*1000L,wc[i],wf[i],wlength[i],qint64(wdur[i])*1000L,-128,128); //FlowRate
                    session->AddWaveform(w);
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
        wf[i]=new SampleFormat [wlength[i]];
        for (int j=0;j<wlength[i];j++) {
            if (whl[i].sample_format)
                wf[i][j]=((unsigned char*)waveform[i])[j];
            else
                wf[i][j]=(waveform[i])[j];
        }
        Waveform *w=new Waveform(qint64(start)*1000L,wc[i],wf[i],wlength[i],qint64(wdur[i])*1000L,-128,128); //FlowRate
        session->AddWaveform(w);
    }
    return true;
}
/*bool PRS1Loader::OpenWaveforms(Session *session,QString filename)
{
    int size,sequence,seconds,br,htype,version,numsignals;
    unsigned cnt=0;
    qint32 lastts,ts1;
    qint64 timestamp;

    qint64 start=0;
    unsigned char header[20];
    unsigned char ext;

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    const int max_signals=16;
    static qint16 interleave[max_signals]={0};
    static char sampletype[max_signals]={0};

    int hl=0;
    long samples=0;
    qint64 duration=0;
    char * buffer=(char *)m_buffer;
    //bool first2=true;
    long fpos=0;
    //int bsize=0;
    int lasthl=0;
    qint32 expected_timestamp=0;
    while (true) {
        lasthl=hl;
        hl=20;
        br=f.read((char *)header,hl);
        fpos+=hl;
        if (br<hl) {
            if (cnt==0)
                return false;
            break;
        }

        if (header[0]!=PRS1_MAGIC_NUMBER) {
            if (cnt==0)
                return false;
            qWarning() << "Corrupt waveform, trying to recover" << sequence;
            session->summary[CPAP_BrokenWaveform]=true;
            // read the damn bytes anyway..

            br=f.read((char *)header,lasthl-hl+1); // last bit of the header
            if (br<lasthl-hl+1) {
                qWarning() << "End of file, couldn't recover";
                break;
            }
            hl=lasthl;

            br=f.read((char *)&buffer[samples],size);
            if (br<size) {
                //delete [] buffer;
                break;
            }
            samples+=size;
            duration+=seconds;
            br=f.read((char *)header,2);
            fpos+=size+3+lasthl-hl;

            continue;
        }

        //sequence=size=timestamp=seconds=ext=0;
        size=(header[2] << 8) | header[1];
        version=header[4];
        htype=header[3];
        ext=header[6];
        sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
        ts1=timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
        seconds=(header[16] << 8) | header[15];
        numsignals=header[19] << 8 | header[18];

        sequence=sequence;
        version=version;
        //htype=htype;

        unsigned char sum=0,hchk;
        for (int i=0; i<hl; i++) sum+=header[i];

        if (numsignals>=max_signals) {
            qWarning() << "PRS1Loader::OpenWaveforms() numsignals >= max_signals";
            return false;
        }
        char buf[3];
        for (int i=0;i<numsignals;i++) {
            f.read(buf,3);
            fpos+=3;
            if (br<3) {
                if (cnt==0)
                    return false;
                break;
            }
            sum+=buf[0]+buf[1]+buf[2];
            sampletype[i]=buf[2];
            interleave[i]=buf[1] << 8 | buf[0];
            hl+=3;
        }

        if (f.read((char *)&hchk,1)!=1) { // Read Header byte.
            if (cnt==0)
                return false;
            break;
        }
        fpos+=1;
        if (sum!=hchk)
            return false;

        size-=(hl+3); // 3 == the 8 bit checksum on the end of the header, and the 16bit at the end of the block

        if (!htype) {
            qDebug() << "PRS1 Waveform data has htype set differently";
        }


        if (!start) {
            lastts=timestamp;
         //   expected_timestamp=timestamp+seconds;
            start=timestamp*1000; //convert from epoch to msecs since epoch
            //qDebug() << "Wave: " << cnt << seconds;
        } else {
            qint32 diff=timestamp-expected_timestamp;
            if (version==5) {
                qWarning() << "ASV Waveform out of sync" << sequence << duration << diff << timestamp-lastts;
            } else {
                if (diff<0) {
                    if (duration>abs(diff)) {
                        duration+=diff;  // really Subtracting..
                        samples+=diff*5;
                    } else {
                        qWarning() << "Waveform out of sync beyond the first entry" << sequence << duration << diff;
                    }
                } else if (diff>0) {
                    qDebug() << "Fixing up Waveform sync" << sequence;
                    for (int i=0;i<diff*5;i++) {
                        buffer[samples++]=0;
                    }
                    duration+=diff;
                }
            }
            //qDebug() << "Wave: " << cnt << seconds << diff;
        }


        expected_timestamp=timestamp+seconds*numsignals;

        if (ext!=PRS1_WAVEFORM_FILE) {
            if (cnt==0)
                return false;
            break;
        }

        if (samples+size>=max_load_buffer_size) {
            qWarning("max_load_buffer_size is too small in PRS1 Loader");
            if (cnt==0)
                return false;
            break;
        }
        br=f.read((char *)&buffer[samples],size);
        if (br<size) {
            delete [] buffer;
            break;
        }
        fpos+=size;

        cnt++;

        duration+=seconds;
        samples+=size;

        char chkbuf[2];
        qint16 chksum;
        br=f.read(chkbuf,2);
        fpos+=2;
        if (br<2)
            return false;
        chksum=chkbuf[0] << 8 | chkbuf[1];
        chksum=chksum;
        lastts=ts1;
    }

    if (samples==0)
        return false;

    // Create the buffers for real sample data
    SampleFormat * data[numsignals];
    SampleFormat min[numsignals],max[numsignals];
    bool first[numsignals];
    int pos[numsignals];

    int samplespersignal=samples/numsignals; // TODO: divide by type?
    int ichunksize=0; // interleave chunk size
    for (int i=0;i<numsignals;i++) {
        ichunksize+=interleave[i];
        data[i]=new SampleFormat[samplespersignal];
        min[i]=max[i]=0;
        first[i]=true;
        pos[i]=0;
    }

    // Deinterleave the samples
    SampleFormat c;
    unsigned char * ucbuffer=(unsigned char *)buffer;
    int k=0;
    for (int i=0;i<samples/ichunksize;i++) {
        for (int s=0;s<numsignals;s++) {
            for (int j=0;j<interleave[numsignals-1-s];j++) {
                if (sampletype[numsignals-1-s]==0)
                    c=buffer[k++];
                else if (sampletype[numsignals-1-s]==1) {
                    c=ucbuffer[k++];
                    if (c<40) {
                        c=min[s];
                        //int q=0;
                    }
                }
                if (first[s]) {
                    min[s]=max[s]=c;
                    first[s]=false;
                } else {
                    if (min[s]>c) min[s]=c;
                    if (max[s]<c) max[s]=c;
                }
                data[s][pos[s]++]=c;
            }
        }
    }

    duration*=1000;

    Waveform *w=new Waveform(start,CPAP_FlowRate,data[0],pos[0],duration,min[0],max[0]); //FlowRate
    session->AddWaveform(w);
    if (numsignals>1) {
        Waveform *w=new Waveform(start,CPAP_MaskPressure,data[1],pos[1],duration,min[1],max[1]);
        session->AddWaveform(w);
    }
    return true;
} */

void InitModelMap()
{
    ModelMap[34]="RemStar Pro with C-Flex+";
    ModelMap[35]="RemStar Auto with A-Flex";
    ModelMap[37]="RemStar BiPAP Auto with Bi-Flex";
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

