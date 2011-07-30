/*

SleepLib ResMed Loader Implementation

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

#include "resmed_loader.h"
#include "SleepLib/session.h"

extern QProgressBar *qprogress;
map<int,QString> RMS9ModelMap;

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

    version=QString::fromAscii(header.version,8).toLong(&ok);
    if (!ok)
        return false;

    //patientident=QString::fromAscii(header.patientident,80);
    recordingident=QString::fromAscii(header.recordingident,80); // Serial number is in here..
    int snp=recordingident.indexOf("SRN=");
    serialnumber.clear();
    /*char * idx=index(header.recordingident,'=');
    idx++;
    for (int i=0;i<16;++i) {
        if (*idx==0x20) break;
        serialnumber+=*idx;
        ++idx;
    } */

    for (int i=snp+4;i<recordingident.length();i++) {
        if (recordingident[i]==' ')
            break;
        serialnumber+=recordingident[i];
    }
    QDateTime startDate=QDateTime::fromString(QString::fromAscii(header.datetime,16),"dd.MM.yyHH.mm.ss");
    QDate d2=startDate.date();
    if (d2.year()<2000) {
        d2.setYMD(d2.year()+100,d2.month(),d2.day());
        startDate.setDate(d2);
    }
    if (!startDate.isValid()) {
        qDebug() << "Invalid date time retreieved parsing EDF File " << filename;
        return false;
    }
    startdate=startDate.toMSecsSinceEpoch();

    //qDebug() << startDate.toString("yyyy-MM-dd HH:mm:ss");

    num_header_bytes=QString::fromAscii(header.num_header_bytes,8).toLong(&ok);
    if (!ok)
        return false;
    //reserved44=QString::fromAscii(header.reserved,44);
    num_data_records=QString::fromAscii(header.num_data_records,8).toLong(&ok);
    if (!ok)
        return false;

    dur_data_record=QString::fromAscii(header.dur_data_records,8).toDouble(&ok)*1000.0;
    if (!ok)
        return false;
    num_signals=QString::fromAscii(header.num_signals,4).toLong(&ok);
    if (!ok)
        return false;

    enddate=startdate+dur_data_record*qint64(num_data_records);
   // if (dur_data_record==0)
     //   return false;

    // this could be loaded quicker by transducer_type[signal] etc..

    for (int i=0;i<num_signals;i++) {
        EDFSignal *signal=new EDFSignal;
        edfsignals.push_back(signal);
        signal->data=NULL;
        edfsignals[i]->label=Read(16);
    }

    for (int i=0;i<num_signals;i++) edfsignals[i]->transducer_type=Read(80);

    for (int i=0;i<num_signals;i++) edfsignals[i]->physical_dimension=Read(8);
    for (int i=0;i<num_signals;i++) edfsignals[i]->physical_minimum=Read(8).toDouble(&ok);
    for (int i=0;i<num_signals;i++) edfsignals[i]->physical_maximum=Read(8).toDouble(&ok);
    for (int i=0;i<num_signals;i++) edfsignals[i]->digital_minimum=Read(8).toDouble(&ok);
    for (int i=0;i<num_signals;i++) {
        EDFSignal & e=*edfsignals[i];
        e.digital_maximum=Read(8).toDouble(&ok);
        e.gain=(e.physical_maximum-e.physical_minimum)/(e.digital_maximum-e.digital_minimum);
        e.offset=0;
    }

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
            // big endian will screw up without this..
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
    datasize=filesize-EDFHeaderSize;
    if (datasize<0) return false;
    f.read((char *)&header,EDFHeaderSize);
    //qDebug() << "Opening " << name;
    buffer=new char [datasize];
    f.read(buffer,datasize);
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
    if (!profile) return NULL;
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

    qDebug() << "Create ResMed Machine" << serial;
    Machine *m=new CPAP(profile,0);
    m->SetClass(resmed_class_name);

    ResmedList[serial]=m;
    profile->AddMachine(m);

    m->properties["Serial"]=serial;
    m->properties["Brand"]="ResMed";
    QString a;
    a.sprintf("%i",resmed_data_version);
    m->properties["DataVersion"]=a;

    return m;

}

long event_cnt=0;

int ResmedLoader::Open(QString & path,Profile *profile)
{

    QString newpath;

    QString dirtag="DATALOG";
    if (path.endsWith("/"+dirtag)) {
        return 0; // id10t user..
        //newpath=path;
    } else {
        newpath=path+"/"+dirtag;
    }
    QString idfile=path+"/Identification.tgt";
    QFile f(idfile);
    map<QString,QString> idmap;
    if (f.open(QIODevice::ReadOnly)) {
        if (!f.isReadable())
            return 0;

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

    qDebug() << "ResmedLoader::Open newpath=" << newpath;
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
        QApplication::processEvents();

    }

    Machine *m=NULL;

    QString fn;
    Session *sess;
    int cnt=0;
    size=sessfiles.size();
    for (map<SessionID,vector<QString> >::iterator si=sessfiles.begin();si!=sessfiles.end();si++) {
        sessionid=si->first;
        //qDebug() << "Parsing Session " << sessionid;
        bool done=false;
        bool first=true;
        sess=NULL;
        for (size_t i=0;i<si->second.size();++i) {
            fn=si->second[i].section("_",-1).toLower();
            EDFParser edf(si->second[i]);
            //qDebug() << "Parsing File " << i << " "  << edf.filesize;

            if (!edf.Parse())
                continue;

            if (first) { // First EDF file parsed, check if this data set is already imported
                m=CreateMachine(edf.serialnumber,profile);
                for (map<QString,QString>::iterator i=idmap.begin();i!=idmap.end();i++) {
                    if (i->first=="SRN") {
                        if (edf.serialnumber!=i->second) {
                            qDebug() << "edf Serial number doesn't match Identification.tgt";
                        }
                    } else if (i->first=="PNA") {
                        m->properties["Model"]=i->second;
                    } else if (i->first=="PCD") {
                        bool ok;
                        int j=i->second.toInt(&ok);
                        if (RMS9ModelMap.find(j)!=RMS9ModelMap.end()) {
                            m->properties["SubModel"]=RMS9ModelMap[j];
                        }
                    } else {
                        m->properties[i->first]=i->second;
                    }
                }
                if (m->SessionExists(sessionid)) {
                    done=true;
                    break;
                }
                sess=new Session(m,sessionid);
                first=false;
            }
            if (!done) {
                if (fn=="eve.edf") LoadEVE(sess,edf);
                else if (fn=="pld.edf") LoadPLD(sess,edf);
                else if (fn=="brp.edf") LoadBRP(sess,edf);
                else if (fn=="sad.edf") LoadSAD(sess,edf);

                //if (first) {
                    //first=false;
                //}
            }
        }
        if (qprogress) qprogress->setValue(33.0+(float(++cnt)/float(size)*33.0));
        QApplication::processEvents();

        if (!sess) continue;
        if (!sess->first()) {
            delete sess;
            continue;
        } else {
            sess->SetChanged(true);
            m->AddSession(sess,profile); // Adding earlier than I really like here..
        }
        if (!done && sess) {
            sess->summary[CPAP_PressureMedian]=sess->avg(CPAP_Pressure);
            sess->summary[CPAP_PressureAverage]=sess->weighted_avg(CPAP_Pressure);
            sess->summary[CPAP_PressureMinAchieved]=sess->min(CPAP_Pressure);
            sess->summary[CPAP_PressureMaxAchieved]=sess->max(CPAP_Pressure);
            sess->summary[CPAP_PressureMin]=sess->summary[CPAP_PressureMinAchieved];
            sess->summary[CPAP_PressureMax]=sess->summary[CPAP_PressureMaxAchieved];

            sess->summary[CPAP_LeakMinimum]=sess->min(CPAP_Leak);
            sess->summary[CPAP_LeakMaximum]=sess->max(CPAP_Leak); // should be merged..
            sess->summary[CPAP_LeakMedian]=sess->avg(CPAP_Leak);
            sess->summary[CPAP_LeakAverage]=sess->weighted_avg(CPAP_Leak);

            sess->summary[CPAP_Snore]=sess->sum(CPAP_Snore);
            sess->summary[CPAP_SnoreMinimum]=sess->min(CPAP_Snore);
            sess->summary[CPAP_SnoreMaximum]=sess->max(CPAP_Snore);
            sess->summary[CPAP_SnoreMedian]=sess->avg(CPAP_Snore);
            sess->summary[CPAP_SnoreAverage]=sess->weighted_avg(CPAP_Snore);
            sess->summary[CPAP_Obstructive]=sess->count(CPAP_Obstructive);
            sess->summary[CPAP_Hypopnea]=sess->count(CPAP_Hypopnea);
            sess->summary[CPAP_ClearAirway]=sess->count(CPAP_ClearAirway);
            sess->summary[CPAP_Mode]=MODE_APAP;
        }

    }
    m->Save();
    if (qprogress) qprogress->setValue(100);
    qDebug() << "Total Events " << event_cnt;
    return 1;
}

bool ResmedLoader::LoadEVE(Session *sess,EDFParser &edf)
{
    // EVEnt records have useless duration record.

    QString t;
    long recs;
    double duration;
    char * data;
    char c;
    long pos;
    bool sign,ok;
    double d;
    double tt;
    MachineCode code;
    //Event *e;
    //totaldur=edf.GetNumDataRecords()*edf.GetDuration();

    EventList *EL[4]={NULL};
    sess->UpdateFirst(edf.startdate);
    //if (edf.enddate>edf.startdate) sess->set_last(edf.enddate);
    for (int s=0;s<edf.GetNumSignals();s++) {
        recs=edf.edfsignals[s]->nr*edf.GetNumDataRecords()*2;

        //qDebug() << edf.edfsignals[s]->label << " " << t;
        data=(char *)edf.edfsignals[s]->data;
        pos=0;
        tt=edf.startdate;
        sess->UpdateFirst(tt);
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
                qDebug() << "Faulty EDF EVE file " << edf.filename;
                break;
            }
            if (!sign) d=-d;
            tt=edf.startdate+qint64(d*1000.0);
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
                    qDebug() << "Faulty EDF EVE file (at %" << pos << ") " << edf.filename;
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
                    //code=MC_UNKNOWN;
                    if (t=="obstructive apnea") {
                        code=CPAP_Obstructive;
                        if (!EL[0]) {
                            EL[0]=new EventList(code,EVL_Event);
                            sess->eventlist[code].push_back(EL[0]);
                        }
                        EL[0]->AddEvent(tt,duration);
                    } else if (t=="hypopnea") {
                        code=CPAP_Hypopnea;
                        if (!EL[1]) {
                            EL[1]=new EventList(code,EVL_Event);
                            sess->eventlist[code].push_back(EL[1]);
                        }
                        EL[1]->AddEvent(tt,duration);
                    } else if (t=="apnea") {
                        code=CPAP_Apnea;
                        if (!EL[2]) {
                            EL[2]=new EventList(code,EVL_Event);
                            sess->eventlist[code].push_back(EL[2]);
                        }
                        EL[2]->AddEvent(tt,duration);
                    } else if (t=="central apnea") {
                        code=CPAP_ClearAirway;
                        if (!EL[3]) {
                            EL[3]=new EventList(code,EVL_Event);
                            sess->eventlist[code].push_back(EL[3]);
                        }
                        EL[3]->AddEvent(tt,duration);
                    } else {
                        if (t!="recording starts") {
                            qDebug() << "Unknown ResMed annotation field: " << t;
                        }
                    }
                }
                if (pos>=recs) {
                    qDebug() << "Short EDF EVE file" << edf.filename;
                    break;
                }
               // pos++;
            }
            while ((data[pos]==0) && pos<recs) pos++;
            if (pos>=recs) break;
        }
        sess->UpdateLast(tt);
       // qDebug(data);
    }
    return true;
}
bool ResmedLoader::LoadBRP(Session *sess,EDFParser &edf)
{
    QString t;
    sess->UpdateFirst(edf.startdate);
    qint64 duration=edf.GetNumDataRecords()*edf.GetDuration();
    sess->UpdateLast(edf.startdate+duration);

    for (int s=0;s<edf.GetNumSignals();s++) {
        EDFSignal & es=*edf.edfsignals[s];
        //qDebug() << "BRP:" << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum;
        long recs=edf.edfsignals[s]->nr*edf.GetNumDataRecords();
        MachineCode code;
        if (edf.edfsignals[s]->label=="Flow") {
            code=CPAP_FlowRate;
        } else if (edf.edfsignals[s]->label=="Mask Pres") {
            code=CPAP_MaskPressure;
            //for (int i=0;i<recs;i++) edf.edfsignals[s]->data[i]/=50.0;
        } else {
            qDebug() << "Unknown Signal " << edf.edfsignals[s]->label;
            continue;
        }
        double rate=double(duration)/double(recs);
        //es.gain=1;
        EventList *a=new EventList(code,EVL_Waveform,es.gain,es.offset,0,0,rate);

        a->AddWaveform(edf.startdate,es.data,recs,duration);

        if (code==CPAP_MaskPressure) {
            /*int v=ceil(a->max()/1);
            a->setMax(v*1);
            v=floor(a->min()/1);
            a->setMin(v*1); */
        } else if (code==CPAP_FlowRate) {
            a->setMax(1);
            a->setMin(-1);
        }
        sess->eventlist[code].push_back(a);
        //delete edf.edfsignals[s]->data;
        //edf.edfsignals[s]->data=NULL; // so it doesn't get deleted when edf gets trashed.
    }
    return true;
}
EventList * ResmedLoader::ToTimeDelta(Session *sess,EDFParser &edf, EDFSignal & es, MachineCode code, long recs, qint64 duration,EventDataType min,EventDataType max)
{
    bool first=true;
    double rate=(duration/recs); // milliseconds per record
    double tt=edf.startdate;
    //sess->UpdateFirst(tt);
    EventDataType c,last;
    //if (gain==0) gain=1;
    EventList *el=new EventList(code,EVL_Event,es.gain,es.offset,min,max);
    sess->eventlist[code].push_back(el);
    for (int i=0;i<recs;i++) {
        c=es.data[i];

        if (first) {
            el->AddEvent(tt,c);
            first=false;
        } else {
            if (last!=c) {
                el->AddEvent(tt,c);
            }
        }
        tt+=rate;

        last=c;
    }
    el->AddEvent(tt,c);
    sess->UpdateLast(tt);
    return el;
}
bool ResmedLoader::LoadSAD(Session *sess,EDFParser &edf)
{
    // Oximeter bull crap.. this oximeter is not reported of highly..
    // nonetheless, the data is easy to access.
    return true;
}


bool ResmedLoader::LoadPLD(Session *sess,EDFParser &edf)
{
    // Is it save to assume the order does not change here?
    enum PLDType { MaskPres=0, TherapyPres, ExpPress, Leak, RR, Vt, Mv, SnoreIndex, FFLIndex, U1, U2 };

    sess->UpdateFirst(edf.startdate);
    qint64 duration=edf.GetNumDataRecords()*edf.GetDuration();
    sess->UpdateLast(edf.startdate+duration);
    QString t;
    int emptycnt=0;
    for (int s=0;s<edf.GetNumSignals();s++) {
        EDFSignal & es=*edf.edfsignals[s];
        long recs=es.nr*edf.GetNumDataRecords();
        double rate=double(duration)/double(recs);
        //qDebug() << "EVE:" << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum << es.gain;
        MachineCode code;
        if (es.label=="Snore Index") {
            code=CPAP_Snore;

            //EventList *a=new EventList(code,EVL_Waveform,es.gain,es.offset,es.physical_minimum,es.physical_maximum,rate);
            //sess->eventlist[code].push_back(a);
            //a->AddWaveform(edf.startdate,es.data,recs,duration);
            EventList *a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
            a->setMax(1);
            a->setMin(0);
        } else if (es.label=="Therapy Pres") {
            code=CPAP_TherapyPressure;
            //EventList *a=new EventList(code,EVL_Waveform,es.gain,es.offset,es.physical_minimum,es.physical_maximum,rate);
            //sess->eventlist[code].push_back(a);
            //a->AddWaveform(edf.startdate,es.data,recs,duration);
            EventList *a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="MV") {
            code=CPAP_MinuteVentilation;
            //EventList *a=new EventList(code,EVL_Waveform,es.gain,es.offset,es.physical_minimum,es.physical_maximum,rate);
            //sess->eventlist[code].push_back(a);
            //a->AddWaveform(edf.startdate,es.data,recs,duration);
            EventList *a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="RR") {
            code=CPAP_RespiratoryRate;
            EventList *a=new EventList(code,EVL_Waveform,es.gain,es.offset,0,0,rate);
            sess->eventlist[code].push_back(a);
            a->AddWaveform(edf.startdate,es.data,recs,duration);
            //ToTimeDelta(sess,edf,es, code,recs,duration);
        } else if (es.label=="Vt") {
            code=CPAP_TidalVolume;
            //EventList *a=new EventList(code,EVL_Waveform,es.gain,es.offset,es.physical_minimum,es.physical_maximum,rate);
            //sess->eventlist[code].push_back(a);
            //a->AddWaveform(edf.startdate,es.data,recs,duration);
            es.physical_maximum=es.physical_minimum=0;
            es.gain*=1000.0;
            EventList *a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="Leak") {
            code=CPAP_Leak;
           // es.gain*=100.0;
            //es.gain=1;//10.0;
            es.offset=-0.5;
            EventList *a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
            a->setMax(1);
            a->setMin(0);
        } else if (es.label=="FFL Index") {
            code=CPAP_FlowLimitGraph;
            //es.gain=1;//10.0;
            EventList *a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
            a->setMax(1);
            a->setMin(0);
        } else if (es.label=="Mask Pres") {
            code=CPAP_Pressure;
            //es.gain=1;
            EventList *a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="Exp Press") {
            code=CPAP_ExpPressure;
            EventList *a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="") {
            if (emptycnt==0) {
                code=ResMed_Empty1;
                ToTimeDelta(sess,edf,es, code,recs,duration);
            } else if (emptycnt==1) {
                code=ResMed_Empty2;
                ToTimeDelta(sess,edf,es, code,recs,duration);
            } else {
                qDebug() << "Unobserved Empty Signal " << es.label;
            }
            emptycnt++;
        } else {
            qDebug() << "Unobserved Signal " << es.label;
        }
    }
    return true;
}

void ResInitModelMap()
{
    // Courtesy Troy Schultz
    RMS9ModelMap[36001]="ResMed S9 Escape";
    RMS9ModelMap[36002]="ResMed S9 Escape Auto";
    RMS9ModelMap[36003]="ResMed S9 Elite";
    RMS9ModelMap[36004]="ResMed S9 VPAP S";
    RMS9ModelMap[36005]="ResMed S9 AutoSet";
    RMS9ModelMap[36006]="ResMed S9 VPAP Auto";
    RMS9ModelMap[36007]="ResMed S9 VPAP Adapt";
    RMS9ModelMap[36008]="ResMed S9 VPAP ST";
// S8 Series
    RMS9ModelMap[33007]="ResMed S8 Escape";
    RMS9ModelMap[33039]="ResMed S8 Elite II";
    RMS9ModelMap[33051]="ResMed S8 Escape II";
    RMS9ModelMap[33064]="ResMed S8 Escape II AutoSet";
    RMS9ModelMap[33064]="ResMed S8 Escape II AutoSet";
    RMS9ModelMap[33129]="ResMed S8 AutoSet II";
};


bool resmed_initialized=false;
void ResmedLoader::Register()
{
    if (resmed_initialized) return;
    qDebug() << "Registering ResmedLoader";
    RegisterLoader(new ResmedLoader());
    ResInitModelMap();
    resmed_initialized=true;
}

