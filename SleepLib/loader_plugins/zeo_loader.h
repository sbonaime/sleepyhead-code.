/*
SleepLib ZEO Loader Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/

#ifndef ZEOLOADER_H
#define ZEOLOADER_H

#include "SleepLib/machine_loader.h"

const QString zeo_class_name="ZEO";
const int zeo_data_version=1;


/*! \class ZEOLoader
    \brief Unfinished stub for loading ZEO Personal Sleep Coach data
*/
class ZEOLoader : public MachineLoader
{
    public:
        ZEOLoader();
        virtual ~ZEOLoader();
        virtual int Open(QString & path,Profile *profile);
        virtual int OpenFile(QString filename);
        static void Register();

        virtual int Version() { return zeo_data_version; }
        virtual const QString & ClassName() { return zeo_class_name; }


        Machine *CreateMachine(Profile *profile);

    protected:
    private:
};

#endif // ZEOLOADER_H
