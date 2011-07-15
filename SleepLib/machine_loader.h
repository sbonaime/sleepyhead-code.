/*

SleepLib MachineLoader Base Class Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef MACHINE_LOADER_H
#define MACHINE_LOADER_H
#include "profiles.h"
#include "machine.h"

class MachineLoader
{
public:
    MachineLoader();
    virtual ~MachineLoader();


    //virtual Machine * CreateMachine() {};

    virtual int Open(QString &,Profile *)=0;   // Scans for new content
    virtual int Version()=0;
    virtual const QString & ClassName()=0;


 /*
    MachineLoader(Profile *profile,QString & classname);
    virtual void LoadMachineList();
    virtual void SaveMachineList();
    virtual bool LoadSummaries();
    virtual bool LoadEvents();
    virtual bool LoadWaveforms();
    virtual bool Scan(QString &)=0;   // Scans for new content

    virtual bool LoadAll();
    virtual bool SaveAll();

    virtual bool LoadSummary(Machine * m, QString & filename);
    virtual bool LoadEvent(Machine * m, QString & filename);
    virtual bool LoadWaveform(Machine * m, QString & filename);

    virtual bool SaveSummary(Machine * m, QString & filename);
    virtual bool SaveEvent(Machine * m, QString & filename);
    virtual bool SaveWaveform(Machine * m, QString & filename);*/

protected:
    vector<Machine *> m_machlist;
    QString m_class;
    MachineType m_type;
    Profile * m_profile;
};

void RegisterLoader(MachineLoader *loader);
void DestroyLoaders();
list<MachineLoader *> GetLoaders();

#endif //MACHINE_LOADER_H
