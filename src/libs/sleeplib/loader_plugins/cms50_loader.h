/*
SleepLib CMS50X Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#ifndef CMS50LOADER_H
#define CMS50LOADER_H

#include "../machine_loader.h"


class CMS50Loader : public MachineLoader
{
    public:
        CMS50Loader();
        virtual ~CMS50Loader();
        virtual bool Open(wxString & path,Profile *profile);
        static void Register();

        Machine *CreateMachine(Profile *profile);

    protected:
    private:
};

#endif // CMS50LOADER_H
