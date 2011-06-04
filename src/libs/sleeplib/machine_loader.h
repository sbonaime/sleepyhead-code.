/*

SleepLib MachineLoader Base Class Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef MACHINE_LOADER_H
#define MACHINE_LOADER_H
#include "profiles.h"

class MachineLoader
{
public:
    MachineLoader();
    virtual ~MachineLoader();
    virtual bool Open(wxString &,Profile *profile)=0;
};

void RegisterLoader(MachineLoader *loader);
void DestroyLoaders();
list<MachineLoader *> GetLoaders();

#endif //MACHINE_LOADER_H
