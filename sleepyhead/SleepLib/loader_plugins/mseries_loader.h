/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib RemStar M-Series Loader Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef MSERIES_LOADER_H
#define MSERIES_LOADER_H

#include "SleepLib/machine.h"
#include "SleepLib/machine_loader.h"
#include "SleepLib/profiles.h"

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int mseries_data_version = 2;
//
//********************************************************************************************

/*! \class MSeries
    \brief RemStar M-Series customized machine object
    */
class MSeries: public CPAP
{
  public:
    MSeries(MachineID id = 0);
    virtual ~MSeries();
};


const int mseries_load_buffer_size = 1024 * 1024;


const QString mseries_class_name = STR_MACH_MSeries;

class MSeriesLoader : public MachineLoader
{
  public:
    MSeriesLoader();
    virtual ~MSeriesLoader();

    //! \brief Detect if the given path contains a valid Folder structure
    virtual bool Detect(const QString & path) { Q_UNUSED(path); return false; }

    //! \brief Opens M-Series block device
    virtual int Open(QString file);

    //! \brief Returns the database version of this loader
    virtual int Version() { return mseries_data_version; }

    //! \brief Return the ClassName, in this case "MSeries"
    virtual const QString &ClassName() { return mseries_class_name; }

    //! \brief Create a new PRS1 machine record, indexed by Serial number.
    Machine *CreateMachine(QString serial);

    //! \brief Register this Module to the list of Loaders, so it knows to search for PRS1 data.
    static void Register();
  protected:
    QHash<QString, Machine *> MachList;
    quint32 epoch;
};

#endif // MSERIES_LOADER_H
