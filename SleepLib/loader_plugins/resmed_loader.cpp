/*

SleepLib ResMed Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#include <QString>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProgressBar>

#include "resmed_loader.h"
#include "SleepLib/session.h"

EDFParser::EDFParser(QString name)
{
    buffer=NULL;
    Open(name);
}
EDFParser::~EDFParser()
{
    vector<EDFSignal *>::iterator s;
    for (s=edfsignals.begin();s!=edfsignals.end();s++) {
        if ((*s)->data) delete [] (*s)->data;
        delete *s;
    }
    if (buffer) delete [] buffer;
}
qint16 EDFParser::Read16()
{
    unsigned char *buf=(unsigned char *)buffer;
    if (pos>=filesize) return 0;
    qint16 res=*(qint16 *)&buf[pos];
    //qint16 res=(buf[pos] ^128)<< 8 | buf[pos+1] ^ 128;
    pos+=2;
    return res;
}
QString EDFParser::Read(int si)
{
    QString str;
    if (pos>=filesize) return "";
    for (int i=0;i<si;i++) {
        str+=buffer[pos++];
    }
    return str.trimmed();
}
bool EDFParser::Parse()
{
    bool ok;
    QString temp,temp2;
    version=Read(8).toLong(&ok);
    if (!ok)
        return false;

    patientident=Read(80);
    recordingident=Read(80); // Serial number is in here..
    int snp=recordingident.indexOf("SRN=");

    for (int i=snp+4;i<recordingident.length();i++) {
        if (recordingident[i]==' ')
            break;
        serialnumber+=recordingident[i];
    }
    temp=Read(8);
    temp+=" "+Read(8);
    startdate=QDateTime::fromString(temp,"dd.MM.yy HH.mm.ss");
    QDate d2=startdate.date();
    if (d2.year()<2000) {
        d2.setYMD(d2.year()+100,d2.month(),d2.day());
        startdate.setDate(d2);
    }
    if (!startdate.isValid()) {
        qDebug(("Invalid date time retreieved parsing EDF File"+filename).toLatin1());
        return false;
    }

    qDebug(startdate.toString("yyyy-MM-dd HH:mm:ss").toLatin1());

    num_header_bytes=Read(8).toLong(&ok);
    if (!ok)
        return false;
    reserved44=Read(44);
    num_data_records=Read(8).toLong(&ok);
    if (!ok)
        return false;
    temp=Read(8);
  //  temp="0.00";
    dur_data_record=temp.toDouble(&ok);
    if (!ok)
        return false;
    num_signals=Read(4).toLong(&ok);
    if (!ok)
        return false;

    for (int i=0;i<num_signals;i++) {
        EDFSignal *signal=new EDFSignal;
        edfsignals.push_back(signal);
        signal->data=NULL;
        edfsignals[i]->label=Read(16);
    }

    for (int i=0;i<num_signals;i++) edfsignals[i]->transducer_type=Read(80);
    for (int i=0;i<num_signals;i++) edfsignals[i]->physical_dimension=Read(8);
    for (int i=0;i<num_signals;i++) edfsignals[i]->physical_minimum=Read(8).toLong(&ok);
    for (int i=0;i<num_signals;i++) edfsignals[i]->physical_maximum=Read(8).toLong(&ok);
    for (int i=0;i<num_signals;i++) edfsignals[i]->digital_minimum=Read(8).toLong(&ok);
    for (int i=0;i<num_signals;i++) edfsignals[i]->digital_maximum=Read(8).toLong(&ok);
    for (int i=0;i<num_signals;i++) edfsignals[i]->prefiltering=Read(80);
    for (int i=0;i<num_signals;i++) edfsignals[i]->nr=Read(8).toLong(&ok);
    for (int i=0;i<num_signals;i++) edfsignals[i]->reserved=Read(32);

    // allocate the buffers
    for (int i=0;i<num_signals;i++) {
        //qDebug//cout << "Reading signal " << signals[i]->label << endl;
        EDFSignal & sig=*edfsignals[i];

        long recs=sig.nr * num_data_records;
        if (num_data_records<0)
            return false;
        sig.data=new qint16 [recs];
        sig.pos=0;
    }

    for (int x=0;x<num_data_records;x++) {
        for (int i=0;i<num_signals;i++) {
            EDFSignal & sig=*edfsignals[i];
            for (int j=0;j<sig.nr;j++) {
                qint16 t=Read16();
                sig.data[sig.pos++]=t;
            }
        }
    }

    return true;
}
bool EDFParser::Open(QString name)
{
    QFile f(name);
    if (!f.open(QIODevice::ReadOnly)) return false;
    if (!f.isReadable()) return false;
    filename=name;
    filesize=f.size();
    qDebug(("Opening "+name).toLatin1());
    buffer=new char [filesize];
    f.read(buffer,filesize);
    f.close();
    pos=0;
    return true;
}

ResmedLoader::ResmedLoader()
{
}
ResmedLoader::~ResmedLoader()
{
}

Machine *ResmedLoader::CreateMachine(QString serial,Profile *profile)
{
    assert(profile!=NULL);
    vector<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    for (vector<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()==resmed_class_name) && ((*i)->properties["Serial"]==serial)) {
            ResmedList[serial]=*i; //static_cast<CPAP *>(*i);
            found=true;
            break;
        }
    }
    if (found) return ResmedList[serial];

    qDebug(("Create ResMed Machine %s"+serial).toLatin1());
    Machine *m=new CPAP(profile,0);
    m->SetClass(resmed_class_name);

    ResmedList[serial]=m;
    profile->AddMachine(m);

    m->properties["Serial"]=serial;
    m->properties["Brand"]="ResMed";

    return m;

}

bool ResmedLoader::Open(QString & path,Profile *profile)
{

    QString newpath;

    QString dirtag="DATALOG";
    if (path.endsWith("/"+dirtag)) {
        newpath=path;
    } else {
        newpath=path+"/"+dirtag;
    }

    QDir dir(newpath);

    if ((!dir.exists() || !dir.isReadable()))
        return 0;

    qDebug(("ResmedLoader::Open newpath="+newpath).toLatin1());
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist=dir.entryInfoList();
    map<SessionID,vector<QString> > sessfiles;

    QString ext,rest,datestr,s,codestr;
    SessionID sessionid;
    QDateTime date;

    for (int i=0;i<flist.size();i++) {
        QFileInfo fi=flist.at(i);
        QString filename=fi.fileName();
        ext=filename.section(".",1).toLower();
        if (ext!="edf") continue;

        rest=filename.section(".",0,0);
        datestr=filename.section("_",0,1);
        date=QDateTime::fromString(datestr,"yyyyMMdd_HHmmss");
        sessionid=date.toTime_t();

        sessfiles[sessionid].push_back(fi.canonicalFilePath());
    }

    Machine *m=NULL;

    Session *sess=NULL;
    for (map<SessionID,vector<QString> >::iterator si=sessfiles.begin();si!=sessfiles.end();si++) {
        sessionid=si->first;
        qDebug("Parsing Session %li",sessionid);
        bool done=false;
        bool first=true;
        for (int i=0;i<si->second.size();i++) {
            QString fn=si->second[i].section("_",-1).toLower();
            EDFParser edf(si->second[i]);
            qDebug("Parsing File %i %i",i,edf.filesize);

            if (!edf.Parse())
                continue;

            if (first) { // First EDF file parsed, check if this data set is already imported
                m=CreateMachine(edf.serialnumber,profile);
                if (m->SessionExists(sessionid)) {
                    done=true;
                    break;
                }
                sess=new Session(m,sessionid);
            }
            if (!done) {
                if (fn=="eve.edf") LoadEVE(m,sess,edf);
                else if (fn=="pld.edf") LoadPLD(m,sess,edf);
                else if (fn=="brp.edf") LoadBRP(m,sess,edf);
                else if (fn=="sad.edf") LoadSAD(m,sess,edf);
            }
            if (first) {
                sess->SetChanged(true);
                m->AddSession(sess,profile); // Adding earlier than I really like here..
                first=false;
            }
        }
    }
    return 0;
}
bool ResmedLoader::LoadEVE(Machine *mach,Session *sess,EDFParser &edf)
{
    QString t;
    for (int s=0;s<edf.GetNumSignals();s++) {
        long recs=edf.edfsignals[s]->nr*edf.GetNumDataRecords();
        double duration=edf.GetNumDataRecords()*edf.GetDuration();
        duration/=3600.0;
        t.sprintf("EVE: %li %.2f",recs,duration);
        qDebug((edf.edfsignals[s]->label+" "+t).toLatin1());
        char * data=(char *)edf.edfsignals[s]->data;
    }
}
bool ResmedLoader::LoadBRP(Machine *mach,Session *sess,EDFParser &edf)
{
    QString t;
    for (int s=0;s<edf.GetNumSignals();s++) {
        long recs=edf.edfsignals[s]->nr*edf.GetNumDataRecords();
        double duration=edf.GetNumDataRecords()*edf.GetDuration();
        MachineCode code;
        if (edf.edfsignals[s]->label=="Flow") code=CPAP_FlowRate;
        else if (edf.edfsignals[s]->label=="Mask Pres") code=CPAP_MaskPressure;
        sess->set_first(edf.startdate);
        QDateTime e=edf.startdate.addSecs(duration);
        sess->set_last(e);
        //duration/=3600.0;
        sess->set_hours(duration/3600.0);
        Waveform *w=new Waveform(edf.startdate,code,edf.edfsignals[s]->data,recs,duration,edf.edfsignals[s]->digital_minimum,edf.edfsignals[s]->digital_maximum);
        edf.edfsignals[s]->data=NULL; // so it doesn't get deleted when edf gets trashed.
        sess->AddWaveform(w);
        t.sprintf("BRP: %li %.2f",recs,duration);
        qDebug((edf.edfsignals[s]->label+" "+t).toLatin1());
    }
}
bool ResmedLoader::LoadSAD(Machine *mach,Session *sess,EDFParser &edf)
{
}
bool ResmedLoader::LoadPLD(Machine *mach,Session *sess,EDFParser &edf)
{
}

void ResInitModelMap()
{
   // ModelMap[34]="S9 with some weird feature";
};


bool resmed_initialized=false;
void ResmedLoader::Register()
{
    if (resmed_initialized) return;
    qDebug("Registering ResmedLoader");
    RegisterLoader(new ResmedLoader());
    ResInitModelMap();
    resmed_initialized=true;
}

