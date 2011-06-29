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
        if ((*s)->adata) delete [] (*s)->adata;
        delete *s;
    }
    if (buffer) delete [] buffer;
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
    startdate.fromString(temp,"dd.MM.yy HH.mm.ss");
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
        signal->adata=NULL;
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

    for (int i=0;i<num_signals;i++) {
        //qDebug//cout << "Reading signal " << signals[i]->label << endl;
        if (edfsignals[i]->label!="EDF Annotations") {
            // Waveforms
            edfsignals[i]->data=new qint16 [edfsignals[i]->nr];
            for (int j=0;j<edfsignals[i]->nr;j++){
                long t;
                t=Read(2).toLong(&ok);
                if (!ok)
                    return false;
                edfsignals[i]->data[j]=t & 0xffff;
            }

        } else { // Annotation data..
            edfsignals[i]->adata=(char *) new char [edfsignals[i]->nr*2];
            //cout << signals[i]->nr << endl;;
            for (int j=0;j<edfsignals[i]->nr*2;j++)
                edfsignals[i]->adata[j]=buffer[pos++];
        }
        //cout << "Read Signal" << endl;
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
    buffer=new char [filesize];
    f.read(buffer,filesize);
    f.close();
    pos=0;
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
    map<SessionID,Session *> sessions;
    Session *sess;
    Machine *m;

    for (int i=0;i<flist.size();i++) {
        QFileInfo fi=flist.at(i);
        QString filename=fi.fileName();
        ext=filename.section(".",1).toLower();
        if (ext!="edf") continue;
        rest=filename.section(".",0,0);
        datestr=filename.section("_",0,1);
        date=QDateTime::fromString(datestr,"yyyyMMdd_HHmmss");
        sessionid=date.toTime_t();

        s.sprintf("%li",sessionid);
        codestr=rest.section("_",2).toUpper();
        int code;
        if (codestr=="EVE") code=0;
        else if (codestr=="PLD") code=1;
        else if (codestr=="BRP") code=2;
        else if (codestr=="SAD") code=3;
        else {
            qDebug(("Unknown file EDF type"+filename).toLatin1());
            continue;
        }
        qDebug(("Parsing "+filename).toLatin1());
        EDFParser edf(fi.canonicalFilePath());
        if (!edf.Parse()) continue;

        Machine *m=CreateMachine(edf.serialnumber,profile);

        if (sessions.find(sessionid)==sessions.end()) {
            sessions[sessionid]=new Session(m,sessionid);
        }

        sess=sessions[sessionid];
        sess->SetChanged(true);
        switch(code) {
        case 0: LoadEVE(m,sess,edf);
                break;
        case 1: LoadPLD(m,sess,edf);
                break;
        case 2: LoadBRP(m,sess,edf);
                break;
        case 3: LoadSAD(m,sess,edf);
                break;
        }
    }
    return 0;
}
bool ResmedLoader::LoadEVE(Machine *mach,Session *sess,EDFParser &edf)
{
    QString t;
    for (int s=0;s<edf.GetNumSignals();s++) {
        t.sprintf("%i",edf.edfsignals[s]->nr);
        qDebug((edf.edfsignals[s]->label+" "+t).toLatin1());
        if (edf.edfsignals[s]->adata) {
        }
    }
}
bool ResmedLoader::LoadBRP(Machine *mach,Session *sess,EDFParser &edf)
{
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

