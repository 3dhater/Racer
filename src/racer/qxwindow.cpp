/*
 * QXWindow - encapsulates XWindows
 * 20-02-98: Created! (from qwindow.cpp)
 * 09-09-99: Going for the colormap problems with USEX controls.
 * 11-03-00: Visual info data is stored for later on. This was done
 * for MT's Media Deck, which needed a copy of the GLXContext so it can
 * draw to the same QXWindow (Q_BC) from more than 1 thread.
 * 26-07-00: Support for Win32
 * 06-09-00: Support for Fullscreen viewing on Win32
 * BUGS:
 * - QXWindow::GetDepth() returns 32 just like that.
 * - QXWindow::SetName() probably loses some memory
 * FUTURE:
 * - Create() should create a shared colormap
 * (C) MarketGraph/RVG
 */

#include <qlib/window.h>
#include <qlib/canvas.h>
#include <qlib/cursor.h>
#include <qlib/app.h>
#include <qlib/opengl.h>
#include <stdio.h>
#include <GL/glu.h>
//#include <stdlib.h>
#include <X11/Xmu/StdCmap.h>
#include <X11/Xatom.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Define to get a double buffered shell by default
#define DBLBUF_SHELL

// Define to force software rendering on Win32
// This is to get around a Banshee bug on Win2K
//#define USE_SOFTWARE_RENDERING

// Use GLX to get default visual?
// The returned visual is the DefaultVisual() on O2 when using 32x32
// but 0x26 or such (12-bit) when using 16x16 (Lingo machine)
// With X11, I try to avoid that
//#define DEFAULT_VISUAL_USING_GLX

// If XPARENT is defined, parenting will take in X11 (so you can connect
// your QShells relatively.
#define USE_XPARENT

// Report pixelformat data when opening window? (Win32)
#define USE_REPORT_PIXELFORMAT

/***********
* QXWindow *
***********/
QXWindow::QXWindow(QXWindow *iparent,int x,int y,int wid,int hgt)
  : QDrawable()
{
  parent=iparent;
  gl=0;
  xwindow=0;
  xflags=ZBUFFER;
#ifdef WIN32
  // Win32 specific attributes
  hDC=0;
  hWnd=0;
#endif
  ipos.x=x; ipos.y=y;
  ipos.wid=wid; ipos.hgt=hgt;
  if(parent)
  { 
#ifdef OBS
    // Offset creation location to that of the parent
    ipos.x+=parent->GetX();
    ipos.y+=parent->GetY();
#endif
  }
  //internalx=(QXWindowInternal*)qcalloc(sizeof(*internalx));
  //QASSERT_V(internalx);
  //internalx->win=0;
#ifdef OBS
  app->AddQXWindow(this);
#endif
}
QXWindow::~QXWindow()
{
  if(gl)delete gl;
#ifdef WIN32
qdbg("QXWindow dtor; IsFS=%d\n",IsFullScreen());
  // Destroy window
  if(IsFullScreen())
  {
    // Restore desktop settings
    ChangeDisplaySettings(NULL,0);
  }
  DestroyWindow(hWnd);
#else
  // SGI/X11
  if(xwindow)
  { Display *d=app->GetDisplay()->GetXDisplay();
    XDestroyWindow(d,xwindow);
    XSync(d,FALSE);
    XSync(d,FALSE);
  }
#endif
}

/***********
* CREATION *
***********/
void QXWindow::PrefWM(bool wm)
// Must be called BEFORE create
// Gives window to XWM for managing (borders etc)
{ if(wm)
    xflags|=XMANAGED;
  else
    xflags&=~XMANAGED;
}
void QXWindow::PrefDefaultVisual()
// Use default visual when creating
{
  xflags|=DEFVISUAL;
}
void QXWindow::PrefFullScreen()
// Turn on fullscreen when creating
{
  xflags|=FULLSCREEN;
}
void QXWindow::EnableZBuffer()
{
  xflags|=ZBUFFER;
}
void QXWindow::DisableZBuffer()
{
  xflags&=~ZBUFFER;
}

static Bool WaitForNotify(Display *d,XEvent *e,char *arg)
{
  qdbg("QW: WaitForNotify %d\n",e->type);
#ifdef WIN32
  // Don't wait
  return TRUE;
#else
  return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
#endif
}
static void ListWins()
{
  QWindow *w;
  QWindowManager *wm;
  int n,i;
  wm=app->GetWindowManager();
  n=wm->GetNoofWindows();
  qdbg("-- Window list --\n");
  for(i=0;i<n;i++)
  { w=wm->GetWindowN(i);
    qdbg("Win %8s %d,%d %dx%d, Window=%x\n",w->ClassName(),
      w->GetX(),w->GetY(),w->GetWidth(),w->GetHeight(),
      w->GetQXWindow()->GetXWindow());
  }
}
Colormap getShareableColormap(XVisualInfo * vi)
// To avoid taking 1 hardware color map per X window, this function
// tries to get a shared color map that it places in a property of the server,
// so any new windows might share the color map.
{
#if defined(WIN32) || defined(linux)
  // No colormaps
  return 0;
#else
  // SGI
  Status status;
  XStandardColormap *standardCmaps;
  Colormap cmap;
  int i, numCmaps;
  Display *dpy=XDSP;

  // Check for existing default pseudocolor map (default visual)
  if(vi->visual->visualid==DefaultVisual(XDSP,0)->visualid)
  {
//qdbg("getCMap: use default cmap\n");
    return DefaultColormap(XDSP,0);
  }
//qdbg("visual=%x, defvis=%x\n",vi->visual->visualid,DefaultVisual(XDSP,0)->visualid);

  // Be lazy; using DirectColor too involved for this example.
  if(vi->c_class==DirectColor)
  { qdbg("vi class = DirectColor\n");
  }
  //if(vi->c_class==PseudoColor)qdbg("vi class = PseudoColor\n");
  if (vi->c_class != TrueColor)
    qerr("no support for non-TrueColor visual");
  // If no standard colormap but TrueColor, just make an unshared one
  status = XmuLookupStandardColormap(dpy, vi->screen, vi->visualid,
    vi->depth, XA_RGB_DEFAULT_MAP,
    False,              /* Replace. */
    True);              /* Retain. */
  if (status == 1) {
    status = XGetRGBColormaps(dpy, RootWindow(dpy, vi->screen),
      &standardCmaps, &numCmaps, XA_RGB_DEFAULT_MAP);
    if (status == 1)
      for (i = 0; i < numCmaps; i++)
        if (standardCmaps[i].visualid == vi->visualid) {
          cmap = standardCmaps[i].colormap;
          XFree(standardCmaps);
          return cmap;
        }
  }
  cmap=XCreateColormap(dpy,RootWindow(dpy,vi->screen),vi->visual,AllocNone);
  return cmap;
#endif
}

#ifdef WIN32

static void EnableOpenGL(HWND hWnd, HDC * hDC,HGLRC *hRC)
// Set pixel format for window so OpenGL will be possible
{
  PIXELFORMATDESCRIPTOR pfd;
  int iFormat;

  // Get a device context (DC)
  *hDC=GetDC(hWnd);
  if(!*hDC)
  { qerr("(QXWindow) EnableOpenGL() can't get HDC for window");
    return;
  }

  // Set the pixel format for the DC
  ZeroMemory(&pfd,sizeof(pfd));
  pfd.nSize=sizeof( pfd );
  pfd.nVersion=1;
  // Double buffered with OpenGL, no GDI
  pfd.dwFlags=PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType=PFD_TYPE_RGBA;
  // Note that 24 color bits (RGB only) and 24-bit depth gives best
  // results on Geforce2MX/nVidia6.50 drivers. Set depth bits to
  // 32 bits and you get severe Z-fighting in Modeler for example.
  pfd.cColorBits=24;
  pfd.cDepthBits=24;
  pfd.iLayerType=PFD_MAIN_PLANE;
  iFormat=ChoosePixelFormat(*hDC,&pfd);
  if(!iFormat)
  { qerr("(QXWindow) ChoosePixelFormat() failed");
    return;
  }
#ifdef USE_SOFTWARE_RENDERING
// Force software rendering
iFormat=5;
DescribePixelFormat(*hDC,iFormat,sizeof(pfd),&pfd);
#endif

#ifdef USE_REPORT_PIXELFORMAT
  {
    // Info on pixel format
//qdbg("QXWindow: chosen pixel format=%d\n",iFormat);
    PIXELFORMATDESCRIPTOR pfdNew;
    DescribePixelFormat(*hDC,iFormat,sizeof(pfdNew),&pfdNew);
    int genFormat=pfdNew.dwFlags&PFD_GENERIC_FORMAT;
    int genAcc=pfdNew.dwFlags&PFD_GENERIC_ACCELERATED;
    if(genFormat&&!genAcc)
    { qdbg("%d: Software pixel format",iFormat);
    } else if(genFormat&&genAcc)
    { qdbg("%d: Hardware pixel format - MCD",iFormat);
    } else
    { qdbg("%d:Hardware pixel format - ICD");
    }
  }
#endif

  if(!SetPixelFormat( *hDC, iFormat, &pfd ))
  { qerr("(QXWindow) SetPixelFormat() failed");
    return;
  }

//#define ND_RAW_TEST_HERE
#ifdef ND_RAW_TEST_HERE
  //HGLRC hRC;
  *hRC=wglCreateContext(*hDC);
  wglMakeCurrent(*hDC,*hRC);

  // Test
  bool bQuit=FALSE;
  float theta=0.0;
  MSG msg;
  MessageBox(0,"Raw test start","test",MB_OK);
  while ( !bQuit ) {

    // check for messages
    if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {

      // handle or dispatch messages
      if ( msg.message == WM_QUIT ) {
        bQuit = TRUE;
      } else {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
      }

    } else {

      // OpenGL animation code goes here

      glClearColor( 0.5f, 0.0f, 0.0f, 0.0f );
      glClear( GL_COLOR_BUFFER_BIT );

      glPushMatrix();
      glRotatef( theta, 0.0f, 0.0f, 1.0f );
      glBegin( GL_TRIANGLES );
      glColor3f( 1.0f, 0.0f, 0.0f ); glVertex2f( 0.0f, 1.0f );
      glColor3f( 0.0f, 1.0f, 0.0f ); glVertex2f( 0.87f, -0.5f );
      glColor3f( 0.0f, 0.0f, 1.0f ); glVertex2f( -0.87f, -0.5f );
      glEnd();
      glPopMatrix();

      SwapBuffers( *hDC );

      theta += 1.0f;

    }

  }
  MessageBox(0,"Raw test end","test",MB_OK);
  app->Exit(0);
#endif
///// RAW TEST

//#define ND_NO_TEST_HERE
#ifdef ND_NO_TEST_HERE
  *hRC=wglCreateContext(*hDC);
  wglMakeCurrent(*hDC,*hRC);
  /*glDrawBuffer(GL_FRONT);
  glDrawBuffer(GL_BACK);
  glClearColor(.8,.7,.5,1);
  glClear(GL_COLOR_BUFFER_BIT);*/
      glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
      glClear( GL_COLOR_BUFFER_BIT );

      glPushMatrix();
      glRotatef( 25.0f, 0.0f, 0.0f, 1.0f );
      glBegin( GL_TRIANGLES );
      glColor3f( 1.0f, 0.0f, 0.0f ); glVertex2f( 0.0f, 1.0f );
      glColor3f( 0.0f, 1.0f, 0.0f ); glVertex2f( 0.87f, -0.5f );
      glColor3f( 0.0f, 0.0f, 1.0f ); glVertex2f( -0.87f, -0.5f );
      glEnd();
      glPopMatrix();
  SwapBuffers(*hDC);
  int i;
  glFlush();
  for(i=0;i<100000000;i++);
  glFinish();
  for(i=0;i<100000000;i++);
  SwapBuffers(hDC);
  for(i=0;i<100000000;i++);
  glFinish();
  for(i=0;i<100000000;i++);
#endif
}
void TestRC(HWND hWnd, HDC * hDC,HGLRC *hRC)
{
  int i;
  //return;
  HGLRC hrc,hrc2;
  HDC hdc;
  hdc=*hDC;
  hrc=wglCreateContext(hdc);
  hrc2=wglCreateContext(hdc);
  wglMakeCurrent(hdc,hrc);
  wglMakeCurrent(hdc,0);
  wglMakeCurrent(0,0);
  wglMakeCurrent(hdc,hrc2);

  *hRC=wglCreateContext(*hDC);
if(!wglMakeCurrent(*hDC,*hRC))qerr("TEstRC mc");
if(!wglMakeCurrent(NULL,NULL))qerr("TestRC mc0");
if(!wglMakeCurrent(NULL,NULL))qerr("TestRC mc0");
  //*hRC=wglCreateContext(*hDC);
if(!wglMakeCurrent(*hDC,*hRC))qerr("TEstRC mc2_");
i=4;
if(!wglMakeCurrent(NULL,NULL))qerr("TestRC mc0");
i=2;
if(!wglMakeCurrent(*hDC,*hRC))qerr("TEstRC mc2_");
i=3;
//if(!wglMakeCurrent(0,0))qerr("TestRC mc2_0");
i=4;
//wglDeleteContext(*hRC);
//*hRC=0;
//return;

  wglMakeCurrent(*hDC,*hRC);
      glClearColor( 0.2f, 0.5f, 0.3f, 0.0f );
      glClear( GL_COLOR_BUFFER_BIT );

      glPushMatrix();
      glRotatef( 25.0f, 0.0f, 0.0f, 1.0f );
      glBegin( GL_TRIANGLES );
      glColor3f( 1.0f, 0.0f, 0.0f ); glVertex2f( 0.0f, 1.0f );
      glColor3f( 0.0f, 1.0f, 0.0f ); glVertex2f( 0.87f, -0.5f );
      glColor3f( 0.0f, 0.0f, 1.0f ); glVertex2f( -0.87f, -0.5f );
      glEnd();
      glPopMatrix();
  SwapBuffers(*hDC);
  glFlush();
  for(i=0;i<100000000;i++);
      glClearColor( 0.2f, 0.5f, 0.3f, 0.0f );
      glClear( GL_COLOR_BUFFER_BIT );
  SwapBuffers(*hDC);
  for(i=0;i<100000000;i++);

/*  glFinish();
  for(i=0;i<100000000;i++);
  SwapBuffers(hDC);
  for(i=0;i<100000000;i++);
  glFinish();
  for(i=0;i<100000000;i++);*/
  wglMakeCurrent(0,0);
  wglDeleteContext(*hRC);
}

#endif

#ifdef WIN32
void ReSizeGLScene(GLsizei width, GLsizei height)
// Resize And Initialize The GL Window
{
	if(height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}
#endif

bool QXWindow::Create()
// Create actual X11 window based on information given by user
{
  bool direct;

#ifdef WIN32
  //
  // WIN32: Creating a window
  //
//qdbg("QXWindow:Create; %d,%d %dx%d\n",ipos.x,ipos.y,ipos.wid,ipos.hgt);
//MessageBox(0,"QXWindow::Create()","qlib",MB_OK);
  HINSTANCE hInstance=0;
  static bool fClassRegistered;
  DWORD style,exStyle;
  HWND  parentHwnd;
  RECT  wRect;
  int   addX,addY;

  hInstance=GetModuleHandle(0);
  if(!fClassRegistered)
  {
    // Register window class for this window
    WNDCLASS wc;
    wc.style=CS_OWNDC|CS_VREDRAW|CS_HREDRAW;
    wc.lpfnWndProc=QEventWindowProc;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hInstance=hInstance;
    wc.hIcon=LoadIcon( NULL, IDI_WINLOGO); //APPLICATION );
    wc.hCursor=LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground=0; // (HBRUSH)GetStockObject( BLACK_BRUSH );
    wc.lpszMenuName=NULL;
    wc.lpszClassName="OpenGL";
    if(!RegisterClass(&wc))
    {
      qerr("Can't register window class");
    }
    fClassRegistered=TRUE;
  }

  wRect.left=0;
  wRect.top=0;
  wRect.right=ipos.wid;
  wRect.bottom=ipos.hgt;

  if(IsFullScreen())
  {
    // Switch to fullscreen mode
    DEVMODE dm;
    QFullScreenSettings *fss=app->GetFullScreenSettings();
    memset(&dm,0,sizeof(dm));
    dm.dmSize=sizeof(dm);
    dm.dmPelsWidth=fss->width;
    dm.dmPelsHeight=fss->height;
    dm.dmBitsPerPel=fss->bits;
    dm.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
    // Try to set selected mode
    // CDS_FULLSCREEN gets ride of start bar (and doesn't resize windows
    // on desktop)
    if(ChangeDisplaySettings(&dm,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
    {
      qwarn("QXWindow: Can't set fullscreen mode %dx%dx%d",
        fss->width,fss->height,fss->bits);
    }
    addX=addY=0;
  } else
  { 
    if(!parent)
    {
      // Window size adjustment on Win32 (to get the CLIENT area to
      // be as big as requested)
      //addX=::GetSystemMetrics(SM_CXBORDER);       // No size
      addX=::GetSystemMetrics(SM_CXFRAME);       // Sizing
      addY=::GetSystemMetrics(SM_CYCAPTION);
      addY+=::GetSystemMetrics(SM_CYDLGFRAME);
      // Empirical
      addX+=4;
      addY+=6;
      // Adjust window rect
      wRect.right+=addX;
      wRect.bottom+=addY;
    }
  }

  // Determine window style
  if(!parent)
  {
    // A top level window
    if(IsFullScreen())
    {
      // WS_POPUP leaves out a border
      style=WS_POPUP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
      exStyle=WS_EX_APPWINDOW;
      // Hide the cursor?
      //ShowCursor(FALSE);
      ShowCursor(TRUE);
      // Adjust window to get rid of border so we are REALLY fullscreen
      AdjustWindowRectEx(&wRect,style,FALSE,exStyle);
    } else
    {
      // Old style (Racer0.1)
      //style=WS_CAPTION|WS_POPUPWINDOW|WS_VISIBLE|WS_SIZEBOX; 
      style=WS_OVERLAPPEDWINDOW/* |WS_CLIPSIBLINGS|WS_CLIPCHILDREN*/;
//style=WS_DLGFRAME;
     exStyle=WS_EX_APPWINDOW|WS_EX_WINDOWEDGE;
    }
    parentHwnd=0;
  } else
  {
    style=WS_CHILD /*|WS_CLIPSIBLINGS|WS_CLIPCHILDREN*/;
    //style=WS_OVERLAPPEDWINDOW;
    exStyle=0;
    parentHwnd=parent->GetHWND();
  }
  // Adjust window to fully encompass the desired size
  //AdjustWindowRectEx(&wRect,style,FALSE,exStyle);
/*qdbg("CreateWin: parentHwnd=%x, @%d,%d %dx%d\n",
parentHwnd,ipos.x,ipos.y,wRect.right-wRect.left,wRect.bottom-wRect.top);*/
  hWnd=CreateWindowEx(exStyle,"OpenGL",/*app->GetAppName(),*/ "QLib",
    style,
    ipos.x,ipos.y,
    wRect.right-wRect.left,wRect.bottom-wRect.top,
    parentHwnd,NULL,hInstance,NULL);
  // Push hWnd through to the OS-independent variable of a 'window id'
  // A 'Window' should perhaps be a union or at least the #bits that
  // each supported OS uses to represent a window.
  // Say, a QOSWindowID typedef. And use that in event handling and such
  // (always only in the background in Q)
  xwindow=(Window)hWnd;

  // Make it an OpenGL window    
  // FUTURE: Check xflags for ZBUFFER, DOUBLEBUFFERED (everything is dblbuf'd?)
  HGLRC hRC;
  EnableOpenGL(hWnd,&hDC,&hRC);
//ShowWindow(hWnd,SW_SHOW);TestRC(hWnd,&hDC,&hRC);

  // Create OpenGL context
  direct=TRUE;
  if(parent)gl=new QGLContext(this,vi,parent->GetGLContext(),direct);
  else      gl=new QGLContext(this,vi,0,direct);
  // Activate
  gl->Select();
#ifdef FUTURE_CHECK_GLRC
  if(FALSE)
  { qerr("QXWindow: can't create rendering context");
    // Go on nevertheless (black windows)
  }
#endif

  // Show and boost its priority a bit
  ShowWindow(hWnd,SW_SHOW);
  // Bring it to the top; on X11, this is the default. On Win32, this
  // does not seem to be the case.
  BringWindowToTop(hWnd);
  //SetForegroundWindow(hWnd);
  SetFocus(hWnd);

  // Set size
  ReSizeGLScene(ipos.wid,ipos.hgt);

#else
  //
  // SGI GLX Create window
  //
#ifdef DBLBUF_SHELL
#ifdef linux
  static int attribListZBuf[]=
  { //GLX_RGBA,None
    GLX_RGBA,GLX_DOUBLEBUFFER,GLX_RED_SIZE,1,GLX_DEPTH_SIZE,1, None
  };
#else
  static int attribListZBuf[]=
  { GLX_RGBA,GLX_DOUBLEBUFFER,GLX_RED_SIZE,1,GLX_DEPTH_SIZE,1, None };
#endif
  static int attribList[]=
  { GLX_RGBA,GLX_DOUBLEBUFFER,GLX_RED_SIZE,1, None };
  //static int attribListDV[]=		// Visual for X GC's
  //{ GLX_DOUBLEBUFFER,GLX_RED_SIZE,1,None };
  static int attribListDV[]=		// Visual for X GC's
  //{ /*GLX_RGBA,*/ GLX_DOUBLEBUFFER,GLX_RED_SIZE,1,None };
  { GLX_RED_SIZE,0,None };
  static int attribListMinimal[]=
  { GLX_RGBA,GLX_DOUBLEBUFFER,GLX_RED_SIZE,1, None };
#else
  static int attribList[]={ GLX_RGBA,GLX_RED_SIZE,1, None };
#endif
  XVisualInfo viFull;
  Visual  *visual;
  Colormap cmap;
  int      screen,depth;
  XSetWindowAttributes swa;
  Window win,pWin;
  Display *dpy=app->GetDisplay()->GetXDisplay();
  //GLXContext cx;
  //XEvent event;
  int *list;			// Which attrib list is used

//qdbg("QXWindow::Create()");

  // Get an appropriate visual
#ifdef OBS
  if(xflags&DEFVISUAL)
  { vi=0;
    screen=0;
    visual=DefaultVisual(dpy,screen);
    depth=DefaultDepth(dpy,screen);
  } else
#endif
  {
    vi=0;
    if(xflags&DEFVISUAL)
    {
//qdbg("xflags&DEFVISUAL\n");
#ifdef DEFAULT_VISUAL_USING_GLX
      // OpenGL preferred
      vi=glXChooseVisual(dpy,DefaultScreen(dpy),attribListDV);
#else
      // X11; try to match visual 0x20, 8 planes, PseudoColor
      // A bit too hard, I'm afraid (for other machines)
      // Can't do this anymore, since vi is stored in the window structure
      // Need to maintain viFull in that case (!)
      if(XMatchVisualInfo(XDSP,0,8,PseudoColor,&viFull))
      { vi=&viFull;
        qerr("QXWindow 11-3-00; store viFull in your QXWindow class!");
      }
#endif
    } else
    {
//qdbg("No DEFVISUAL\n");
      if(xflags&ZBUFFER)list=attribListZBuf;
      else              list=attribList;
//qdbg("  list=%p, Z=%p, L=%p\n",list,attribListZBuf,attribList);
      vi=glXChooseVisual(dpy,DefaultScreen(dpy),list);
//qdbg("  vi=%p\n",vi);
    }
    if(!vi)
    { qwarn("QXWindow:Create; no optimal visual found");
      vi=glXChooseVisual(dpy,DefaultScreen(dpy),attribListMinimal);
    }
    if(!vi)
    { qerr("QXWindow:Create; glXChooseVisual failed");
      return FALSE;
    }

    // Create the GL context
    direct=TRUE;
//printf("Try direct ctx\n");
   retry:
    if(parent)gl=new QGLContext(this,vi,parent->GetGLContext(),direct);
    else      gl=new QGLContext(this,vi,0,direct);
    if(!gl->GetGLXContext())
    { if(direct==TRUE)
      {
        qwarn("QXWindow: Can't create direct rendering ctx; trying indirect");
        direct=FALSE;
        goto retry;
      } else
      { qerr("QXWindow: can't create rendering context");
        // Go on nevertheless (black windows)
      }
    }

    screen=vi->screen;
    depth=vi->depth;
    visual=vi->visual;
//qdbg("QXWindow:Create(); visual id = 0x%x\n",visual->visualid);
//qdbg("QXWindow:Create(); def id = 0x%x\n",DefaultVisual(XDSP,0)->visualid);
  }
  // Create a color map
#ifdef linux
  cmap=XCreateColormap(dpy, RootWindow(dpy,screen),visual,AllocNone);
#else
  cmap=getShareableColormap(vi);
#endif

 //qdbg("1; cmap=%x\n",cmap);
  // Select parent if any
#ifdef USE_XPARENT
  if(parent!=0&&parent->IsX())
  { //qdbg("QShell: parent is QXWindow\n");
    //pWin=((QXWindow*)parent)->GetXWindow();
    pWin=parent->GetXWindow();
  } else pWin=RootWindow(dpy,screen);
#else
  pWin=RootWindow(dpy,screen);
#endif

  // create a window
  swa.colormap=cmap;
  swa.border_pixel=0;
  swa.event_mask=PointerMotionMask|KeyPressMask|KeyReleaseMask
    |ButtonPressMask|ButtonReleaseMask|StructureNotifyMask
    |EnterWindowMask|LeaveWindowMask|ExposureMask
    ;
  // We'll handle things, thank you
  if(xflags&XMANAGED)
    swa.override_redirect=0;
  else
    swa.override_redirect=1;
#ifdef OBS
  // There are some (major) differences when 4Dwm manages our window
  // (dragging, resizing, iconising etc)
  if(swa.override_redirect==0)
    xflags|=XMANAGED;
#endif
//qdbg("QXWindow:Create(); window %d,%d %dx%d\n",ipos.x,ipos.y,ipos.wid,ipos.hgt);
  win=XCreateWindow(dpy,pWin,
    ipos.x,ipos.y,ipos.wid,ipos.hgt,
    0,depth,InputOutput,visual,
    CWOverrideRedirect|CWBorderPixel|CWColormap|CWEventMask,&swa);

#ifdef FUTURE_SIZE_AND_POSITION_MANAGED
  // Size hints for managed windows (to get actual positioning)
  // (code taken from Usenet article for 4Dwm managed windows)
  XSizeHints sizehints;
  sizehints.x=ipos.x;
  sizehints.y=ipos.y;
  sizehints.width=ipos.wid;
  sizehints.height=ipos.hgt;
  sizehints.flags=USSize|USPosition;
  XSetNormalHints(dpy,win,&sizehints);
  XSetStandardProperties(dpy,win,name,name,None,(char**)NULL,0,&sizehints);
#endif

  // Show
 //qdbg("3; win=0x%x\n",win);
  XMapWindow(dpy, win);
#ifdef ND_LOSES_EVENTS
  // Wait for window to be actually present
  XIfEvent(dpy,&event,WaitForNotify,(char*)win);
#endif
//#ifdef ND_TEST
  // Get map request through
  XSync(app->GetDisplay()->GetXDisplay(),FALSE);
  // Get MapNotify through
  XSync(app->GetDisplay()->GetXDisplay(),FALSE);
//#endif
  // Remember
  xwindow=win;
  //FocusKeys();
  // Make us the current focus
  //XSetInputFocus(dpy,win,RevertToParent,CurrentTime);

#endif
// end SGI GLX

  // Setup the OpenGL context for a default view
  gl->Select(this);             // Unnecessary if viewport is set below
  // Setup the context so we are in 2D view
  gl->Setup2DWindow();
  // No fancy old contents
  gl->DrawBuffer(GL_FRONT);
  gl->Clear(GL_COLOR_BUFFER_BIT);
  gl->DrawBuffer(GL_BACK);
  return TRUE;
}
void QXWindow::Destroy()
{
  qerr("QXWindow:Destroy NYI");
}

//
// Information
//

// Inherited from QDrawable
GLXDrawable QXWindow::GetGLXDrawable()
{ return xwindow;
}
Drawable QXWindow::GetDrawable()
{ return xwindow;
}

int QXWindow::GetX()
// XWindow may have moved or been relocated by the Window Manager
// Notice: when override_redirect==1, XGetGeometry returns actual root location
// if not, it returns 0 for the first shell
{ 
#ifdef WIN32
  //qerr("QXWindow::GetX() nyi/win32");
  return ipos.x;
#else
  Window root;
  int x,y;
  unsigned int wid,hgt,bwid,dep;
  //qdbg("QXWindow::GetX\n");
  if(xwindow)
  { XGetGeometry(app->GetDisplay()->GetXDisplay(),xwindow,
      &root,&x,&y,&wid,&hgt,&bwid,&dep);
    //qdbg("  GetX: %d,%d %dx%d\n",x,y,wid,hgt);
    return x;
  } else return 0;
#endif
}
int QXWindow::GetY()
{ 
#ifdef WIN32
  //qerr("QXWindow::GetY() nyi/win32");
  return ipos.y;
#else
  Window root;
  int x,y;
  unsigned int wid,hgt,bwid,dep;
  if(xwindow)
  { XGetGeometry(app->GetDisplay()->GetXDisplay(),xwindow,
      &root,&x,&y,&wid,&hgt,&bwid,&dep);
    return y;
  } else return 0;
#endif
}
int QXWindow::GetWidth()
{ return ipos.wid;
}
int QXWindow::GetHeight()
{ return ipos.hgt;
}
int QXWindow::GetDepth()
{ return 32;			// 32-bit visuals fake
}
void QXWindow::GetPos(QRect *r)
{ r->x=ipos.x;
  r->y=ipos.y;
  r->wid=ipos.wid;
  r->hgt=ipos.hgt;
}

//
// Modifications
//

void QXWindow::SetTitle(string s)
// Shells that are managed by the Window Manager may have a title
{
#ifdef WIN32
  qerr("QXWindow::SetTitle() nyi/win32");
#else
  // X Properties
  XTextProperty windowName;

  if(xflags&XMANAGED)
  { // Give the window a name
    if(XStringListToTextProperty(&s,1,&windowName)==0)
    { qerr("no mem for X WM_NAME property");
    } else
    { XSetWMName(XDSP,xwindow,&windowName);
      XStoreName(XDSP,xwindow,s);
    }
  }
#endif
}
void QXWindow::SetIconName(string s)
// Window manager iconisable windows may have an icon name
{ QASSERT_V(s);
#ifdef WIN32
  qerr("QXWindow::SetIconName() nyi/win32");
#else
  if(xflags&XMANAGED)
    XSetIconName(XDSP,xwindow,s);
#endif
}

void QXWindow::Move(int x,int y)
{
#ifdef WIN32
  qerr("QXWindow::Move() nyi/win32 @%d,%d",x,y);
  MoveWindow(hWnd,x,y,ipos.wid,ipos.hgt,FALSE);
  ipos.x=x; ipos.y=y;
#else
  if(xwindow)
  { //qerr("QXWindow::Move NYI");
    XMoveWindow(app->GetDisplay()->GetXDisplay(),xwindow,x,y);
    ipos.x=x; ipos.y=y;
  } else
  { // Initial position
    ipos.x=x; ipos.y=y;
  }
#endif
}
void QXWindow::Size(int newWid,int newHgt)
{
#ifdef WIN32
  qerr("QXWindow::Size() nyi/win32");
#else
  /*Window root;
  int x,y;
  unsigned int wid,hgt,bwid,dep;*/
  XWindowAttributes attribs;

  if(xwindow)
  { //qerr("QXWindow::Move NYI");
    /*XGetGeometry(app->GetDisplay()->GetXDisplay(),xwindow,
      &root,&x,&y,&wid,&hgt,&bwid,&dep);*/
    XGetWindowAttributes(XDSP,xwindow,&attribs);
//qdbg("QXW:Size(%d,%d); current size=%dx%d\n",newWid,newHgt,wid,hgt);
    if(attribs.width!=newWid||attribs.height!=newHgt)
    { // This will generate a RESIZE event
      XResizeWindow(app->GetDisplay()->GetXDisplay(),xwindow,newWid,newHgt);
    }
    // Remember new size
    ipos.wid=newWid; ipos.hgt=newHgt;
    // Setup GL context to map entire window
    gl->Setup2DWindow();
  } else
  { // Preferred size for Create()
    ipos.wid=newWid; ipos.hgt=newHgt;
  }
#endif
}

void QXWindow::Raise()
{
#ifdef WIN32
  qerr("QXWindow::Raise() nyi/win32");
#else
  XRaiseWindow(XDSP,xwindow);
#endif
}
void QXWindow::Lower()
{
#ifdef WIN32
  qerr("QXWindow::Lower() nyi/win32");
#else
  XLowerWindow(XDSP,xwindow);
#endif
}

bool QXWindow::IsX()
{ //qwarn("QXWindow::IsX OBSOLETE CALL");
  return TRUE;			// Duh, after 20-2-98
}
bool QXWindow::IsCreated()
{
  if(xwindow)return TRUE;
  return FALSE;
}

bool QXWindow::IsDirect()
// Returns TRUE if this window is managed by the window manager (4Dwm)
// This has a major impact on moving/sizing/exposing etc.
// Also on event coordinates
{
  if(xflags&XMANAGED)return FALSE;
  return TRUE;
}
bool QXWindow::IsFullScreen()
// Returns TRUE if this window is being shown (preferably) fullscreen
{
  if(xflags&FULLSCREEN)return TRUE;
  return FALSE;
}

void QXWindow::SetCursor(QCursor *c)
{
#ifdef WIN32
  //qerr("QXWindow::SetCursor() nyi/win32");
#else
  XDefineCursor(app->GetDisplay()->GetXDisplay(),xwindow,
    c->GetXCursor());
#endif
}

void QXWindow::FocusKeys(int time)
// Sets X keyboard focus to our X window, so may receive
// input although the mouse is not even over it
// Note the usage of 'time' which is the time which relates
// to WHEN you would have the focus changed. If it is in the future
// then nothing will happen. If it is in the past, then it will only
// change the focus if focus hasn't been changed between 'time' and
// CurrentTime. All this in X11 millisecs. Not to be used lightly
// in anything but QLib.
{ //printf("FocusKeys()\n");
#ifdef WIN32
  qerr("QXWindow::FocusKeys() nyi/win32");
#else
  if(time==0)time=CurrentTime;
  XSetInputFocus(app->GetDisplay()->GetXDisplay(),xwindow,
    RevertToParent,time);
#endif
}

void QXWindow::Swap()
{ //qdbg("QXWindow::Swap\n");
  if(gl)
    gl->Swap();
}

void QXWindow::Map()
{ //XEvent event;
#ifdef WIN32
  qerr("QXWindow::Map() nyi/win32");
#else
  XMapWindow(app->GetDisplay()->GetXDisplay(),xwindow);
#ifdef ND_LOSES_EVENTS
  // Wait for window to be actually present
  XIfEvent(app->GetDisplay()->GetXDisplay(),
    &event,WaitForNotify,(char*)xwindow);
#endif
#endif
}

void QXWindow::Unmap()
{
#ifdef WIN32
  qerr("QXWindow::Unmap() nyi/win32");
#else
  XUnmapWindow(app->GetDisplay()->GetXDisplay(),xwindow);
#endif
}
