/********************************************************************
 SleepLib Machine Loader Class Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include "machine_loader.h"


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

