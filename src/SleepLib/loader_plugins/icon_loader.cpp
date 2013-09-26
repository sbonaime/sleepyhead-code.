/*

SleepLib Fisher & Paykel Icon Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
Copyright: (c)2012 Mark Watkins

*/

#include <QDir>
#include <QProgressBar>
#include <QMessageBox>
#include <QDataStream>
#include <QTextStream>
#include <cmath>

#include "icon_loader.h"

extern QProgressBar *qprogress;

FPIcon::FPIcon(Profile *p,MachineID id)
    :CPAP(p,id)
{
    m_class=fpicon_class_name;
    properties[STR_PROP_Brand]="Fisher & Paykel";
    properties[STR_PROP_Model]=STR_MACH_FPIcon;
}

FPIcon::~FPIcon()
{
}

FPIconLoader::FPIconLoader()
{
    m_buffer=NULL;
}

FPIconLoader::~FPIconLoader()
{
}

int FPIconLoader::Open(QString & path,Profile *profile)
{
    QString newpath;

    path=path.replace("\\","/");

    if (path.endsWith("/"))
        path.chop(1);

    QString dirtag="FPHCARE";
    if (path.endsWith("/"+dirtag)) {
        newpath=path;
    } else {
        newpath=path+"/"+dirtag;
    }

    newpath+="/ICON/";

    QString filename;

    QDir dir(newpath);

    if ((!dir.exists() || !dir.isReadable()))
        return 0;

    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist=dir.entryInfoList();

    QStringList SerialNumbers;

    bool ok;
    for (int i=0;i<flist.size();i++) {
        QFileInfo fi=flist.at(i);
        QString filename=fi.fileName();

        filename.toInt(&ok);
        if (ok) {
            SerialNumbers.push_back(filename);
        }
    }

    Machine *m;

    QString npath;
    for (int i=0;i<SerialNumbers.size();i++) {
        QString & sn=SerialNumbers[i];
        m=CreateMachine(sn,profile);

        npath=newpath+"/"+sn;
        try {
            if (m) OpenMachine(m,npath,profile);
        } catch(OneTypePerDay e) {
            profile->DelMachine(m);
            MachList.erase(MachList.find(sn));
            QMessageBox::warning(NULL,"Import Error","This Machine Record cannot be imported in this profile.\nThe Day records overlap with already existing content.",QMessageBox::Ok);
            delete m;
        }
    }
    return MachList.size();
}

struct FPWaveChunk {
    FPWaveChunk(){
        st=0;
        duration=0;
        flow=NULL;
        pressure=NULL;
        leak=NULL;
        file=0;
    }
    FPWaveChunk(qint64 start, qint64 dur,int f) { st=start; duration=dur; file=f, flow=NULL; leak=NULL; pressure=NULL; }
    FPWaveChunk(const FPWaveChunk & copy) {
        st=copy.st;
        duration=copy.duration;
        flow=copy.flow;
        leak=copy.leak;
        pressure=copy.pressure;
        file=copy.file;
    }
    qint64 st;
    qint64 duration;
    int file;
    EventList * flow;
    EventList * leak;
    EventList * pressure;
};

bool operator<(const FPWaveChunk & a, const FPWaveChunk & b) {
    return (a.st < b.st);
}

int FPIconLoader::OpenMachine(Machine *mach, QString & path, Profile * profile)
{
    qDebug() << "Opening FPIcon " << path;
    QDir dir(path);
    if (!dir.exists() || (!dir.isReadable()))
         return false;

    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Name);
    QFileInfoList flist=dir.entryInfoList();

    QString filename,fpath;
    if (qprogress) qprogress->setValue(0);

    QStringList summary, log, flw, det;

    for (int i=0;i<flist.size();i++) {
        QFileInfo fi=flist.at(i);
        filename=fi.fileName();
        fpath=path+"/"+filename;
        if (filename.left(3).toUpper()=="SUM") {
            summary.push_back(fpath);
            OpenSummary(mach,fpath,profile);
        } else if (filename.left(3).toUpper()=="DET") {
            det.push_back(fpath);
        } else if (filename.left(3).toUpper()=="FLW") {
            flw.push_back(fpath);
        } else if (filename.left(3).toUpper()=="LOG") {
            log.push_back(fpath);
        }
    }
    for (int i=0;i<det.size();i++) {
        OpenDetail(mach,det[i],profile);
    }
    for (int i=0;i<flw.size();i++) {
        OpenFLW(mach,flw[i],profile);
    }
    SessionID zz,sid;//,st;
    float hours,dur,mins;

    qDebug() << "Last 20 Sessions";

    int cnt=0;
    QDateTime dt;
    QString a;
    QMap<SessionID, Session *>::iterator it=Sessions.end();
    it--;
    dt=QDateTime::fromTime_t(qint64(it.value()->first())/1000L);
    QDate date=dt.date().addDays(-7);
    it++;

    do {
        it--;
        Session *sess=it.value();
        sid=sess->session();
        hours=sess->hours();
        mins=hours*60;
        dt=QDateTime::fromTime_t(sid);

        if (sess->channelDataExists(CPAP_FlowRate)) a="(flow)"; else a="";
        qDebug() << cnt << ":" << dt << "session" << sid << "," << mins << "minutes" << a;

        if (dt.date()<date) break;
        ++cnt;

    } while (it!=Sessions.begin());


    qDebug() << "Unmatched Sessions";
    QList<FPWaveChunk> chunks;
    for (QMap<int,QDate>::iterator dit=FLWDate.begin();dit!=FLWDate.end();dit++) {
        int k=dit.key();
        //QDate date=dit.value();
//        QList<Session *> values = SessDate.values(date);
        for (int j=0;j<FLWTS[k].size();j++) {

            FPWaveChunk chunk(FLWTS[k].at(j),FLWDuration[k].at(j),k);
            chunk.flow=FLWMapFlow[k].at(j);
            chunk.leak=FLWMapLeak[k].at(j);
            chunk.pressure=FLWMapPres[k].at(j);

            chunks.push_back(chunk);

            zz=FLWTS[k].at(j)/1000;
            dur=double(FLWDuration[k].at(j))/60000.0;
            bool b,c=false;
            if (Sessions.contains(zz)) b=true; else b=false;
            if (b) {
                if (Sessions[zz]->channelDataExists(CPAP_FlowRate)) c=true;
            }
            qDebug() << k << "-" <<j << ":" << zz << qRound(dur) << "minutes" << (b ? "*" : "") << (c ? QDateTime::fromTime_t(zz).toString() : "");
        }
    }
    qSort(chunks);
    bool b,c;
    for (int i=0;i<chunks.size();i++) {
        const FPWaveChunk & chunk=chunks.at(i);
        zz=chunk.st/1000;
        dur=double(chunk.duration)/60000.0;
        if (Sessions.contains(zz)) b=true; else b=false;
        if (b) {
            if (Sessions[zz]->channelDataExists(CPAP_FlowRate)) c=true;
        }
        qDebug() << chunk.file << ":" << i << zz << dur << "minutes" << (b ? "*" : "") << (c ? QDateTime::fromTime_t(zz).toString() : "");
    }

    mach->Save();

    return true;
}


bool FPIconLoader::OpenFLW(Machine * mach,QString filename, Profile * profile)
{
    Q_UNUSED(mach);
    Q_UNUSED(profile);
    QByteArray data;
    quint16 t1;
    quint32 ts;
    double ti;
    //qint8 b;
    EventList * flow=NULL, * pressure=NULL, *leak=NULL;
    QDateTime datetime;
    quint8 a1,a2;
    unsigned char * buf, *endbuf;

    int month,day,year,hour,minute,second;

    long pos=0;

    qDebug() << filename;
    QByteArray header;
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Couldn't open" << filename;
        return false;
    }
    header=file.read(0x200);
    if (header.size()!=0x200) {
        qDebug() << "Short file" << filename;
        return false;
    }
    unsigned char hsum=0xff;
    for (int i=0;i<0x1ff;i++) {
        hsum+=header[i];
    }
    if (hsum!=header[0x1ff]) {
        qDebug() << "Header checksum mismatch" << filename;
    }
    QTextStream htxt(&header);
    QString h1,version,fname,serial,model,type;
    htxt >> h1;
    htxt >> version;
    htxt >> fname;
    htxt >> serial;
    htxt >> model;
    htxt >> type;

    fname.chop(4);
    QString num=fname.right(4);
    int filenum=num.toInt();

    data=file.readAll();

    buf=(unsigned char *)data.data();
    endbuf=buf+data.size();

    a1=*buf++;
    a2=*buf++;
    t1=a2 << 8 | a1;
    pos+=2;

    if (t1==0xfafe)  // End of file marker..
    {
        qDebug() << "FaFE observed in" << filename;

    }

    day=t1 & 0x1f;
    month=((t1 >> 5) & 0x0f);
    year=2000+((t1 >> 9) & 0x7f);

    a1=*buf++;
    a2=*buf++;

    t1=a2 << 8 | a1;

    // Why the heck does F&P do this? This affects the MINUTES field and the HOURS field.
    // But is clearly not a valid UTC conversion.. Bug? Or idiotic obfuscation attempt?
    // It would of made (idiotic) sense if they shifted the bits on bit further to the right.
    t1-=0xc0;

    pos+=2;

    second=(t1 & 0x1f) * 2;
    minute=(t1 >> 5) & 0x3f;
    hour=(t1 >> 11) & 0x1f;

    datetime=QDateTime(QDate(year,month,day),QTime(hour,minute,second),Qt::UTC);

    QDate date;
    QTime time;
    if (!datetime.isValid() || (datetime.date()<QDate(2010,1,1))) {
        pos=0;
        buf-=4;
        datetime=QDateTime(QDate(2000,1,1),QTime(0,0,0));
        date=datetime.date();
        ts=datetime.toTime_t();
    }  else {
        date=datetime.date();
        time=datetime.time();
        ts=datetime.toTime_t();
    }

    FLWDate[filenum]=date;

    ti=qint64(ts)*1000L;
    qint64 st=ti;

    EventStoreType pbuf[256];

    QMap<SessionID, Session *>::iterator sit=Sessions.find(ts);

    Session *sess;

    if (sit!=Sessions.end()) {
        sess=sit.value();
        qDebug() << filenum << ":" << date << sess->session() << ":" << sess->hours()*60.0;
    } else {
        sess=NULL;
        qDebug() << filenum << ":" << date << "couldn't find matching session for" << ts;
    }


    const double rate=1000.0/50.0;


    int count;
    for (int chunks=0;buf<endbuf;++chunks) { // each chunk is a seperate session

        flow=new EventList(EVL_Waveform,1.0,0,0,0,rate);
        leak=new EventList(EVL_Event,1.0,0,0,0,rate*50.0); // 1 per second
        pressure=new EventList(EVL_Event,0.01,0,0,0,rate*50.0); // 1 per second

        flow->setFirst(ti);
        leak->setFirst(ti);
        pressure->setFirst(ti);

        FLWMapFlow[filenum].push_back(flow);
        FLWMapLeak[filenum].push_back(leak);
        FLWMapPres[filenum].push_back(pressure);
        FLWTS[filenum].push_back(ti);

        count=0;
        int len;
        qint16 pr;
        quint16 lkaj;
        do {
            unsigned char * p=buf,*p2;

            // scan ahead to 0xffff marker
            p2=buf+103;
            if (p2>endbuf)
                break;
            if (!((p2[0]==0xff) && (p2[1]==0xff))) {
                if (count>0) {
                    //int i=5;
                }
                do {
                    while ((*p++ != 0xff) && (p < endbuf)) {
                        pos++;
                    }
                    if (p >= endbuf)
                        break;
                        pos++;
                } while ((*p++ != 0xff) && (p < endbuf));
                if (p >= endbuf)
                    break;
            } else {
                //if (count>0)
                p=p2+2;
            }
            p2=p-5;
            len=p2-buf;
            pr=p2[1] << 8 | p2[0]; // pressure * 100
            lkaj=p2[2];  // Could this value perhaps be Leak???
            len/=2;

            count++;

            //if (pr<0) {
                //quint16 z3=pr;
               // int i=5;
            //}

            if (leak) {
                leak->AddEvent(ti,lkaj);
            }
            if (pressure) {
                pressure->AddEvent(ti,pr);
            }

            if (flow) {
                qint16 tmp;
                unsigned char * bb=(unsigned char *)buf;
                //char c;
                //if (len>50) {
                    //int i=5;
                //}

                EventDataType val;
                for (int i=0;i<len;i++) {
                    //c=bb[1];
                    tmp=bb[1] << 8 | bb[0];
                    val=(EventDataType(tmp)/100.0)-lkaj;
                    if (val<-128) val=-128;
                    else if (val>128) val=128;
                    bb+=2;

                    pbuf[i]=val;
                }

                flow->AddWaveform(ti,pbuf,len,rate);
            }

            ti+=len*rate;

            buf=p;

            if (buf >= endbuf-1) break;
            if ((p[0]==0xff) && (p[1]==0x7f)) {
                buf+=2;
                pos+=2;
                while ((*buf++ == 0) && (buf < endbuf)) pos++;
                break;
            }
        } while (buf < endbuf);
//        pressure->setType(EVL_Waveform);
//        pressure->getTime().clear();
//        leak->getTime().clear();
//        leak->setType(EVL_Waveform);

        if (sess && FLWMapFlow[filenum].size()==1 && (st==sess->first())) {
            sess->eventlist[CPAP_FlowRate].push_back(FLWMapFlow[filenum].at(0));
            sess->eventlist[CPAP_Leak].push_back(FLWMapLeak[filenum].at(0));
            sess->eventlist[CPAP_MaskPressure].push_back(FLWMapPres[filenum].at(0));
//            FLWMapFlow[filenum].erase(FLWMapFlow[filenum].begin());
//            FLWMapLeak[filenum].erase(FLWMapLeak[filenum].begin());
//            FLWMapPres[filenum].erase(FLWMapPres[filenum].begin());
        }


        qDebug() << ts << dec << double(ti-st)/60000.0;
        FLWDuration[filenum].push_back(ti-st);
        st=ti;
    } while (buf < endbuf);


    QList<Session *> values = SessDate.values(date);
    for (int i = 0; i < values.size(); ++i) {
        sess=values.at(i);
        qDebug() << date << sess->session() << ":" << QString::number(sess->hours()*60.0,'f',0);
    }
    return true;
}

bool FPIconLoader::OpenSummary(Machine * mach,QString filename, Profile * profile)
{
    qDebug() << filename;
    QByteArray header;
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Couldn't open" << filename;
        return false;
    }
    header=file.read(0x200);
    if (header.size()!=0x200) {
        qDebug() << "Short file" << filename;
        return false;
    }
    unsigned char hsum=0xff;
    for (int i=0;i<0x1ff;i++) {
        hsum+=header[i];
    }
    if (hsum!=header[0x1ff]) {
        qDebug() << "Header checksum mismatch" << filename;
    }
    QTextStream htxt(&header);
    QString h1,version,fname,serial,model,type;
    htxt >> h1;
    htxt >> version;
    htxt >> fname;
    htxt >> serial;
    htxt >> model;
    htxt >> type;
    mach->properties[STR_PROP_Model]=model+" "+type;


    QByteArray data;
    data=file.readAll();
    //long size=data.size(),pos=0;
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 t1;//,t2;
    quint32 ts;
    //QByteArray line;
    unsigned char a1,a2, a3,a4, a5, p1, p2,  p3, p4, p5, j1, j2, j3 ,j4,j5,j6,j7, x1, x2;

    quint16 d1,d2,d3;


    QDateTime datetime;

    int runtime,usage;

    int day,month,year,hour,minute,second;
    QDate date;

    do {
        in >> a1;
        in >> a2;
        t1=a2 << 8 | a1;

        if (t1==0xfafe)
            break;

        day=t1 & 0x1f;
        month=(t1 >> 5) & 0x0f;
        year=2000+((t1 >> 9) & 0x7f);

        in >> a1;
        in >> a2;

        t1=a2 << 8 | a1;

        second=(t1 & 0x1f) * 2;
        minute=(t1 >> 5) & 0x3f;
        hour=(t1 >> 11) & 0x1f;

        datetime=QDateTime(QDate(year,month,day),QTime(hour,minute,second),Qt::UTC);

        date=datetime.date();
        ts=datetime.toTime_t();

        // the following two quite often match in value
        in >> a1;  // 0x04 Run Time
        in >> a2;  // 0x05 Usage Time
        runtime=a1 * 360; // durations are in tenth of an hour intervals
        usage=a2 * 360;

        in >> a3;  // 0x06  // Ramps???
        in >> a4;  // 0x07  // a pressure value?
        in >> a5;  // 0x08  // ?? varies.. always less than 90% leak..

        in >> d1;  // 0x09
        in >> d2;  // 0x0b
        in >> d3;  // 0x0d   // 90% Leak value..

        in >> p1;  // 0x0f
        in >> p2;  // 0x10

        in >> j1;  // 0x11
        in >> j2;  // 0x12  // Apnea Events
        in >> j3;  // 0x13  // Hypopnea events
        in >> j4;  // 0x14  // Flow Limitation events
        in >> j5;  // 0x15
        in >> j6;  // 0x16
        in >> j7;  // 0x17

        in >> p3;  // 0x18
        in >> p4;  // 0x19
        in >> p5;  // 0x1a

        in >> x1;  // 0x1b
        in >> x2;  // 0x1c    // humidifier setting

        if (!mach->SessionExists(ts)) {
            Session *sess=new Session(mach,ts);
            sess->really_set_first(qint64(ts)*1000L);
            sess->really_set_last(qint64(ts+usage)*1000L);
            sess->SetChanged(true);
            sess->setCount(CPAP_Obstructive, j2);
            sess->setCount(CPAP_Hypopnea, j3);
            SessDate.insert(date,sess);
//            sess->setCount(CPAP_Obstructive,j1);
//            sess->setCount(CPAP_Hypopnea,j2);
//            sess->setCount(CPAP_ClearAirway,j3);
//            sess->setCount(CPAP_Apnea,j4);
            //sess->setCount(CPAP_,j5);
            if (p1!=p2) {
                sess->settings[CPAP_Mode]=(int)MODE_APAP;
                sess->settings[CPAP_PressureMin]=p4/10.0;
                sess->settings[CPAP_PressureMax]=p3/10.0;
            } else {
                sess->settings[CPAP_Mode]=(int)MODE_CPAP;
                sess->settings[CPAP_Pressure]=p1/10.0;
            }
            sess->settings[CPAP_HumidSetting]=x2;
            //sess->settings[CPAP_PresReliefType]=PR_SENSAWAKE;
            Sessions[ts]=sess;
            mach->AddSession(sess,profile);
        }
    } while (!in.atEnd());

    return true;
}

bool FPIconLoader::OpenDetail(Machine * mach, QString filename, Profile * profile)
{
    Q_UNUSED(mach);
    Q_UNUSED(profile);

    qDebug() << filename;
    QByteArray header;
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Couldn't open" << filename;
        return false;
    }
    header=file.read(0x200);
    if (header.size()!=0x200) {
        qDebug() << "Short file" << filename;
        return false;
    }
    unsigned char hsum=0;
    for (int i=0;i<0x1ff;i++) {
        hsum+=header[i];
    }
    if (hsum!=header[0x1ff]) {
        qDebug() << "Header checksum mismatch" << filename;
    }

    QByteArray index;
    index=file.read(0x800);
    //long size=index.size(),pos=0;
    QDataStream in(index);

    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);
    quint32 ts;
    QDateTime datetime;
    QDate date;
    QTime time;
    //FPDetIdx *idx=(FPDetIdx *)index.data();


    QVector<quint32> times;
    QVector<quint16> start;
    QVector<quint8> records;

    quint16 t1;
    quint16 strt;
    quint8 recs,z1,z2;


    int day,month,year,hour,minute,second;

    int totalrecs=0;
    do {
        in >> z1;
        in >> z2;
        t1=z2 << 8 | z1;

        if (t1==0xfafe)
            break;

        day=t1 & 0x1f;
        month=(t1 >> 5) & 0x0f;
        year=2000+((t1 >> 9) & 0x7f);

        in >> z1;
        in >> z2;
        t1=z2 << 8 | z1;

        second=(t1 & 0x1f) * 2;
        minute=(t1 >> 5) & 0x3f;
        hour=(t1 >> 11) & 0x1f;

        datetime=QDateTime(QDate(year,month,day),QTime(hour,minute,second),Qt::UTC);
        //datetime=datetime.toTimeSpec(Qt::UTC);

        ts=datetime.toTime_t();

        date=datetime.date();
        time=datetime.time();
        in >> strt;
        in >> recs;
        totalrecs+=recs;
        if (Sessions.contains(ts)) {
            times.push_back(ts);
            start.push_back(strt);
            records.push_back(recs);
        }
    } while (!in.atEnd());

    QByteArray databytes=file.readAll();

    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::BigEndian);
    // 5 byte repeating patterns

    quint8 * data=(quint8 *)databytes.data();

    qint64 ti;
    quint8 pressure,leak, a1,a2,a3;
    SessionID sessid;
    Session *sess;
    int idx;
    for (int r=0;r<start.size();r++) {
        sessid=times[r];
        sess=Sessions[sessid];
        ti=qint64(sessid)*1000L;
        sess->really_set_first(ti);
        EventList * LK=sess->AddEventList(CPAP_LeakTotal,EVL_Event,1);
        EventList * PR=sess->AddEventList(CPAP_Pressure,EVL_Event,0.1);
        EventList * FLG=sess->AddEventList(CPAP_FLG,EVL_Event);
        EventList * OA=sess->AddEventList(CPAP_Obstructive,EVL_Event);
        EventList * H=sess->AddEventList(CPAP_Hypopnea,EVL_Event);
        EventList * FL=sess->AddEventList(CPAP_FlowLimit,EVL_Event);

        unsigned stidx=start[r];
        int rec=records[r];

        idx=stidx*15;
        for (int i=0;i<rec;i++) {
            for (int j=0;j<3;j++) {
                pressure=data[idx];
                leak=data[idx+1];
                a1=data[idx+2];
                a2=data[idx+3];
                a3=data[idx+4];
                PR->AddEvent(ti,pressure);
                LK->AddEvent(ti,leak);
                if (a1>0) OA->AddEvent(ti,a1);
                if (a2>0) H->AddEvent(ti,a2);
                if (a3>0) FL->AddEvent(ti,a3);
                FLG->AddEvent(ti,a3);
                ti+=120000L;
                idx+=5;
            }
        }
      //  sess->really_set_last(ti-360000L);
//        sess->SetChanged(true);
 //       mach->AddSession(sess,profile);
    }

    return 1;
}


Machine *FPIconLoader::CreateMachine(QString serial,Profile *profile)
{
    if (!profile)
        return NULL;
    qDebug() << "Create Machine " << serial;

    QList<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    QList<Machine *>::iterator i;
    for (i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()==fpicon_class_name) && ((*i)->properties[STR_PROP_Serial]==serial)) {
            MachList[serial]=*i;
            found=true;
            break;
        }
    }
    if (found) return *i;

    Machine *m=new FPIcon(profile,0);

    MachList[serial]=m;
    profile->AddMachine(m);

    m->properties[STR_PROP_Serial]=serial;
    m->properties[STR_PROP_DataVersion]=QString::number(fpicon_data_version);

    QString path="{"+STR_GEN_DataFolder+"}/"+m->GetClass()+"_"+serial+"/";
    m->properties[STR_PROP_Path]=path;
    m->properties[STR_PROP_BackupPath]=path+"Backup/";

    return m;
}

bool fpicon_initialized=false;
void FPIconLoader::Register()
{
    if (fpicon_initialized) return;
    qDebug() << "Registering F&P Icon Loader";
    RegisterLoader(new FPIconLoader());
    //InitModelMap();
    fpicon_initialized=true;
}
