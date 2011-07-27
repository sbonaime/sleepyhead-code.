/*
SleepLib CMS50X Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the cms50_data_version in cms50_loader.h when making changes to this loader
// that change loader behaviour or modify channels.
//********************************************************************************************

#include <QProgressBar>
#include <QDir>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include <list>

using namespace std;

#include "cms50_loader.h"
#include "SleepLib/machine.h"
#include "SleepLib/session.h"

extern QProgressBar *qprogress;

CMS50Loader::CMS50Loader()
{
    //ctor
}

CMS50Loader::~CMS50Loader()
{
    //dtor
}
int CMS50Loader::Open(QString & path,Profile *profile)
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

    QDir dir(path);
    QString tmp=path+"/Data";
    if ((dir.exists("SpO2 Review.ini"))
        && (dir.exists("SpO2.ini"))
        && (dir.exists("Data"))) {
             // Their software

            return OpenCMS50(tmp,profile);
    } else {
        // My Logger
    }

    return 0;
}
int CMS50Loader::OpenCMS50(QString & path, Profile *profile)
{
    QString filename,pathname;
    list<QString> files;
    QDir dir(path);

    if (!dir.exists())
        return 0;

    if(qprogress) qprogress->setValue(0);

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist=dir.entryInfoList();

    for (int i=0;i<flist.size();i++) {
        QFileInfo fi=flist.at(i);

        if (fi.fileName().toLower().endsWith(".spor")) {
            files.push_back(fi.canonicalFilePath());
        }
        //if (loader_progress) loader_progress->Pulse();

    }
    int size=files.size();
    if (size==0) return 0;

    Machine *mach=CreateMachine(profile);
    int cnt=0;
    for (list<QString>::iterator n=files.begin();n!=files.end();n++,++cnt) {
        if (qprogress) qprogress->setValue((float(cnt)/float(size)*50.0));
        OpenSPORFile((*n),mach,profile);
    }
    mach->Save();
    if (qprogress) qprogress->setValue(100);
    return 1;
}
bool CMS50Loader::OpenSPORFile(QString path,Machine *mach,Profile *profile)
{
    if (!mach || !profile)
        return false;

    QFile f(path);
    unsigned char tmp[256];

    qint16 data_starts;
    qint16 some_code;
    qint16 some_more_code;
    int num_records;
    int br;

    if (!f.open(QIODevice::ReadOnly))
        return false;

    // Find everything after the last _

    QString str=path.section("/",-1); //AfterLast(wxChar('/'));
    str=str.section("_",-1);
    str=str.section(".",0,0); //BeforeFirst(wxChar('.'));

    QDateTime dt=QDateTime::fromString(str,"yyyyMMddHHmm");
    if (!dt.isValid())
        return false;

    SessionID sessid=dt.toTime_t(); // Import date becomes session id

    if (mach->SessionExists(sessid))
        return false; // Already imported


    br=f.read((char *)tmp,2);
    if (br!=2) return false;
    data_starts=tmp[0] | (tmp[1] << 8);

    br=f.read((char *)tmp,2);
    if (br!=2) return false;
    some_code=tmp[0] | (tmp[1] << 8); // == 512

    br=f.read((char *)tmp,2);
    if (br!=2) return false;
    num_records=tmp[0] | (tmp[1] << 8);
    if (num_records<300) return false; // dont bother.

    num_records <<= 1;

    br=f.read((char *)tmp,2);
    if (br!=2) return false;
    some_more_code=tmp[0] | (tmp[1] << 8);  // == 0

    br=f.read((char *)tmp,34);
    if (br!=34) return false;

    for (int i=0;i<17;i++) {
        tmp[i]=tmp[i << 1];
    }
    tmp[17]=0;
    QString datestr=(char *)tmp;

    QDateTime date=QDateTime::fromString(datestr,"MM/dd/yy HH:mm:ss");
    QDate d2=date.date();

    if (d2.year()<2000) {
        d2.setYMD(d2.year()+100,d2.month(),d2.day());
        date.setDate(d2);
    }
    if (!date.isValid()) {
        qDebug() << "Invalid date time retreieved in CMS50::OpenSPORFile";
        return false;
    }
    qint64 starttime=date.toMSecsSinceEpoch();

    f.seek(data_starts);

    buffer=new char [num_records];
    br=f.read(buffer,num_records);
    if (br!=num_records) {
        qDebug() << "Short .spoR File: " << path;
        delete [] buffer;
        return false;
    }

    QDateTime last_pulse_time=date;
    QDateTime last_spo2_time=date;

    EventDataType last_pulse=buffer[0];
    EventDataType last_spo2=buffer[1];
    EventDataType cp,cs;

    Session *sess=new Session(mach,sessid);
    sess->UpdateFirst(starttime);
    EventList *oxip=new EventList(OXI_Pulse,EVL_Event);
    EventList *oxis=new EventList(OXI_SPO2,EVL_Event);
    sess->eventlist[OXI_Pulse].push_back(oxip);
    sess->eventlist[OXI_SPO2].push_back(oxis);
    oxip->AddEvent(starttime,last_pulse);
    oxis->AddEvent(starttime,last_spo2);
    //sess->AddEvent(new Event(starttime,OXI_Pulse,0,&last_pulse,1));
    //sess->AddEvent(new Event(starttime,OXI_SPO2,0,&last_spo2,1));

    EventDataType PMin=0,PMax=0,SMin=0,SMax=0,PAvg=0,SAvg=0;
    int PCnt=0,SCnt=0;
    //wxDateTime
    qint64 tt=starttime;
    //fixme: Need two lasttime values here..
    qint64 lasttime=starttime;

    bool first_p=true,first_s=true;

    for (int i=2;i<num_records;i+=2) {
        cp=buffer[i];
        cs=buffer[i+1];
        if (last_pulse!=cp) {
            oxip->AddEvent(tt,cp);
            //sess->AddEvent(new Event(tt,OXI_Pulse,0,&cp,1));
            if (tt>lasttime) lasttime=tt;
            if (cp>0) {
                if (first_p) {
                    PMin=cp;
                    first_p=false;
                } else {
                    if (PMin>cp) PMin=cp;
                }
                PAvg+=cp;
                PCnt++;
            }
        }
        if (last_spo2!=cs) {
            oxis->AddEvent(tt,cs);
            //sess->AddEvent(new Event(tt,OXI_SPO2,0,&cs,1));
            if (tt>lasttime) lasttime=tt;
            if (cs>0) {
                if (first_s) {
                    SMin=cs;
                    first_s=false;
                } else {
                    if (SMin>cs) SMin=cs;
                }
                SAvg+=cs;
                SCnt++;
            }
        }
        last_pulse=cp;
        last_spo2=cs;
        if (PMax<cp) PMax=cp;
        if (SMax<cs) SMax=cs;
        tt+=1000; // An educated guess of 1 second. Verified by gcz@cpaptalk
    }
    if (cp) oxip->AddEvent(tt,cp);//sess->AddEvent(new Event(tt,OXI_Pulse,0,&cp,1));
    if (cs) oxis->AddEvent(tt,cs);//sess->AddEvent(new Event(tt,OXI_SPO2,0,&cs,1));

    sess->UpdateLast(tt);
    //double t=sess->last()-sess->first().toTime_t();
    //double hours=(t/3600.0);
    //sess->set_hours(hours);

    EventDataType pa=0,sa=0;
    if (PCnt>0) pa=PAvg/double(PCnt);
    if (SCnt>0) sa=SAvg/double(SCnt);
    sess->summary[OXI_PulseAverage]=pa;
    sess->summary[OXI_PulseMin]=PMin;
    sess->summary[OXI_PulseMax]=PMax;
    sess->summary[OXI_SPO2Average]=sa;
    sess->summary[OXI_SPO2Min]=SMin;
    sess->summary[OXI_SPO2Max]=SMax;

    mach->AddSession(sess,profile);
    sess->SetChanged(true);
    delete [] buffer;

    return true;
}
Machine *CMS50Loader::CreateMachine(Profile *profile)
{
    if (!profile)
        return NULL;

    // NOTE: This only allows for one CMS50 machine per profile..
    // Upgrading their oximeter will use this same record..

    vector<Machine *> ml=profile->GetMachines(MT_OXIMETER);

    for (vector<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
        if ((*i)->GetClass()==cms50_class_name)  {
            return (*i);
            break;
        }
    }

    qDebug() << "Create CMS50 Machine Record";

    Machine *m=new Oximeter(profile,0);
    m->SetClass(cms50_class_name);
    m->properties["Brand"]="Contec";
    m->properties["Model"]="CMS50X";
    QString a;
    a.sprintf("%i",cms50_data_version);
    m->properties["DataVersion"]=a;
    profile->AddMachine(m);

    return m;
}



static bool cms50_initialized=false;

void CMS50Loader::Register()
{
    if (cms50_initialized) return;
    qDebug() << "Registering CMS50Loader";
    RegisterLoader(new CMS50Loader());
    //InitModelMap();
    cms50_initialized=true;
}

