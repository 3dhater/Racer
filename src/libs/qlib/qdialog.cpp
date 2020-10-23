/*
 * QDialog - basic dialog support
 * 25-05-97: Created!
 * 21-12-99: Some changes for new window system.
 * 18-03-00: Titlebar added (with close button). Use that to drag now.
 * NOTES:
 * - Dialog implementation is still very ugly, since it doesn't really fit
 *   in well with the QApp's idea of an application.
 *   Hacks around.
 * - Cursor is inherited from parent.
 * FUTURE:
 * - Modeless dialogs (mind keyboard focus!)
 * - Modal, system modal, application modal
 * (C) MG/RVG
 */

#include <qlib/dialog.h>
#include <qlib/opengl.h>
#include <qlib/app.h>
#include <bstring.h>
#include <X11/Xlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#ifndef WIN32
// Define USE_DRAG to enable dialog dragging; a bit slow now
#define USE_DRAG
#endif

static int orgX,orgY;		// Drag original location

QDialog::QDialog(QWindow *parent,QRect *r,cstring title)
  : QShell(parent,r->x,r->y,r->wid,r->hgt)
{
//qdbg("QDialog ctor\n");
  //bzero(&wmInfo,sizeof(wmInfo));
  bDrag=FALSE;
  // Remember keyboard focus to return to later on
  oldKeyboardFocus=QWM->GetKeyboardFocus();

  // Disable all parent windows
  int n,i;
  QWindow *w;
  n=app->GetWindowManager()->GetNoofWindows();
  for(i=0;i<n;i++)
  { w=app->GetWindowManager()->GetWindowN(i);
    if(w!=this)
      w->Disable();
  }
  CompressMotion();
  Create();

  titleBar=new QTitleBar(this,title);
  // User interface is single buffered
  //cv->SetMode(QCanvas::SINGLEBUF);

  Catch(CF_BUTTONPRESS|CF_BUTTONRELEASE|CF_MOTIONNOTIFY|CF_EXPOSE);
//qdbg("QDialog ctor RET\n");
}
QDialog::~QDialog()
{
//qdbg("QDialog dtor\n");
  // (Re)Enable all parent windows
  int n,i;
  QWindow *w;
  n=app->GetWindowManager()->GetNoofWindows();
  for(i=0;i<n;i++)
  { w=app->GetWindowManager()->GetWindowN(i);
    if(w!=this)
      w->Enable();
  }
  if(titleBar)delete titleBar;
  // Return keyboard focus to control before dialog was put up
  QWM->SetKeyboardFocus(oldKeyboardFocus);
}

void QDialog::Show()
{
  //Invalidate();
  //app->GetWindowManager()->Paint();
  GetQXWindow()->Map();
}

void QDialog::Hide()
{
  GetQXWindow()->Unmap();
}

int QDialog::Execute()
{
//qdbg("QDlg: execute\n");
  Show();
  cv->Clear();
  Swap();
  QNap(200);
  Hide();
  return 0;
}

void QDialog::Paint(QRect *r)
// Overrule by your dialog class
{
}

/*********
* EVENTS *
*********/
bool QDialog::Event(QEvent *e)
{
//qdbg("QDialog:Event(%d) win=%p\n",e->type,e->win);
#ifdef OBS
  // Drag only with titleBar
  if(e->type==QEvent::BUTTONPRESS||e->type==QEvent::BUTTONRELEASE)
  { if(e->win!=titleBar)return FALSE;
  }
#endif
  return QWindow::Event(e);
}

bool QDialog::EvButtonPress(int n,int x,int y)
{
  //qdbg("QDlg: button press\n");
#ifdef USE_DRAG
  QEvent *e=app->GetCurrentEvent();
  QRect pos;

  if(e->win!=titleBar)return FALSE;
  bDrag=TRUE;
  GetPos(&pos);
  dragX=e->xRoot; dragY=e->yRoot;
  orgX=pos.x;
  orgY=pos.y;
//qdbg("QDlg: drag=%d,%d, org=%d,%d\n",dragX,dragY,orgX,orgY);
  //Focus(TRUE);
  QWM->BeginMouseCapture(titleBar);
  //QWM->BeginMouseCapture(this);
  //app->GetWindowManager()->HandleExpose(FALSE);
#endif
  return FALSE;
}

bool QDialog::EvMotionNotify(int x,int y)
{ int nx,ny;
  QEvent *e;
  if(!bDrag)return FALSE;
  // Get root x/y from this event
  e=app->GetCurrentEvent();
  x=e->xRoot; y=e->yRoot;
//qdbg("QDlg:EvMotion(%d,%d)\n",x,y);

  // Move window
  nx=orgX+x-dragX;
  ny=orgY+y-dragY;
  //qerr("QDialog:EvMotionNotify; no drag NYI");
  Move(nx,ny);
#ifdef OBS
  pos->x=nx;
  pos->y=ny;
  XMoveWindow(app->GetDisplay()->GetXDisplay(),GetXWindow(),pos->x,pos->y);
#endif
  //bDrag=FALSE;
  return TRUE;
}

bool QDialog::EvButtonRelease(int n,int x,int y)
{ int nx,ny;
#ifdef USE_DRAG
  bDrag=FALSE;
  //Focus(FALSE);
  QWM->EndMouseCapture();
#endif
  return FALSE;
}

bool QDialog::EvExpose(int x,int y,int wid,int hgt)
{
//qdbg("QDialog::EvExpose %d,%d %dx%d\n",x,y,wid,hgt);
  QShell::EvExpose(x,y,wid,hgt);
  Paint();
  return TRUE;
}

