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
#include <math.h>

#include "resmed_loader.h"
#include "SleepLib/session.h"

extern QProgressBar *qprogress;

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

    //qDebug(startdate.toString("yyyy-MM-dd HH:mm:ss").toLatin1());

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
            memcpy((char *)&sig.data[sig.pos],(char *)&buffer[pos],sig.nr*2);
            sig.pos+=sig.nr;
            pos+=sig.nr*2;
            /*for (int j=0;j<sig.nr;j++) {
                qint16 t=Read16();
                sig.data[sig.pos++]=t;
            } */
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
    //qDebug(("Opening "+name).toLatin1());
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

    qDebug(("Create ResMed Machine"+serial).toLatin1());
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
        return false; // id10t user..
        //newpath=path;
    } else {
        newpath=path+"/"+dirtag;
    }
    QString idfile=path+"/Identification.tgt";
    QFile f(idfile);
    map<QString,QString> idmap;
    if (f.open(QIODevice::ReadOnly)) {
        if (!f.isReadable())
            return false;

        while (!f.atEnd()) {
            QString line=f.readLine().trimmed();
            QString key,value;
            if (!line.isEmpty()) {
                key=line.section(" ",0,0);
                value=line.section(" ",1);
                key=key.section("#",1);
                idmap[key]=value;
            }
        }
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
    QString filename;
    int size=flist.size();
    for (int i=0;i<size;i++) {
        QFileInfo fi=flist.at(i);
        filename=fi.fileName();
        ext=filename.section(".",1).toLower();
        if (ext!="edf") continue;

        rest=filename.section(".",0,0);
        datestr=filename.section("_",0,1);
        date=QDateTime::fromString(datestr,"yyyyMMdd_HHmmss");
        sessionid=date.toTime_t();
        // Resmed bugs up on the session filenames.. 1 second either way
        if (sessfiles.find(sessionid)==sessfiles.end()) {
            if (sessfiles.find(sessionid+1)!=sessfiles.end()) sessionid++;
            if (sessfiles.find(sessionid-1)!=sessfiles.end()) sessionid--;
        }

        sessfiles[sessionid].push_back(fi.canonicalFilePath());
        if (qprogress) qprogress->setValue((float(i+1)/float(size)*33.0));
    }

    Machine *m=NULL;

    QString fn;
    Session *sess=NULL;
    int cnt=0;
    size=sessfiles.size();
    for (map<SessionID,vector<QString> >::iterator si=sessfiles.begin();si!=sessfiles.end();si++) {
        sessionid=si->first;
        //qDebug("Parsing Session %li",sessionid);
        bool done=false;
        bool first=true;
        for (int i=0;i<si->second.size();i++) {
            fn=si->second[i].section("_",-1).toLower();
            EDFParser edf(si->second[i]);
            //qDebug("Parsing File %i %i",i,edf.filesize);

            if (!edf.Parse())
                continue;

            if (first) { // First EDF file parsed, check if this data set is already imported
                m=CreateMachine(edf.serialnumber,profile);
                for (map<QString,QString>::iterator i=idmap.begin();i!=idmap.end();i++) {
                    if (i->first=="SRN") {
                        if (edf.serialnumber!=i->second) {
                            qDebug("edf Serial number doesn't match Identification.tgt");
                        }
                    } else if (i->first=="PNA") {
                        m->properties["Model"]=i->second;
                    } else {
                        m->properties[i->first]=i->second;
                    }
                }
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
        if (sess) {
            sess->summary[CPAP_PressureMedian]=sess->avg_event_field(CPAP_Pressure,0);
            sess->summary[CPAP_PressureAverage]=sess->weighted_avg_event_field(CPAP_Pressure,0);
            sess->summary[CPAP_PressureMinAchieved]=sess->min_event_field(CPAP_Pressure,0);
            sess->summary[CPAP_PressureMaxAchieved]=sess->max_event_field(CPAP_Pressure,0);
            sess->summary[CPAP_PressureMin]=sess->summary[CPAP_PressureMinAchieved];
            sess->summary[CPAP_PressureMax]=sess->summary[CPAP_PressureMaxAchieved];

            sess->summary[CPAP_LeakMinimum]=sess->min_event_field(CPAP_Leak,0);
            sess->summary[CPAP_LeakMaximum]=sess->max_event_field(CPAP_Leak,0); // should be merged..
            sess->summary[CPAP_LeakMedian]=sess->avg_event_field(CPAP_Leak,0);
            sess->summary[CPAP_LeakAverage]=sess->weighted_avg_event_field(CPAP_Leak,0);

            sess->summary[CPAP_Snore]=sess->sum_event_field(CPAP_Snore,0);
            sess->summary[CPAP_SnoreMinimum]=sess->min_event_field(CPAP_Snore,0);
            sess->summary[CPAP_SnoreMaximum]=sess->max_event_field(CPAP_Snore,0);
            sess->summary[CPAP_SnoreMedian]=sess->avg_event_field(CPAP_Snore,0);
            sess->summary[CPAP_SnoreAverage]=sess->weighted_avg_event_field(CPAP_Snore,0);
            sess->summary[CPAP_Obstructive]=sess->count_events(CPAP_Obstructive);
            sess->summary[CPAP_Hypopnea]=sess->count_events(CPAP_Hypopnea);
            sess->summary[CPAP_ClearAirway]=sess->count_events(CPAP_ClearAirway);
            sess->summary[CPAP_Mode]=MODE_APAP;
        }

        if (qprogress) qprogress->setValue(33.0+(float(++cnt)/float(size)*33.0));
    }
    //m->Save();
    if (qprogress) qprogress->setValue(100);
    return 0;
}

bool ResmedLoader::LoadEVE(Machine *mach,Session *sess,EDFParser &edf)
{
    QString t;
    long recs;
    double totaldur,duration;
    char * data;
    char c;
    long pos;
    bool sign,ok;
    double d;
    QDateTime tt;
    EventDataType fields[3];
    MachineCode code;
    Event *e;
    for (int s=0;s<edf.GetNumSignals();s++) {
        recs=edf.edfsignals[s]->nr*edf.GetNumDataRecords()*2;
        totaldur=edf.GetNumDataRecords()*edf.GetDuration();
        totaldur/=3600.0;
        //t.sprintf("EVE: %li %.2f",recs,totaldur);
        //qDebug((edf.edfsignals[s]->label+" "+t).toLatin1());
        data=(char *)edf.edfsignals[s]->data;
        pos=0;
        tt=edf.startdate;
        duration=0;
        while (pos<recs) {
            c=data[pos];
            if ((c!='+') && (c!='-'))
                break;
            if (data[pos++]=='+') sign=true; else sign=false;
            t="";
            c=data[pos];
            do {
                t+=c;
                pos++;
                c=data[pos];
            } while ((c!=20) && (c!=21)); // start code
            d=t.toDouble(&ok);
            if (!ok) {
                qDebug(("Faulty EDF EVE file"+edf.filename).toLatin1());
                break;
            }
            if (!sign) d=-d;

            tt=edf.startdate.addMSecs(d*1000.0);
            duration=0;
            // First entry

            if (data[pos]==21) {
                pos++;
                // get duration.
                t="";
                do {
                    t+=data[pos];
                    pos++;
                } while ((data[pos]!=20) && (pos<recs)); // start code
                duration=t.toDouble(&ok);
                if (!ok) {
                    qDebug(("Faulty EDF EVE file (at %li) "+edf.filename).toLatin1(),pos);
                    break;
                }
            }
            while ((data[pos]==20) && (pos<recs)) {
                t="";
                pos++;
                if (data[pos]==0)
                    break;
                if (data[pos]==20) {
                    pos++;
                    break;
                }

                do {
                    t+=tolower(data[pos++]);
                } while ((data[pos]!=20) && (pos<recs)); // start code
                if (!t.isEmpty()) {
                    code=MC_UNKNOWN;
                    if (t=="obstructive apnea") code=CPAP_Obstructive;
                    else if (t=="hypopnea") code=CPAP_Hypopnea;
                    else if (t=="central apnea") code=CPAP_ClearAirway;
                    if (code!=MC_UNKNOWN) {
                        fields[0]=duration;
                        sess->AddEvent(new Event(tt,code,fields,1));
                    } else {
                        if (t!="recording starts") {
                            qDebug(("Unknown ResMed annotation field: "+t).toLatin1());
                        }
                    }
                   // qDebug((tt.toString("yyyy-MM-dd HH:mm:ss")+t).toLatin1());
                }
                if (pos>=recs) {
                    qDebug(("Short EDF EVE file"+edf.filename).toLatin1());
                    break;
                }
               // pos++;
            }
            while ((data[pos]==0) && pos<recs) pos++;
            if (pos>=recs) break;
        }
       // qDebug(data);
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
        else {
            qDebug(("Unknown Signal "+edf.edfsignals[s]->label).toLatin1());
            continue;
        }
        sess->set_first(edf.startdate);
        QDateTime e=edf.startdate.addSecs(duration);
        sess->set_last(e);
        //duration/=3600.0;
        sess->set_hours(duration/3600.0);
        Waveform *w=new Waveform(edf.startdate,code,edf.edfsignals[s]->data,recs,duration,edf.edfsignals[s]->digital_minimum,edf.edfsignals[s]->digital_maximum);
        edf.edfsignals[s]->data=NULL; // so it doesn't get deleted when edf gets trashed.
        sess->AddWaveform(w);
        //t.sprintf("BRP: %li %.2f",recs,duration);
        //qDebug((edf.edfsignals[s]->label+" "+t).toLatin1());
    }
}
void ResmedLoader::ToTimeDelta(Machine *mach,Session *sess,EDFParser &edf, qint16 *data, MachineCode code, long recs, double duration,EventDataType divisor)
{
    bool first=true;
    double rate=(duration/recs)*1000.0;
    QDateTime tt=edf.startdate;
    EventDataType c,last;
    for (int i=0;i<recs;i++) {
        c=data[i]/divisor;
        //c=EventDataType(q)/2.0; //data[i]/divisor;

        if (first) {
            sess->AddEvent(new Event(tt,code,&c,1));
            first=false;
        } else {
            if (last!=c) {
                sess->AddEvent(new Event(tt,code,&c,1));
            }
        }
        tt=tt.addMSecs(rate);

        last=c;
    }
    sess->AddEvent(new Event(tt,code,&c,1)); // add one at the end..
}
bool ResmedLoader::LoadSAD(Machine *mach,Session *sess,EDFParser &edf)
{
    // Oximeter bull crap..
}


bool ResmedLoader::LoadPLD(Machine *mach,Session *sess,EDFParser &edf)
{
    // Is it save to assume the order does not change here?
    enum PLDType { MaskPres=0, TherapyPres, ExpPress, Leak, RR, Vt, Mv, SnoreIndex, FFLIndex, U1, U2 };

    //qDebug(edf.edfsignals[MaskPres]->label.toLatin1());

    QString t;
    for (int s=0;s<edf.GetNumSignals();s++) {
        long recs=edf.edfsignals[s]->nr*edf.GetNumDataRecords();
        double duration=edf.GetNumDataRecords()*edf.GetDuration();
        if (!sess->first().isValid()) {
            sess->set_first(edf.startdate);
            sess->set_last(edf.startdate.addMSecs(duration*1000.0));
            sess->set_hours(duration/3600.0);
        }
        MachineCode code;
       // if (s==TherapyPres) {
//            for (int i=0;i<recs;i++) qDebug("%04i %i",i,edf.edfsignals[s]->data[i]);
//        } else
        if (edf.edfsignals[s]->label=="Snore Index") {
            code=CPAP_Snore;
            ToTimeDelta(mach,sess,edf,edf.edfsignals[s]->data, code,recs,duration);
        } else if (edf.edfsignals[s]->label=="Therapy Pres") {
            code=CPAP_Pressure;
            ToTimeDelta(mach,sess,edf,edf.edfsignals[s]->data, code,recs,duration,100.0); //50.0
        } else if (edf.edfsignals[s]->label=="MV") {
            code=CPAP_MinuteVentilation;
            //ToTimeDelta(mach,sess,edf,edf.edfsignals[s]->data, code,recs,duration,1.0);
            Waveform *w=new Waveform(edf.startdate,code,edf.edfsignals[s]->data,recs,duration,edf.edfsignals[s]->digital_minimum,edf.edfsignals[s]->digital_maximum);
            edf.edfsignals[s]->data=NULL; // so it doesn't get deleted when edf gets trashed.
            sess->AddWaveform(w);
        } else if (edf.edfsignals[s]->label=="RR") {
            code=CPAP_RespiratoryRate;
            //ToTimeDelta(mach,sess,edf,edf.edfsignals[s]->data, code,recs,duration,1.0);
            Waveform *w=new Waveform(edf.startdate,code,edf.edfsignals[s]->data,recs,duration,edf.edfsignals[s]->digital_minimum,edf.edfsignals[s]->digital_maximum);
            edf.edfsignals[s]->data=NULL; // so it doesn't get deleted when edf gets trashed.
            sess->AddWaveform(w);
        } else if (edf.edfsignals[s]->label=="Vt") {
            code=CPAP_TidalVolume;
            //ToTimeDelta(mach,sess,edf,edf.edfsignals[s]->data, code,recs,duration,1.0);
            Waveform *w=new Waveform(edf.startdate,code,edf.edfsignals[s]->data,recs,duration,edf.edfsignals[s]->digital_minimum,edf.edfsignals[s]->digital_maximum);
            edf.edfsignals[s]->data=NULL; // so it doesn't get deleted when edf gets trashed.
            sess->AddWaveform(w);
        } else if (edf.edfsignals[s]->label=="Leak") {
            code=CPAP_Leak;
            ToTimeDelta(mach,sess,edf,edf.edfsignals[s]->data, code,recs,duration,1.0);
        } else if (edf.edfsignals[s]->label=="FFL Index") {
            code=CPAP_FlowLimitGraph;
            ToTimeDelta(mach,sess,edf,edf.edfsignals[s]->data, code,recs,duration,1.0);
        } else {
            qDebug(("Unknown Signal "+edf.edfsignals[s]->label).toLatin1());
        }
    }

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

