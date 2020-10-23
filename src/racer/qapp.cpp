/*
 * QApp
 * 30-09-96: Created!
 * 28-04-97: X11 base
 * NOTES:
 * - Provides basic functionality that most Q programs will need; a display
 *   and a current screen to opens windows on (if wanted).
 * - Is an event manager (Run())
 * FUTURE:
 * - Multiple screen support? Multiple display (Xserver) support?
 * BUGS:
 * - StateManager's VCR buttons don't get exposed during start (needs
 *   explicit repaints now)
 * - 'sm' is not exposed correctly (no expose event?!)
 * - RunPoll() should paint 'sm' if there and bug of sm still exists
 * (C) MarketGraph/RVG
 */

#include <qlib/app.h>
#include <qlib/event.h>
#include <qlib/display.h>
#include <qlib/cursor.h>
#include <qlib/keys.h>
//#include <stdio.h>
#include <stdlib.h>
#include <dmedia/dmedia.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <qlib/debug.h>
DEBUG_ENABLE

//#define USE_XEVENT_LOOP		// Define to catch events ourselves
#define USE_QEVENT_LOOP			// Use Q's own event handling

#define MAX_QXWINDOW	10		// Number of QXWindow to accept

// Global; 1 instance of QApp in every application
QApp *app;

/*************
* X11 ERRORS *
*************/
#ifndef WIN32
static int (*oldXhandler)(Display *,XErrorEvent *);
static int myXhandler(Display *dsp,XErrorEvent *e)
// X11 error handler
{
  char buf[128],msg[128];
  int  r;

  // Display message
  XGetErrorText(dsp,e->error_code,buf,sizeof(buf));
  qerr("X error %d; %s\n",e->error_code,buf);
  //XGetErrorText(dsp,e->request_code,buf,sizeof(buf));
  buf[0]=0;
  qdbg("  major opcode: %d (%s)\n",e->request_code,buf);
  //XGetErrorText(dsp,e->minor_code,buf,sizeof(buf));
  qdbg("  minor opcode: %d (%s)\n",e->minor_code,buf);
  sprintf(msg,"%d.%d",e->request_code,e->minor_code);
  XGetErrorDatabaseText(dsp,"qapp",msg,"def",buf,sizeof(buf));
  qdbg("  dbase text  : %s\n",buf);
  qdbg("  serial      : %d\n",e->serial);
  qdbg("  XID         : 0x%x\n",e->resourceid);
  qdbg("  type        : %d\n",e->type);
  // Dump core for stack trace
  abort();
qdbg("QApp BUG; still here\n");
  return oldXhandler(dsp,e);
}
static int myXIOhandler(Display *dsp)
{
  return 0;
}
#endif
static void RedirectXErrors()
{
#if defined(__sgi)
  //int (*oldh)(Display *,XErrorEvent *);
  oldXhandler=XSetErrorHandler(myXhandler);
  //XSetIOErrorHandler(myXIOhandler);
#endif
}

/********
* CDTOR *
********/
static void ShowCol(XColor *c)
{
  qdbg("  pixel=%d, rgb=%d,%d,%d, flags=%d\n",
    c->pixel,c->red,c->green,c->blue,c->flags);
}
static bool AllocCol(string name,XColor *col)
{
#ifdef WIN32
  // No color support on Win32
  return FALSE;
#else
  Colormap cmap;
  XColor dummy;
  cmap=DefaultColormap(XDSP,0);
  if(!XAllocNamedColor(XDSP,cmap,name,col,&dummy))
  { qerr("Can't allocate default color '%s'\n",name);
    return FALSE;
  }
  //qdbg("QApp: AllocCol: col, dummy=\n");
  //ShowCol(col); ShowCol(&dummy);
  return TRUE;
#endif
}
QApp::QApp(cstring _appName,cstring displayName)
// ctor
{ //qdbg("QApp (%s)\n",name);

  QASSERT(_appName);

  //
  // Search for debugging commands in environment
  //
  cstring dbg;
  dbg=getenv("DBG_TRACE");
  if(dbg)
  { if(!strcmp(dbg,"ON"))DBG_TRACE_ON()
  }

  DBG_C("ctor");

  app=this;
  appName=qstrdup(_appName);
  shell=0; cursShell=0;
  bc=0; cursBC=0;
  sm=0;
  sysFont=0;
  sysFontNP=0;
  winmgr=0;
  fm=0; vfm=0; ffm=0;
  gl=0;
  vs=0;
  modFlags=0;
  mouseX=mouseY=0;
  mouseButtons=0;
  // Default fullscreen size
  fss.width=640;
  fss.height=480;
  fss.bits=16;

  idleProc=0;
  evProc=0;
  exitProc=0;

  rUI=rBC=0;
  createFlags=0;

  RedirectXErrors();

  // Open display (fatal if display can't be opened)
  dsp=new QDisplay(displayName);
  if(!XDSP)
  { qerr("Can't open X11 display (%s)",displayName);
    exit(0);
  }
  scr=0;

  // Get default pixel values for our user interface stuff (default visual)
  XColor   col[10];
  // White, black, ltgry, mdgray dkgray
  char *colName[10]={ "grey100","grey0","grey65","grey37","grey20","red",0 };
  int i;
  for(i=0;i<10;i++)
  { if(!colName[i])break;
    if(!AllocCol(colName[i],&col[i]))
    { uiPixel[i]=BlackPixel(XDSP,0);
    } else
    { uiPixel[i]=col[i].pixel;
      //qdbg("%s: pixel %d\n",colName[i],uiPixel[i]);
    }
  }
  //qdbg("QApp ctor RET\n");
}
QApp::~QApp()
// Never called, I think
{
  Destroy();
}
void QApp::Destroy()
// Destroy allocated resources
{
  // Make sure Fullscreen windows die
  if(QSHELL)delete QSHELL;
  QFREE(appName);
  QDELETE(sysFont);
  QDELETE(sysFontNP);
  QDELETE(sm);
  QDELETE(winmgr);
  QDELETE(dsp);
  QEventQuit();
}

/***********
* CREATION *
***********/
void QApp::PrefUIRect(QRect *r)
{
  rUI=r;
}
void QApp::PrefBCRect(QRect *r)
{ rBC=r;
}
void QApp::PrefNoBC()
{ createFlags|=NO_BC;
}
void QApp::PrefNoUI()
{ createFlags|=NO_UI;
}
void QApp::PrefNoStates()
{ createFlags|=NO_SM;
}
void QApp::PrefWM(bool yn)
{ if(yn)createFlags|=PREF_WM;
  else  createFlags&=~PREF_WM;
}
void QApp::PrefFullScreen()
// Make QSHELL fullscreen (Win32)
{ createFlags|=FULLSCREEN;
}
void QApp::Create()
// Create default elements, based on preferences
{
  QRect *r;
  QASSERT_V(winmgr==0);	// Duplicate call to QApp::Create

#ifndef WIN32
  // Fullscreen not supported on SGI
  if(createFlags&FULLSCREEN)
  {
    qwarn("Fullscreen not supported on SGI");
  }
#endif

  // Initialize Q's event stream
  QEventInit();

  winmgr=new QWindowManager();
  QColor colBG(94,94,94);		// grey37
  winmgr->SetBackgroundColor(&colBG);

  if(!(createFlags&NO_UI))
  {
    // Create a shell for the UI
    if(rUI)r=rUI;
    else   r=new QRect(0,0,1280,1024);	// Default=fullscreen
    shell=new QShell(0,r->x,r->y,r->wid,r->hgt);
    if(createFlags&PREF_WM)
      shell->PrefWM(TRUE);
    if(createFlags&FULLSCREEN)
      shell->PrefFullScreen();
    shell->Create();
    shell->GetCanvas()->SetMode(QCanvas::SINGLEBUF);
    cursShell=new QCursor(QC_ARROW);
    shell->GetQXWindow()->SetCursor(cursShell);
    if(!rUI){ QDELETE(r); }
    SetShell(shell);
    // Default mouse position
    mouseX=shell->GetWidth()/2;
    mouseY=shell->GetHeight()/2;
  }
  if(!(createFlags&NO_BC))
  {
    // Create a gel/broadcast shell (default QApp behavior)
    // If no UI exists, the parent is 0
    if(rBC)r=rBC;
    //else   r=new QRect(45,45,1280,1024);	// Default=fullscreen
//qdbg("QApp: create rBC\n");
    if(rBC)
      bc=new QBC(shell,QBC::CUSTOM,r->x,r->y,r->wid,r->hgt);
    else
      bc=new QBC(shell,QBC::PAL,45,45);
    bc->Create();
    bc->GetCanvas()->SetMode(QCanvas::DOUBLEBUF);
    // Default is to hide cursor from bc area
    cursBC=new QCursor(QC_EMPTY);
    bc->GetQXWindow()->SetCursor(cursBC);
    SetBC(bc);
  }

  // Fonts are actually only created when a GLX context is ready
  CreateSysFonts();

  // Generate state manager if possible (VCR ctrls)
  if(!(createFlags&NO_SM))
  { QRect rsm(45,576+48+10,150,40);
    sm=new QStateManager(shell,&rsm);
  }

  //qdbg("QApp::Create RET\n");
}

/*************
* INFORMATION *
**************/
Pixel QApp::GetPixel(int col)
{
  if(col<0||col>10-1)
  { return BlackPixel(XDSP,0);
  }
  return uiPixel[col];
}
QVideoServer *QApp::GetVideoServer()
// Returns video server or open it if not present
{
#if defined(__sgi)
  if(!vs)
    vs=new QVideoServer();
  return vs;
#else
  // Win32/Linux don't have video
  return 0;
#endif
}
int QApp::GetEventFD()
{
  return ConnectionNumber(XDSP);
}
void QApp::GetCursorPosition(int *x,int *y)
// Returns the current mouse position in (x,y)
// Notice that this function is slow (round trip to XServer); rely on events
// This one is handy if you want to know the mouse position before the 1st
// event rolls in.
// FUTURE: scan events and record (x,y) so it IS fast all the time
{
#ifdef WIN32
  // NYI on Win32
  qwarn("QApp:GetCursorPosition() NYI/Win32");
  *x=*y=0;
#else
  Window xw,dummyWindow;
  int dummy;
  unsigned int mask;

  // Use root window (always present) of screen 0
  xw=RootWindow(XDSP,0);
  *x=*y=0;				// In case of an error
  XQueryPointer(XDSP,xw,&dummyWindow,&dummyWindow,x,y,&dummy,&dummy,&mask);
#endif
}

/**********
* WINDOWS *
**********/
void QApp::CreateSysFonts()
// If not already present, generate the system fonts (which can only
// be generated after a window is present; needs a GLX context)
{
  if(sysFont)return;		// Done already
  if(!shell)return;		// No GLX
  //qdbg("QApp:CreateSysFonts: sysFont=%p, shell=%p\n",sysFont,shell);
  // Default font (create AFTER shell, so we have a GLX context current)
#ifdef WIN32
  sysFont=new QFont("arial",16,QFONT_SLANT_DONTCARE,QFONT_WEIGHT_BOLD);
#else
  sysFont=new QFont("helvetica",17);
#endif
  sysFont->Create();
  sysFontNP=new QFont("courier",17);
  sysFontNP->Create();
}
void QApp::SetShell(QShell *sh)
{
  //qdbg("QApp::SetShell\n");
  shell=sh;
  /*if(!sm)
  { QRect rsm(45,576+48+10,150,40);
    sm=new QStateManager(shell,&rsm);
  }*/
  if(shell)CreateSysFonts();
}
void QApp::SetBC(QBC *sh)
{
  //qdbg("QApp::SetBC\n");
  bc=sh;
  if(bc)
  { CreateSysFonts();
    // Animation
    fm=new QFXManager();
    fm->SetWindow(bc);
    vfm=new QFXManager();
    vfm->SetWindow(bc);
    ffm=new QFXManager();
    ffm->SetWindow(bc);
    gl=new QGelList();
    // Link it all together
    fm->SetGelList(gl);
  }
}

/******
* RUN *
******/
void QApp::SetIdleProc(QAppIdleProc func)
{ idleProc=func;
}
void QApp::SetEventProc(QAppEventProc func)
{ evProc=func;
}
void QApp::SetExitProc(QAppExitProc func)
{ exitProc=func;
}
void QApp::Exit(int rc)
{
  if(exitProc)exitProc();
  // Destroy the app children, for debugging memory's sake
  Destroy();
  // Nicely kill shell window if it exists
  // For Win32 this may be MUCH nicer, since it might restore fullscreen
  // mode to normal mode
  //if(QSHELL)delete QSHELL;
#ifdef DEBUG
  // Report any unfreed memory blocks
  Mqcheckmem(0);
  Mqreport(1);
#endif
  exit(rc);
}

/*********
* TIMERS *
*********/
#ifndef WIN32
typedef unsigned long long apptimer_t;
apptimer_t tNext;			// Next timeout
int        tInterval;			// In millisecs
#endif

int QApp::AddTimer(int msecs)
{
#if defined(WIN32) || defined(linux)
  qerr("QApp:AddTimer() not supported on Win32/Linux");
  return 0;
#else
  dmGetUST(&tNext);
  tNext+=msecs*1000000;
  tInterval=msecs;
  return 1;
#endif
}
void QApp::RemoveTimer(int msecs)
{
}

/*****************
* EVENT MANAGING *
*****************/
int QApp::GetKeyModifiers()
// Get last recorded state of modifier keys (Shift, Ctrl etc)
{
  return modFlags;
}

static string GetNextScreenShotFile(cstring prefix,string d)
// Automatically numbers screenshot pictures so a non-existing
// file can be created
// 'prefix' might be 'bc' for example.
// Filename is stored 'd', which must be some 40 chars minimal.
{
  int i;
  for(i=1;;i++)
  {
    sprintf(d,"/tmp/%s%05d.tga",prefix,i);
    if(!QFileExists(d))break;
  }
  return d;
}

int QApp::Run1()
// Do one time event fetching and dispatching
// Return value:
// 0: event processed and handled by Q system
// 1: event processed but no-one handled it (for QML; picks up the event)
// 2: no event; idle procedure called
// Every event passes through here, nice place to tuck some hardcoded
// keys in.
{ bool r;
  int  rc;
  char buf[256];
  string ssName;		// Name of screenshot

  if(QEventPending())
  { QEventNext(&event);
    // Memorize modifiers
    if(event.type==QEvent::KEYPRESS)
    {
//qdbg("QApp: key 0x%x\n",event.n);
      switch(QK_Key(event.n))
      {
#ifdef WIN32
        // Win32 sees no difference between left and right modifier keys
        case _QK_LSHIFT: modFlags|=QK_SHIFT; break;
        case _QK_LCTRL : modFlags|=QK_CTRL;  break;
        case _QK_LALT  : modFlags|=QK_ALT;   break;
#else
        case _QK_LSHIFT: case _QK_RSHIFT: modFlags|=QK_SHIFT; break;
        case _QK_LCTRL : case _QK_RCTRL : modFlags|=QK_CTRL;  break;
        case _QK_LALT  : case _QK_RALT  : modFlags|=QK_ALT;   break;
#endif
      }
#ifndef WIN32
      switch(event.n)
      { case QK_ALT|QK_CTRL|QK_F12:
          // Screenshot of BC
          ssName=GetNextScreenShotFile("bc",buf);
          qdbg("> Saving screenshot of BC in '%s'\n",ssName);
          if(app->GetBC())app->GetBC()->ScreenShot(ssName);
          else qdbg("> No BC present.\n");
          break;
        case QK_ALT|QK_CTRL|QK_F11:
          // Screenshot of UI
          ssName=GetNextScreenShotFile("ui",buf);
          qdbg("> Saving screenshot of UI in '%s'\n",ssName);
          if(app->GetShell())app->GetShell()->ScreenShot(ssName);
          else qdbg("> No UI present.\n");
          break;
      }
#endif
    } else if(event.type==QEvent::KEYRELEASE)
    { 
//qdbg("QApp: keyrelease 0x%x\n",event.n);
      switch(QK_Key(event.n))
      {
#ifdef WIN32
        // Win32 sees no difference between left and right modifier keys
        case _QK_LSHIFT: modFlags&=~QK_SHIFT; break;
        case _QK_LCTRL : modFlags&=~QK_CTRL;  break;
        case _QK_LALT  : modFlags&=~QK_ALT;   break;
#else
        case _QK_LSHIFT: case _QK_RSHIFT: modFlags&=~QK_SHIFT; break;
        case _QK_LCTRL : case _QK_RCTRL : modFlags&=~QK_CTRL;  break;
        case _QK_LALT  : case _QK_RALT  : modFlags&=~QK_ALT;   break;
#endif
      }
    } else if(event.type==QEvent::MOTIONNOTIFY)
    {
      // Remember last mouse location
      mouseX=event.x;
      mouseY=event.y;
    } else if(event.type==QEvent::BUTTONPRESS)
    {
      mouseButtons|=(1<<event.n);
    } else if(event.type==QEvent::BUTTONRELEASE)
    {
      mouseButtons&=~(1<<event.n);
    }
   do_event:
    // First let the window manager taste the event
    if(winmgr->ProcessEvent(&event))
    { rc=0;
      goto ret;
    }
    // Then pass the event to an event procedure
    if(evProc)
    { r=evProc(&event);
      if(r){ rc=0; goto ret; }
    }
    // Pass on to application state if it is there
    if(sm)
    { // Dispatch event to global & current state
      if(sm->ProcessEvent(&event))
      { rc=0;
        goto ret;
      }
    }

    //
    // Default event handler for non-handled events
    //
    if(event.type==QEvent::CLOSE&&event.win==QSHELL)
    {
      // Request to close; no counter action was taken
      app->Exit();
    }

    // Event was seen, but no-one ate it
    rc=1;
  } else
  { // Idle processing
#if defined(__sgi)
    // Check timer
    if(tNext)
    { //qdbg("QApp: chk timer\n");
      apptimer_t curTime;
      dmGetUST(&curTime);
      if(curTime>=tNext)
      { // Generate event
        //qdbg("  ** timer evt\n");
        //QEvent e;
        event.type=QEvent::TIMER;
        event.n=1;				// Timer ID
        //QEventPush(&e);
        tNext+=tInterval*1000000;
        goto do_event;
      }
    }
#endif
    if(idleProc)idleProc();
    rc=2;
  }
 ret:
#ifdef WIN32
  // Some OpenGL libraries don't flush after a certain timeout, so flush explicitly
  // here if there is a chance that drawing operations have taken place
  // Events like BUTTONPRESS and MOTIONNOTIFY may all result in drawing, so let's
  // just always flush
  // Don't flush if idle though (too much overhead)
  if(rc!=2/*||TRUE*/)
    glFlush();
#endif
//qdbg("QApp:Run1 RET (%d)\n",rc);
  return rc;
}
void QApp::Run(QState *start,QState *globState)
// Acts as an event manager
// If 'start==0', no state will be selected (the current state is preserved)
{
  //qdbg("QApp::Run\n");
  //winmgr->Paint();
  //qdbg("  setglobst\n");
  if(sm)sm->SetGlobalState(globState);
  //qdbg("  goto start\n");
  if(start)if(sm)sm->GotoState(start);
  // Paint 1st state (from ZLib)
  if(start)start->Paint();

  // Bug: paint sm (?) doesn't get expose
  if(sm)sm->Paint();

  // Q event loop
  while(1)
  { Run1();
  }
}

void QApp::RunPoll(QState *start,QState *globState)
// As Run(...), but runs until no events are available anymore
// So no idle processing!
// Used to get the user interface up and painted after creating windows
// See Run(...) for arguments
{
  //winmgr->Paint();
  if(sm)sm->SetGlobalState(globState);
  if(start)sm->GotoState(start);
  // Paint 1st state (from ZLib)
  if(start)start->Paint();

  // Q event loop
  while(QEventPending())
    Run1();
}

void QApp::GotoState(QState *ns)
{ GetStateManager()->GotoState(ns);
}

void QApp::Sync()
{
  //QBC *bc;
  GetFXManager()->Run();
  // Dirty gels
  if(!bc)return;
  while(bc->GetGelList()->IsDirty())
  { GetFXManager()->Iterate();
  }
}

int QApp::GetVTraceCount()
// Returns #vertical traces
{
#ifndef __sgi
  return 0;
#else
  unsigned int count;
  int r;
  r=glXGetVideoSyncSGI(&count);
  if(r==GLX_BAD_CONTEXT)
  { qerr("QApp:GetVTraceCount needs GLX context to work");
    count=0;
  } else if(r!=0)
  { qerr("QApp:GetVTraceCount() failed err %d",r);
  }
//qdbg("vtrace: count=%d, int=%d\n",count,(int)count);
  return (int)count;
#endif
}

