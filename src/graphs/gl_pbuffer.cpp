/*
pBuffer OpenGL Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
*/
#include "gl_pbuffer.h"

#if defined(__DARWIN__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <wx/utils.h>
#include <wx/glcanvas.h>
#include <wx/image.h>

long roundup2(long v)
{
    int j;
    for (int i=4;i<32;i++) {  // min size 16x16.. probably a usless size.
        j=1 << i;
        if (j >= v) break;
    }
    return j;
}


pBuffer::pBuffer(wxGLContext * gc)
{
    m_gc=gc;
}
pBuffer::pBuffer(int width, int height,wxGLContext * gc)
:m_gc(gc),m_width(width),m_height(height)
{
}
pBuffer::~pBuffer()
{
}
wxBitmap *pBuffer::Snapshot(int width, int height)
{
    glDrawBuffer(GL_BACK_LEFT);

    void* pixels = malloc(3 * width * height); // must use malloc
    glReadBuffer(GL_BACK_LEFT);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Put the image into a wxImage
    wxImage image(width, height, true);
    image.SetData((unsigned char*) pixels);
    image = image.Mirror(false);
    wxBitmap *bmp=new wxBitmap(image);
    return bmp;
}
void pBuffer::SelectContext(wxGLCanvas * glc)
{
    assert(glc!=NULL);

    if (glc->IsShownOnScreen()) glc->SetCurrent(*m_gc);
    // else wx sucks..
}


FBO::FBO(int width, int height,wxGLContext * gc)
:pBuffer(gc)
{
    //wxGLContext a((wxGLCanvas *)NULL,(wxGLContext *)NULL);
    int m=MAX(width,height);
    //int j=roundup2(m) << 2;
    m_width=width; //roundup2(width) << 2;
    m_height=height; //roundup2(height) << 2;

    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

    m_depth_buffer=false;
    m_color_buffer=true; //false;

    if (m_depth_buffer) {
        glGenRenderbuffersEXT(1, &depthbuffer);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthbuffer);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, m_width, m_height);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthbuffer);
    }

    if (m_color_buffer) {
        glGenRenderbuffersEXT(1, &colorbuffer);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, colorbuffer);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, m_width, m_height);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, colorbuffer);
    }

    // create texture to render to.
    /*glGenTextures(1, &img);
    glBindTexture(GL_TEXTURE_2D, img);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, img, 0);*/

    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status!=GL_FRAMEBUFFER_COMPLETE_EXT) {
        wxString a;
        a << (int)status; //((char *)gluErrorString(status),wxConvUTF8);
      //  glDeleteTextures(1, &img);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        glDeleteFramebuffersEXT(1, &fbo);
        if (m_depth_buffer)
            glDeleteRenderbuffersEXT(1, &depthbuffer);
        if (m_color_buffer)
            glDeleteRenderbuffersEXT(1, &colorbuffer);

        throw GLException(wxT("glCheckFramebufferStatusEXT failed: ")+a);
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
//    GLenum err = glGetError();
  //  if (err!=GL_NO_ERROR) {

}


FBO::~FBO()
{
    glDeleteTextures(1, &img);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glDeleteFramebuffersEXT(1, &fbo);
    if (m_depth_buffer)
        glDeleteRenderbuffersEXT(1, &depthbuffer);
    if (m_color_buffer)
        glDeleteRenderbuffersEXT(1, &colorbuffer);
}
void FBO::SelectBuffer()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
}
void FBO::SelectContext(wxGLCanvas * glc)
{
    // Don't need the context in this case..
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
wxBitmap *FBO::Snapshot(int width,int height)
{
    //width=m_width;
    //height=m_height;
    //glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

    void* pixels = malloc(3 * width * height); // must use malloc
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);


    // Put the image into a wxImage
    wxImage image(width, height, true);
    image.SetData((unsigned char*) pixels);
    image = image.Mirror(false);
    wxBitmap *bmp=new wxBitmap(image);
    return bmp;
}



#if defined(__WXMSW__)

#if !defined(wglGetExtensionsStringARB)
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = NULL;

    // WGL_ARB_pbuffer
    PFNWGLCREATEPBUFFERARBPROC    wglCreatePbufferARB    = NULL;
    PFNWGLGETPBUFFERDCARBPROC     wglGetPbufferDCARB     = NULL;
    PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB = NULL;
    PFNWGLDESTROYPBUFFERARBPROC   wglDestroyPbufferARB   = NULL;
    PFNWGLQUERYPBUFFERARBPROC     wglQueryPbufferARB     = NULL;

    // WGL_ARB_pixel_format
    PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB = NULL;
    PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB = NULL;
    PFNWGLCHOOSEPIXELFORMATARBPROC      wglChoosePixelFormatARB      = NULL;
#endif


pBufferWGL::pBufferWGL(int width, int height,wxGLContext * gc)
:pBuffer(width,height,gc),m_texture(0)
{

    hGlRc=0;
    hBuffer=0;
    hdc=0;
    saveHdc=0;
    saveHglrc=0;

    if (!InitGLStuff()) {
        throw GLException(wxT("Could not Init GL Stuff"));
    }

    // Adjust to square power of 2
    int ms=MAX(width,height);
    int j;
    for (int i=4;i<32;i++) {  // min size 16x16.. probably a usless size.
        j=1 << i;
        if (j >= ms) break;
    }
    //j <<= 2;
    //assert (j>=ms); // I seriously doubt this will ever happen ;)

    // WGL only supports square pBuffers (apparently..)
    m_width=j;
    m_height=j;

    wxLogDebug(wxString::Format(wxT("Adjusting pBuffer width and height to %ix%i"),j,j));

    // Create Texture
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, 4,  m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    // Get the context to work with (must be valid, will die otherwise)
    saveHdc = wglGetCurrentDC();
    saveHglrc = wglGetCurrentContext();

    int pixelFormats;
	int intAttrs[32] ={
        WGL_RED_BITS_ARB,8,
        WGL_GREEN_BITS_ARB,8,
        WGL_BLUE_BITS_ARB,8,
        WGL_ALPHA_BITS_ARB,8,
        WGL_DRAW_TO_PBUFFER_ARB, GL_TRUE,
        WGL_BIND_TO_TEXTURE_RGBA_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
        WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
        WGL_DOUBLE_BUFFER_ARB,GL_FALSE,
        0
    }; // 0 terminate the list

	unsigned int numFormats = 0;

    if (!wglChoosePixelFormatARB) {
        throw GLException(wxT("No wglChoosePixelFormatARB available"));
    }
	if (!wglChoosePixelFormatARB( saveHdc, intAttrs, NULL, 1, &pixelFormats, &numFormats)) {
        throw GLException(wxT("WGL: pbuffer creation error: Couldn't find a suitable pixel format."));
	}
    if (numFormats==0) {
        throw GLException(wxT("WGL: pbuffer creation error: numFormats==0"));
    }
    const int attributes[]= {
        WGL_TEXTURE_FORMAT_ARB,
        WGL_TEXTURE_RGBA_ARB, // p-buffer will have RBA texture format
        WGL_TEXTURE_TARGET_ARB,
        WGL_TEXTURE_2D_ARB,
        0
    }; // Of texture target will be GL_TEXTURE_2D

    //wglCreatePbufferARB(hDC, pixelFormats, pbwidth, pbheight, attributes);
	hBuffer=wglCreatePbufferARB(saveHdc, pixelFormats, m_width, m_height, attributes );
	if (!hBuffer) {
	    throw GLException(wxT("wglCreatePbufferARB failure"));
	}

    hdc=wglGetPbufferDCARB( hBuffer );
	if (!hdc) {
	    throw GLException(wxT("wglGetPbufferDCARB failure"));
	}

    hGlRc=wglCreateContext(hdc);
	if (!hGlRc) {
	    throw GLException(wxT("wglCreateContext failure"));
	}

        //printf("PBuffer size: %d x %d\n",w,h);
    int w,h;
	wglQueryPbufferARB(hBuffer, WGL_PBUFFER_WIDTH_ARB, &w);
	wglQueryPbufferARB(hBuffer, WGL_PBUFFER_HEIGHT_ARB, &h);

	// compare w & h to m_width & m_height.

 /*   wglMakeCurrent(hdc,hGlRc);

    glEnable(GL_TEXTURE_2D);		      // Enable Texture Mapping
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); // enable transparency

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT); */


    // switch back to the screen context
	wglMakeCurrent(saveHdc, saveHglrc);
    // So we can share the main context
    wglShareLists(saveHglrc, hGlRc);

    // Jump back to pBuffer ready for rendering
    //wglMakeCurrent(hdc, hGlRc);
}
pBufferWGL::~pBufferWGL()
{
    if (hGlRc) wglDeleteContext(hGlRc);
    if (hBuffer) wglReleasePbufferDCARB(hBuffer, hdc);
    if (hBuffer) wglDestroyPbufferARB(hBuffer);
}
void pBufferWGL::SelectBuffer()
{
    wglMakeCurrent(hdc, hGlRc);
}
void pBufferWGL::SelectContext(wxGLCanvas * glc)
{
    wglMakeCurrent(saveHdc, saveHglrc);
}

bool pBufferWGL::InitGLStuff()
{

    // Technically don't need this wrangling with glewInit being actually called now....
    //wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	char *ext = NULL;

	if (wglGetExtensionsStringARB)
		ext = (char*)wglGetExtensionsStringARB( wglGetCurrentDC() );
	else {
	    wxLogError(wxT("Unable to get address for wglGetExtensionsStringARB!"));
	    return false;
	}

	if (strstr(ext, "WGL_ARB_pbuffer" ) == NULL) {
        wxLogError(wxT("WGL_ARB_pbuffer extension was not found"));
        return false;
	} else {
		//wglCreatePbufferARB    = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
		//wglGetPbufferDCARB     = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
		//wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
		//wglDestroyPbufferARB   = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
		//wglQueryPbufferARB     = (PFNWGLQUERYPBUFFERARBPROC)wglGetProcAddress("wglQueryPbufferARB");

		if (!wglCreatePbufferARB || !wglGetPbufferDCARB || !wglReleasePbufferDCARB ||
			!wglDestroyPbufferARB || !wglQueryPbufferARB) {
            wxLogError(wxT("One or more WGL_ARB_pbuffer functions were not found"));
            return false;
		}
	}

	// WGL_ARB_pixel_format
	if (strstr( ext, "WGL_ARB_pixel_format" ) == NULL) {
	    wxLogError(wxT("WGL_ARB_pixel_format extension was not found"));
		return false;
	} else {
		wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
		wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
		wglChoosePixelFormatARB      = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

		if (!wglGetExtensionsStringARB || !wglCreatePbufferARB || !wglGetPbufferDCARB) {
			wxLogError(wxT("One or more WGL_ARB_pixel_format functions were not found"));
			return false;
		}
	}


	return true;

}

#elif defined(__UNIX__) && !defined(__WXMAC__)

GLXContext real_shared_context=NULL;

pBufferGLX::pBufferGLX(int width, int height,wxGLContext * gc)
:pBuffer(width,height,gc)
{

    /*int ms=MAX(width,height);
    int j;
    for (int i=4;i<32;i++) {  // min size 16x16.. probably a usless size.
        j=1 << i;
        if (j >= ms) break;
    }
    j <<= 2;

    m_width=j;
    m_height=j; */

    hBuffer=0;
    m_context=0,m_shared=0;
    int ret;
    display=NULL;
    GLXFBConfig *fbc=NULL;

    int attrib[]={
        GLX_PBUFFER_WIDTH,m_width,
        GLX_PBUFFER_HEIGHT,m_height,
        GLX_PRESERVED_CONTENTS, True
    };


    //bool fbc_dontfree=false;
/*#if wxCHECK_VERSION(2,9,0)
    display=wxGetX11Display();
    fbc = GetGLXFBConfig(); // wxGLCanvas call
    fbc = &fbc[0];
    pBuffer=glXCreatePbuffer(display, fbc[0], attrib );

#else */
    display=(Display *)wxGetDisplay();
    int fb_attrib[] = {
        GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        None
    };
//#if wxCHECK_VERSION(2,9,0)
    //fbc = gc->GetGLXFBConfig(); // wxGLCanvas call
    //fbc = &fbc[0];
    //fbc_donefree=true;
//#else
    //fbc=(GLXFBConfig*)gc->ChooseGLFBC(fb_attrib); // This adds glcanvas's attribs
    fbc=glXChooseFBConfig(display, DefaultScreen(display), fb_attrib, &ret);
//#endif

    hBuffer=glXCreatePbuffer(display, *fbc, attrib );
//#endif

    if (hBuffer == 0) {
        wxLogError(wxT("pBuffer not availble"));
    }



    // This function is not in WX sourcecode yet :(
    //shared=shared_context->GetGLXContext();

    if (real_shared_context) m_shared=real_shared_context; // Only available after redraw.. :(
    else {
        // First render screws up unless we do this..
        m_shared=m_context= glXCreateNewContext(display,*fbc,GLX_RGBA_TYPE, NULL, True);
    }

    if (m_shared == 0) {
        wxLogError(wxT("Context not availble"));
    }

#if !wxCHECK_VERSION(2,9,0)
    //XFree(fbc);
#endif

    glXMakeCurrent(display,hBuffer,m_shared);

    //UseBuffer(true);
}
pBufferGLX::~pBufferGLX()
{
    if (m_context) glXDestroyContext(display,m_context); // Destroy the context only if we created it..
    if (hBuffer) glXDestroyPbuffer(display, hBuffer);
}
void pBufferGLX::SelectBuffer()
{
    if (glXMakeCurrent(display,hBuffer,m_shared)!=True) {
        wxLogError(wxT("Couldn't make pBuffer current"));
    }
}
#elif defined(__DARWIN__) || defined(__WXMAC__)

pBufferAGL::pBufferAGL(int width, int height,wxGLContext * gc)
:pBuffer(gc)
{
    m_width=width;
    m_height=height;
    GLint attribs[] = {
        AGL_RGBA,
        AGL_DOUBLEBUFFER,
        AGL_ACCELERATED,
        AGL_NO_RECOVERY,
        AGL_DEPTH_SIZE, 24,
        AGL_NONE
    };
    /*display2 = CGDisplayIDToOpenGLDisplayMask(CGMainDisplayID());

    display = GetMainDevice();
    wxRect windowRect = { 100, 100, 100 + width, 100 + height };
    WindowRef window;

    pixelFormat = aglChoosePixelFormat( display2, 1, attribs );
    pbContext = aglCreateContext( pixelFormat, NULL );
    aglDestroyPixelFormat(pixelFormat);

    aglCreatePBuffer( width, height, GL_TEXTURE_2D,GL_RGBA, 0, &pbuffer );

    // bind pbuffer
    virtualScreen = aglGetVirtualScreen( pbContext );
    aglSetCurrentContext( pbContext );
    aglSetPBuffer( pbContext, pbuffer, 0, 0, virtualScreen );
    // draw into pbuffer

    drawPBuffer();


    // window pixelformat and context setup and creation
    pixelFormat = aglChoosePixelFormat( &display, 1, attribs );
    context = aglCreateContext( pixelFormat, NULL );
    aglDestroyPixelFormat( pixelFormat );
    aglSetCurrentContext( context );

    // window creation
    CreateNewWindow(kDocumentWindowClass, kWindowStandardDocumentAttributes | kWindowStandardHandlerAttribute, &windowRect, &window);

    //Alternative Rendering Destinations 111
    SetWindowTitleWithCFString( window, CFSTR("AGL PBuffer Texture") );
    ActivateWindow(window, true);
    ShowWindow(window);
    // bind context to window
    aglSetDrawable( context, GetWindowPort(window) );
    aglUpdateContext( context );
    // initialize window context & draw window
    GLuint texid;
    init( context, pbuffer, &texid );
    drawWindow( context, texid ); */
    // stub event loop
}

pBufferAGL::~pBufferAGL()
{
    /*aglSetCurrentContext( NULL );
    aglDestroyContext( context );
    aglDestroyContext( pbContext );
    aglDestroyPBuffer( pbuffer ); */

}
void pBufferAGL::UseBuffer(bool b)
{
/*    if (b) {
        wglMakeCurrent(hdc, hGlRc);
    } else {
        wglMakeCurrent(saveHdc, saveHglrc);
    } */
}

#endif
