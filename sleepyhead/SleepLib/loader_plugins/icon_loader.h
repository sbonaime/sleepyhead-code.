/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Fisher & Paykel Icon Loader Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

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
const int fpicon_data_version = 3;
//
//********************************************************************************************

/*! \class FPIcon
    \brief F&P Icon customized machine object
    */
class FPIcon: public CPAP
{
  public:
    FPIcon(MachineID id = 0);
    virtual ~FPIcon();
};


const int fpicon_load_buffer_size = 1024 * 1024;


const QString fpicon_class_name = STR_MACH_FPIcon;

/*! \class FPIconLoader
    \brief Loader for Fisher & Paykel Icon data
    This is only relatively recent addition and still needs more work
    */

class FPIconLoader : public MachineLoader
{
  public:
    FPIconLoader();
    virtual ~FPIconLoader();

    //! \brief Detect if the given path contains a valid Folder structure
    virtual bool Detect(const QString & path);

    //! \brief Scans path for F&P Icon data signature, and Loads any new data
    virtual int Open(QString path);

    int OpenMachine(Machine *mach, QString &path);

    bool OpenSummary(Machine *mach, QString path);
    bool OpenDetail(Machine *mach, QString path);
    bool OpenFLW(Machine *mach, QString filename);

    //! \brief Returns SleepLib database version of this F&P Icon loader
    virtual int Version() { return fpicon_data_version; }

    //! \brief Returns the machine class name of this CPAP machine, "FPIcon"
    virtual const QString &loaderName() { return fpicon_class_name; }

    // ! \brief Creates a machine object, indexed by serial number
    //Machine *CreateMachine(QString serial);

    virtual MachineInfo newInfo() {
        return MachineInfo(MT_CPAP, fpicon_class_name, QObject::tr("Fisher & Paykel"), QString(), QString(), QString(), QObject::tr("ICON"), QDateTime::currentDateTime(), fpicon_data_version);
    }


    //! \brief Registers this MachineLoader with the master list, so F&P Icon data can load
    static void Register();

  protected:
    QDateTime readFPDateTime(quint8 *data);

    QString last;
    QHash<QString, Machine *> MachList;
    QMap<SessionID, Session *> Sessions;
    QMultiMap<QDate, Session *> SessDate;
    //QMap<int,QList<EventList *> > FLWMapFlow;
    //QMap<int,QList<EventList *> > FLWMapLeak;
    //QMap<int,QList<EventList *> > FLWMapPres;
    //QMap<int,QList<qint64> > FLWDuration;
    //QMap<int,QList<qint64> > FLWTS;
    //QMap<int,QDate> FLWDate;

    unsigned char *m_buffer;
};

#endif // ICON_LOADER_H
