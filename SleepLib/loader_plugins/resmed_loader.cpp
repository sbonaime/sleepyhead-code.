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
QHash<int,QString> RMS9ModelMap;
QHash<ChannelID, QVector<QString> > resmed_codes;

EDFParser::EDFParser(QString name)
{
    buffer=NULL;
    Open(name);
}
EDFParser::~EDFParser()
{
    QVector<EDFSignal *>::iterator s;
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
    startdate=qint64(startDate.toTime_t())*1000L;

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
        lookup[edfsignals[i]->label]=signal;
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
    QVector<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    QVector<Machine *>::iterator i;
    for (i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()==resmed_class_name) && ((*i)->properties["Serial"]==serial)) {
            ResmedList[serial]=*i; //static_cast<CPAP *>(*i);
            found=true;
            break;
        }
    }
    if (found) return *i;

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
    QHash<QString,QString> idmap;
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
    QString strfile=path+"/STR.edf";
    EDFParser stredf(strfile);

    if (!stredf.Parse()) {
        qDebug() << "Faulty file" << strfile;
        return 0;
    }

    qint64 duration=stredf.GetNumDataRecords()*stredf.GetDuration();
    int days=duration/86400000L;

    QDateTime dt1=QDateTime::fromTime_t(stredf.startdate/1000L);
    QDateTime dt2=QDateTime::fromTime_t(stredf.enddate/1000L);
    QDate dd1=dt1.date();
    QDate dd2=dt2.date();
    for (int s=0;s<stredf.GetNumSignals();s++) {
        EDFSignal & es=*stredf.edfsignals[s];
        long recs=stredf.edfsignals[s]->nr*stredf.GetNumDataRecords();

        qDebug() << "STREDF:" << stredf.edfsignals[s]->label << recs;
    }


    QDir dir(newpath);

    if ((!dir.exists() || !dir.isReadable()))
        return 0;

    qDebug() << "ResmedLoader::Open newpath=" << newpath;
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist=dir.entryInfoList();
    QMap<SessionID,QVector<QString> > sessfiles;

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
            if (sessfiles.find(sessionid+2)!=sessfiles.end()) sessionid+=2;
            else if (sessfiles.find(sessionid+1)!=sessfiles.end()) sessionid++;
            else if (sessfiles.find(sessionid-1)!=sessfiles.end()) sessionid--;
            else if (sessfiles.find(sessionid-2)!=sessfiles.end()) sessionid-=2;
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
    for (QMap<SessionID,QVector<QString> >::iterator si=sessfiles.begin();si!=sessfiles.end();si++) {
        sessionid=si.key();
        //qDebug() << "Parsing Session " << sessionid;
        bool done=false;
        bool first=true;
        sess=NULL;
        for (int i=0;i<si.value().size();++i) {
            fn=si.value()[i].section("_",-1).toLower();
            EDFParser edf(si.value()[i]);
            //qDebug() << "Parsing File " << i << " "  << edf.filesize;

            if (!edf.Parse())
                continue;

            if (first) { // First EDF file parsed, check if this data set is already imported
                m=CreateMachine(edf.serialnumber,profile);
                for (QHash<QString,QString>::iterator i=idmap.begin();i!=idmap.end();i++) {
                    if (i.key()=="SRN") {
                        if (edf.serialnumber!=i.value()) {
                            qDebug() << "edf Serial number doesn't match Identification.tgt";
                        }
                        if (edf.serialnumber!=stredf.serialnumber) {
                            qDebug() << "edf Serial number doesn't match STR.edf!";
                        }
                    } else if (i.key()=="PNA") {
                        m->properties["Model"]=i.value();
                    } else if (i.key()=="PCD") {
                        bool ok;
                        int j=i.value().toInt(&ok);
                        if (RMS9ModelMap.find(j)!=RMS9ModelMap.end()) {
                            m->properties["SubModel"]=RMS9ModelMap[j];
                        }
                    } else {
                        m->properties[i.key()]=i.value();
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
            qint64 dif=sess->first()-stredf.startdate;
            int dn=dif/86400000L;
            if (dn<days) {
                int mode;
                if (stredf.lookup.contains("Mode"))
                    mode=(*stredf.lookup["Mode"]).data[dn];
                else mode=0;
                if (stredf.lookup.contains("EPR"))
                    sess->settings["EPR"]=(*stredf.lookup["EPR"]).data[dn];
                if (stredf.lookup.contains("EPRSet"))
                    sess->settings["EPRSet"]=(*stredf.lookup["EPR Level"]).data[dn];

                EDFSignal *sig;
                if (mode==0) {
                    sess->settings[CPAP_Mode]=MODE_CPAP;
                    sig=stredf.lookup["Set Pressure"];
                    EventDataType pressure=sig->data[dn]*sig->gain;
                    sess->settings[CPAP_Pressure]=pressure;
                    sess->setWavg(CPAP_Pressure,pressure);
                    sess->setAvg(CPAP_Pressure,pressure);
                    sess->set90p(CPAP_Pressure,pressure);
                    sess->setMax(CPAP_Pressure,pressure);
                    sess->setMin(CPAP_Pressure,pressure);
                } else {
                    if (mode>5) {
                        sess->settings[CPAP_Mode]=MODE_BIPAP;
                    } else {
                        sess->settings[CPAP_Mode]=MODE_APAP;
                    }
                    sig=stredf.lookup["Min Pressure"];
                    if (sig)
                        sess->setMin(CPAP_Pressure,sig->data[dn]*sig->gain);
                    sig=stredf.lookup["Max Pressure"];
                    if (sig)
                        sess->setMax(CPAP_Pressure,sig->data[dn]*sig->gain);
                }

            }
            m->AddSession(sess,profile); // Adding earlier than I really like here..
        }
    }

    if (m) {
        m->Save();
    }
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
    ChannelID code;
    //Event *e;
    //totaldur=edf.GetNumDataRecords()*edf.GetDuration();

    EventList *EL[4]={NULL};
    sess->updateFirst(edf.startdate);
    //if (edf.enddate>edf.startdate) sess->set_last(edf.enddate);
    for (int s=0;s<edf.GetNumSignals();s++) {
        recs=edf.edfsignals[s]->nr*edf.GetNumDataRecords()*2;

        //qDebug() << edf.edfsignals[s]->label << " " << t;
        data=(char *)edf.edfsignals[s]->data;
        pos=0;
        tt=edf.startdate;
        sess->updateFirst(tt);
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
                    if (t=="obstructive apnea") {
                        if (!EL[0]) {
                            if (!(EL[0]=sess->AddEventList(CPAP_Obstructive,EVL_Event))) return false;
                        }
                        EL[0]->AddEvent(tt,duration);
                    } else if (t=="hypopnea") {
                        if (!EL[1]) {
                            if (!(EL[1]=sess->AddEventList(CPAP_Hypopnea,EVL_Event))) return false;
                        }
                        EL[1]->AddEvent(tt,duration+10); // Only Hyponea's Need the extra duration???
                    } else if (t=="apnea") {
                        if (!EL[2]) {
                            if (!(EL[2]=sess->AddEventList(CPAP_Apnea,EVL_Event))) return false;
                        }
                        EL[2]->AddEvent(tt,duration);
                    } else if (t=="central apnea") {
                        code=CPAP_ClearAirway;
                        if (!EL[3]) {
                            if (!(EL[3]=sess->AddEventList(CPAP_ClearAirway,EVL_Event))) return false;
                        }
                        EL[3]->AddEvent(tt,duration);
                    } else {
                        if (t!="recording starts") {
                            qDebug() << "Unobserved ResMed annotation field: " << t;
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
        sess->updateLast(tt);
       // qDebug(data);
    }
    return true;
}
bool ResmedLoader::LoadBRP(Session *sess,EDFParser &edf)
{
    QString t;
    sess->updateFirst(edf.startdate);
    qint64 duration=edf.GetNumDataRecords()*edf.GetDuration();
    sess->updateLast(edf.startdate+duration);

    for (int s=0;s<edf.GetNumSignals();s++) {
        EDFSignal & es=*edf.edfsignals[s];
        //qDebug() << "BRP:" << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum;
        long recs=edf.edfsignals[s]->nr*edf.GetNumDataRecords();
        ChannelID code;
        if (edf.edfsignals[s]->label=="Flow") {
            es.gain*=60;
            es.physical_dimension="L/M";
            code=CPAP_FlowRate;
        } else if (edf.edfsignals[s]->label.startsWith("Mask Pres")) {
            code=CPAP_MaskPressureHi;
        } else if (es.label.startsWith("Resp Event")) {
            code=CPAP_RespEvent;
        } else {
            qDebug() << "Unobserved ResMed BRP Signal " << edf.edfsignals[s]->label;
            continue;
        }
        double rate=double(duration)/double(recs);
        EventList *a=sess->AddEventList(code,EVL_Waveform,es.gain,es.offset,0,0,rate);
        a->setDimension(es.physical_dimension);
        a->AddWaveform(edf.startdate,es.data,recs,duration);
        sess->setMin(code,a->min());
        sess->setMax(code,a->max());
        //delete edf.edfsignals[s]->data;
        //edf.edfsignals[s]->data=NULL; // so it doesn't get deleted when edf gets trashed.
    }
    return true;
}
EventList * ResmedLoader::ToTimeDelta(Session *sess,EDFParser &edf, EDFSignal & es, ChannelID code, long recs, qint64 duration,EventDataType min,EventDataType max)
{
    bool first=true;
    double rate=(duration/recs); // milliseconds per record
    double tt=edf.startdate;
    //sess->UpdateFirst(tt);
    EventDataType c,last;

    EventList *el=sess->AddEventList(code,EVL_Event,es.gain,es.offset,min,max);
    int startpos=0;

    if ((code==CPAP_Pressure) || (code==CPAP_IPAP) || (code==CPAP_EPAP)) {
        startpos=20; // Shave the first 20 seconds of pressure data
        tt+=rate*startpos;
    }
    for (int i=startpos;i<recs;i++) {
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
    sess->updateLast(tt);
    return el;
}
bool ResmedLoader::LoadSAD(Session *sess,EDFParser &edf)
{
    QString t;
    sess->updateFirst(edf.startdate);
    qint64 duration=edf.GetNumDataRecords()*edf.GetDuration();
    sess->updateLast(edf.startdate+duration);

    for (int s=0;s<edf.GetNumSignals();s++) {
        EDFSignal & es=*edf.edfsignals[s];
        //qDebug() << "SAD:" << es.label << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum;
        long recs=edf.edfsignals[s]->nr*edf.GetNumDataRecords();
        ChannelID code;
        if (edf.edfsignals[s]->label=="Pulse") {
            code=OXI_Pulse;
        } else if (edf.edfsignals[s]->label=="SpO2") {
            code=OXI_SPO2;
        } else {
            qDebug() << "Unobserved ResMed SAD Signal " << edf.edfsignals[s]->label;
            continue;
        }
        bool hasdata=false;
        for (int i=0;i<recs;i++) {
            if (es.data[i]!=-1) {
                hasdata=true;
                break;
            }
        }
        if (hasdata) {
            EventList *a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
            if (a) {
                sess->setMin(code,a->min());
                sess->setMax(code,a->max());
            }
        }

    }
    return true;
}


bool ResmedLoader::LoadPLD(Session *sess,EDFParser &edf)
{
    // Is it save to assume the order does not change here?
    enum PLDType { MaskPres=0, TherapyPres, ExpPress, Leak, RR, Vt, Mv, SnoreIndex, FFLIndex, U1, U2 };

    qint64 duration=edf.GetNumDataRecords()*edf.GetDuration();
    sess->updateFirst(edf.startdate);
    sess->updateLast(edf.startdate+duration);
    QString t;
    int emptycnt=0;
    EventList *a;
    double rate;
    long recs;
    ChannelID code;
    for (int s=0;s<edf.GetNumSignals();s++) {
        EDFSignal & es=*edf.edfsignals[s];
        recs=es.nr*edf.GetNumDataRecords();
        if (recs<=0) continue;
        rate=double(duration)/double(recs);
        //qDebug() << "EVE:" << es.digital_maximum << es.digital_minimum << es.physical_maximum << es.physical_minimum << es.gain;
        if (es.label=="Snore Index") {
            code=CPAP_Snore;
            a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="Therapy Pres") {
            code=CPAP_Pressure; //TherapyPressure;
            a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="Insp Pressure") {
            code=CPAP_IPAP; //TherapyPressure;
            sess->settings[CPAP_Mode]=MODE_BIPAP;
            a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="MV") {
            code=CPAP_MinuteVent;
            a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="RR") {
            code=CPAP_RespRate;
            a=sess->AddEventList(code,EVL_Waveform,es.gain,es.offset,0,0,rate);
            a->AddWaveform(edf.startdate,es.data,recs,duration);
        } else if (es.label=="Vt") {
            code=CPAP_TidalVolume;
            es.physical_maximum=es.physical_minimum=0;
            es.gain*=1000.0;
            a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="Leak") {
            code=CPAP_Leak;
            es.gain*=60;
            es.physical_dimension="L/M";
            a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="FFL Index") {
            code=CPAP_FLG;
            a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label.startsWith("Mask Pres")) {
            code=CPAP_MaskPressure;
            a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label.startsWith("Exp Press")) {
            code=CPAP_EPAP;//ExpiratoryPressure;
            a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label.startsWith("I:E")) {
            code=CPAP_IE;//I:E;
            a=sess->AddEventList(code,EVL_Waveform,es.gain,es.offset,0,0,rate);
            a->AddWaveform(edf.startdate,es.data,recs,duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label.startsWith("Ti")) {
            code=CPAP_Ti;//Ti;
            a=sess->AddEventList(code,EVL_Waveform,es.gain,es.offset,0,0,rate);
            a->AddWaveform(edf.startdate,es.data,recs,duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label.startsWith("Te")) {
            code=CPAP_Te;//Te;
            a=sess->AddEventList(code,EVL_Waveform,es.gain,es.offset,0,0,rate);
            a->AddWaveform(edf.startdate,es.data,recs,duration);
            //a=ToTimeDelta(sess,edf,es, code,recs,duration,0,0);
        } else if (es.label=="") {
            if (emptycnt==0) {
                code=RMS9_E01;
                a=ToTimeDelta(sess,edf,es, code,recs,duration);
            } else if (emptycnt==1) {
                code=RMS9_E02;
                a=ToTimeDelta(sess,edf,es, code,recs,duration);
            } else {
                qDebug() << "Unobserved Empty Signal " << es.label;
            }
            emptycnt++;
        } else {
            qDebug() << "Unobserved ResMed PLD Signal " << es.label;
            a=NULL;
        }
        if (a) {
            sess->setMin(code,a->min());
            sess->setMax(code,a->max());
            a->setDimension(es.physical_dimension);
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

    resmed_codes[CPAP_FlowRate].push_back("Flow");
    resmed_codes[CPAP_MaskPressureHi].push_back("Mask Pres");
    resmed_codes[CPAP_MaskPressureHi].push_back("Mask Pressure"); // vpap
    resmed_codes[CPAP_RespEvent].push_back("Resp Event");

    resmed_codes[CPAP_MaskPressure].push_back("Mask Pres");
    resmed_codes[CPAP_MaskPressure].push_back("Mask Pressure"); // vpap

    resmed_codes[CPAP_Pressure].push_back("Therapy Pres"); // not on vpap
    resmed_codes[CPAP_IPAP].push_back("Insp Pressure"); // on vpap

    resmed_codes[CPAP_EPAP].push_back("Exp Press");
    resmed_codes[CPAP_EPAP].push_back("Exp Pressure"); // vpap
    resmed_codes[CPAP_Leak].push_back("Leak");
    resmed_codes[CPAP_RespRate].push_back("RR");
    resmed_codes[CPAP_TidalVolume].push_back("Vt");
    resmed_codes[CPAP_MinuteVent].push_back("MV");
    resmed_codes[CPAP_IE].push_back("I:E"); // vpap
    resmed_codes[CPAP_Snore].push_back("Snore Index");
    resmed_codes[CPAP_FLG].push_back("FFL Index");

    resmed_codes[CPAP_RespEvent].push_back("RE");
    resmed_codes[CPAP_Ti].push_back("Ti");
    resmed_codes[CPAP_Te].push_back("Te");

    // Sad (oximetry)
    resmed_codes[OXI_Pulse].push_back("Pulse");
    resmed_codes[OXI_SPO2].push_back("SpO2");

    // Event annotations
    resmed_codes[CPAP_Obstructive].push_back("Obstructive apnea");
    resmed_codes[CPAP_Hypopnea].push_back("Hypopnea");
    resmed_codes[CPAP_Apnea].push_back("Apnea");
    resmed_codes[CPAP_ClearAirway].push_back("Central apnea");

    // STR.edf
}


bool resmed_initialized=false;
void ResmedLoader::Register()
{
    if (resmed_initialized) return;
    qDebug() << "Registering ResmedLoader";
    RegisterLoader(new ResmedLoader());
    ResInitModelMap();
    resmed_initialized=true;
}

