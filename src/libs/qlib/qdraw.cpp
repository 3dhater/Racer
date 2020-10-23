/*
 * QDraw - user-drawn window (easy paint overriding)
 * 28-02-98: Created!
 * BUGS:
 * FUTURE:
 * (C) MarketGraph/RVG
 */

#include <qlib/app.h>
#include <qlib/gel.h>
#include <qlib/draw.h>
#include <qlib/opengl.h>
#include <qlib/info.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QDraw::QDraw(QWindow *parent,int x,int y,int wid,int hgt,QDrawPaintProc pp,
  void *ud)
  : QWindow(parent,x,y,wid,hgt)
  //: QShell(parent,x,y,wid,hgt)
{
  SetPaintProc(pp,ud);
  MakeRealX();
  flags=0;
  Catch(CF_MOTIONNOTIFY|CF_BUTTONPRESS|CF_BUTTONRELEASE);
}
QDraw::~QDraw()
{
}

void QDraw::Paint(QRect *r)
// Repaint entirely (after an expose)
{ 
  //qdbg("QDraw::Paint\n");
  if(paintProc)
    paintProc(this,userData);
}

void QDraw::SetPaintProc(QDrawPaintProc pp,void *ud)
{
  paintProc=pp;
  userData=ud;
}

bool QDraw::ScreenShot(cstring fileName)
{
  QBitMap *bm;
  bm=new QBitMap(32,GetWidth(),GetHeight());
  // Get front buffer image
  cv->Select();
  glReadBuffer(GL_FRONT);
  cv->ReadPixels(bm);
  glReadBuffer(GL_BACK);
  // Save image
  bm->Write(fileName,QBMFORMAT_TGA);
  delete bm;
  return TRUE;
}

void QDraw::Swap()
// Swaps buffers of the associated X window
// Note that this only works for double-buffered windows, AND ofcourse
// the entire X window is swapped. (mind non-X child windows)
{ 
  QXWindow *qxw;
  qxw=GetQXWindow();
  if(qxw)
    qxw->Swap();
}
