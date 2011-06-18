/*
pBuffer OpenGL Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/
#ifndef GL_PBUFFER_H
#define GL_PBUFFER_H

#define GL_GLEXT_PROTOTYPES


#undef Yield
#include <wx/log.h>

#if defined(__WXMSW__) // windows gl extensions

#define GLEW_STATIC
#define WGL_WGLEXT_PROTOTYPES

#include <GL/glew.h>
#include <GL/wglew.h>
//#include <GL/gl.h>

#elif defined(__DARWIN__)
#include <OpenGL/gl.h>
#include <AGL/agl.h>

// Not sure about these
#if defined(__WXCARBON__)
#include <Carbon/Carbon.h>
#elif defined(__WXCOCOA__)
#include <Cocoa/Cocoa.h>
#endif


#elif defined(__UNIX__)
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#define MIN(a,b) (((a)<(b)) ? (a) : (b));
#define MAX(a,b) (((a)<(b)) ? (b) : (a));

#include <wx/bitmap.h>
#include <wx/glcanvas.h>

long roundup2(long v);

class GLException {
public:
    GLException(wxString s=wxT("Lazy Programmer forgot to specify error")) { wxLogError(wxT("GLException: ")+s); };
};



class pBuffer {
public:
    pBuffer();
    pBuffer(int width, int height,wxGLCanvas * gc);
    virtual ~pBuffer();
    virtual void UseBuffer(bool b) {};
    int Width() { return m_width; };
    int Height() { return m_height; };
    virtual wxBitmap *Snapshot(int width, int height);
protected:
    int m_width;
    int m_height;
};

class FBO:public pBuffer
{
public:
    FBO(int width, int height,wxGLCanvas * gc);
    virtual ~FBO();
    virtual void UseBuffer(bool b);
    virtual wxBitmap *Snapshot(int width, int height);
protected:
    GLuint depthbuffer,colorbuffer;
    GLuint img;
    GLuint fbo;
    bool m_depth_buffer;
    bool m_color_buffer;

};

#if defined(__WXMSW__)
class pBufferWGL:public pBuffer
{
public:
    pBufferWGL(int width, int height,wxGLCanvas * gc);
    virtual ~pBufferWGL();
    virtual void UseBuffer(bool b);
protected:

    bool InitGLStuff();

    unsigned int m_texture;

    HDC saveHdc;
    HGLRC saveHglrc;

    HPBUFFERARB hBuffer;
    HDC hdc;
    HGLRC hGlRc;

};
#elif defined(__UNIX__) && !defined(__WXMAC__)
// No way around having to do this.
// Until wx provides access to the data we need
extern GLXContext real_shared_context;

class pBufferGLX:public pBuffer
{
public:
    pBufferGLX(int width, int height,wxGLCanvas * gc);
    virtual ~pBufferGLX();
    virtual void UseBuffer(bool b);
protected:

    Display *display;
    GLXPbuffer pBuffer;
    GLXContext m_context;
    GLXContext m_shared;
};

#elif defined(__DARWIN__) || defined(__WXMAC__)
class pBufferAGL:public pBuffer
{
public:
    pBufferAGL(int width, int height,wxGLCanvas * gc);
    virtual ~pBufferAGL();
    virtual void UseBuffer(bool b);
protected:
/*    AGLPixelFormat pixelFormat;
    AGLPbuffer pbuffer;
    AGLContext context, pbContext;
    long virtualScreen;
    GDHandle display2;
    GDHandle display; */
};
#endif

#endif // GL_PBUFFER_H
