/* SleepLib RESMED Loader Header
 *
 * Copyright (C) 2011-2018 Mark Watkins <mark@jedimark.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef RESMED_LOADER_H
#define RESMED_LOADER_H
//#include <map>
//using namespace std;
#include <QVector>
#include "SleepLib/machine.h" // Base class: MachineLoader
#include "SleepLib/machine_loader.h"
#include "SleepLib/profiles.h"
#include "SleepLib/loader_plugins/edfparser.h"

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int resmed_data_version = 11;
//
//********************************************************************************************

enum EDFType { EDF_UNKNOWN, EDF_BRP, EDF_PLD, EDF_SAD, EDF_EVE, EDF_CSL };

EDFType lookupEDFType(QString text);

const QString resmed_class_name = STR_MACH_ResMed;

class ResMedEDFParser:public EDFParser
{
public:
    ResMedEDFParser(QString filename = "");
    ~ResMedEDFParser();
    EDFSignal *lookupSignal(ChannelID ch);

};

struct STRRecord
{
    STRRecord() {
        maskon = 0;
        maskoff = 0;
        maskdur = 0;
        maskevents = -1;
        mode = -1;
        rms9_mode = -1;
        set_pressure = -1;
        epap = -1;
        max_pressure = -1;
        min_pressure = -1;
        max_epap = -1;
        min_epap = -1;
        max_ps = -1;
        min_ps = -1;
        ps = -1;
        ipap = -1;
        max_ipap = -1;
        min_ipap = -1;
        epr = -1;
        epr_level = -1;
        sessionid = 0;

        ahi = -1;
        ai = -1;
        hi = -1;
        uai = -1;
        cai = -1;

        leakmed = -1;
        leak95 = -1;
        leakmax = -1;
        leakgain = 0;

        s_RampTime = -1;
        s_RampEnable = -1;
        s_EPR_ClinEnable = -1;
        s_EPREnable = -1;

        s_PtAccess = -1;
        s_ABFilter = -1;
        s_Mask = -1;
        s_Tube = -1;
        s_ClimateControl = -1;
        s_HumEnable = -1;
        s_HumLevel = -1;
        s_TempEnable = -1;
        s_Temp = -1;
        s_SmartStart = -1;

        ramp_pressure = -1;



        date=QDate();
    }
    STRRecord(const STRRecord & copy) {
        maskon = copy.maskon;
        maskoff = copy.maskoff;
        maskdur = copy.maskdur;
        maskevents = copy.maskevents;
        mode = copy.mode;
        rms9_mode = copy.rms9_mode;
        set_pressure = copy.set_pressure;
        epap = copy.epap;
        max_pressure = copy.max_pressure;
        min_pressure = copy.min_pressure;
        max_ps = copy.max_ps;
        min_ps = copy.min_ps;
        ps = copy.ps;
        max_epap = copy.max_epap;
        min_epap = copy.min_epap;
        ipap = copy.ipap;
        max_ipap = copy.max_ipap;
        min_ipap = copy.min_ipap;
        epr = copy.epr;
        epr_level = copy.epr_level;
        sessionid = copy.sessionid;
        ahi = copy.ahi;
        ai = copy.ai;
        hi = copy.hi;
        uai = copy.uai;
        cai = copy.cai;
        date = copy.date;
        leakmed = copy.leakmed;
        leak95 = copy.leak95;
        leakmax = copy.leakmax;
        leakgain = copy.leakgain;

        s_SmartStart = copy.s_SmartStart;
        s_PtAccess = copy.s_PtAccess;
        s_ABFilter = copy.s_ABFilter;
        s_Mask = copy.s_Mask;

        s_Tube = copy.s_Tube;
        s_ClimateControl = copy.s_ClimateControl;
        s_HumEnable = copy.s_HumEnable;
        s_HumLevel = copy.s_HumLevel;
        s_TempEnable = copy.s_TempEnable;
        s_Temp = copy.s_Temp;
        ramp_pressure = copy.ramp_pressure;
    }
    quint32 maskon;
    quint32 maskoff;
    EventDataType maskdur;
    EventDataType maskevents;
    EventDataType mode;
    EventDataType rms9_mode;
    EventDataType set_pressure;
    EventDataType max_pressure;
    EventDataType min_pressure;
    EventDataType epap;
    EventDataType max_ps;
    EventDataType min_ps;
    EventDataType ps;
    EventDataType max_epap;
    EventDataType min_epap;
    EventDataType ipap;
    EventDataType max_ipap;
    EventDataType min_ipap;
    EventDataType epr;
    EventDataType epr_level;
    quint32 sessionid;
    EventDataType ahi;
    EventDataType ai;
    EventDataType hi;
    EventDataType uai;
    EventDataType cai;
    EventDataType leakmed;
    EventDataType leak95;
    EventDataType leakmax;
    EventDataType leakgain;
    EventDataType ramp_pressure;
    QDate date;

    EventDataType s_RampTime;
    int s_RampEnable;
    int s_EPR_ClinEnable;
    int s_EPREnable;

    int s_PtAccess;
    int s_ABFilter;
    int s_Mask;
    int s_Tube;
    int s_ClimateControl;
    int s_HumEnable;
    EventDataType s_HumLevel;
    int s_TempEnable;
    EventDataType s_Temp;
    int s_SmartStart;

};



class ResmedLoader;

struct EDFGroup {
    EDFGroup() { }
    EDFGroup(QString brp, QString eve, QString pld, QString sad, QString csl) {
        BRP = brp;
        EVE = eve;
        CSL = csl;
        PLD = pld;
        SAD = sad;
    }
    EDFGroup(const EDFGroup & copy) {
        BRP = copy.BRP;
        EVE = copy.EVE;
        CSL = copy.CSL;
        PLD = copy.PLD;
        SAD = copy.SAD;
    }
    QString BRP;
    QString EVE;
    QString CSL;
    QString PLD;
    QString SAD;
};

class ResmedImport:public ImportTask
{
public:
    ResmedImport(ResmedLoader * l, SessionID s, QHash<EDFType, QStringList> grp, Machine * m): loader(l), sessionid(s), files(grp), mach(m) {}
    virtual ~ResmedImport() {}
    virtual void run();

protected:
    ResmedLoader * loader;
    SessionID sessionid;
    QHash<EDFType, QStringList> files;
    Machine * mach;
};

class ResmedImportStage2:public ImportTask
{
public:
    ResmedImportStage2(ResmedLoader * l, STRRecord r, Machine * m): loader(l), R(r), mach(m) {}
    virtual ~ResmedImportStage2() {}
    virtual void run();

protected:
    ResmedLoader * loader;
    STRRecord R;
    Machine * mach;
};


/*! \class ResmedLoader
    \brief Importer for ResMed S9 Data
    */
class ResmedLoader : public CPAPLoader
{
    friend class ResmedImport;
    friend class ResmedImportStage2;
  public:
    ResmedLoader();
    virtual ~ResmedLoader();

    //! \brief Detect if the given path contains a valid Folder structure
    virtual bool Detect(const QString & path);


    //! \brief Look up machine model information of ResMed file structure stored at path
    virtual MachineInfo PeekInfo(const QString & path);


    //! \brief Scans for S9 SD folder structure signature, and loads any new data if found
    virtual int Open(QString path);

    //! \brief Returns the version number of this ResMed loader
    virtual int Version() { return resmed_data_version; }

    //! \brief Returns the Machine class name of this loader. ("ResMed")
    virtual const QString &loaderName() { return resmed_class_name; }

    //! \brief Converts EDFSignal data to time delta packed EventList, and adds to Session
    void ToTimeDelta(Session *sess, ResMedEDFParser &edf, EDFSignal &es, ChannelID code, long recs,
                     qint64 duration, EventDataType min = 0, EventDataType max = 0, bool square = false);

    //! \brief Register the ResmedLoader with the list of other machine loaders
    static void Register();

    //! \brief Parse the EVE Event annotation data, and save to Session * sess
    //! This contains all Hypopnea, Obstructive Apnea, Central and Apnea codes
    bool LoadEVE(Session *sess, const QString & path);

    //! \brief Parse the CSL Event annotation data, and save to Session * sess
    //! This contains Cheyne Stokes Respiration flagging on the AirSense 10
    bool LoadCSL(Session *sess, const QString & path);

    //! \brief Parse the BRP High Resolution data, and save to Session * sess
    //! This contains Flow Rate, Mask Pressure, and Resp. Event  data
    bool LoadBRP(Session *sess, const QString & path);

    //! \brief Parse the SAD Pulse oximetry attachment data, and save to Session * sess
    //! This contains Pulse Rate and SpO2 Oxygen saturation data
    bool LoadSAD(Session *sess, const QString & path);

    //! \brief Parse the PRD low resolution data, and save to Session * sess
    //! This contains the Pressure, Leak, Respiratory Rate, Minute Ventilation, Tidal Volume, etc..
    bool LoadPLD(Session *sess, const QString & path);

    virtual MachineInfo newInfo() {
        return MachineInfo(MT_CPAP, 0, resmed_class_name, QObject::tr("ResMed"), QString(), QString(), QString(), QObject::tr("S9"), QDateTime::currentDateTime(), resmed_data_version);
    }

    virtual void initChannels();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Now for some CPAPLoader overrides
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual QString PresReliefLabel() { return QObject::tr("EPR: "); }

    virtual ChannelID PresReliefMode();
    virtual ChannelID PresReliefLevel();
    virtual ChannelID CPAPModeChannel();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////


protected:
    void ParseSTR(Machine *mach, QStringList strfiles);

    //! \brief Scan for new files to import, group into sessions and add to task que
    int scanFiles(Machine * mach, QString datalog_path);

    QString backup(QString file, QString backup_path);

    QMap<SessionID, QStringList> sessfiles;
    QMap<quint32, STRRecord> strsess;
    QMap<QDate, QList<STRRecord *> > strdate;

#ifdef DEBUG_EFFICIENCY
    QHash<ChannelID, qint64> channel_efficiency;
    QHash<ChannelID, qint64> channel_time;
#endif
};

#endif // RESMED_LOADER_H
