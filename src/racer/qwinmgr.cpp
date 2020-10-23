/*
 * QWinMgr - Window Manager (yes, my own)
 * 09-05-97: Created! (from zwindow.cpp)
 * 05-06-97: Now qevent.cpp has taken over window deduction; used to be
 * entangled quite a lot into qevent AND qwinmgr (fuzzy).
 * BUGS:
 * - KEYPRESS special focus must also do KEYRELEASE focusing
 * FUTURE:
 * - Behave like 4Dwm (mwm); use borders if requested, drag windows etc.
 *   Automatically create border windows to paint in.
 * (C) MarketGraph/RVG
 */

#include <qlib/window.h>
#include <qlib/dialog.h>
#include <qlib/canvas.h>
#include <qlib/cursor.h>
#include <qlib/app.h>
#include <qlib/keys.h>
#include <qlib/opengl.h>
#include <stdio.h>
#include <qlib/debug.h>
DEBUG_ENABLE

/*******************
* Q WINDOW MANAGER *
*******************/

#undef  DBG_CLASS
#define DBG_CLASS "QWindowManager"

/*
   The task of the window manager is to:
   - maintain the hierarchy of generated windows like shells, buttons etc.
   - see which window gets to see the event: windows can request events
     to be directed to them; they may eat events before it reaches
     the application.
   - it dispatches the events to the window(s) in question (ProcessEvent).

   21-09-99:
   - Tracking the windows that exist in the application
   - Interwindow management (propagating events)
   - Keeping focus windows (keyboard focus/mouse capture) up to date
   - Noticing TAB order
   - Dispatching events to windows that care
   - QApp dispatches uncatched events to the application's event handler
*/

#define QWM_MAX_WINDOW  100             // Yes, just an array

QWindowManager::QWindowManager()
{
  maxWindow=QWM_MAX_WINDOW;
  winList=(QWindow**)qcalloc(maxWindow*sizeof(QWindow*));
  activeWins=0;
  keyboardFocus=0;
  mouseCapture=0;
  imgBgr=0;
  flags=0;
  colBG=new QColor(0,0,0);              // Default is black bgr
}
QWindowManager::~QWindowManager()
{
  DBG_C("dtor")
  QDELETE(colBG);
  QFREE(winList);
}

/**********
* ATTRIBS *
**********/
void QWindowManager::DisableTABControl()
// Disable TAB going from one control to another
// Used in Lingo for example where TAB is a game key
{
  flags|=NO_TAB_CONTROL;
}
void QWindowManager::EnableTABControl()
{
  flags&=NO_TAB_CONTROL;
}
bool QWindowManager::IsTABControlEnabled()
// Returns TRUE if TAB can be used to navigate controls
{
  if(flags&NO_TAB_CONTROL)return FALSE;
  return TRUE;
}

/*****************
* EVENT HANDLING *
*****************/
static bool IsXYEvent(int type)
// Returns TRUE of event type 'type' has valid XY settings
{
  if(type==QEVENT_BUTTONPRESS||
     type==QEVENT_BUTTONRELEASE||
     type==QEVENT_KEYPRESS||
     type==QEVENT_KEYRELEASE||
     type==QEVENT_EXPOSE||
     type==QEVENT_MOTIONNOTIFY)
    return TRUE;
  return FALSE;
}
static int MapEventToCatchFlag(int eType)
{ int zcFlag;
  switch(eType)
  { case QEVENT_BUTTONPRESS:
      zcFlag=QWindow::CF_BUTTONPRESS; break;
    case QEVENT_BUTTONRELEASE:
      zcFlag=QWindow::CF_BUTTONRELEASE; break;
    case QEVENT_KEYPRESS:
      zcFlag=QWindow::CF_KEYPRESS; break;
    case QEVENT_KEYRELEASE:
      zcFlag=QWindow::CF_KEYRELEASE; break;
    case QEVENT_MOTIONNOTIFY:
      zcFlag=QWindow::CF_MOTIONNOTIFY; break;
    case QEVENT_EXPOSE:
      zcFlag=QWindow::CF_EXPOSE; break;
    default:
      zcFlag=0; break;
  }
  return zcFlag;
}

bool QWindowManager::ProcessEvent(QEvent *e)
// Send the event to the corresponding event window,
// and/or if it doesn't handle the event, propagate it to its
// parent(s) if applicable (some events never propagate).
{ QWindow *w;
  int zcFlag;           // Q catch flags
  int i,x,y;
  bool r=FALSE;
  static int skip=0;    // Exposes to ignore
  QRect rWin;

#ifdef ND_DBG
if(e->type!=QEvent::MOTIONNOTIFY&&WindowExists(e->win))
{QRect pos;
 e->win->GetPos(&pos);
qdbg("QWMGR:ProcessEvt: event %d in win %p (%s) (%d,%d %dx%d) n=%d"
  " (w%d,%d %dx%d)\n",
  e->type,e->win,e->win?e->win->ClassName():"-",e->x,e->y,e->wid,e->hgt,e->n,
  pos.x,pos.y,pos.wid,pos.hgt);
}
#endif

  // System keys
#ifndef WIN32
  if(e->type==QEVENT_KEYPRESS&&e->n==(QK_ALT|QK_F4))
  { // Request to close in a graceful way
    e->type=QEvent::CLOSE;
    e->win=QSHELL;
  }

  // Define a hard escape key to get out of nasty situations
  if(e->type==QEVENT_KEYPRESS&&e->n==(QK_CTRL|QK_ALT|QK_DEL))
  { //printf("** Q quit\n");
    app->Exit();
  }
#endif

  // Check if window is valid
  w=e->win;
  if(!w)return FALSE;

  // Window has been destroyed perhaps
  if(!WindowExists(w))return FALSE;
  // Handle keyboard focus
  if(e->type==QEvent::KEYPRESS||e->type==QEvent::KEYRELEASE)
  { if(keyboardFocus)
    { r=keyboardFocus->Event(e);
      if(r)
        return TRUE;
    }
  }

  // Handle mouse capturing
  if(e->type==QEvent::BUTTONPRESS||e->type==QEvent::BUTTONRELEASE||
     e->type==QEvent::MOTIONNOTIFY)
  { if(mouseCapture)
    {
#ifdef ND_NO_RELOCATE
      // Relocate mouse position to capture window
      mouseCapture->GetRootPos(&rWin);
      e->x=e->xRoot-rWin.x;
      e->y=e->yRoot-rWin.y;
#endif
      r=mouseCapture->Event(e);
      if(r)
        return TRUE;
    }
  }

  // Window has been destroyed perhaps (in focus calls)
  if(!WindowExists(w))return FALSE;

  // Handle hotkeys (ShortCut())
  if(e->type==QEvent::KEYPRESS)
  { for(i=0;i<maxWindow;i++)
    { if(winList[i])
      { if(winList[i]->Catches(QWindow::CF_KEYPRESS))
        { r=winList[i]->EvKeyPress(e->n,e->x,e->y);
          if(r)
          { // Break on first window to handle event
            return TRUE;
          }
        }
      }
    }
  }

 next:
  // Disabled windows only respond to expose events
  if(e->type!=QEvent::EXPOSE&&w->IsEnabled()==FALSE)
    return FALSE;

//qdbg("4\n");
//qdbg("  QWM; w eats event?\n");

  // See if window eats event
  r=w->Event(e);
  if(r)return TRUE;

//qdbg("5\n");
//qdbg("  QWM; propagate %d?\n",e->type);
  // Propagate event to parent 
  switch(e->type)
  { case QEvent::KEYPRESS:
    case QEvent::KEYRELEASE:
    case QEvent::BUTTONPRESS:
    case QEvent::BUTTONRELEASE:
    case QEvent::MOTIONNOTIFY:
    case QEvent::CLICK:
    case QEvent::DBL_CLICK:
      w=w->GetParent();
      if(w)goto next;
      break;
  }

#ifdef OBS
 next:
  if(w)
  { // Check if window still exists
    if(WindowExists(w))
    { if(w->IsEnabled()==TRUE||e->type==QEvent::EXPOSE)
      {
        //qdbg("  try %p/%s\n",w,w->Name());
        /*if(!w->IsEnabled())
        { qdbg("*** Leak EXPOSE to %s\n",w->ClassName());
        }*/
        if(e->type==QEvent::KEYPRESS)
        { // Keyboard focus window get first right
          if(keyboardFocus)
          { r=keyboardFocus->EvKeyPress(e->n,e->x,e->y);
            if(r)
              return TRUE;
          }
        }
        if(w->Event(e))
          return TRUE;
        // Expose events don't propagate
        if(e->type!=QEvent::EXPOSE)
        { // Try parent's interest
          w=w->GetParent();
          goto next;
        }
      }
    }
  }
#endif

//qdbg("QWMGR:ProcessEvt RET\n");
  return r;
}
void QWindowManager::PushEvent(QEvent *e)
// Push a Q event
// Uses Q's queue; no real Q event queue exists
{ 
#ifdef OBS
  QEvent qe;
  e->MapToQ(&qe);
  QEventPush(&qe);
#else
  QEventPush(e);
#endif
}

void QWindowManager::Paint()
// Paint entire user interface
{ int i;
  //qdbg("QWM:Paint\n");
  RestoreBackground(app->GetShell());
  for(i=0;i<maxWindow;i++)
  { if(winList[i])
    { if(winList[i]->IsVisible())
      { //DEBUG_F(qdbg("  paint win %p ",winList[i],winList[i]->ClassName()));
        //DEBUG_F(qdbg("(%s)\n",winList[i]->ClassName()));
        winList[i]->Paint();
      }
    }
  }
  //qdbg("QWM:Paint RET\n");
}

void QWindowManager::WaitForExposure()
// Wait until an Expose event drops in, so we know we are displayed
{
  // Q event loop
qdbg("waitforexposure\n");
  QEvent e;
  while(1)
  { if(QEventPending())
    { QEventNext(&e);
      //printf("event %d\n",e.type);
      if(e.type==QEVENT_EXPOSE)
      { //printf("WaitForExp finds Exposure\n");
        while(QEventPending())QEventNext(&e);   // Flush
        break;
      }
    } else QNap(1);             // Be nice to other apps and X ofcourse
  }
}

/*********************
* BACKGROUND SUPPORT *
*********************/
void QWindowManager::SetBackgroundColor(QColor *col)
{ colBG->SetRGBA(col->GetR(),col->GetG(),col->GetB());
}

static void RectUnion(QRect *r1,QRect *r2,QRect *d)
{
  if(r1->x<r2->x)d->x=r2->x;
  else           d->x=r1->x;
  if(r1->y<r2->y)d->y=r2->y;
  else           d->y=r1->y;
  if(r1->x+r1->wid<r2->x+r2->wid)d->wid=(r1->x+r1->wid)-d->x;
  else                           d->wid=(r2->x+r2->wid)-d->x;
  if(r1->y+r1->hgt<r2->y+r2->hgt)d->hgt=(r1->y+r1->hgt)-d->y;
  else                           d->hgt=(r2->y+r2->hgt)-d->y;
  // Restrict
  if(d->wid<0)d->wid=0;
  if(d->hgt<0)d->hgt=0;
}

static void TileImage(QCanvas *cv,QImage *img,QRect *r)
// Tile an image across the canvas
{ int iwid,ihgt;
  int x,y;
  int txL,txR;                  // Left & right tile min/max
  int tyT,tyB;                  // Top/bottom
  QRect tr;                     // Tile rect
  QRect rPaint;                 // Part of the tile that should be painted

  if(!img)return;

  // Find the enclosing set of tiles
  iwid=img->GetWidth();
  ihgt=img->GetHeight();
  txL=r->x/iwid;
  txR=((r->x+r->wid-1)/iwid);
  tyT=r->y/ihgt;
  tyB=(r->y+r->hgt-1)/ihgt;
 
  cv->Select(); //glDrawBuffer(GL_FRONT);
  /*
  printf("tileimage\n");
  printf("  img=%p, cv=%p, r=%p\n",img,cv,r);
  printf("  idims=%dx%d, r=%d,%d %dx%d\n",iwid,ihgt,r->x,r->y,r->wid,r->hgt);
  printf("  tile from %d-%d, y %d-%d\n",txL,txR,tyT,tyB);
  */
  //qdbg("  tileimage\n");
  cv->Blend(FALSE);             // No blending tiles
  tr.SetSize(iwid,ihgt);
  for(x=txL;x<=txR;x++)
  { for(y=tyT;y<=tyB;y++)
    { // Calc tile location
      tr.SetXY(x*iwid,y*ihgt);
      RectUnion(&tr,r,&rPaint);
      /*printf("  union paint %d,%d %dx%d\n",
        rPaint.x,rPaint.y,rPaint.wid,rPaint.hgt);*/
      // Paint translated tile
      //qdbg("  blit\n");
      if(rPaint.wid>0&&rPaint.hgt>0)
        cv->Blit(img,rPaint.x,rPaint.y,rPaint.wid,rPaint.hgt,
          rPaint.x-tr.x,rPaint.y-tr.y);
      //qdbg("  blit end\n");
    }
  }
  //printf("TileImage RET\n");
}

void QWindowManager::RestoreBackground(QWindow *w,QRect *r)
// Restore part of or the entire background
{ QRect *rr=r;
  QRect rdef;
  QCanvas *cv;
//qdbg("QWinMgr: restorebgr w=%p\n",w);
  if(!w)return;
  cv=w->GetCanvas();
  //QCanvas *cv=app->GetShell()->GetCanvas();
  //qdbg("QWMgr: restorebgr %p\n",r);
  if(r==0)
  { rdef.SetXY(0,0);
    //rdef.SetSize(app->GetWidth(),app->GetHeight());
    rdef.SetSize(w->GetWidth(),w->GetHeight());
    rr=&rdef;
  }
//qdbg("QWM:RestoreBgr: w=%p, hasbgr=%d\n",w,w->HasImageBackground());
  if(imgBgr==0||w->HasImageBackground()==FALSE)
  { 
//qdbg("restbgr r!=0\n");
    cv->Select();
    cv->SetColor(QApp::PX_DKGRAY);
    //cv->SetColor(QApp::PX_MDGRAY);
    cv->Rectfill(rr);
//qlog(QLOG_INFO,"rectfill %d,%d,%d,%d\n",r->x,r->y,r->wid,r->hgt);
//glFlush();
  } else
  { 
//qdbg("tile cv=%p, imgBgr=%p, rr=%p\n",cv,imgBgr,rr);
//qdbg("  rr %d,%d %dx%d\n",rr->x,rr->y,rr->wid,rr->hgt);
    TileImage(cv,imgBgr,rr);
  }
//w->GetQXWindow()->Swap();
//qdbg("RestoreBgr RET\n");
}

void QWindowManager::SetBackground(string fileName)
{
  //printf("setbgr %s\n",fileName);
  if(imgBgr)delete imgBgr;
  imgBgr=new QImage(fileName);
  if(!imgBgr->IsRead())
  { // Don't attempt to use this file
    delete imgBgr;
    imgBgr=0;
  }
  //Paint();
}

/*******************
* Window iteration *
*******************/
int QWindowManager::GetNoofWindows()
{ return activeWins;
}
QWindow *QWindowManager::GetWindowN(int n)
// Returns 0 if out of windows
{ int w=0,i;
  for(i=0;i<maxWindow;i++)
  { if(winList[i])
    { if(w==n)
        return winList[i];
      w++;
    }
  }
  return 0;
}

bool QWindowManager::AddWindow(QWindow *win)
{ int i;
  //qdbg("QWinMgr: AddWindow(); activeWins=%d\n",activeWins);
  if(activeWins>=QWM_MAX_WINDOW)
  { qerr("WindowManager: max number of windows reached (%d)",QWM_MAX_WINDOW);
    return FALSE;
  }
  for(i=0;i<maxWindow;i++)
  { if(!winList[i])
    { winList[i]=win;
      activeWins++;
      return TRUE;
    }
  }
  return FALSE;
}
bool QWindowManager::RemoveWindow(QWindow *win)
// Returns TRUE if 'win' was found
{
  DBG_C("RemoveWindow")
  DBG_ARG_P(win)
  QASSERT_VALID()

  int i;
  //qdbg("QWinMgr: RemoveWindow(); activeWins=%d\n",activeWins);
  for(i=0;i<maxWindow;i++)
  { if(winList[i]==win)
    { winList[i]=0;
      activeWins--;
      // Check for mouse capture/focus
      if(keyboardFocus==win)keyboardFocus=0;
      if(mouseCapture==win)mouseCapture=0;
      return TRUE;
    }
  }
  qdbg("  RemoveWindow: win %p was NOT found\n",win);
  return FALSE;
}

bool QWindowManager::WindowExists(QWindow *win)
{
  int i;
  for(i=0;i<maxWindow;i++)
  { if(winList[i]==win)
      return TRUE;
  }
  return FALSE;
}

/********
* FOCUS *
********/
bool QWindowManager::SetKeyboardFocus(QWindow *w)
// Set keyboard focus on specific window.
// Old focus gets EvLoseFocus() call, new focus receives a EvSetFocus() call
// (immediately! not using the event queue)
// Returns FALSE if focus could NOT be set. This may happen when
// the control losing focus refuses to lose focus.
// Returns TRUE if all is ok and you can assume the focus was changed.
// You may pass 'w'==0 to get no focus at all
{ //QWindow *oldFocus;
  QEvent e;

  // Check first if the focus really changes
  // (this helps for clicking into edit controls for example,
  // not having to generate CLICK events every time)
  if(keyboardFocus==w)return TRUE;
  if(keyboardFocus)
  { // Exit the current focus window
    e.type=QEvent::LOSEFOCUS;
    e.win=keyboardFocus;
    PushEvent(&e);
#ifdef ND_NO_METHOD_TO_CANCEL_LOSING_FOCUS
    if(keyboardFocus->EvLoseFocus()==TRUE)
    { // Control refuses to lose focus; cancel the operation
      return FALSE;
    }
#endif
    //oldFocus=keyboardFocus;
    //keyboardFocus=w;			// Notify future focus
    //oldFocus->EvLoseFocus();
  }
  // Focus on new window
  keyboardFocus=w;
  if(keyboardFocus)
  { e.type=QEvent::SETFOCUS;
    e.win=keyboardFocus;
    PushEvent(&e);
#ifdef ND_NO_DIRECT_CALL
    keyboardFocus->EvSetFocus();
#endif
  }
  return TRUE;
}

// Focus moves

void QWindowManager::FocusFirst()
// Sets the input focus to the first control
{
  int n,min,i;
  QWindow *w,*bestWin;

//qdbg("QWM:FocusFirst()\n");
  min=999999999;
  bestWin=0;
  n=GetNoofWindows();
  for(i=0;i<n;i++)
  { w=GetWindowN(i);
    if(w->IsEnabled()&&w->IsVisible())
    { if(w->GetTabStop()>=0&&w->GetTabStop()<min)
      { min=w->GetTabStop();
        bestWin=w;
      }
    }
  }
  if(bestWin)
  {
//qdbg("  moving focus to %p\n",bestWin);
    SetKeyboardFocus(bestWin);
  }
}
void QWindowManager::FocusLast()
// Sets the input focus to the last control
{
  int n,min,i;
  QWindow *w,*bestWin;

  min=-1;
  bestWin=0;
  n=GetNoofWindows();
  for(i=0;i<n;i++)
  { w=GetWindowN(i);
    if(w->IsEnabled()&&w->IsVisible())
    { if(w->GetTabStop()>min)
      { min=w->GetTabStop();
        bestWin=w;
      }
    }
  }
  if(bestWin)
  {
    SetKeyboardFocus(bestWin);
  }
}
void QWindowManager::FocusNext()
// Sets the input focus to the next control
{
  int cur,n,min,i;
  QWindow *w,*bestWin;

//qdbg("QWM:FocusNext()\n");
  if(GetKeyboardFocus()==0)cur=-1;
  else cur=GetKeyboardFocus()->GetTabStop();
  min=999999999;
  bestWin=0;
  n=GetNoofWindows();
  for(i=0;i<n;i++)
  { w=GetWindowN(i);
    if(w->IsEnabled()&&w->IsVisible())
    { if(w->GetTabStop()>cur&&w->GetTabStop()<min)
      { min=w->GetTabStop();
//qdbg("  min set to %d; w=%s %d,%d %dx%d\n",min,w->ClassName(),
  //w->GetX(),w->GetY(),w->GetWidth(),w->GetHeight());
        bestWin=w;
      }
    }
  }
  if(bestWin)
  {
//qdbg("  moving focus to %p; %s\n",bestWin,bestWin->ClassName());
    SetKeyboardFocus(bestWin);
  } else
  { // No next; loop back to first control
    FocusFirst();
  }
}
void QWindowManager::FocusPrev()
// Sets the input focus to the previous control
{
  int cur,n,max,i;
  QWindow *w,*bestWin;

//qdbg("QWM:FocusPrev()\n");
  if(GetKeyboardFocus()==0)cur=-1;
  else cur=GetKeyboardFocus()->GetTabStop();
  max=-1;
  bestWin=0;
  n=GetNoofWindows();
  for(i=0;i<n;i++)
  { w=GetWindowN(i);
    if(w->IsEnabled()&&w->IsVisible())
    { if(w->GetTabStop()<cur&&w->GetTabStop()>max)
      { max=w->GetTabStop();
        bestWin=w;
      }
    }
  }
  if(bestWin)
  {
//qdbg("  moving focus to %p\n",bestWin);
    SetKeyboardFocus(bestWin);
  } else FocusLast();
}

int QWindowManager::GetMaxTabStop()
// Returns the maximum tabstop index in use by any window.
// Disabled windows are also taken into account.
// This function is used for example to generate automatic tab stop indices.
{
  int max,n,i;
  QWindow *w;

  max=-1;
  n=GetNoofWindows();
  for(i=0;i<n;i++)
  { w=GetWindowN(i);
    if(w->GetTabStop()>max)
    { max=w->GetTabStop();
    }
  }
  return max;
}

/****************
* MOUSE CAPTURE *
****************/
void QWindowManager::BeginMouseCapture(QWindow *w)
{
  mouseCapture=w;
}
void QWindowManager::EndMouseCapture()
{
  mouseCapture=0;
}

QWindow *QWindowManager::WhichWindowAt(int x,int y,int cFlag,Window xw)
// Returns the window which is currently on top (and visible) at point (x,y)
// If 'xw' != 0, only QWindow in 'xw' are considered
{ int i;
  QWindow *w;
  QRect pos;
  //printf("WhichWindowAt(%d,%d, f%d, xw=%p)\n",x,y,cFlag,xw);
  for(i=maxWindow-1;i>=0;i--)
  { if(winList[i])
    { // Filter out QWindow that don't match xw
      if(xw!=0&&winList[i]->GetQXWindow()->GetXWindow()!=xw)continue;
      if(winList[i]->IsVisible()!=0&&winList[i]->Catches(cFlag))
      { 
        winList[i]->GetPos(&pos);
        /*qdbg("  wwa: [%d].xw=%p; %d,%d %dx%d\n",
          i,winList[i]->GetXWindow(),pos.x,pos.y,pos.wid,pos.hgt);*/
        if(winList[i]->IsX())
        { // (Q)X windows 'pos' member are relative to root
          // Don't use pos.x/pos.y, but use 0,0 as base
          if(0<=x&&0<=y&&pos.wid>x&&pos.hgt>y)
            return winList[i];
        } else
        { // Q windows are relative to the QXWindow they live in
          if(pos.x<=x&&pos.y<=y&&
            pos.x+pos.wid>x&&pos.y+pos.hgt>y)
          { return winList[i];
          }
        }
      }
    }
  }
  //printf(" no win found\n");
  return 0;
}

QWindow *QWindowManager::FindOSWindow(Window xw,int x,int y)
// Find window in which event happened; 'w' is the real OS window id
// (x,y) is the coordinate in the window.
// The child window is found within the indow (the smallest subwindow)
{
  QWindow *w;
  QRect    r;
  int i;
  int      bestArea;
  QWindow *bestWindow;

//qdbg("QWM:FindWindow(%p,%d,%d)\n",xw,x,y);
  //bestArea=0;			// Not needed
  bestWindow=0;
  for(i=0;i<maxWindow;i++)
  { w=winList[i];
    if(w)
    { if(w->GetQXWindow()->GetXWindow()==xw)
      { // Is the cursor in the window?
        if(w->IsVisible()==FALSE||w->IsEnabled()==FALSE)continue;
        w->GetXPos(&r);
        // X windows have an offset to their parent; ignore
        if(w->IsX())
        { r.x=r.y=0;
        }
//qdbg("  win %d,%d %dx%d contains %d,%d?\n",r.x,r.y,r.wid,r.hgt,x,y);
        if(r.Contains(x,y))
        { // Is this a better (smaller) window than the last hit?
//qdbg("  hit win %p (%s) area %d\n",w,w->ClassName(),r.Area());
          if(bestWindow==0||r.Area()<bestArea)
          { bestArea=r.Area();
            bestWindow=w;
          }
        }
      }
    }
  }
//qdbg("  found win %p (%s)\n",winList[i],winList[i]->ClassName());
  return bestWindow;
}

QWindow *QWindowManager::FindXWindow(Window w)
// Find window which owns X window 'w'
// Mostly this will be the shell or the bc (or other QDraw derivatives)
{ QWindow *xw;
  int i;

//qdbg("QWinMgr::FindXWindow(%p)\n",w);
  for(i=0;i<maxWindow;i++)
  { if(winList[i])
    { if(winList[i]->IsX())
      { if(winList[i]->GetQXWindow()->GetXWindow()==w)
        { 
          return winList[i];
        }
      }
    }
  }
  return 0;
}

void QWindowManager::Run()
{
}

/**********
* DIALOGS *
**********/
#ifdef OBS_OLD_DIALOG
void QWindowManager::ShowDialog(QDialog *d)
{ QWMInfo *i=d->GetQWMInfo();
  // Fill info in dialog
  i->winList=winList;
  i->activeWins=activeWins; 
  i->maxWindow=maxWindow;
  i->keyboardFocus=keyboardFocus;
  // New window space for dialog
  maxWindow=QWM_MAX_WINDOW;
  winList=(QWindow**)qcalloc(maxWindow*sizeof(QWindow*));
  activeWins=0;
  keyboardFocus=0;
}

void QWindowManager::HideDialog(QDialog *d)
{ QWMInfo *i=d->GetQWMInfo();
  // Fill info in dialog
  winList=i->winList;
  activeWins=i->activeWins; 
  maxWindow=i->maxWindow;
  keyboardFocus=i->keyboardFocus;
}
#endif
