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

/*#if defined(__WXMSW__) // windows gl extensions

#define GLEW_STATIC
#define WGL_WGLEXT_PROTOTYPES

#include <GL/glew.h>
#include <GL/wglew.h>
//#include <GL/gl.h>

#endif
#undef Yield */

#include <wx/app.h>
//#include <wx/glcanvas.h>

class SleepyHeadApp : public wxApp //wxGLApp
{
    public:
        virtual bool OnInit();
        virtual int OnExit();
};

#endif // SLEEPYHEADAPP_H
