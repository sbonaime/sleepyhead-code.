/*

SleepLib Fisher & Paykel Icon Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
Copyright: (c)2012 Mark Watkins

*/

#ifndef ICON_LOADER_H
#define ICON_LOADER_H

#include <QMultiMap>
#include "SleepLib/machine.h"
#include "SleepLib/machine_loader.h"
#include "SleepLib/profiles.h"


//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int fpicon_data_version=2;
//
//********************************************************************************************

/*! \class FPIcon
    \brief F&P Icon customized machine object
    */
class FPIcon:public CPAP
{
public:
    FPIcon(Profile *p,MachineID id=0);
    virtual ~FPIcon();
};


const int fpicon_load_buffer_size=1024*1024;


const QString fpicon_class_name=STR_MACH_FPIcon;

/*! \class FPIconLoader
    \brief Loader for Fisher & Paykel Icon data
    This is only relatively recent addition and still needs more work
    */

class FPIconLoader : public MachineLoader
{
public:
    FPIconLoader();
    virtual ~FPIconLoader();

    //! \brief Scans path for F&P Icon data signature, and Loads any new data
    virtual int Open(QString & path,Profile *profile);

    int OpenMachine(Machine *mach, QString & path, Profile * profile);

    bool OpenSummary(Machine *mach, QString path, Profile * profile);
    bool OpenDetail(Machine *mach, QString path, Profile * profile);
    bool OpenFLW(Machine * mach,QString filename, Profile * profile);

    //! \brief Returns SleepLib database version of this F&P Icon loader
    virtual int Version() { return fpicon_data_version; }

    //! \brief Returns the machine class name of this CPAP machine, "FPIcon"
    virtual const QString & ClassName() { return fpicon_class_name; }

    //! \brief Creates a machine object, indexed by serial number
    Machine *CreateMachine(QString serial,Profile *profile);

    //! \brief Registers this MachineLoader with the master list, so F&P Icon data can load
    static void Register();

protected:
    QString last;
    QHash<QString,Machine *> MachList;
    QMap<SessionID, Session *> Sessions;
    QMultiMap<QDate,Session *> SessDate;
    QMap<int,QList<EventList *> > FLWMapFlow;
    QMap<int,QList<EventList *> > FLWMapLeak;
    QMap<int,QList<EventList *> > FLWMapPres;
    QMap<int,QList<qint64> > FLWDuration;
    QMap<int,QList<qint64> > FLWTS;
    QMap<int,QDate> FLWDate;

    unsigned char * m_buffer;
};

#endif // ICON_LOADER_H
