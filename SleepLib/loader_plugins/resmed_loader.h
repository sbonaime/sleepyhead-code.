/*

SleepLib RESMED Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#ifndef RESMED_LOADER_H
#define RESMED_LOADER_H
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
const int resmed_data_version=1;
//
//********************************************************************************************

const QString resmed_class_name="ResMed";

class ResmedLoader : public MachineLoader
{
public:
    ResmedLoader();
    virtual ~ResmedLoader();
    virtual bool Open(QString & path,Profile *profile);

    virtual int Version() { return resmed_data_version; };
    virtual const QString & ClassName() { return resmed_class_name; };
    Machine *CreateMachine(QString serial,Profile *profile);

    static void Register();
protected:
    map<QString,Machine *> ResmedList;
};

#endif // RESMED_LOADER_H
