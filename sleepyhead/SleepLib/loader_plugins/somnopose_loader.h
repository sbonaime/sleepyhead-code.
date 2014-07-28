/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib Somnopose Loader Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef SOMNOPOSELOADER_H
#define SOMNOPOSELOADER_H

#include "SleepLib/machine_loader.h"

const QString somnopose_class_name = "Somnopose";
const int somnopose_data_version = 1;


/*! \class SomnoposeLoader
    \brief Unfinished stub for loading Somnopose Positional CSV data
*/
class SomnoposeLoader : public MachineLoader
{
  public:
    SomnoposeLoader();
    virtual ~SomnoposeLoader();
    virtual bool Detect(const QString &path) { Q_UNUSED(path); return false; }  // bypass autoscanner

    virtual int Open(QString path);
    virtual int OpenFile(QString filename);
    static void Register();

    virtual int Version() { return somnopose_data_version; }
    virtual const QString &loaderName() { return somnopose_class_name; }

    virtual MachineInfo newInfo() {
        return MachineInfo(MT_POSITION, somnopose_class_name, QObject::tr("Somnopose"), QString(), QString(), QString(), QObject::tr("Somnopose Software"), QDateTime::currentDateTime(), somnopose_data_version);
    }


  //Machine *CreateMachine();

  protected:
  private:
};

#endif // SOMNOPOSELOADER_H
