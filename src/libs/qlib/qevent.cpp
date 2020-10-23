/*
 * QEvent.cpp - managing a Q version of events
 * 21-12-96: Created!
 * 21-12-99: Windows may now have or not have a real X window
 * 22-12-99: Important change; event windows may be 0 in case the window
 * was not found. It is reported for menus (for example) so you can detect
 * a click outside your active (modal) windows. 
 * 18-03-00: Double click generation.
 * NOTES
 * - As things here are not so difficult, I have taken the liberty to optimize
 *   some functions to inline QEventPending() for example; for speed.
 * - A bit of key handling is contained here to get easy accents.
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <stdlib.h>
#include <qlib/event.h>
#include <qlib/app.h>
#include <qlib/display.h>
#include <qlib/keys.h>
#include <X11/Xlib.h>
//#include <Xm/Xm.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Buffersize must be power of 2
#define QEVENT_DEFAULT_BUFFERSIZE	1024	// Default #events to memorize

// Double click support; time to count 2 clicks as 1 double click (msecs)
#define DOUBLE_CLICK_MAX_TIME	260

// Accent modifiers (ALT-'`~: etc)
enum AccentModifiers
{
  AM_NONE,
  AM_DIAERESIS,
  AM_GRAVE,
  AM_ACUTE,
  AM_CIRCUMFLEX,
  AM_TILDE,
  AM_RING,
  AM_CEDILLA,
  AM_OBLIQUE
};

static QEvent *evBuffer;
static int     bufSize;		// # allocated events
static int     events;		// #events pending
static int     head,tail;	// Circular buffer, for efficiency
//static QApp   *app;

/************************
* STATIC EVENT HANDLING *
************************/
void QEventInit()
{ // Init buffer
  if(evBuffer)return;
  QASSERT_V(app);
  //app=_app;
  bufSize=QEVENT_DEFAULT_BUFFERSIZE;
  evBuffer=(QEvent*)qcalloc(bufSize*sizeof(QEvent));
  if(!evBuffer)
  { qerr("No memory for Q event buffer\n");
    exit(0);
  }
  events=0;
  head=tail=0;
}
void QEventQuit()
// Global event quit (called by QApp::Destroy())
{
  if(evBuffer)
  { qfree(evBuffer);
    evBuffer=0;
  }
}

/******
* X11 *
******/
void QEventDemultiplexExpose(QEvent *e)
// Split X expose event into multiple events
// for the child windows that may live in the X window
{
  QEvent    qe;
  int       i,windows;
  QWindow  *w;
  QXWindow *xw;
  QRect     r,rExpose;

//qdbg("Demultiplex expose %d,%d %dx%d count=%d\n",e->x,e->y,e->wid,e->hgt,e->n);
  memset(&qe,0,sizeof(qe));

  qe.type=QEvent::EXPOSE;
  windows=QWM->GetNoofWindows();
  xw=e->win->GetQXWindow();
  rExpose.x=e->x;
  rExpose.y=e->y;
  rExpose.wid=e->wid;
  rExpose.hgt=e->hgt;

  // Generate expose for the owner window
  qe.win=e->win;
  qe.x=e->x;
  qe.y=e->y;
  qe.wid=e->wid;
  qe.hgt=e->hgt;
  qe.n=e->n;
  QEventPush(&qe);

  for(i=0;i<windows;i++)
  { w=QWM->GetWindowN(i);
    if(w->IsX())
    { // X owner window is already handled above
      continue;
    }
    // Belongs to the same X window?
    if(w->GetQXWindow()==xw)
    { // Do the exposes overlap?
      w->GetPos(&r);
      r.Intersect(&rExpose);
      if(!r.IsEmpty())
      { // Generate expose for this child window
        qe.win=w;
        qe.x=r.x;
        qe.y=r.y;
        qe.wid=r.wid;
        qe.hgt=r.hgt;
        qe.n=0;
        QEventPush(&qe);
      }
    }
  }
}

static int HandleAccents(int n)
// Look at keycode 'n' and handle special keys (accents)
// Modifies 'n' to the modified version
// Shouldn't this be in qkey.cpp?
{
  static int modify;
  int k,m,oldModify;

  // Don't handle modifier keys
  if(n==_QK_LSHIFT||n==_QK_RSHIFT||
     n==_QK_LCTRL||n==_QK_RCTRL||
     n==_QK_LALT||n==_QK_RALT)return n;

//qdbg("HandleAccents(%d)\n",n);
  // Detect accent keys
#ifndef WIN32
  switch(n)
  { case QK_ALT|QK_SEMICOLON     : modify=AM_DIAERESIS; return n;
    case QK_ALT|QK_GRAVE         : modify=AM_GRAVE; return n;
    case QK_ALT|QK_APOSTROPHE    : modify=AM_ACUTE; return n;
    case QK_ALT|QK_SHIFT|QK_GRAVE: modify=AM_TILDE; return n;
    case QK_ALT|QK_PERIOD        : modify=AM_RING; return n;
    case QK_ALT|QK_SHIFT|QK_6    : modify=AM_CIRCUMFLEX; return n;
    case QK_ALT|QK_COMMA         : modify=AM_CEDILLA; return n;
    case QK_ALT|QK_SLASH         : modify=AM_OBLIQUE; return n;
  }
#endif
  // Other keys turn off accents
  oldModify=modify;
  modify=AM_NONE;
  // Check for modifyable keys
  k=QK_Key(n);
  m=QK_Modifiers(n);
//qdbg("HandleAccents: modify=%d, k=%d, m=%d, QK_E=%d\n",modify,k,m,QK_C);
#ifndef WIN32
  switch(oldModify)
  { case AM_CEDILLA:
      if(k==QK_C)return QK_C_CEDILLA|m;
      break;
    case AM_ACUTE:
      if(k==QK_A)return QK_A_ACUTE|m;
      if(k==QK_E)return QK_E_ACUTE|m;
      if(k==QK_I)return QK_I_ACUTE|m;
      if(k==QK_O)return QK_O_ACUTE|m;
      if(k==QK_U)return QK_U_ACUTE|m;
      if(k==QK_Y)return QK_Y_ACUTE|m;
      break;
    case AM_GRAVE:
      if(k==QK_A)return QK_A_GRAVE|m;
      if(k==QK_E)return QK_E_GRAVE|m;
      if(k==QK_I)return QK_I_GRAVE|m;
      if(k==QK_O)return QK_O_GRAVE|m;
      if(k==QK_U)return QK_U_GRAVE|m;
      break;
    case AM_CIRCUMFLEX:
      if(k==QK_A)return QK_A_CIRCUMFLEX|m;
      if(k==QK_E)return QK_E_CIRCUMFLEX|m;
      if(k==QK_I)return QK_I_CIRCUMFLEX|m;
      if(k==QK_O)return QK_O_CIRCUMFLEX|m;
      if(k==QK_U)return QK_U_CIRCUMFLEX|m;
      break;
    case AM_DIAERESIS:
      if(k==QK_A)return QK_A_DIAERESIS|m;
      if(k==QK_E)return QK_E_DIAERESIS|m;
      if(k==QK_I)return QK_I_DIAERESIS|m;
      if(k==QK_O)return QK_O_DIAERESIS|m;
      if(k==QK_U)return QK_U_DIAERESIS|m;
#ifndef linux
      if(k==QK_Y)return QK_Y_DIAERESIS|m;
#endif
      break;
    case AM_TILDE:
      if(k==QK_A)return QK_A_TILDE|m;
      if(k==QK_N)return QK_N_TILDE|m;
      if(k==QK_O)return QK_O_TILDE|m;
      break;
    case AM_OBLIQUE:
      if(k==QK_O)return QK_O_OBLIQUE|m;
      break;
    case AM_RING:
      if(k==QK_A)return QK_A_RING|m;
      break;
  }
#endif
//qdbg("no modifers\n");
  return n;
}

static void X2Q(XEvent *e)
// Convert 1 XEvent to 0 or more QEvents
// 0  events may happen if the window has been destroyed
// >1 events may happen on Expose events
{
  QEvent qe;
  KeySym ks;
  QWindowManager *wm;
  Window xwindow;         // Real OS window id
  QRect  r;
  // Double click generation
  static Time dblButTime;
  static unsigned int dblButton;

//qdbg("X2Q(%d)\n",e->type);

  memset(&qe,0,sizeof(qe));

  wm=app->GetWindowManager();
  xwindow=e->xany.window;
  //qe.win=0;
  //qe.p=0;

  // Default behavior is to pass on events to Q (if masked)
  switch(e->type)
  { case MotionNotify:
      qe.type=QEvent::MOTIONNOTIFY;
      qe.xRoot=e->xmotion.x_root;
      qe.yRoot=e->xmotion.y_root;
      qe.x=e->xmotion.x;
      qe.y=e->xmotion.y;
      break;
    case ConfigureNotify:
      qe.type=QEvent::RESIZE;
      //qe.xRoot=e->xconfigure.x_root;
      //qe.yRoot=e->xconfigure.x_root;
      qe.xRoot=qe.yRoot=0;
      qe.x=e->xconfigure.x;
      qe.y=e->xconfigure.y;
      qe.wid=e->xconfigure.width;
      qe.hgt=e->xconfigure.height;
      break;
    case ButtonPress:
      qe.type=QEvent::BUTTONPRESS;
      qe.x=e->xbutton.x;
      qe.y=e->xbutton.y;
      qe.n=e->xbutton.button;
      qe.xRoot=e->xbutton.x_root;
      qe.yRoot=e->xbutton.y_root;
      break;
    case ButtonRelease:
      qe.type=QEvent::BUTTONRELEASE;
      qe.x=e->xbutton.x;
      qe.y=e->xbutton.y;
      qe.n=e->xbutton.button;
      qe.xRoot=e->xbutton.x_root;
      qe.yRoot=e->xbutton.y_root;
      break;
    case KeyPress:
      qe.type=QEvent::KEYPRESS;
#ifdef WIN32
      // Take raw keycode
      ks=e->xkey.keycode;
#else
      ks=XLookupKeysym((XKeyEvent*)e,0);
#endif
//qdbg("QEvent: X11 KeyPress event: state=%d, keycode=%d, time=%d\n",
        //e->xkey.state,e->xkey.keycode,e->xkey.time);
      qe.x=e->xkey.x;
      qe.y=e->xkey.y;
      qe.n=((int)ks)|(e->xkey.state<<16);
      qe.n=HandleAccents(qe.n);
      qe.xRoot=e->xkey.x_root;
      qe.yRoot=e->xkey.y_root;
      break;
    case KeyRelease:
      qe.type=QEvent::KEYRELEASE;
#ifdef WIN32
      // Take raw keycode
      ks=e->xkey.keycode;
#else
      ks=XLookupKeysym((XKeyEvent*)e,0);
#endif
      qe.x=e->xkey.x;
      qe.y=e->xkey.y;
      qe.n=((int)ks)|(e->xkey.state<<16);
      qe.xRoot=e->xkey.x_root;
      qe.yRoot=e->xkey.y_root;
      break;
#ifndef WIN32
    case EnterNotify:
      //printf("EnterNotify @%d\n",e->xcrossing.time);
      qe.type=QEVENT_ENTER;
      qe.n=e->xcrossing.time;		// Time at which it happened
      break;
    case LeaveNotify:
      qe.type=QEVENT_LEAVE;
      qe.n=0;
      break;
#endif
    case Expose:
//qdbg("expose count=%d\n",e->xexpose.count);
      qe.type=QEvent::EXPOSE;
      qe.n=e->xexpose.count;
      qe.x=e->xexpose.x;
      qe.y=e->xexpose.y;
      qe.wid=e->xexpose.width;
      qe.hgt=e->xexpose.height;
      // Find OS window without looking at subwindows in a (real) OS window
      // So don't use FindOSWindow() here (expose is special)
      qe.win=wm->FindXWindow(xwindow);
      if(!qe.win)return;
      QEventDemultiplexExpose(&qe);
      // All queueing was done by the demultiplexing
      goto no_q;
    default:
      goto no_q;
  }

  // Find Q window and adjust coordinates for X/Y events
  switch(qe.type)
  { case QEvent::BUTTONPRESS:
    case QEvent::BUTTONRELEASE:
    case QEvent::MOTIONNOTIFY:
      if(wm->GetMouseCapture())
      { qe.win=wm->GetMouseCapture();
      } else
      { 
        qe.win=wm->FindOSWindow(xwindow,qe.x,qe.y);
      }
      //if(!qe.win)return;
      if(qe.win)
      { if(!qe.win->IsX())
        { // Bring back to local coordinate system
          qe.win->GetXPos(&r);
          qe.x-=r.x; qe.y-=r.y;
        }
      }
//if(qe.type==QEvent::MOTIONNOTIFY)
//qdbg("  XPos=%d,%d %dx%d; adjust to %d,%d\n",r.x,r.y,r.wid,r.hgt,qe.x,qe.y);
      break;
    case QEvent::KEYPRESS:
    case QEvent::KEYRELEASE:
      if(wm->GetKeyboardFocus())
      { qe.win=wm->GetKeyboardFocus();
      } else
      {
        qe.win=wm->FindOSWindow(xwindow,qe.x,qe.y);
      }
      //if(!qe.win)return;
      if(qe.win)
      { // Bring back to local coordinate system
        qe.win->GetXPos(&r);
        qe.x-=r.x; qe.y-=r.y;
      }
      break;
    case QEvent::ENTER:
    case QEvent::LEAVE:
    case QEvent::RESIZE:
      // Find actual X window
      qe.win=wm->FindXWindow(xwindow);
      break;
  }

 skip_win:
  // Event's window is the QXWindow in which the event took place
  QEventPush(&qe);
  // Handle double click generation
  if(qe.type==QEvent::BUTTONPRESS)
  { if(dblButTime!=0&&dblButton==e->xbutton.button)
    { int diff;
      diff=e->xbutton.time-dblButTime;
//qdbg("dblclick time %d\n",diff);
      if(diff<DOUBLE_CLICK_MAX_TIME)
      { // Count as double click
//qdbg("DBL!\n");
        qe.type=QEvent::DBL_CLICK;
        QEventPush(&qe);
      }
      // Reset time for next click
      dblButTime=e->xbutton.time;
    } else
    { // Remember time for this button
      dblButTime=e->xbutton.time;
      dblButton=e->xbutton.button;
    }
  } else if(qe.type==QEvent::MOTIONNOTIFY)
  { // Mouse has moved; don't double click
    dblButTime=0;
  }
 no_q:
  return;
}

#ifdef WIN32
LRESULT CALLBACK QEventWindowProc(HWND hWnd,UINT message,WPARAM wParam,
  LPARAM lParam)
// Window procedure, used by all QXWindow() windows on the Win32 platform
// Note that Windows may sometimes call THIS proc directly, instead of
// delivering everything through the normal message system (see X11toQ).
// This makes it more difficult to process all messages like X does.
// So on Win32, messages go from: Win32/HWND => X11 (emulation) => Q
{
  // Convert to XEvent, so X2Q() can be used with its detailed thingies
  XEvent e;
  QEvent qe;

  // Clear
  memset(&qe,0,sizeof(qe));

  // OS Window of event
  e.xany.window=(Window)hWnd;

  switch(message)
  {
    case WM_CREATE:
      return 0;

  /*case WM_CLOSE:
    PostQuitMessage( 0 );
    return 0;*/

    case WM_DESTROY:
      return 0;

    // Mouse
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      e.type=ButtonPress;
      // Which button?
      if(message==WM_LBUTTONDOWN)e.xbutton.button=1;
      else if(message==WM_MBUTTONDOWN)e.xbutton.button=2;
      else e.xbutton.button=3;
      e.xbutton.x=LOWORD(lParam);
      e.xbutton.y=HIWORD(lParam);
      // Root handling doesn't work yet
      e.xbutton.x_root=LOWORD(lParam);
      e.xbutton.y_root=HIWORD(lParam);
      // Event time is not supported on Win32
      e.xbutton.time=0;
//qdbg("e.xbutton: @%d,%d (%x); but=%d\n",(int)e.xbutton.x,(int)e.xbutton.y,(int)lParam,e.xbutton.button);
      X2Q(&e);
      break;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      e.type=ButtonRelease;
      // Which button?
      if(message==WM_LBUTTONUP)e.xbutton.button=1;
      else if(message==WM_MBUTTONUP)e.xbutton.button=2;
      else e.xbutton.button=3;
      e.xbutton.x=LOWORD(lParam);
      e.xbutton.y=HIWORD(lParam);
      // Root handling doesn't work yet
      e.xbutton.x=LOWORD(lParam);
      e.xbutton.y=HIWORD(lParam);
//qdbg("e.xbuttonUP: @%d,%d (%x); but=%d\n",(int)e.xbutton.x,(int)e.xbutton.y,(int)lParam,e.xbutton.button);
      e.xbutton.x_root=LOWORD(lParam);
      e.xbutton.y_root=HIWORD(lParam);
      X2Q(&e);
      break;
    case WM_MOUSEMOVE:
      e.type=MotionNotify;
      e.xmotion.x=LOWORD(lParam);
      e.xmotion.y=HIWORD(lParam);
      e.xmotion.x_root=LOWORD(lParam);
      e.xmotion.y_root=HIWORD(lParam);
      X2Q(&e);
      break;

    // Keyboard
    case WM_KEYDOWN:
      e.type=KeyPress;
      e.xkey.x=0;
      e.xkey.y=0;
      e.xkey.state=0;
      e.xkey.x_root=0;
      e.xkey.y_root=0;
      e.xkey.keycode=wParam;
      e.xkey.state=app->GetKeyModifiers()>>16;
//qdbg("QEvent: keydown; keycode=0x%x, state=%x\n",(int)e.xkey.keycode,(int)e.xkey.state);
      X2Q(&e);
//qlog(QLOG_INFO,"keypress code 0x%x",e.xkey.keycode);
      break;
    case WM_KEYUP:
      e.type=KeyRelease;
      e.xkey.x=0;
      e.xkey.y=0;
      e.xkey.state=0;
      e.xkey.x_root=0;
      e.xkey.y_root=0;
      e.xkey.keycode=wParam;
      e.xkey.state=app->GetKeyModifiers()>>16;
      X2Q(&e);
      break;
    case WM_PAINT:
      // Windows does not give a rect in the message
      // Instead, an update region is used (perhaps multiple rectangles)
      // Here, we currently simplify things by requesting an entire expose
      //MessageBox(0,"Expose","dbg",MB_OK);
      e.type=Expose;
      e.xexpose.count=0;      // No other exposes pending (hm)
      e.xexpose.x=0;
      e.xexpose.y=0;
      e.xexpose.width=1280;   // Theoretically EVERYTHING
      e.xexpose.height=1024;
      //e.xexpose.window=QSHELL;  // Should use actual window
      X2Q(&e);
      goto defproc;
      break;

    case WM_CLOSE:
    case WM_QUIT:
      // Direct request to close up
      qe.type=QEvent::CLOSE;
      qe.win=QSHELL;
      QEventPush(&qe);
      break;
    case WM_NCPAINT:
      // The non-client area needs painting
      // Q does its own borders
      // Should also handle NCCALCSIZE, NCACTIVATE
      goto defproc;
      //return 0;
    default:
      // Rest is left upto Windows (a LOT of messages go through here)
defproc:
      return DefWindowProc(hWnd,message,wParam,lParam);
  }
  // We've handled things (ahem)
  return 0;
  //return DefWindowProc(hWnd,message,wParam,lParam);
}

#endif

static void X11toQ(void)
// This function is called to handle incoming X(t) events.
// This function must be called now & then to keep the X11 -> Q stream
// flowing (it is called by QEventPending/Peek/Next)
{
#ifdef WIN32
  //qerr("X11toQ nyi/win32");
  MSG msg;
  while(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
  { // Message found
    // Pass it on to the window procedure (QEventWindowProc() for Win32)
    // because the window proc is THE place where it all comes together (some messages
    // are sent directly to the window proc, instead of being delivered as messages in
    // this message loop)
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
#else
  XEvent event,lastEvent;
  bool   first=TRUE;
  Display *dpy=app->GetDisplay()->GetXDisplay();
  while(XPending(dpy))
  { XNextEvent(dpy,&event);
#ifdef OBS
    // Compress events
    if(first)first=FALSE;
    else
    { // Compress expose events if for equal windows
      if(lastEvent.type==event.type)
      { //&&lastEvent.xany.window==event.xany.window)
        if(lastEvent.type==Expose)
        { //printf("COMPRESS EXPOSE\n");
          goto skip;
        }
      }
    }
#endif
    X2Q(&event);
    lastEvent=event;
   skip:;
  }
#endif
}

/***********
* FETCHING *
***********/
bool QEventPending()
{ X11toQ();
  if(events)return TRUE;
  return FALSE;
}

void QEventPeek(QEvent *e)
{ //if(!QEventPending())
  if(!events)
  { e->type=0;
    return;
  }
  // Get event
  *e=evBuffer[head];
}
void QEventNext(QEvent *e)
{ while(!QEventPending());		// Idle?
  // Get next event and advance event ptr
  *e=evBuffer[head];
  head=(head+1)&(bufSize-1);
  events--;
}

void QEventList()
// Debug function to show the current event list
{
}

/**********
* PUSHING *
**********/
void QEventPush(QEvent *e)
// Push the event into the queue
{ QEvent lastEvent;
  QWindow *w;
  int cur,i;

//qdbg("QEventPush(%d)\n",e->type);
  if(events==bufSize)
  { // Too much events
    qmsg("%c",7);
    return;
  }
  //if(QEventPending())

  // Compress motion?
  if(e->type==QEvent::MOTIONNOTIFY)
  { // Compress?
    w=e->win;
    if(w!=0&&w->CompressesMotion())
    { // Find previous motion event
      for(i=0,cur=head;i<events;i++)
      {
        if(evBuffer[cur].type==QEvent::MOTIONNOTIFY&&evBuffer[cur].win==w)
        { // Modify this event
//qdbg("QEventPush: found previous motion; compressing\n");
          evBuffer[cur].x=e->x;
          evBuffer[cur].y=e->y;
          // Don't insert new one
          return;
        }
        cur=(cur+1)&(bufSize-1);
      }
    }
#ifdef ND_NO_EXPOSE_COMPRESS_MISSES_HITS
  } else if(e->type==QEvent::EXPOSE)
  { // Compress?
    w=e->win;
    if(w->CompressesExpose())
    { // Find previous motion event
      for(i=0,cur=head;i<events;i++)
      {
        if(evBuffer[cur].type==QEvent::EXPOSE&&evBuffer[cur].win==w)
        { // Modify this event
qdbg("QEventPush: found previous expose; compressing\n");
          QRect r(evBuffer[cur].x,evBuffer[cur].y,
            evBuffer[cur].wid,evBuffer[cur].hgt),
            r2(e->x,e->y,e->wid,e->hgt);
          r.Union(&r2);
          evBuffer[cur].x=r.x;
          evBuffer[cur].y=r.y;
          evBuffer[cur].wid=r.wid;
          evBuffer[cur].hgt=r.hgt;
          return;
        }
        cur=(cur+1)&(bufSize-1);
      }
    }
#endif
  }

#ifdef OBS_COMPRESS_OTHERWISE
  if(events)
  {
    QEventPeek(&lastEvent);
    if(lastEvent.type==e->type)
    {
      if(e->type==QEVENT_MOTIONNOTIFY)
      { //printf("Compress MOTION NOTIFY\n");
        // Update last event x/y and don't push
        QEvent *el=&evBuffer[head];
        el->x=e->x;
        el->y=e->y;
        return;
      }
#ifdef OBS
      if(e->type==QEVENT_EXPOSE)
      { //printf("Compress Q EXPOSE\n");
        return;
      }
#endif
    }
  }
#endif

  evBuffer[tail]=*e;
  tail=(tail+1)&(bufSize-1);
  events++;
}

