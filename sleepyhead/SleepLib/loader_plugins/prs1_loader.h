/*

SleepLib PRS1 Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
Copyright: (c)2011 Mark Watkins

*/

#ifndef PRS1LOADER_H
#define PRS1LOADER_H
//#include <map>
//using namespace std;
#include "SleepLib/machine.h" // Base class: MachineLoader
#include "SleepLib/machine_loader.h"
#include "SleepLib/profiles.h"


//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int prs1_data_version=10;
//
//********************************************************************************************

/*! \class PRS1
    \brief PRS1 customized machine object (via CPAP)
    */
class PRS1:public CPAP
{
public:
    PRS1(Profile *p,MachineID id=0);
    virtual ~PRS1();
};


const int max_load_buffer_size=1024*1024;


const QString prs1_class_name=STR_MACH_PRS1;

/*! \class PRS1Loader
    \brief Philips Respironics System One Loader Module
    */
class PRS1Loader : public MachineLoader
{
public:
    PRS1Loader();
    virtual ~PRS1Loader();
    //! \brief Scans directory path for valid PRS1 signature
    virtual int Open(QString & path,Profile *profile);

    //! \brief Returns the database version of this loader
    virtual int Version() { return prs1_data_version; }

    //! \brief Return the ClassName, in this case "PRS1"
    virtual const QString & ClassName() { return prs1_class_name; }

    //! \brief Create a new PRS1 machine record, indexed by Serial number.
    Machine *CreateMachine(QString serial,Profile *profile);

    //! \brief Register this Module to the list of Loaders, so it knows to search for PRS1 data.
    static void Register();
protected:
    QString last;
    QHash<QString,Machine *> PRS1List;

    //! \brief Opens the SD folder structure for this machine, scans for data files and imports any new sessions
    int OpenMachine(Machine *m,QString path,Profile *profile);

    //! \brief Parses "properties.txt" file containing machine information
    bool ParseProperties(Machine *m,QString filename);

    //bool OpenSummary(Session *session,QString filename);
    //bool OpenEvents(Session *session,QString filename);

    //! \brief Parse a .005 waveform file, extracting Flow Rate waveform (and Mask Pressure data if available)
    bool OpenWaveforms(SessionID sid, QString filename);

    // //! \brief ParseWaveform chunk.. Currently unused, as the old one works fine.
    //bool ParseWaveform(qint32 sequence, quint32 timestamp, unsigned char *data, quint16 size, quint16 duration, quint16 num_signals, quint16 interleave, quint8 sample_format);

    //! \brief Parse a data chunk from the .000 (brick) and .001 (summary) files.
    bool ParseSummary(Machine *mach, qint32 sequence, quint32 timestamp, unsigned char *data, quint16 size, int family, int familyVersion);

    //! \brief Parse a single data chunk from a .002 file containing event data for a standard system one machine
    bool Parse002(qint32 sequence, quint32 timestamp, unsigned char *data, quint16 size, int family, int familyVersion);

    //! \brief Parse a single data chunk from a .002 file containing event data for a family 5 ASV machine (which has a different format)
    bool Parse002v5(qint32 sequence, quint32 timestamp, unsigned char *data, quint16 size);

    //! \brief Open a PRS1 data file, and break into data chunks, delivering them to the correct parser.
    bool OpenFile(Machine *mach, QString filename);

    //bool Parse002(Session *session,unsigned char *buffer,int size,qint64 timestamp,long fpos);
    //bool Parse002ASV(Session *session,unsigned char *buffer,int size,qint64 timestamp,long fpos);
    unsigned char * m_buffer;
    QHash<SessionID, Session *> extra_session;

    //! \brief PRS1 Data files can store multiple sessions, so store them in this list for later processing.
    QHash<SessionID, Session *> new_sessions;
    qint32 summary_duration;
};

#endif // PRS1LOADER_H
