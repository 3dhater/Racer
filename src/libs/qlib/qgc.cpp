/*
 * QGC - wrapper around X GC context
 * 21-03-98: Created!
 * NOTES:
 * - Can only be used on X11-drawable drawables (windows/pixmaps)
 * - Does little on Win32
 * BUGS:
 * - Rectfill() shading is not implemented
 * (C) MarketGraph/RVG
 */

#include <qlib/opengl.h>
#include <qlib/gc.h>
#include <qlib/app.h>
#include <qlib/display.h>
#include <X11/Xlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QGC::QGC(QDrawable *w)
{
#ifndef WIN32
  XGCValues gcv;

  drw=w;
  flags=0;
  font=0;
  xgc=0;
  xgc=XCreateGC(XDSP,drw->GetDrawable(),0,&gcv);
#endif
}
QGC::~QGC()
{
#ifndef WIN32
  if(xgc)
    XFreeGC(XDSP,xgc);
#endif
}

void QGC::SetFont(QFont *f)
{
  QASSERT_V(f);
  //qdbg("QGC:SetFont()\n");
  font=f;
#ifndef WIN32
  XSetFont(XDSP,xgc,font->GetXFont());
#endif
}

void QGC::SetForeground(int indPixel)
// indPixel=QApp::PX_xxx
{
#ifndef WIN32
  //qdbg("QGC:SetFg\n");
  XSetForeground(XDSP,xgc,app->GetPixel(indPixel));
  //qdbg("pixel=%d\n",app->GetPixel(indPixel));
#endif
}

void QGC::Select()
{
  // Hmm
}

void QGC::Enable(int newFlags)
{
  flags|=newFlags;
}
void QGC::Disable(int clrFlags)
{
  flags&=~clrFlags;
}

void QGC::Text(cstring txt,int dx,int dy,int len)
{
  QASSERT_V(txt);
  //qdbg("QGC:Text\n");
  if(len==-1)
  { len=strlen(txt);
  }
#ifndef WIN32
  //glXWaitGL();
  XDrawString(XDSP,drw->GetDrawable(),xgc,dx,dy,txt,len);
#endif
}
void QGC::Rectfill(QRect *rz,QColor *colUL,QColor *colUR,QColor *colLR,
    QColor *colLL)
// May not do the shading
{
#ifndef WIN32
  XFillRectangle(XDSP,drw->GetDrawable(),xgc,rz->x,rz->y,rz->wid,rz->hgt);
#endif
}
 
void QGC::Triangle(int x1,int y1,int x2,int y2,int x3,int y3)
{
#ifndef WIN32
  XPoint pts[3];
  pts[0].x=x1; pts[0].y=y1;
  pts[1].x=x2; pts[1].y=y2;
  pts[2].x=x3; pts[2].y=y3;
  XFillPolygon(XDSP,drw->GetDrawable(),xgc,pts,3,Convex,CoordModeOrigin);
#endif
}

