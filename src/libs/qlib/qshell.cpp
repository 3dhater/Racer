/*
 * QShell - placeholder for smaller windows (Form; Frame)
 * 28-04-1997: Created!
 * 21-02-1998: Stripped, since QWindows are now all XWindows
 * 09-08-1999: Now derived from QDraw (was QWindow)
 * FUTURE:
 * - EvExpose() should see if the damaged area hits the outline, and only
 *   repaint the border if needed.
 * (C) MarketGraph/RVG
 */

#include <qlib/shell.h>
#include <qlib/app.h>
#include <qlib/display.h>
#include <qlib/opengl.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Default paint procedure (callback)
static void UIPaint(QDraw *draw,void *ud)
{ QShell *sh;
  sh=(QShell*)ud;
  if(!sh)return;
  sh->Paint();
}

QShell::QShell(QWindow *parent,int x,int y,int wid,int hgt) //,QXWindow *ishare)
  //: QWindow(parent,x,y,wid,hgt)
  : QDraw(parent,x,y,wid,hgt)
{
  // Install paint callback
  SetPaintProc(UIPaint,this);
  CompressExpose();
}
QShell::~QShell()
{
//qdbg("QShell dtor\n");
}

void QShell::Paint(QRect *r)
{
//qdbg("QShell::Paint\n");
  Restore();
  cv->Outline(0,0,GetWidth(),GetHeight());
}

bool QShell::EvExpose(int x,int y,int wid,int hgt)
{ 
//qlog(QLOG_INFO,"QShell::EvExpose(%d,%d %dx%d)\n",x,y,wid,hgt);
  QRect r(x,y,wid,hgt);
  // Restore only the damaged part
  app->GetWindowManager()->RestoreBackground(this,&r);
  //Paint();
  // Paint outline in case it is damaged by 'r'
  cv->Outline(0,0,GetWidth(),GetHeight());
  return TRUE;
}

