#ifndef DATABASE_H
#define DATABASE_H

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the following value when making changes to this loaders implementation.
//
const int sleeplib_data_version=1;
//
//********************************************************************************************

/*#include <QString>
#include "SleepLib/profiles.h"
const QString sleeplib_class_name="SleepDB";

class SleepDatabase : public MachineLoader
{
public:
    SleepDatabase();
    virtual ~SleepDatabase();
    virtual bool Open(QString & path,Profile *profile);
    virtual int Version() { return sleeplib_data_version; };
    virtual const QString & ClassName() { return sleeplib_class_name; };
    Machine *CreateMachine(QString serial,Profile *profile);

    static void Register();
protected:
    map<QString,Machine *> MachList;

};*/

#endif // DATABASE_H
