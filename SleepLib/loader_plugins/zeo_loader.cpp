/*
SleepLib ZEO Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

//********************************************************************************************
// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the zeo_data_version in zel_loader.h when making changes to this loader
// that change loader behaviour or modify channels.
//********************************************************************************************


#include <QDir>
#include <QTextStream>
#include "zeo_loader.h"
#include "SleepLib/machine.h"

ZEOLoader::ZEOLoader()
{
    //ctor
}

ZEOLoader::~ZEOLoader()
{
    //dtor
}
int ZEOLoader::Open(QString & path,Profile *profile)
{
    Q_UNUSED(path)
    Q_UNUSED(profile)

    QString newpath;

    QString dirtag="zeo";

    // Could Scan the ZEO folder for a list of CSVs

    if (path.toLower().endsWith(QDir::separator()+dirtag)) {
        return 0;
        //newpath=path;
    } else {
        newpath=path+QDir::separator()+dirtag.toUpper();
    }

    QString filename;

    // ZEO folder structure detection stuff here.

    return 0; // number of machines affected
}
Machine *ZEOLoader::CreateMachine(Profile *profile)
{
    if (!profile)
        return NULL;

    // NOTE: This only allows for one ZEO machine per profile..
    // Upgrading their ZEO will use this same record..

    QList<Machine *> ml=profile->GetMachines(MT_SLEEPSTAGE);

    for (QList<Machine *>::iterator i=ml.begin(); i!=ml.end(); i++) {
        if ((*i)->GetClass()==zeo_class_name)  {
            return (*i);
            break;
        }
    }

    qDebug("Create ZEO Machine Record");

    Machine *m=new SleepStage(profile,0);
    m->SetType(MT_SLEEPSTAGE);
    m->SetClass(zeo_class_name);
    m->properties[STR_PROP_Brand]="ZEO";
    m->properties[STR_PROP_Model]="Personal Sleep Coach";
    m->properties[STR_PROP_DataVersion]=QString::number(zeo_data_version);

    profile->AddMachine(m);

    QString path="{"+STR_GEN_DataFolder+"}/"+m->GetClass()+"_"+m->hexid()+"/";
    m->properties[STR_PROP_Path]=path;
    m->properties[STR_PROP_BackupPath]=path+"Backup/";

    return m;
}
/*15233: "Sleep Date"
15234: "ZQ"
15236: "Total Z"
15237: "Time to Z"
15237: "Time in Wake"
15238: "Time in REM"
15238: "Time in Light"
15241: "Time in Deep"
15242: "Awakenings"
15245: "Start of Night"
15246: "End of Night"
15246: "Rise Time"
15247: "Alarm Reason"
15247: "Snooze Time"
15254: "Wake Tone"
15259: "Wake Window"
15259: "Alarm Type"
15260: "First Alarm Ring"
15261: "Last Alarm Ring"
15261: "First Snooze Time"
15265: "Last Snooze Time"
15266: "Set Alarm Time"
15266: "Morning Feel"
15267: "Sleep Graph"
15267: "Detailed Sleep Graph"
15268: "Firmware Version"  */

int ZEOLoader::OpenFile(QString filename)
{
    QFile file(filename);
    if (filename.toLower().endsWith(".csv")) {
        if (!file.open(QFile::ReadOnly)) {
            qDebug() << "Couldn't open zeo file" << filename;
            return 0;
        }
    } else {// if (filename.toLower().endsWith(".dat")) {

        return 0;
        // not supported.
    }
    QTextStream text(&file);
    QString headerdata=text.readLine();
    QStringList header=headerdata.split(",");
    QString line;
    QStringList linecomp;
    QDateTime start_of_night, end_of_night, rise_time;
    SessionID sid;

    const qint64 WindowSize=30000;
    qint64 st,tt;
    int stage;

    int ZQ, TotalZ, TimeToZ, TimeInWake, TimeInREM, TimeInLight, TimeInDeep, Awakenings;
    int AlarmReason, SnoozeTime, WakeTone, WakeWindow, AlarmType, MorningFeel, FirmwareVersion;

    QDateTime FirstAlarmRing, LastAlarmRing, FirstSnoozeTime, LastSnoozeTime, SetAlarmTime;

    QStringList SG, DSG;

    Machine *mach=CreateMachine(p_profile);


    bool ok;
    bool dodgy;
    do {
        line=text.readLine();
        dodgy=false;
        if (line.isEmpty()) continue;
        linecomp=line.split(",");
        ZQ=linecomp[1].toInt(&ok);
        if (!ok) dodgy=true;
        TotalZ=linecomp[2].toInt(&ok);
        if (!ok) dodgy=true;
        TimeToZ=linecomp[3].toInt(&ok);
        if (!ok) dodgy=true;
        TimeInWake=linecomp[4].toInt(&ok);
        if (!ok) dodgy=true;
        TimeInREM=linecomp[5].toInt(&ok);
        if (!ok) dodgy=true;
        TimeInLight=linecomp[6].toInt(&ok);
        if (!ok) dodgy=true;
        TimeInDeep=linecomp[7].toInt(&ok);
        if (!ok) dodgy=true;
        Awakenings=linecomp[8].toInt(&ok);
        if (!ok) dodgy=true;
        start_of_night=QDateTime::fromString(linecomp[9],"MM/dd/yyyy HH:mm");
        if (!start_of_night.isValid()) dodgy=true;
        end_of_night=QDateTime::fromString(linecomp[10],"MM/dd/yyyy HH:mm");
        if (!end_of_night.isValid()) dodgy=true;
        rise_time=QDateTime::fromString(linecomp[11],"MM/dd/yyyy HH:mm");
        if (!rise_time.isValid()) dodgy=true;

        AlarmReason=linecomp[12].toInt(&ok);
        if (!ok) dodgy=true;
        SnoozeTime=linecomp[13].toInt(&ok);
        if (!ok) dodgy=true;
        WakeTone=linecomp[14].toInt(&ok);
        if (!ok) dodgy=true;
        WakeWindow=linecomp[15].toInt(&ok);
        if (!ok) dodgy=true;
        AlarmType=linecomp[16].toInt(&ok);
        if (!ok) dodgy=true;

        if (!linecomp[17].isEmpty()) {
            FirstAlarmRing=QDateTime::fromString(linecomp[9],"MM/dd/yyyy HH:mm");
            if (!FirstAlarmRing.isValid()) dodgy=true;
        }
        if (!linecomp[18].isEmpty()) {
            LastAlarmRing=QDateTime::fromString(linecomp[9],"MM/dd/yyyy HH:mm");
            if (!LastAlarmRing.isValid()) dodgy=true;
        }
        if (!linecomp[19].isEmpty()) {
            FirstSnoozeTime=QDateTime::fromString(linecomp[9],"MM/dd/yyyy HH:mm");
            if (!FirstSnoozeTime.isValid()) dodgy=true;
        }
        if (!linecomp[20].isEmpty()) {
            LastSnoozeTime=QDateTime::fromString(linecomp[9],"MM/dd/yyyy HH:mm");
            if (!LastSnoozeTime.isValid()) dodgy=true;
        }
        if (!linecomp[21].isEmpty()) {
            SetAlarmTime=QDateTime::fromString(linecomp[9],"MM/dd/yyyy HH:mm");
            if (!SetAlarmTime.isValid()) dodgy=true;
        }
        MorningFeel=linecomp[22].toInt(&ok);
        if (!ok) dodgy=true;

        FirmwareVersion=linecomp[25].toInt(&ok);
        if (!ok) dodgy=true;

        if (dodgy)
            continue;
        SG=linecomp[23].split(" ");
        DSG=linecomp[24].split(" ");

        const int WindowSize=30000;
        sid=start_of_night.toTime_t();
        if (DSG.size()==0)
            continue;
        if (mach->SessionExists(sid))
            continue;
        Session *sess=new Session(mach,sid);

        sess->settings[ZEO_Awakenings]=Awakenings;
        sess->settings[ZEO_MorningFeel]=MorningFeel;
        sess->settings[ZEO_TimeToZ]=TimeToZ;
        sess->settings[ZEO_ZQ]=ZQ;
        sess->settings[ZEO_TimeInWake]=TimeInWake;
        sess->settings[ZEO_TimeInREM]=TimeInREM;
        sess->settings[ZEO_TimeInLight]=TimeInLight;
        sess->settings[ZEO_TimeInDeep]=TimeInDeep;

        st=qint64(start_of_night.toTime_t()) * 1000L;
        sess->really_set_first(st);
        tt=st;
        EventList * sleepstage=sess->AddEventList(ZEO_SleepStage,EVL_Event,1,0,0,4);
        for (int i=0;i<DSG.size();i++) {
            stage=DSG[i].toInt(&ok);
            if (ok) {
                sleepstage->AddEvent(tt,stage);
            }
            tt+=WindowSize;
        }
        sess->really_set_last(tt);
        int size=DSG.size();
        sess->SetChanged(true);
        mach->AddSession(sess,p_profile);


        qDebug() << linecomp[0] << start_of_night << end_of_night << rise_time << size << "30 second chunks";

    } while (!line.isNull());
    mach->Save();
    return true;
}


static bool zeo_initialized=false;

void ZEOLoader::Register()
{
    if (zeo_initialized) return;
    qDebug("Registering ZEOLoader");
    RegisterLoader(new ZEOLoader());
    //InitModelMap();
    zeo_initialized=true;
}

