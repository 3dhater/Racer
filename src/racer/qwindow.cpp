/*
 * QWindow - a very basic window in a screen
 *   also includes QXWindow; encapsulates XWindows
 * 01-10-96: Created!
 * 11-11-96: Using Xt instead of Vk.
 * 05-06-97: All windows now have an X-window member; this was necessary
 * for the event manager to trace down the Q window.
 * 27-09-97: Bugfix; QXWindow didn't delete gl in dtor
 * 20-02-98: Every window is an X window
 * BUGS:
 * - GetPos() and GetXPos() don't work for multi-level parented windows.
 * They should follow the parents to get their REAL locations.
 * - GetRootPos() doesn't look at nested X windows (will not happen very
 * quickly; a button inside Q_BC or a QDraw will expose the bug)
 * NOTES:
 * (C) MarketGraph/RVG
 */

#include <qlib/window.h>
#include <qlib/canvas.h>
#include <qlib/cursor.h>
#include <qlib/app.h>
#include <qlib/opengl.h>
#include <qlib/keys.h>
#include <stdio.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QWindow::QWindow(QWindow *_parent,int x,int y,int wid,int hgt)
  : QDrawable()
  //: QXWindow(iparent,x,y,wid,hgt)
{
  parent=_parent;
  userData=0;
  flags=0;
  catchFlags=0;
  // Default is to NOT use tab stops
  tabStop=-1;
  qxwin=0;
  compoundWindow=0;

  // Remember creation params
  rCreation.x=x;
  rCreation.y=y;
  rCreation.wid=wid;
  rCreation.hgt=hgt;

  // Derive canvas from parent if possible
  // This canvas has a local coordinate system (0,0 is top-left of the window)
  // If you call MakeRealX() before Create(), a new canvas/OpenGL context
  // will be created for the window (plus non-X descendants)
  if(parent)
  { cv=new QCanvas(parent->GetCanvas());
    // Set origin for painting
    cv->SetOffset(x,y);
  } else
  { // No parent; create canvas; must be real X window
    cv=new QCanvas(this);
    // Origin is (0,0), since this is a true X window
  }
  //qdbg("QWin ctor: parent=%p, cv=%p\n",parent,cv);
  //if(cv)qdbg("  cv->gl=%p\n",cv->GetGLContext());
}

QWindow::~QWindow()
{ 
qdbg("QWindow dtor\n");
  // Remove us from the window manager's list
  app->GetWindowManager()->RemoveWindow(this);

  // Canvas is always ours
  QDELETE(cv);
  // Delete X window if we created it
  if(IsX())
  { delete qxwin;
  }
}

/***********
* CREATION *
***********/
void QWindow::MakeRealX()
// Declare this window to be an actual X window (before calling Create())
// This is only done for shells (in which GUI elements appear), and
// broadcast and QDraw windows, which may use different visuals, and
// different cursor images.
// Windows such as buttons and edit controls live 'inside' a shell window.
// Real X QWindows follow the size of the X window.
{
  if(flags&CREATED)
  { qwarn("QWindow:MakeRealX() called after window was created");
    return;
  }
  flags|=ISX;
}
void QWindow::MakeSoftWindow()
// The reverse of MakeRealX()
{
  if(flags&CREATED)
  { qwarn("QWindow:MakeSoftWindow() called after window was created");
    return;
  }
  flags&=~ISX;
}
void QWindow::PrefWM(bool yn)
// Declare window to be X managed (must have called MakeRealX())
{
  if(flags&CREATED)
  { qwarn("QWindow:PrefWM() called after window was created");
    return;
  }
  if(yn)flags|=XMANAGED;
  else  flags&=~XMANAGED;
}
void QWindow::PrefFullScreen()
// Declare window to be X managed (must have called MakeRealX())
{
  if(flags&CREATED)
  { qwarn("QWindow:PrefFullScreen() called after window was created");
    return;
  }
  flags|=FULLSCREEN;
}

bool QWindow::Create()
{ //printf("QWindow::Realize!\n");
  if(IsX())
  { // Create an X window
    qxwin=new QXWindow(parent?parent->GetQXWindow():0,
      rCreation.x,rCreation.y,rCreation.wid,rCreation.hgt);
    // Use window manager for dragging, sizing etc?
    if(flags&XMANAGED)
      qxwin->PrefWM(TRUE);
    if(flags&FULLSCREEN)
      qxwin->PrefFullScreen();
    qxwin->Create();
    // Create (new) canvas for this window + descendants
    QDELETE(cv);
    cv=new QCanvas(this);
    // Link GLX/OpenGL context of QXWindow to canvas
    cv->SetGLContext(qxwin->GetGLContext());
    cv->SetMode(QCanvas::SINGLEBUF);

//qdbg("QWindow:Create(); cv=%p, cv->gl=%p\n",cv,cv->GetGLContext());
//cv->Select(); glClearColor(.4,.5,.6,.3); cv->Clear();
//qxwin->Swap(); qxwin->Swap();
  } else
  { // Make sure this window is really inside an X window
    if(GetQXWindow()==0)
    { qerr("QWindow::Create() without a parent real X window");
      return FALSE;
    }
  }
  flags|=CREATED;
  // Add window to application
  app->GetWindowManager()->AddWindow(this);
  return TRUE;
}
void QWindow::Destroy()
{
  if(qxwin)
    qxwin->Destroy();
}

/**********
* ATTRIBS *
**********/
QXWindow *QWindow::GetQXWindow()
// Finds QXWindow in which this window lives
// This member is only stored in the QWindow which owns the QXWindow
{
  QWindow *w;
  // Go up the hierarchy until you find the window with the X window
  for(w=this;w!=0&&w->qxwin==0;w=w->GetParent());
    //qdbg("  GetQXWindow: w=%p\n",w);
  // 'w' should be != 0
  return w->qxwin;
}

bool QWindow::IsX()
// Returns TRUE if this window 'owns' an X window
{
  if(flags&ISX)return TRUE;
  return FALSE;
}

void QWindow::GetPos(QRect *r)
// Returns position of the window relative to its parent window
// This may NOT be the position in the X window (for Paint()-ing)
{ r->x=rCreation.x;
  r->y=rCreation.y;
  r->wid=rCreation.wid;
  r->hgt=rCreation.hgt;
}
void QWindow::GetXPos(QRect *r)
// Returns position of the window relative to its X window parent (QXWindow)
// This differs with GetPos() in that it specifies the drawing location
// inside the actual X window, while GetPos() returns the position relative
// to this window's parent.
// This function is useful to find the drawing location for Paint() functions.
{ r->x=rCreation.x;
  r->y=rCreation.y;
  r->wid=rCreation.wid;
  r->hgt=rCreation.hgt;
}
void QWindow::GetRootPos(QRect *r)
// Returns position of window relative to root window (screen)
// This is used in mouse capturing, where mouse coordinates must be related
// to the capture window. This way we can follow the mouse relative to
// the capture window.
{ QXWindow *xw;
  int ox,oy;

  ox=oy=0;
  for(xw=GetQXWindow();xw;xw=xw->GetParent())
  { 
    ox+=xw->GetX();
    oy+=xw->GetY();
  }

  r->x=rCreation.x+ox;
  r->y=rCreation.y+oy;
  r->wid=rCreation.wid;
  r->hgt=rCreation.hgt;
}

int QWindow::GetX()
{
  return rCreation.x;
}
int QWindow::GetY()
{
  return rCreation.y;
}
int QWindow::GetWidth()
{
  return rCreation.wid;
}
int QWindow::GetHeight()
{
  return rCreation.hgt;
}

void QWindow::SetUserData(void *p)
{
  userData=p;
}

/************
* Modifying *
************/
void QWindow::Invalidate(QRect *rInvalid)
// Declare area 'r' of this window invalid
// Generates an expose event
{
  QEvent qe;
  QRect *r,rAll;

  if(!rInvalid)
  { // Entire window
    r=&rAll;
    GetPos(&rAll); rAll.x=rAll.y=0;
  } else r=rInvalid;

  qe.type=QEvent::EXPOSE;
  qe.x=r->x;
  qe.y=r->y;
  qe.wid=r->wid;
  qe.hgt=r->hgt;
  qe.n=0;
  qe.win=this;
  if(IsX())
  { // Expose us + children
//qdbg("demultiplex expose %d,%d %dx%d\n",r->x,r->y,r->wid,r->hgt);
    QEventDemultiplexExpose(&qe);
  } else
  { QEventPush(&qe);
  }
}

void QWindow::Hide()
// Hides window
// Future: also work for Real X windows (Unmap)
{
  QRect pos;

  if(flags&HIDDEN)return;

  if(IsX())
  { qwarn("Can't hide X QWindow");
    return;
  }
  GetPos(&pos);
  parent->Invalidate(&pos);
  flags|=HIDDEN;
}
void QWindow::Show()
{
  if(!(flags&HIDDEN))return;
  flags&=~HIDDEN;
  Invalidate();
}

void QWindow::SetCompoundWindow(QWindow *w)
// Set this window to be part of a compound window.
// An example is a button in a scrollbar, which must report its events
// to the scrollbar so that the it can generate scrollbar events and
// handle movements in the associated QProp
{
  compoundWindow=w;
}
QWindow *QWindow::GetCompoundWindow()
{
  return compoundWindow;
}

bool QWindow::Move(int x,int y)
// Move window
{
  if(flags&CREATED)
  {
//qdbg("QWindow::Move(); window was created; do move\n");
    // Move canvas virtual origin
    if(IsX())
    { // Really move
      qxwin->Move(x,y);
      rCreation.x=x;
      rCreation.y=y;
      // Events will be generated by X?
      //Invalidate();
      // Canvas offset remains (0,0) since this is an X window
    } else
    { // Virtual move
      parent->Invalidate(&rCreation);
      rCreation.x=x;
      rCreation.y=y;
      // Adjust local coordinate system of canvas
      cv->SetOffset(x,y);
      // Redraw ourselves
      Invalidate();
    }
  } else
  { // Creation
    rCreation.x=x;
    rCreation.y=y;
    // Adjust local coordinate system of canvas
    cv->SetOffset(x,y);
  }
  return TRUE;
}
bool QWindow::Size(int w,int h)
// Resize window
{
  if(flags&CREATED)
  {
//qdbg("QWindow::Size(); was created; do resize\n");
    // Move canvas virtual origin
    if(IsX())
    { // Really move
      qxwin->Size(w,h);
      rCreation.wid=w;
      rCreation.hgt=h;
      // Events will be generated by X?
      //Invalidate();
    } else
    { // Virtual move
      parent->Invalidate(&rCreation);
      rCreation.wid=w;
      rCreation.hgt=h;
      // Redraw ourselves
      Invalidate();
    }
  } else
  { // Creation size
    rCreation.wid=w;
    rCreation.hgt=h;
  }
  return TRUE;
}
bool QWindow::HasImageBackground()
// Returns TRUE if the window uses the GUI image for a background
{
  if(flags&NO_IMG_BGR)return FALSE;
  return TRUE;
}
void QWindow::SetImageBackground(bool yn)
// Turns on or off the GUI image as a background
{
  if(yn)flags&=~NO_IMG_BGR;
  else  flags|=NO_IMG_BGR;
}

/************
* TAB STOPS *
************/
void QWindow::SetTabStop(int n)
// Define TAB stop index for this window
// n==-2; automatic; use next available tab stop index
// n==-1; no tab stop used
// n>=0 ; absolute tab stop index
{
  if(n==-2)
  {
    // Take the next best tab stop index
    n=QWM->GetMaxTabStop()+1;
  }
  tabStop=n;
}
void QWindow::NoTabStop()
// Disable TAB coming through here
{
  tabStop=-1;
}

/**************
* INFORMATION *
**************/
bool QWindow::IsDescendantOf(QWindow *root)
// Checks to see if this window is somehow a descendant of 'root'
{ QWindow *w;
  w=this;
 check:
  if(root==w)return TRUE;
  w=w->GetParent();
  if(w)goto check;
  return FALSE;
}
bool QWindow::IsFocus()
// Returns TRUE if this window has the input focus
{
  if(QWM->GetKeyboardFocus()==this)return TRUE;
  return FALSE;
}

void QWindow::Catch(int cf)
{ catchFlags=cf;
}
bool QWindow::Catches(int cf)
{ // Hidden or disabled windows don't catch anything
  if(flags&(HIDDEN|DISABLED))return FALSE;
  if(catchFlags&cf)return TRUE;
  return FALSE;
}

void QWindow::CompressExpose()
{
  flags|=COMPRESS_EXPOSE;
}
bool QWindow::CompressesExpose()
{ if(flags&COMPRESS_EXPOSE)return TRUE;
  return FALSE;
}
void QWindow::CompressMotion()
{
  flags|=COMPRESS_MOTION;
}
bool QWindow::CompressesMotion()
{ if(flags&COMPRESS_MOTION)return TRUE;
  return FALSE;
}

bool QWindow::IsVisible()
{ if(flags&HIDDEN)return FALSE;
  return TRUE;
}
bool QWindow::IsEnabled()
{ if(flags&DISABLED)return FALSE;
  return TRUE;
}

void QWindow::Restore()
{
  QRect pos;
  GetPos(&pos); pos.x=pos.y=0;
  if(cv->UsesX())
  { // Use flat color; no bitmap
    cv->SetColor(QApp::PX_UIBGR);
    cv->Rectfill(&pos);
  } else
  { app->GetWindowManager()->RestoreBackground(this,&pos);
  }
}

void QWindow::Paint(QRect *r)
// Default window paint paints nothing (compound windows for example)
{
  //qerr("QWindow::Paint: abstract function!\n");
}

/*********
* EVENTS *
*********/
void QWindow::PushEvent(QEvent *e)
// Generate an event.
// This is passed on to the Q event queue in case the window is NOT part
// of a compound window. Otherwise the event is DIRECTLY passed on to
// the compound window so it can take appropriate action.
{
  if(compoundWindow)
    compoundWindow->Event(e);
  else
    QWM->PushEvent(e);
}

bool QWindow::EvEnter()
// Default when entering X window is to focus keystrokes to it
{
  QXWindow *xw;
  //qdbg("QWindow::EvEnter this=%p @ms%d\n",this,app->GetCurrentEvent()->n);
  xw=GetQXWindow();
  xw->FocusKeys(app->GetCurrentEvent()->n);
  return TRUE;
}
bool QWindow::EvLeave()
{
  //qdbg("QWindow::EvLeave this=%p\n",this);
  return TRUE;
}
bool QWindow::EvButtonPress(int button,int x,int y)
{ return FALSE;
}
bool QWindow::EvButtonRelease(int button,int x,int y)
{ return FALSE;
}
bool QWindow::EvKeyPress(int key,int x,int y)
// Handle default keys in controls
// TAB       = move to next control
// Shift-TAB = move to prev control
// TAB control may be turned off in the window manager
{
  if(key==QK_TAB)
  { if(QWM->IsTABControlEnabled())
    { QWM->FocusNext();
      return TRUE;
    }
  } else if(key==(QK_SHIFT|QK_TAB))
  { if(QWM->IsTABControlEnabled())
    { QWM->FocusPrev();
      return TRUE;
    }
  }
  return FALSE;
}
bool QWindow::EvKeyRelease(int key,int x,int y)
{ return FALSE;
}
bool QWindow::EvMotionNotify(int x,int y)
{ return FALSE;
}
bool QWindow::EvExpose(int x,int y,int wid,int hgt)
{ 
//qdbg("QWindow::EvExpose this=%p\n",this);
  //proftime_t t;
  //profStart(&t);
  Paint();
  //profReport(&t,"EvExpose");
//qdbg("QWindow::EvExpose RET\n");
  return TRUE;
}
bool QWindow::EvResize(int wid,int hgt)
{ return FALSE;
}

bool QWindow::EvSetFocus()
// Control receives focus.
// Default behavior is to repaint control.
{
  Invalidate();
  return FALSE;
}
bool QWindow::EvLoseFocus()
// Should return TRUE if the focus loss is handled and NOT accepted.
// This way you can keep a focus change from happening.
// Default here is to repaint the control, and let the focus change go ahead.
{
  Invalidate();
  return FALSE;
}

bool QWindow::Event(QEvent *e)
// Dispatches event to the right default handler
// You may override this function to get any-event handling, but
// make sure you call this function to ensure default processing for things
// you don't handle
{
//qdbg("QWindow:Event(%d)\n",e->type);
  switch(e->type)
  { 
    case QEvent::BUTTONPRESS:
      //app->GetWindowManager()->SetKeyboardFocus(this);
      return EvButtonPress(e->n,e->x,e->y);
    case QEvent::BUTTONRELEASE:
      return EvButtonRelease(e->n,e->x,e->y);
    case QEvent::KEYPRESS:
      return EvKeyPress(e->n,e->x,e->y);
    case QEvent::KEYRELEASE:
      return EvKeyRelease(e->n,e->x,e->y);
    case QEvent::MOTIONNOTIFY:
      return EvMotionNotify(e->x,e->y);
    case QEvent::EXPOSE:
      return EvExpose(e->x,e->y,e->wid,e->hgt);
    case QEvent::RESIZE:
      return EvResize(e->wid,e->hgt);
    case QEvent::SETFOCUS:
      return EvSetFocus();
    case QEvent::LOSEFOCUS:
      return EvLoseFocus();
    case QEvent::ENTER:
      return EvEnter();
    case QEvent::LEAVE:
      return EvLeave();
    default:
      return FALSE;
  }
}

#ifdef NOTDEF
Colormap QWindow::getShareableColormap(XVisualInfo * vi)
// To avoid taking 1 hardware color map per X window, this function
// tries to get a shared color map that it places in a property of the server,
// so any new windows might share the color map.
{
  Status status;
  XStandardColormap *standardCmaps;
  Colormap cmap;
  int i, numCmaps;
  Display *dpy=GetApp()->dpy;

  /* Be lazy; using DirectColor too involved for this example. */
  if (vi->c_class != TrueColor)
    XtAppError(0, "no support for non-TrueColor visual");
  /* If no standard colormap but TrueColor, just make an
     unshared one. */
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
  cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
  return cmap;
}
#endif

