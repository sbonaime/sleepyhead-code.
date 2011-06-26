/*
SleepLib ZEO Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#ifndef ZEOLOADER_H
#define ZEOLOADER_H

#include "SleepLib/machine_loader.h"

const QString zeo_class_name="CMS50";
const int zeo_data_version=1;


class ZEOLoader : public MachineLoader
{
    public:
        ZEOLoader();
        virtual ~ZEOLoader();
        virtual bool Open(QString & path,Profile *profile);
        static void Register();

        virtual int Version() { return zeo_data_version; };
        virtual const QString & ClassName() { return zeo_class_name; };


        Machine *CreateMachine(Profile *profile);

    protected:
    private:
};

#endif // ZEOLOADER_H
