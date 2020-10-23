/*
 * QOpenGL - wrapper around OpenGL / GLX contexts / WGL
 * 28-04-97: Created!
 * 03-03-98: Bug removed; if a context was created, then deleted, it might
 * still be cached as current (in curglc), which could lead to spurious
 * problems when a following new Context resulted in the same pointer value!
 * 28-02-99: Major revisions in class hierarchy; QDrawable etc.
 * 27-07-00: Win32 WGL support. A bit woven here & there.
 * (C) MarketGraph/RVG
 */

#include <qlib/opengl.h>
#include <qlib/app.h>
#include <qlib/display.h>
#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#endif
#include <GL/glx.h>
#include <GL/glu.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Shortcut for context selection
#define SEL		if(curglc!=this)Select()

// Global current GL context for fast context switching checks
static QGLContext *curglc;

/********************
* Extension support *
********************/
bool QGLContext::IsExtensionSupported(cstring extension)
// A relatively slow way of checking for an extension, do
// not use in critical speed code
// Taken from www.opengl.org
{
  const GLubyte *extensions=NULL;
  const GLubyte *start;
  GLubyte *where,*terminator;

  /* Extension names should not have spaces. */
  where=(GLubyte *)strchr(extension, ' ');
  if(where||*extension=='\0')
    return 0;
  extensions=glGetString(GL_EXTENSIONS);

  // It takes a bit of care to be fool-proof about parsing the
  // OpenGL extensions string. Don't be fooled by sub-strings,
  // etc.
  start=extensions;
  while(1)
  {
    where=(GLubyte *)strstr((const char *)start,extension);
    if(!where)
      break;
    terminator=where+strlen(extension);
    if(where==start||*(where-1)==' ')
      if(*terminator==' '||*terminator=='\0')
        return 1;
    start=terminator;
  }
  return 0;
}
bool QGLContext::IsExtensionSupported(int flags)
// Fast way to check precached extensions. More than
// 1 extension may be checked at the same time.
{
  if((extFlags&flags)==flags)return TRUE;
  return FALSE;
}
void QGLContext::CheckExtensions()
// Check for a number of often-used extensions
{
  // Done this already?
  if(flags&F_EXTS_CHECKED)return;

  extFlags=0;
  if(IsExtensionSupported("GL_ARB_multitexture"))
    extFlags|=ARB_multitexture;
  if(IsExtensionSupported("GL_ARB_texture_env_add"))
    extFlags|=ARB_texture_env_add;
  if(IsExtensionSupported("GL_ARB_texture_env_combine"))
    extFlags|=ARB_texture_env_combine;
  if(IsExtensionSupported("GL_EXT_blend_color"))
    extFlags|=EXT_blend_color;
  if(IsExtensionSupported("GL_EXT_separate_specular_color"))
    extFlags|=EXT_separate_specular_color;
  if(IsExtensionSupported("GL_EXT_texture_filter_anisotropic"))
    extFlags|=EXT_texture_filter_anisotropic;
  if(IsExtensionSupported("GL_NV_fog_distance"))
    extFlags|=NV_fog_distance;
//qlog(QLOG_INFO,"extFlags=0x%x\n",extFlags);

  flags|=F_EXTS_CHECKED;
}

//
// Global functions
//
QGLContext *GetCurrentQGLContext()
{ return curglc;
}
void SetCurrentQGLContext(QGLContext *glc)
// Expert function to influence the current GLContext
{ curglc=glc;
}

/************
* Ctor/dtor *
************/
QGLContext::QGLContext(QXWindow *iwin,XVisualInfo *vi,QGLContext *share,
  bool direct)
{
  GLXContext shctx;

  //qdbg("***** QGLCtx ctor %p\n",this);
  //internal=(QGLContextInternal*)qcalloc(sizeof(*internal));
  flags=0;
  extFlags=0;
  ctx=0;
  //win=iwin;
  drawable=iwin;
  drawableRead=iwin;
  mProjection.SetIdentity();
  mModelView.SetIdentity();

#ifdef WIN32
  // Create WGL context
  //qerr("QGLContext ctor nyi/win32");
  // create the render context (RC)
  hRC=wglCreateContext(iwin->GetHDC());
  // Share display lists
  if(share)
    wglShareLists(share->GetHRC(),hRC);
/*
char buf[80];
sprintf(buf,"QGLCtx: iwin=%p, hdc=%p, hRC=%p",iwin,iwin->GetHDC(),hRC);
MessageBox(0,buf,"QGLContext ctor",MB_OK);
HDC hDC=iwin->GetHDC();
Select(); Select(); Select();
//TestRC(iwin->GetHWND(),&hDC,&hRC);
*/
  //wglMakeCurrent(hDC, *hRC );
#else
  if(share)shctx=share->ctx;
  else     shctx=0;
  ctx=glXCreateContext(XDSP,vi,shctx,direct);
#endif
}
QGLContext::QGLContext(GLXContext _ctx)
// Create from existing context
{
#ifdef WIN32
  qerr("QGLContext ctor(GLXContext ctx) nyi/win32");
#endif
  flags=F_USER_CTX;
  ctx=_ctx;
  drawable=0;
  drawableRead=0;
}

QGLContext::~QGLContext()
{ //qdbg("***** QGLCtx dtor %p\n",this);
  //if(internal->glxContext)
  // Destroy context if we own it
#ifdef WIN32
  //qerr("QGLContext dtor nyi/win32");
#else
  // GLX
  if(ctx!=0&&((flags&F_USER_CTX)==0))
    glXDestroyContext(XDSP,ctx);
#endif
  //if(internal)qfree(internal);
  // 3-3-98; Cancel this context if it is active!!
  if(curglc==this)
    curglc=0;
}

GLXContext QGLContext::GetGLXContext()
{ return ctx;				// internal->glxContext;
}

#ifdef OBS
void QGLContext::SetWindow(QXWindow *nwin)
{ win=nwin;
}
// GetWindow is inline
#endif

void QGLContext::SetDrawable(QDrawable *draw)
// Sets drawable to paint on
{
  drawable=draw;
}

void QGLContext::Select(QDrawable *draw,QDrawable *read,int flags)
// Uses this context as the current context for this thread
// If 'flags&SILENT', then the context is not cached. This is used
// in QMVOut for example, where the movie previewing should not interfere
// with QLib's global (and single-thread) current OpenGL_Context caching.
// Therefore, a movie preview in QMVOut's video thread uses QGLContext::SILENT
// to avoid mixing up contexts with the main thread, which would cause cores.
{
  // Already selected? (caches current context for the main thread)
  if(!(flags&SILENT))
    if(curglc==this)return;

//qdbg("QGLCtx:Select; this=%p, draw=%p\n",this,draw);

  if(draw)drawable=draw;
  drawableRead=read;
#ifdef WIN32
  // Make current
  QXWindow *xw;
  //xw=dynamic_cast<QXWindow*>(drawable);
  xw=(QXWindow*)drawable;
/*char buf[128];
sprintf(buf,"xw=%p, hdc=%p, hRC=%p",xw,xw->GetHDC(),hRC);
MessageBox(0,buf,"trace",MB_OK);*/
  if(xw)
  {
//qlog(QLOG_INFO,"QGLContext:Select: hdc=%p, hrc=%p",xw->GetHDC(),hRC);
    wglMakeCurrent(xw->GetHDC(),hRC);
  }
  else qerr("QGLContext:Select(); drawable doesn't seem to be a QXWindow");
#else
  // GLX
  if(drawableRead==0)
  { glXMakeCurrent(XDSP,drawable->GetGLXDrawable(),ctx);
  } else
  { // Use both DRAW & READ
#if defined(linux)
    qerr("QGLContext::Select() with read is NYI/Linux");
#else
    glXMakeCurrentReadSGI(XDSP,draw->GetGLXDrawable(),read->GetGLXDrawable(),
      ctx);
#endif
  }
#endif

  if(!(flags&F_EXTS_CHECKED))
  {
    // Cache often-used extensions
    CheckExtensions();
  }

  if(!(flags&SILENT))
    curglc=this;
//qdbg("QGLCtx:Select RET\n");
}
void QGLContext::Swap()
{
  SEL;
#ifdef WIN32
  QXWindow *xw;
  xw=(QXWindow*)drawable;
  if(xw)SwapBuffers(xw->GetHDC());
#else
  // GLX
  glXSwapBuffers(XDSP,drawable->GetGLXDrawable());
#endif
}

/*****************
* WINDOW SUPPORT *
*****************/
void QGLContext::Setup2DWindow()
// Use defaults settings to provide a parallel view in 2D
{ int wid,hgt;
  QASSERT_V(drawable);

  SEL;
//qdbg("QGLContext:Setup2DWindow\n");
  //qdbg("  xw=%x, win=%p\n",win->GetXWindow(),win);
  wid=drawable->GetWidth();
  hgt=drawable->GetHeight();
//qdbg("  drawable %p = %dx%d\n",drawable,wid,hgt);
  //qdbg(" vp\n");
  Viewport(0,0,wid,hgt);
  MatrixMode(GL_PROJECTION);
  LoadIdentity();
  MatrixMode(GL_MODELVIEW);
  LoadIdentity();
  //qdbg(" ortho\n");
  Ortho(0,(qdouble)wid,0,(qdouble)hgt,-1,1);
  //qdbg("QGLContext:Setup2DWindow() RET\n");
}

void QGLContext::Setup3DWindow(int dist)
// Generate default 3D view, camera at z=-$dist results in pixel-pixel mapping
{
  int wid,hgt;
  GLfloat idDistance;
  QASSERT_V(drawable);

  SEL;
  idDistance=dist;			// Convert to float
  wid=drawable->GetWidth();
  hgt=drawable->GetHeight();
//qdbg("QGLCtx:3D: %dx%d drawable\n",wid,hgt);
//QShowGLErrors("QGLCtx:1");
  // 3D view
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
//QShowGLErrors("QGLCtx:2");
  //GLfloat idDistance=1000;              // Distance (Z) to fully cover window
  //gluPerspective(60,(GLfloat)720/(GLfloat)576,0.1,1000);
  glFrustum(-wid/idDistance/2,wid/idDistance/2,
            -hgt/idDistance/2,hgt/idDistance/2, 1,100000);
//QShowGLErrors("QGLCtx:3");
  glMatrixMode(GL_MODELVIEW);
//QShowGLErrors("QGLCtx:4");
  glLoadIdentity();

  // Setup camera distance; objects at this distance map 1:1 on the drawable
  glTranslatef(0,0,-idDistance);
//QShowGLErrors("QGLCtx:5");
}

/*********
* OPENGL *
*********/
void QGLContext::Viewport(int x,int y,int wid,int hgt)
// Sets OpenGL's viewport.
// Also stores the values in a cache, to avoid having to call glGet()
// to get back at them.
{ 
//qdbg("QGLCtx:Viewport()\n");
//qdbg("  curglc=%p, this=%p\n",curglc,this);
  SEL;
//qdbg("  vprt\n");
  glViewport(x,y,wid,hgt);

  // Store values in the cache
  viewport[0]=x;
  viewport[1]=y;
  viewport[2]=wid;
  viewport[3]=hgt;
//qdbg("QGLCtx:Viewport() RET\n");
}

void QGLContext::DrawBuffer(int d)
{ SEL;
  //printf("GLC:DrawBuffer %d ctx %p\n",d,glXGetCurrentContext());
  glDrawBuffer(d);
}
void QGLContext::ReadBuffer(int d)
{ SEL;
  //printf("GLC:ReadBuffer %d ctx %p\n",d,glXGetCurrentContext());
  glReadBuffer(d);
}
void QGLContext::Clear(int buffers)
{ SEL;
  glClear(buffers);
}

void QGLContext::MatrixMode(int m)
{ SEL;
  glMatrixMode(m);
}
void QGLContext::LoadIdentity()
{ SEL;
  glLoadIdentity();
}
void QGLContext::Ortho(qdouble l,qdouble r,qdouble t,qdouble b,
  qdouble n,qdouble f)
// Left/right/top/bottom/near/far
{ SEL;
  glOrtho(l,r,t,b,n,f);
}

void QGLContext::Disable(int n)
{ SEL;
  glDisable(n);
}
void QGLContext::Enable(int n)
{ SEL;
  glEnable(n);
}
void QGLContext::BlendFunc(int sf,int df)
{ SEL;
  glBlendFunc(sf,df);
}

/****************************
* High end OpenGL functions *
****************************/
void QGLContext::SetSeparateSpecularColor(bool yn)
// Attempt to disable/enable separate specular lighting.
// This gives highlights even on textured polygons.
// May not be supported by your include files or driver
{
#ifdef GL_EXT_separate_specular_color
//qlog(QLOG_INFO,"QGLContext:SetSSC(); supported=%d,yn=%d\n",
//IsExtensionSupported(EXT_separate_specular_color),yn);

  if(IsExtensionSupported(EXT_separate_specular_color))
  {
     if(yn)
     {
       // Turn separate specular color on
       glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT,
         GL_SEPARATE_SPECULAR_COLOR_EXT);
     } else
     {
       glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT,
         GL_SINGLE_COLOR_EXT);
     }
  }
#endif
}
void QGLContext::SetMaxAnisotropy(float v)
// Tries to set the maximum anisotropy value, if it
// exists. The more, the better the filtering
// (GL_LINEAR_MIPMAP_LINEAR or GL_LINEAR_MIPMAP_NEAREST)
{
#ifdef GL_EXT_texture_filter_anisotropic
  if(IsExtensionSupported(EXT_texture_filter_anisotropic))
  {
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,
      v);
  }
#endif
}

/*****************************
* Projection matrix juggling *
*****************************/
void QGLContext::Perspective(float fovy,float aspect,float zNear,float zFar)
// Like gluPerspective(), only stores resulting matrix so its cached
// to avoid a glGet(GL_PROJECTION_MATRIX) and stall the graphics pipe.
{
  float f;
  DMatrix4 mPrj;
  //DMatrix4 mProjection;
  float *m=mProjection.GetM();

//qdbg("Persp(fov=%.2f, aspect=%.2f, zNear=%f, zFar=%f\n",fovy,aspect,zNear,zFar);
  //glMatrixMode(GL_PROJECTION);
  //glLoadIdentity();
  //gluPerspective(fovy,aspect,zNear,zFar);
//glGetFloatv(GL_PROJECTION_MATRIX,mPrj.GetM());
//mPrj.DbgPrint("gluPerspective");
//mPrj.SetIdentity();
  f=1.0f/tan((fovy/2.0f)/57.29578f);
  //f=cotan(fovy/2.0f);
  m[0]=f/aspect;
  m[1]=m[2]=m[3]=m[4]=m[6]=m[7]=m[8]=m[9]=m[12]=m[13]=0;
  m[5]=f;
  m[10]=(zFar+zNear)/(zNear-zFar);
  m[11]=-1;
  m[14]=(2.0f*zFar*zNear)/(zNear-zFar);
  m[15]=0;
  glMultMatrixf(m);
//mProjection.DbgPrint("myPerspective");
}

/****************************
* ModelView matrix juggling *
****************************/
/** LookAt code taken from the OpenGL FAQ by Paul Martz **/
static void cross(float dst[3],float srcA[3],float srcB[3])
{
  dst[0]=srcA[1]*srcB[2]-srcA[2]*srcB[1];
  dst[1]=srcA[2]*srcB[0]-srcA[0]*srcB[2];
  dst[2]=srcA[0]*srcB[1]-srcA[1]*srcB[0];
}
static void normalize(float vec[3])
{
  const float sqLen=vec[0]*vec[0]+vec[1]*vec[1]+vec[2]*vec[2];
  const float i=1.0f/(float)sqrt(sqLen);
  vec[0]*=i;
  vec[1]*=i;
  vec[2]*=i;
}
void QGLContext::LookAt(float eyex,float eyey,float eyez,
    float atx,float aty,float atz,
    float upx,float upy,float upz)
// Does about the same as gluLookAt(), but stores the resulting
// matrix in the QGLContext matrix modelview cache, to avoid having
// to call glGet() later on.
// Matrix is:
// | [xaxis] 0 |
// |  [up]   0 |
// | [-at]   0 |
// | [eye]   0 |
{
  float *m=mModelView.GetM();

//#ifdef ND_MAN_PAGE
  float f[3],s[3],u[3],up[3];
  DMatrix4 mTranslate;

  // From the gluLookAt man page
  f[0]=atx-eyex; f[1]=aty-eyey; f[2]=atz-eyez;
  normalize(f);

  // Make a copy of the up vector
  up[0]=upx; up[1]=upy; up[2]=upz;
  normalize(up);

  cross(s,f,up);
  cross(u,s,f);

  m[0]=s[0]; m[4]=s[1]; m[8]=s[2];
  m[1]=u[0]; m[5]=u[1]; m[9]=u[2];
  m[2]=-f[0]; m[6]=-f[1]; m[10]=-f[2];
  m[15]=1;
  m[3]=m[7]=m[11]=m[12]=m[13]=m[14]=0;

  // Translate to eye
  mTranslate.SetIdentity();
  mTranslate.GetM()[12]=-eyex; mTranslate.GetM()[13]=-eyey;
  mTranslate.GetM()[14]=-eyez;
//mTranslate.DbgPrint("Translate");
  mModelView.Multiply(&mTranslate);

//mModelView.DbgPrint("mModelView-myLookAt");

  // Multiply current matrix stack
  glMultMatrixf(m);
  //glLoadMatrixf(m);

//return;
//#endif

#ifdef PAUL_MARTZ_OPENGL_FAQ
  //float *xaxis=&m[0],*up=&m[4],*at=&m[8];
  float xaxis[3],up[3],at[3];

  // Compute look-at vector
  at[0]=atx-eyex; at[1]=aty-eyey; at[2]=atz-eyez;
  normalize(at);

  // Create positive X axis of transformed object
  cross(xaxis,at,up);
  normalize(xaxis);

  // Calculate up vector
  cross(up,xaxis,at);
  at[0]=-at[0];
  at[1]=-at[1];
  at[2]=-at[2];

  // Make the matrix
  m[0]=xaxis[0];
  m[4]=xaxis[1];
  m[8]=xaxis[2];
  m[1]=up[0];
  m[5]=up[1];
  m[9]=up[2];
  m[2]=at[0];
  m[6]=at[1];
  m[10]=at[2];

  // Fill the rest of the matrix
  m[3]=m[7]=m[11]=0.0f;
  m[14]=eyex; m[13]=eyey; m[12]=eyez;
  m[15]=1.0f;
#ifdef OBS_TRANSPOSED
  m[3]=m[7]=m[11]=0.0f;
  m[12]=eyex; m[13]=eyey; m[14]=eyez;
  m[15]=1.0f;
#endif

  // Multiply current matrix stack
  glMultMatrixf(m);
#endif
}

/**********
* Project *
**********/
void QGLContext::Project(const DVector3 *obj,DVector3 *win)
// Like gluProject(), only uses the cached matrices for efficiency.
// Returns the window coordinates in 'win'.
// See 'man gluproject' to see what it exactly does.
{
  DVector3 vEye,vPrj;             // Projected vector
  int *vp;

  // Convert object coordinate into eye into projected (clip coords?)
  mModelView.TransformVector(obj,&vEye);
  mProjection.TransformVector(&vEye,&vPrj);
  vp=viewport;
  //glGetIntegerv(GL_VIEWPORT,vp);

#ifdef ND_DBG
obj->DbgPrint("obj");
vEye.DbgPrint("vEye");
vPrj.DbgPrint("vPrj");

mModelView.DbgPrint("ModelView");
mProjection.DbgPrint("Projection");
#endif

  // Calculate window coordinate
  win->x=vp[0]+vp[2]*(vPrj.x+1.0f)/2.0f;
  win->y=vp[1]+vp[3]*(vPrj.y+1.0f)/2.0f;
  win->z=(vPrj.z+1.0f)/2.0f;

#ifdef OBS_CHECK_ALGO
qdbg("QGLContext:Project()\n");
win->DbgPrint("win");

  //
  // Slow method to compare with GLU
  //
  double mModel[16],mPrj[16];
  int    viewport[4];
  GLdouble wx,wy,wz;
  GLdouble ox,oy,oz;

  // Retrieve matrices and viewport settings
  glGetDoublev(GL_MODELVIEW_MATRIX,mModel);
  glGetDoublev(GL_PROJECTION_MATRIX,mPrj);
  glGetIntegerv(GL_VIEWPORT,viewport);
int i;
qdbg("ModelView:\n");
for(i=0;i<16;i++){ qdbg("%.3f ",mModel[i]); if((i&3)==3)qdbg("\n"); }
qdbg("Projection:\n");
for(i=0;i<16;i++){ qdbg("%.3f ",mPrj[i]); if((i&3)==3)qdbg("\n"); }
  // Reverse projection
  ox=obj->x;
  oy=obj->y;
  oz=obj->z;
  if(!gluProject(ox,oy,oz,mModel,mPrj,viewport,&wx,&wy,&wz))
  {
    qwarn("gluProject failed");
    win->SetToZero();
    return;
  }

  win->x=(dfloat)wx;
  win->y=(dfloat)wy;
  win->z=(dfloat)wz;
win->DbgPrint("win gluProject\n");
#endif
}

