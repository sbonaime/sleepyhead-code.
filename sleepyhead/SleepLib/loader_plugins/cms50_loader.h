/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib CMS50X Loader Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef CMS50LOADER_H
#define CMS50LOADER_H

#include "SleepLib/machine_loader.h"

const QString cms50_class_name = "CMS50";
const int cms50_data_version = 4;


/*! \class CMS50Loader
    \brief Bulk Importer for CMS50 SPO2Review format.. Deprecated, as the Oximetry module does a better job
    */
class CMS50Loader : public MachineLoader
{
  public:


    CMS50Loader();
    virtual ~CMS50Loader();

    virtual bool Detect(const QString &path) { Q_UNUSED(path); return false; }  // bypass autoscanner

    virtual int Open(QString &path, Profile *profile);
    static void Register();

    virtual int Version() { return cms50_data_version; }
    virtual const QString &ClassName() { return cms50_class_name; }

    Machine *CreateMachine(Profile *profile);


  protected:
    int OpenCMS50(QString &path, Profile *profile);
    bool OpenSPORFile(QString path, Machine *machine, Profile *profile);

  private:
    char *buffer;
};

#endif // CMS50LOADER_H
