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

#elif defined(__UNIX__)
#include <GL/glx.h>
#endif

#define MIN(a,b) (a<b) ? a : b;
#define MAX(a,b) (a<b) ? b : a;


class GLException {
public:
    GLException(wxString s=wxT("Lazy Programmer forgot to specify error")) { wxLogError(wxT("GLException: ")+s); };
};



class pBuffer {
public:
    pBuffer();
    pBuffer(int width, int height);
    virtual ~pBuffer();
    virtual void UseBuffer(bool b) {};

protected:
    int m_width;
    int m_height;
};

#if defined(__WXMSW__)
class pBufferWGL:public pBuffer
{
public:
    pBufferWGL(int width, int height);
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
    pBufferGLX(int width, int height);
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
    pBufferAGL(int width, int height){};
    virtual ~pBufferAGL(){};
    virtual void UseBuffer(bool b){};
protected:

    virtual bool InitGLStuff(){};
};
#endif

#endif // GL_PBUFFER_H
