/*
SleepLib CMS50X Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#ifndef CMS50LOADER_H
#define CMS50LOADER_H

#include "sleeplib/machine_loader.h"


class CMS50Loader : public MachineLoader
{
    public:
        CMS50Loader();
        virtual ~CMS50Loader();
        virtual bool Open(wxString & path,Profile *profile);
        static void Register();

        Machine *CreateMachine(Profile *profile);

    protected:
        bool OpenCMS50(wxString & path, Profile *profile);
        bool OpenSPORFile(wxString path, Machine * machine,Profile *profile);

    private:
        char *buffer;
};

#endif // CMS50LOADER_H
