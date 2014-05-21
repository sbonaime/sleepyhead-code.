/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib MachineLoader Base Class Header
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef MACHINE_LOADER_H
#define MACHINE_LOADER_H

#include <QMutex>
#include <QRunnable>

#include "profiles.h"
#include "machine.h"


#include "zlib.h"

class MachineLoader;


/*! \class MachineLoader
    \brief Base class to derive a new Machine importer from
    */
class MachineLoader: public QObject
{
    Q_OBJECT
    friend class ImportThread;
  public:
    MachineLoader();
    virtual ~MachineLoader();

    //! \brief Detect if the given path contains a valid folder structure
    virtual bool Detect(const QString & path) = 0;

    //! \brief Override this to scan path and detect new machine data
    virtual int Open(QString &path, Profile *) = 0; // Scans for new content

    //! \brief Override to returns the Version number of this MachineLoader
    virtual int Version() = 0;

    //! \brief Override to returns the class name of this MachineLoader
    virtual const QString &ClassName() = 0;
    inline MachineType type() { return m_type; }

    void queTask(ImportTask * task);

    //! \brief Process Task list using all available threads.
    void runTasks();
  protected:
    //! \brief Contains a list of Machine records known by this loader
    QList<Machine *> m_machlist;

    MachineType m_type;
    QString m_class;
    Profile *m_profile;

    int m_currenttask;
    int m_totaltasks;

  private:
    QList<ImportTask *> m_tasklist;
};


// Put in machine loader class as static??
void RegisterLoader(MachineLoader *loader);
void DestroyLoaders();
bool compressFile(QString inpath, QString outpath = "");

QList<MachineLoader *> GetLoaders(MachineType mt = MT_UNKNOWN);

#endif //MACHINE_LOADER_H
