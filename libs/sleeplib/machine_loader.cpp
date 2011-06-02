/*

SleepLib Machine Loader Base class Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#include "machine_loader.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Machine Loader implmementation
//////////////////////////////////////////////////////////////////////////////////////////

list<MachineLoader *> m_loaders;

void RegisterLoader(MachineLoader *loader)
{
    m_loaders.push_back(loader);
}
void DestroyLoaders()
{
    for (list<MachineLoader *>::iterator i=m_loaders.begin(); i!=m_loaders.end(); i++) {
        delete (*i);
    }
    m_loaders.clear();
}

MachineLoader::MachineLoader()
{

}
MachineLoader::~MachineLoader()
{

}

list<MachineLoader *> GetLoaders()
{
    return m_loaders;
}

