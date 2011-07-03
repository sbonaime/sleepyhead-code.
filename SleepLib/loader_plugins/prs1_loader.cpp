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
    qDebug() << "Create Machine " << serial;
    assert(profile!=NULL);

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
bool PRS1Loader::Open(QString & path,Profile *profile)
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

    qDebug() << "PRS1Loader::Open newpath=" << newpath;
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

   // return c;
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
    int i=prop["ProductType"].toInt(&ok);
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
        if (session==0x112)
            int q=0;
        if (!OpenSummary(sess,s->second[0])) {
            qWarning() << "PRS1Loader: Could'nt open summary file " << s->second[0];

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
            delete sess;
            continue;
        }
        m->AddSession(sess,profile);

        //if (sess->summary.find(CPAP_Obstructive)==sess->summary.end()) {
            sess->summary[CPAP_Obstructive]=sess->count_events(CPAP_Obstructive);
            sess->summary[CPAP_Hypopnea]=sess->count_events(CPAP_Hypopnea);
            sess->summary[CPAP_ClearAirway]=sess->count_events(CPAP_ClearAirway);
            sess->summary[CPAP_RERA]=sess->count_events(CPAP_RERA);
            sess->summary[CPAP_FlowLimit]=sess->count_events(CPAP_FlowLimit);
        //}

        sess->summary[CPAP_CSR]=sess->sum_event_field(CPAP_CSR,0);
        sess->summary[CPAP_VSnore]=sess->count_events(CPAP_VSnore);
        sess->summary[CPAP_Snore]=sess->sum_event_field(CPAP_Snore,0);


        if (sess->count_events(CPAP_IAP)>0) {
            sess->summary[CPAP_Mode]=MODE_BIPAP;
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
            sess->summary[CPAP_PressureMedian]=sess->avg_event_field(CPAP_Pressure,0);
            //sess->summary[CPAP_PressureAverage]=sess->weighted_avg_event_field(CPAP_Pressure,0);
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
    }
    QString s;
    s.sprintf("%i",prs1_data_version);
    m->properties["DataVersion"]=s;
    m->Save(); // Save any new sessions to disk in our format
    if (qprogress) qprogress->setValue(100);
    return true;
}

bool PRS1Loader::OpenSummary(Session *session,QString filename)
{
    int size,sequence,seconds,br;
    qint64 timestamp;
    unsigned char header[24];
    unsigned char ext,sum;

    //qDebug() << "Opening PRS1 Summary " << filename;
    QFile f(filename);
    //,wxT("rb"));
    if (!f.open(QIODevice::ReadOnly))
          return false;

    if (!f.exists())
        return false;

    int hl=16;

    br=f.read((char *)header,hl);

    if (header[0]!=header[5])
        return false;

    sequence=size=timestamp=seconds=ext=0;
    sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
    timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
    size=(header[2] << 8) | header[1];
    ext=header[6];

    if (ext!=1)
        return false;

    //size|=(header[3]<<16) | (header[4]<<24); // the jury is still out on the 32bitness of one. doesn't matter here anyway.

    size-=(hl+2);
    sum=0;
    for (int i=0; i<hl-1; i++) sum+=header[i];
    if (sum!=header[hl-1])
        return false;

    if (size<=19) {
        qDebug() << "Ignoring short session file " << filename;
        return false;
    }

    qint64 date=timestamp*1000;

    memset(m_buffer,0,size);
    unsigned char * buffer=m_buffer;
    br=f.read((char *)buffer,size);
    if (br<size) {
        return false;
    }
    if (size<0x30)
        return true;

    session->set_first(date);
    //session->set_last(date);
    double max;
    session->summary[CPAP_PressureMin]=(double)buffer[0x03]/10.0;
    session->summary[CPAP_PressureMax]=max=(double)buffer[0x04]/10.0;
    session->summary[CPAP_RampTime]=(int)buffer[0x06]; // Minutes. Convert to seconds/hours here?
    session->summary[CPAP_RampStartingPressure]=(double)buffer[0x07]/10.0;

    if (max>0) { // Ignoring bipap until I see some more data.
        session->summary[CPAP_Mode]=(int)MODE_APAP;
    } else session->summary[CPAP_Mode]=(int)MODE_CPAP;

    // This is incorrect..
    if (buffer[0x08] & 0x80) { // Flex Setting
        if (buffer[0x08] & 0x08) {
            if (max>0) session->summary[CPAP_PressureReliefType]=(int)PR_AFLEX;
            else session->summary[CPAP_PressureReliefType]=(int)PR_CFLEXPLUS;
        } else session->summary[CPAP_PressureReliefType]=(int)PR_CFLEX;
    } else session->summary[CPAP_PressureReliefType]=(int)PR_NONE;

    session->summary[CPAP_PressureReliefSetting]=(int)buffer[0x08] & 3;
    session->summary[CPAP_HumidifierSetting]=(int)buffer[0x09]&0x0f;
    session->summary[CPAP_HumidifierStatus]=(buffer[0x09]&0x80)==0x80;
    session->summary[PRS1_SystemLockStatus]=(buffer[0x0a]&0x80)==0x80;
    session->summary[PRS1_SystemResistanceStatus]=(buffer[0x0a]&0x40)==0x40;
    session->summary[PRS1_SystemResistanceSetting]=(int)buffer[0x0a]&7;
    session->summary[PRS1_HoseDiameter]=(int)((buffer[0x0a]&0x08)?15:22);
    session->summary[PRS1_AutoOff]=(buffer[0x0c]&0x10)==0x10;
    session->summary[PRS1_MaskAlert]=(buffer[0x0c]&0x08)==0x08;
    session->summary[PRS1_ShowAHI]=(buffer[0x0c]&0x04)==0x04;

    unsigned char * b=&buffer[0x14];
    quint16 bb=*(quint16*)b;
    unsigned duration=bb;// | (buffer[0x15] << 8);
    session->summary[CPAP_Duration]=(int)duration;
    //qDebug() << "ID: " << session->session() << " " << duration;
    //float hours=float(duration)/3600.0;
    //session->set_hours(hours);
    if (!duration) return false;

    session->set_last(date+qint64(duration)*1000L);

    session->summary[CPAP_PressureMinAchieved]=buffer[0x16]/10.0;
    session->summary[CPAP_PressureMaxAchieved]=buffer[0x17]/10.0;
    session->summary[CPAP_PressureAverage]=buffer[0x18]/10.0;
    session->summary[CPAP_PressurePercentValue]=buffer[0x19]/10.0;
    session->summary[CPAP_PressurePercentName]=90.0;

    if (max==0) {
        session->summary[CPAP_PressureAverage]=session->summary[CPAP_PressureMin];
    }
    if (size==0x4d) {

        session->summary[CPAP_Obstructive]=(int)buffer[0x1C] | (buffer[0x1D] << 8);
        session->summary[CPAP_ClearAirway]=(int)buffer[0x20] | (buffer[0x21] << 8);
        session->summary[CPAP_Hypopnea]=(int)buffer[0x2A] | (buffer[0x2B] << 8);
        session->summary[CPAP_RERA]=(int)buffer[0x2E] | (buffer[0x2F] << 8);
        session->summary[CPAP_FlowLimit]=(int)buffer[0x30] | (buffer[0x31] << 8);
    }
    return true;
}

bool PRS1Loader::Parse002(Session *session,unsigned char *buffer,int size,qint64 timestamp)
{
    MachineCode Codes[]={
        PRS1_Unknown00, PRS1_Unknown01, CPAP_Pressure, CPAP_EAP, PRS1_PressurePulse, CPAP_RERA, CPAP_Obstructive, CPAP_ClearAirway,
        PRS1_Unknown08, PRS1_Unknown09, CPAP_Hypopnea, PRS1_Unknown0B, CPAP_FlowLimit, CPAP_VSnore, PRS1_Unknown0E, CPAP_CSR, PRS1_Unknown10,
        CPAP_Leak, PRS1_Unknown12
    };
    int ncodes=sizeof(Codes)/sizeof(MachineCode);

    EventDataType data[4];

    qint64 start=timestamp;
    qint64 t=timestamp;
    qint64 tt;
    int pos=0;
    int cnt=0;
    short delta;
    while (pos<size) {
        char code=buffer[pos++];
        assert(code<ncodes);
        if (code!=0x12) {
            delta=buffer[pos] | buffer[pos+1] << 8;
            pos+=2;
            t+=delta*1000;
        }
    //    float data0,data1,data2;
        MachineCode cpapcode=Codes[(int)code];
        tt=t;
        cnt++;
        switch (code) {
        case 0x00: // Unknown
        case 0x01: // Unknown
        case 0x02: // Pressure
            data[0]=buffer[pos++]/10.0;
            session->AddEvent(new Event(t,cpapcode, data,1));
            break;
        case 0x04: // Pressure Pulse
            data[0]=buffer[pos++];
            session->AddEvent(new Event(t,cpapcode, data,1));
            break;

        case 0x05: // RERA
        case 0x06: // Obstructive Apoanea
        case 0x07: // Clear Airway
        case 0x0a: // Hypopnea
        case 0x0c: // Flow Limitation
            data[0]=buffer[pos];

            tt-=buffer[pos++]*1000; // Subtract Time Offset

            session->AddEvent(new Event(tt,cpapcode,data,1));
            break;
        case 0x0d: // Vibratory Snore
            session->AddEvent(new Event(t,cpapcode, data,0));
            break;
        case 0x03: // BIPAP Pressure
        case 0x0b: // Unknown
        case 0x11: // Leak Rate
            data[0]=buffer[pos++];
            data[1]=buffer[pos++];
            if (code==0x11) {
                session->AddEvent(new Event(t,cpapcode, data,1));
                session->AddEvent(new Event(t,CPAP_Snore,&data[1],1));
                if (data[1]>0) {
                    session->AddEvent(new Event(t,PRS1_VSnore2, &data[1],1));
                }
            } else if (code==0x03) {
                data[0]/=10.0;
                data[1]/=10.0;
                session->AddEvent(new Event(t,CPAP_EAP, data, 1));
                session->AddEvent(new Event(t,CPAP_IAP, &data[1], 1));
            } else {
                session->AddEvent(new Event(t,cpapcode, data, 2));
            }
            break;
        case 0x0e: // Unknown
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
            throw exception(); // UnknownCode();
            break;
        }
    }
    return true;
}

bool PRS1Loader::OpenEvents(Session *session,QString filename)
{
    int size,sequence,seconds,br;
    qint64 timestamp;
    unsigned char header[24]; // use m_buffer?
    unsigned char ext;

    //wxLogMessage(wxT("Opening PRS1 Events ")+filename);
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    int hl=16;

    br=f.read((char *)header,hl);

    if (header[0]!=header[5])
        return false;

    sequence=size=timestamp=seconds=ext=0;
    sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
    timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
    size=(header[2] << 8) | header[1];
    ext=header[6];

    if (ext!=2)
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
    Parse002(session,buffer,size,timestamp*1000L);

    return true;
}

bool PRS1Loader::OpenWaveforms(Session *session,QString filename)
{
    //wxLogMessage(wxT("Opening PRS1 Waveforms ")+filename);


    int size,sequence,seconds,br;
    unsigned cnt=0;
    qint64 timestamp;

    qint64 start=0;
    unsigned char header[24];
    unsigned char ext;

    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        return false;


    int hl=24;
    long samples=0;
    qint64 duration=0;
    char * buffer=(char *)m_buffer;

    while (true) {
        br=f.read((char *)header,hl);

        if (br<hl) {
            if (cnt==0)
                return false;
            break;
        }

        if (header[0]!=header[5]) {
            if (cnt==0)
                return false;
            break;
        }

        sequence=size=timestamp=seconds=ext=0;
        sequence=(header[10] << 24) | (header[9] << 16) | (header[8] << 8) | header[7];
        timestamp=(header[14] << 24) | (header[13] << 16) | (header[12] << 8) | header[11];
        size=(header[2] << 8) | header[1];
        ext=header[6];

        if (sequence==328) {
            seconds=0;
        }
        if (!start) start=timestamp*1000;
        seconds=(header[16] << 8) | header[15];

        size-=(hl+2);

        if (ext!=5) {
            if (cnt==0)
                return false;
            break;
        }

        unsigned char sum=0;
        for (int i=0; i<hl-1; i++) sum+=header[i];
        if (sum!=header[hl-1])
            return false;

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

        //wavedata.push_back(buffer);
        //wavesize.push_back(size);
        cnt++;

        duration+=seconds;
        samples+=size;

        char chkbuf[2];
        qint16 chksum;
        br=f.read(chkbuf,2);
        if (br<2)
            return false;
        chksum=chkbuf[0] << 8 | chkbuf[1];

    }

    if (samples==0)
        return false;

    //double rate=double(duration)/double(samples);

    // Convert to SampleFormat
    SampleFormat *data=new SampleFormat [samples];

    SampleFormat min=0,max=0;
    bool first=true;

    SampleFormat c;
    duration*=1000;

    for (long i=0;i<samples;i++) {
        data[i]=buffer[i];
        c=data[i];
        if (first) {
            min=max=c;
            first=false;
        }
        if (min>c) min=c;
        if (max<c) max=c;
    }
    Waveform *w=new Waveform(start,CPAP_FlowRate,data,samples,duration,min,max);
    session->AddWaveform(w);

    return true;
}

void InitModelMap()
{
    ModelMap[34]="RemStar Pro with C-Flex+";
    ModelMap[35]="RemStar Auto with A-Flex";
    ModelMap[37]="RemStar BIPAP Auto with Bi-Flex";
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

