/*
SleepLib CMS50X Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#ifndef CMS50LOADER_H
#define CMS50LOADER_H

#include "sleeplib/machine_loader.h"

const wxString cms50_class_name=wxT("CMS50");
const int cms50_data_version=1;

class CMS50Loader : public MachineLoader
{
    public:


        CMS50Loader();
        virtual ~CMS50Loader();
        virtual bool Open(wxString & path,Profile *profile);
        static void Register();

        virtual int Version() { return cms50_data_version; };
        virtual const wxString & ClassName() { return cms50_class_name; };

        Machine *CreateMachine(Profile *profile);


    protected:
        bool OpenCMS50(wxString & path, Profile *profile);
        bool OpenSPORFile(wxString path, Machine * machine,Profile *profile);

    private:
        char *buffer;
};

#endif // CMS50LOADER_H
