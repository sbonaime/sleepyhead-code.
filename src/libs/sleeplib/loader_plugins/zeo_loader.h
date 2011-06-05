/*
SleepLib ZEO Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#ifndef ZEOLOADER_H
#define ZEOLOADER_H

#include "sleeplib/machine_loader.h"


class ZEOLoader : public MachineLoader
{
    public:
        ZEOLoader();
        virtual ~ZEOLoader();
        virtual bool Open(wxString & path,Profile *profile);
        static void Register();

        Machine *CreateMachine(Profile *profile);

    protected:
    private:
};

#endif // ZEOLOADER_H
