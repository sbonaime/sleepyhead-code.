/*

SleepLib Fisher & Paykel Icon Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#include <QDir>
#include <QProgressBar>
#include <QMessageBox>
#include <QDataStream>

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
    epoch=QDateTime(QDate(1970,1,1),QTime(0,0,0)).secsTo(QDateTime(QDate(1995,1,1),QTime(0,0,0)));
    m_buffer=NULL;
}

FPIconLoader::~FPIconLoader()
{
    for (QHash<QString,Machine *>::iterator i=MachList.begin(); i!=MachList.end(); i++) {
        delete i.value();
    }
}

int FPIconLoader::Open(QString & path,Profile *profile)
{
    // Check for SL directory
    // Check for DV5MFirm.bin?
    QString newpath;

    if (path.endsWith("/"))
        path.chop(1);

    QString dirtag="FPHCARE";
    if (path.endsWith(QDir::separator()+dirtag)) {
        newpath=path;
    } else {
        newpath=path+QDir::separator()+dirtag;
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
    return true;
}

bool FPIconLoader::OpenFLW(Machine * mach,QString filename, Profile * profile)
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

    QByteArray data;
    data=file.readAll();
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 t1,t2;
    quint32 ts;
    qint64 ti;
    qint8 b;
    //QByteArray line;

    char * buf=data.data()+4;

    EventList * flow=NULL;
    qint16 dat[0x34];
    EventDataType rate=200;
    do {
        in >> t1;
        in >> t2;

        ts=(t1*86400)+(t2*1.5);

        ts+=epoch;

        if (!Sessions.contains(ts)) {
            // Skip until ends in 0xFF FF FF 7F
            // skip 0
            break;
        }
        Session *sess=Sessions[ts];

        flow=sess->AddEventList(CPAP_FlowRate,EVL_Waveform,1.0,0,0,0,rate);

        ti=qint64(ts)*1000L;
        int i;
        do {
            in >> t1;
            for (i=0;i<0x34;i++) {
                if (t1==0xffff)
                    break;
                dat[i]=t1;
                in >> t1;
            }
            flow->AddWaveform(ti,dat,i,i*rate);
        } while ((t1!=0xff7f) && !in.atEnd());

        if (in.atEnd()) break;

        do {
            in >> b;
        } while (!b);

        t1=b << 8;
        in >> b;
        t1|=b;
    } while (!in.atEnd());

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

    QByteArray data;
    data=file.readAll();
    long size=data.size(),pos=0;
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_6);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 t1,t2;
    quint32 ts;
    //QByteArray line;
    unsigned char a1,a2, a3,a4, a5, p1, p2,  p3, p4, p5, j1, j2, j3 ,j4,j5,j6,j7, x1, x2;

    quint16 d1,d2,d3;


    QDateTime datetime;
    QDate date;
    QTime time;

    do {
        in >> t1;
        in >> t2;

        ts=(t1*86400)+(t2*1.5);

        ts+=epoch;
        //ts*=1;
        datetime=QDateTime::fromTime_t(ts);
        date=datetime.date();
        time=datetime.time();


        // the following two quite often match in value
        in >> a1;  // 0x04
        in >> a2;  // 0x05

        in >> a3;  // 0x06
        in >> a4;  // 0x07 // pressure value?
        in >> a5;  // 0x08

        in >> d1;  // 0x09
        in >> d2;  // 0x0b
        in >> d3;  // 0x0d   // offset 0x0d is always more than offset 0x08

        in >> p1;  // 0x0f
        in >> p2;  // 0x10

        in >> j1;  // 0x11
        in >> j2;  // 0x12
        in >> j3;  // 0x13
        in >> j4;  // 0x14
        in >> j5;  // 0x15
        in >> j6;  // 0x16
        in >> j7;  // 0x17

        in >> p3;  // 0x18
        in >> p4;  // 0x19
        in >> p5;  // 0x1a

        in >> x1;  // 0x1b
        in >> x2;  // 0x1c

        if (!mach->SessionExists(ts)) {
            Session *sess=new Session(mach,ts);
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
            Sessions[ts]=sess;
        }
    } while (!in.atEnd());

    return true;
}

bool FPIconLoader::OpenDetail(Machine * mach, QString filename, Profile * profile)
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
    unsigned char hsum=0;
    for (int i=0;i<0x1ff;i++) {
        hsum+=header[i];
    }
    if (hsum!=header[0x1ff]) {
        qDebug() << "Header checksum mismatch" << filename;
    }

    QByteArray index;
    index=file.read(0x800);
    long size=index.size(),pos=0;
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

    quint16 t1,t2;
    quint16 strt;
    quint8 recs;

    int totalrecs=0;
    do {
        in >> t1; // 0x00    day?
        in >> t2; // 0x02    time?
        if (t1==0xffff) break;

        //ts = (t1 << 16) + t2;
        ts=(t1*86400)+(t2*1.5);

        ts+=epoch;


        datetime=QDateTime::fromTime_t(ts);
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

        idx=stidx*5;
        for (int i=0;i<rec;i++) {
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
            ti+=300000L;
            idx+=5;
        }
        sess->really_set_last(ti-300000L);
        sess->SetChanged(true);
        mach->AddSession(sess,profile);
    }
    mach->Save();

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
