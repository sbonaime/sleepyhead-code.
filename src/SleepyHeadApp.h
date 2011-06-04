/***************************************************************
 * Name:      SleepyHeadApp.h
 * Purpose:   Defines Application Class
 * Author:    Mark Watkins (jedimark64@gmail.com)
 * Created:   2011-05-20
 * Copyright: Mark Watkins (http://sourceforge.net/projects/sleepyhead/)
 * License:
 **************************************************************/

#ifndef SLEEPYHEADAPP_H
#define SLEEPYHEADAPP_H

#include <wx/app.h>

class SleepyHeadApp : public wxApp
{
    public:
        virtual bool OnInit();
        virtual int OnExit();
};

#endif // SLEEPYHEADAPP_H
