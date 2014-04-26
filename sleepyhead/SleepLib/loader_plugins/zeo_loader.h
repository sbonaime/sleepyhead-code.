/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib ZEO Loader Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef ZEOLOADER_H
#define ZEOLOADER_H

#include "SleepLib/machine_loader.h"

const QString zeo_class_name = "ZEO";
const int zeo_data_version = 1;


/*! \class ZEOLoader
    \brief Unfinished stub for loading ZEO Personal Sleep Coach data
*/
class ZEOLoader : public MachineLoader
{
  public:
    ZEOLoader();
    virtual ~ZEOLoader();
    virtual bool Detect(const QString &path) { Q_UNUSED(path); return false; }  // bypass autoscanner

    virtual int Open(QString &path, Profile *profile);
    virtual int OpenFile(QString filename);
    static void Register();

    virtual int Version() { return zeo_data_version; }
    virtual const QString &ClassName() { return zeo_class_name; }


    Machine *CreateMachine(Profile *profile);

  protected:
  private:
};

#endif // ZEOLOADER_H
