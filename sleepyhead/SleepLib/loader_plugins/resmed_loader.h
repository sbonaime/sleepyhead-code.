/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib RESMED Loader Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
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

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int resmed_data_version = 9;
//
//********************************************************************************************


const QString resmed_class_name = STR_MACH_ResMed;

/*! \struct EDFHeader
    \brief  Represents the EDF+ header structure, used as a place holder while processing the text data.
    \note More information on the EDF+ file format can be obtained from http://edfplus.info
*/
struct EDFHeader {
    char version[8];
    char patientident[80];
    char recordingident[80];
    char datetime[16];
    char num_header_bytes[8];
    char reserved[44];
    char num_data_records[8];
    char dur_data_records[8];
    char num_signals[4];
}
#ifndef BUILD_WITH_MSVC
__attribute__((packed))
#endif
;

const int EDFHeaderSize = sizeof(EDFHeader);

/*! \struct EDFSignal
    \brief Contains information about a single EDF+ Signal
    \note More information on the EDF+ file format can be obtained from http://edfplus.info
    */
struct EDFSignal {
  public:
    //! \brief Name of this Signal
    QString label;

    //! \brief Tranducer Type (source of the data, usually blank)
    QString transducer_type;

    //! \brief The units of measurements represented by this signal
    QString physical_dimension;

    //! \brief The minimum limits of the ungained data
    EventDataType physical_minimum;

    //! \brief The maximum limits of the ungained data
    EventDataType physical_maximum;

    //! \brief The minimum limits of the data with gain and offset applied
    EventDataType digital_minimum;

    //! \brief The maximum limits of the data with gain and offset applied
    EventDataType digital_maximum;

    //! \brief Raw integer data is multiplied by this value
    EventDataType gain;

    //! \brief This value is added to the raw data
    EventDataType offset;

    //! \brief Any prefiltering methods used (usually blank)
    QString prefiltering;

    //! \brief Number of records
    long nr;

    //! \brief Reserved (usually blank)
    QString reserved;

    //! \brief Pointer to the signals sample data
    qint16 *data;

    //! \brief a non-EDF extra used internally to count the signal data
    int pos;
};

struct STRRecord
{
    STRRecord() {
        maskon = 0;
        maskoff = 0;
        maskdur = 0;
        maskevents = -1;
        mode = -1;
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

        date=QDate();
    }
    STRRecord(const STRRecord & copy) {
        maskon = copy.maskon;
        maskoff = copy.maskoff;
        maskdur = copy.maskdur;
        maskevents = copy.maskevents;
        mode = copy.mode;
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
    }
    quint32 maskon;
    quint32 maskoff;
    EventDataType maskdur;
    EventDataType maskevents;
    EventDataType mode;
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
    QDate date;
};


/*! \class EDFParser
    \author Mark Watkins <jedimark64_at_users.sourceforge.net>
    \brief Parse an EDF+ data file into a list of EDFSignal's
    \note More information on the EDF+ file format can be obtained from http://edfplus.info
    */
class EDFParser
{
  public:
    //! \brief Constructs an EDFParser object, opening the filename if one supplied
    EDFParser(QString filename = "");

    ~EDFParser();

    //! \brief Open the EDF+ file, and read it's header
    bool Open(QString name);

    //! \brief Read n bytes of 8 bit data from the EDF+ data stream
    QString Read(unsigned n);

    //! \brief Read 16 bit word of data from the EDF+ data stream
    qint16 Read16();

    //! \brief Vector containing the list of EDFSignals contained in this edf file
    QVector<EDFSignal> edfsignals;

    //! \brief An by-name indexed into the EDFSignal data
    QStringList signal_labels;

    QList<EDFSignal *> signal;

    //! \brief Look up signal names by SleepLib ChannelID.. A little "ResMed"ified.. :/
    EDFSignal *lookupSignal(ChannelID);
    EDFSignal *lookupLabel(QString name);

    //! \brief Returns the number of signals contained in this EDF file
    long GetNumSignals() { return num_signals; }

    //! \brief Returns the number of data records contained per signal.
    long GetNumDataRecords() { return num_data_records; }

    //! \brief Returns the duration represented by this EDF file (in milliseconds)
    qint64 GetDuration() { return dur_data_record; }

    //! \brief Returns the patientid field from the EDF header
    QString GetPatient() { return patientident; }

    //! \brief Parse the EDF+ file into the list of EDFSignals.. Must be call Open(..) first.
    bool Parse();
    char *buffer;

    //! \brief  The EDF+ files header structure, used as a place holder while processing the text data.
    EDFHeader header;

    QString filename;
    long filesize;
    long datasize;
    long pos;

    long version;
    long num_header_bytes;
    long num_data_records;
    qint64 dur_data_record;
    long num_signals;

    QString patientident;
    QString recordingident;
    QString serialnumber;
    qint64 startdate;
    qint64 enddate;
    QString reserved44;
};

class ResmedLoader;

struct EDFGroup {
    EDFGroup() { }
    EDFGroup(QString brp, QString eve, QString pld, QString sad) {
        BRP = brp;
        EVE = eve;
        PLD = pld;
        SAD = sad;
    }
    EDFGroup(const EDFGroup & copy) {
        BRP = copy.BRP;
        EVE = copy.EVE;
        PLD = copy.PLD;
        SAD = copy.SAD;
    }
    QString BRP;
    QString EVE;
    QString PLD;
    QString SAD;
};

class ResmedImport:public ImportTask
{
public:
    ResmedImport(ResmedLoader * l, SessionID s, EDFGroup g, Machine * m): loader(l), sessionid(s), group(g), mach(m) {}
    virtual ~ResmedImport() {}
    virtual void run();

protected:
    ResmedLoader * loader;
    SessionID sessionid;
    EDFGroup group;
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
class ResmedLoader : public MachineLoader
{
    friend class ResmedImport;
    friend class ResmedImportStage2;
  public:
    ResmedLoader();
    virtual ~ResmedLoader();

    //! \brief Detect if the given path contains a valid Folder structure
    virtual bool Detect(const QString & path);

    //! \brief Scans for S9 SD folder structure signature, and loads any new data if found
    virtual int Open(QString path, Profile *profile);

    //! \brief Returns the version number of this ResMed loader
    virtual int Version() { return resmed_data_version; }

    //! \brief Returns the Machine class name of this loader. ("ResMed")
    virtual const QString &ClassName() { return resmed_class_name; }

    //! \brief Converts EDFSignal data to time delta packed EventList, and adds to Session
    void ToTimeDelta(Session *sess, EDFParser &edf, EDFSignal &es, ChannelID code, long recs,
                     qint64 duration, EventDataType min = 0, EventDataType max = 0, bool square = false);

    //! \brief Create Machine record, and index it by serial number
    Machine *CreateMachine(QString serial, Profile *profile);

    //! \brief Register the ResmedLoader with the list of other machine loaders
    static void Register();

    //! \brief Parse the EVE Event annotation data, and save to Session * sess
    //! This contains all Hypopnea, Obstructive Apnea, Central and Apnea codes
    bool LoadEVE(Session *sess, const QString & path);

    //! \brief Parse the BRP High Resolution data, and save to Session * sess
    //! This contains Flow Rate, Mask Pressure, and Resp. Event  data
    bool LoadBRP(Session *sess, const QString & path);

    //! \brief Parse the SAD Pulse oximetry attachment data, and save to Session * sess
    //! This contains Pulse Rate and SpO2 Oxygen saturation data
    bool LoadSAD(Session *sess, const QString & path);

    //! \brief Parse the PRD low resolution data, and save to Session * sess
    //! This contains the Pressure, Leak, Respiratory Rate, Minute Ventilation, Tidal Volume, etc..
    bool LoadPLD(Session *sess, const QString & path);

protected:
    QHash<QString, Machine *> ResmedList;

    void ParseSTR(Machine *mach, QStringList strfiles);


    QString backup(QString file, QString backup_path, bool compress = false);

    QMap<SessionID, QStringList> sessfiles;
    QMap<quint32, STRRecord> strsess;
    QMap<QDate, QList<STRRecord *> > strdate;

#ifdef DEBUG_EFFICIENCY
    QHash<ChannelID, qint64> channel_efficiency;
    QHash<ChannelID, qint64> channel_time;
#endif
};

#endif // RESMED_LOADER_H
