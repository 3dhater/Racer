// qlib/opengl.h
//
// This file should never be included directly by Q applications, since
// it is used internally by qcanvas.cpp and other Q modules.
// Directly rendering (by apps) using QGLContext is NOT advised.
// Note this file includes a lot of GL/X stuff!
//

#ifndef __QLIB_QOPENGL_H
#define __QLIB_QOPENGL_H

#include <qlib/window.h>
//#include <qlib/pbuffer.h>
#include <X11/Xlib.h>			// XVisualInfo
#include <d3/matrix.h>
#ifdef WIN32
#include <winsock.h>
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glx.h>
#ifdef WIN32
// Additional OpenGL extensions
#include <egl/egl.h>
//#include <GL/glext.h>
#endif

struct QGLContextInternal;

class QGLContext : public QObject
// A GLX context, using OpenGL. Call it an OpenGL wrapper.
// Very near to the machine; should be used only indirectly
// (through QLib's canvas)
// Implements OpenGL calls, and buffers the current context
// Does NOT do high calls (like Blit()).
// Combines OpenGL and GLX contexts into one class
// It does not match up with a specific window; one GLContext may be directed
// at different windows/drawables (but only 1 at one time)
{
 public:
  enum Flags
  {
    F_USER_CTX=1,			// User has allocated context, not us
    F_EXTS_CHECKED=2                    // Extensions searched
  };
  enum SelectFlags
  {
    SILENT=1                            // Don't cache selection; use for
                                        // contexts in non-main threads!
  };
  enum Extensions
  {
    // Corresponding to GL_<string> strings found in glGetExtensions()
    ARB_multitexture=1,
    ARB_texture_env_add=2,
    ARB_texture_env_combine=4,
    EXT_blend_color=8,
    EXT_separate_specular_color=16,
    EXT_texture_filter_anisotropic=32,
    NV_fog_distance=0x40,
    ARB_xxx
  };

  //const int F_USER_CTX=1;		// User has allocated context, not us

 protected:
  //QXWindow  *win;			// Current drawable
  QDrawable *drawable;			// Current draw drawable
  QDrawable *drawableRead;		// Current read drawable
  int        flags;			// Internal flags
  int        extFlags;                  // Extensions prechecked & stored
 public:
  GLXContext ctx;			  // The OpenGL/GLX context
#ifdef WIN32
  HGLRC      hRC;       // Win32 WGL OpenGL context
#endif

  // Cached matrices for projection and modelview matrix (and viewport)
  // (if created using functions in this class!)
  // This is done to avoid glGet(MATRIX) which may need to flush the pipeline.
  DMatrix4 mProjection,mModelView;
  int      viewport[4];

 public:
  // Create context for an XWindow
  QGLContext(QXWindow *win,XVisualInfo *vi,QGLContext *share,
    bool direct=TRUE);
  // Create context wrap out of user-generated context
  QGLContext(GLXContext ctx);
  ~QGLContext();

  // Extensions
  void CheckExtensions();
  bool IsExtensionSupported(int flags);
  bool IsExtensionSupported(cstring s);

  // Attribs
  QDrawable *GetDrawable(){ return drawable; }
  void SetDrawable(QDrawable *draw);
  DMatrix4 *GetProjectionMatrix(){ return &mProjection; }
  DMatrix4 *GetModelViewMatrix(){ return &mModelView; }
  int      *GetViewport(){ return viewport; }

  // GLX calls/interface (glXMakeCurrent() variants)
  GLXContext GetGLXContext();
#ifdef WIN32
  HGLRC      GetHRC(){ return hRC; }
#endif

  //void Select(QXWindow *win=0);
  void Select(QDrawable *draw=0,QDrawable *read=0,int flags=0);
  void Swap();

  // Standard drawable support; default Window projections
  void Setup2DWindow();
  void Setup3DWindow(int idDistance=1000);

  // OpenGL calls
  void Viewport(int x,int y,int wid,int hgt);
  void DrawBuffer(int d);
  void ReadBuffer(int d);
  void Clear(int buffers);

  void MatrixMode(int m);
  void LoadIdentity();
  void Ortho(qdouble left,qdouble right,qdouble top,qdouble bottom,
    qdouble fnear,qdouble ffar);

  void Disable(int f);
  void Enable(int f);
  void BlendFunc(int sf,int df);

  // High end OpenGL (for extension functions mainly)
  void SetSeparateSpecularColor(bool yn);
  void SetMaxAnisotropy(float max);
  void Project(const DVector3 *obj,DVector3 *win);
  void Unproject(const DVector3 *win,DVector3 *obj);

  // Projection matrix
  void Perspective(float fovy,float aspect,float zNear,float zFar);

  // ModelView matrix
  void LookAt(float eyex,float eyey,float eyez,
    float atx,float aty,float atz,
    float upx,float upy,float upz);
};

// Global
QGLContext *GetCurrentQGLContext();
void SetCurrentQGLContext(QGLContext *glc);		// EXPERT ONLY!

#endif

