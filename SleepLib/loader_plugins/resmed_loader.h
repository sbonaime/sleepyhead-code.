/*

SleepLib RESMED Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#ifndef RESMED_LOADER_H
#define RESMED_LOADER_H
//#include <map>
//using namespace std;
#include <vector>
#include "SleepLib/machine.h" // Base class: MachineLoader
#include "SleepLib/machine_loader.h"
#include "SleepLib/profiles.h"

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int resmed_data_version=0;
//
//********************************************************************************************

const QString resmed_class_name="ResMed";

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
} __attribute__ ((packed));

const int EDFHeaderSize=sizeof(EDFHeader);

struct EDFSignal {
public:
    QString label;
    QString transducer_type;
    QString physical_dimension;
    long physical_minimum;
    long physical_maximum;
    long digital_minimum;
    long digital_maximum;
    QString prefiltering;
    long nr;
    QString reserved;
    qint16 * data;
    int pos;
};

class EDFParser
{
public:
    EDFParser(QString filename);
    ~EDFParser();
    bool Open(QString name);

    QString Read(int si);
    qint16 Read16();

    vector<EDFSignal *> edfsignals;

    long GetNumSignals() { return num_signals; };
    long GetNumDataRecords() { return num_data_records; };
    double GetDuration() { return dur_data_record; };
    QString GetPatient() { return patientident; };
    bool Parse();
    char *buffer;
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

class ResmedLoader : public MachineLoader
{
public:
    ResmedLoader();
    virtual ~ResmedLoader();
    virtual int Open(QString & path,Profile *profile);

    virtual int Version() { return resmed_data_version; };
    virtual const QString & ClassName() { return resmed_class_name; };
    void ToTimeDelta(Session *sess,EDFParser &edf, qint16 *data, MachineCode code, long recs,qint64 duration,EventDataType divisor=1);

    Machine *CreateMachine(QString serial,Profile *profile);

    static void Register();
protected:
    map<QString,Machine *> ResmedList;
    bool LoadEVE(Session *sess,EDFParser &edf);
    bool LoadBRP(Session *sess,EDFParser &edf);
    bool LoadSAD(Session *sess,EDFParser &edf);
    bool LoadPLD(Session *sess,EDFParser &edf);

};

#endif // RESMED_LOADER_H
