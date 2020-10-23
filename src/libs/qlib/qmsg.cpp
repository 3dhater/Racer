/*
 * QMsg - message window (for debug output and error messages)
 * 21-04-97: Created!
 * FUTURE:
 * - Probably the ctxCV can be cleared from Out(), because glXMakeCurrent()
 *   caches the current context.
 * (C) MarketGraph/RVG
 */

#include <qlib/msg.h>
#include <qlib/app.h>
#include <qlib/canvas.h>
#include <qlib/gel.h>
#include <qlib/window.h>
#include <qlib/opengl.h>
#include <qlib/app.h>
#include <stdio.h>
#include <stdarg.h>
#include <GL/glx.h>
#include <GL/gl.h>
//#include <unistd.h>
//#include <limits.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define borderWidth	2

#undef  DBG_CLASS
#define DBG_CLASS "QMsg"

QMsg::QMsg(QWindow *parent,int x,int y,int wid,int hgt,int captMask)
  : QWindow(parent,x,y,wid,hgt)
{ 
  DBG_C("ctor")
  
  flags=0;
  Create();
  //Move(x,y);
  //qdbg("  qmsgsize\n");
  //Size(wid,hgt);

#ifdef OBS
  // Create local cv
  //QWidget *wg=cv->GetQWidget();
  //cv=new QCanvas(cv);
  qdbg("  zmsg glc: %p\n",cv->GetGLContext()->GetGLXContext());
  //qdbg("  offset\n");
  cv->SetOffset(x,y);
  cv->SetClipRect(x,y,wid,hgt);
  cv->SetMode(QCANVAS_SINGLEBUF);
  //qdbg("  new col\n");
#endif

  col=new QColor(150,150,150);

  // Init cursor
  cx=cy=0;
  //qdbg("  setsysfont\n");
  SetFont(app->GetSystemFont());
  //qdbg("  capture\n");
  Capture(captMask);
}
QMsg::~QMsg()
{
}

static QMsg *captMsg;

static void qmsg(string s)
{ if(captMsg)
    captMsg->Out(s);
}

void QMsg::Capture(int cMask)
// Convenience function to capture Q messages into the box
{ if(cMask&CP_DEBUG)
    QSetDebugHandler(qmsg);
  if(cMask&CP_ERROR)
    QSetErrorHandler(qmsg);
  if(cMask&CP_WARNING)
    QSetWarningHandler(qmsg);
  if(cMask&CP_MESSAGE)
    QSetMessageHandler(qmsg);
  if(cMask)
    captMsg=this;
}

void QMsg::Paint(QRect *r)
// Repaint entirely (after an expose)
{ QCanvas *cvs=app->GetShell()->GetCanvas();
  //DEBUG_C(qdbg("QMsg::Paint\n"));
#ifdef OBS
  if(cvs->IsEnabled(QCANVASF_CLIP))
  { // Take over clipping (expose redraw)
    QRect r;
    cvs->GetClipRect(&r);
    cv->Enable(QCANVASF_CLIP);
    cv->SetClipRect(&r);
  } //else cv->Disable(QCANVASF_CLIP);
#endif
  cv->SetColor(col);
  QRect pos;
  GetPos(&pos); pos.x=pos.y=0;
  cv->Rectfill(borderWidth,borderWidth,
    pos.wid-borderWidth*2,pos.hgt-borderWidth*2);
  cv->Inline(0,0,pos.wid,pos.hgt);
  cv->Disable(QCanvas::CLIP);
  //DEBUG_C(qdbg("QMsg::Paint exit\n"));
  // No text currently
}

void QMsg::SetFont(QFont *nfont)
{ font=nfont;
  cwid=font->GetWidth("W"); if(!cwid)cwid=1;
  chgt=font->GetHeight(); if(!chgt)chgt=1;
  QRect pos;
  GetPos(&pos); pos.x=pos.y=0;
  maxCY=(pos.hgt-2*borderWidth)/chgt;
  maxCX=(pos.wid-2*borderWidth)/cwid;
  cv->SetFont(font);
}

void QMsg::ScrollUp()
{ // Move up; fast
  //printf("scrollup\n");
  /*printf("  copyp %d,%d, %dx%d %d,%d\n",
    pos->x+borderWidth,pos->y+chgt+borderWidth*1,
    pos->wid-borderWidth*2,pos->hgt-chgt-borderWidth*2,
    pos->x+borderWidth,pos->y+borderWidth);*/
  cv->Select();
  glReadBuffer(GL_FRONT);
  glDrawBuffer(GL_FRONT);
  QRect pos;
  GetPos(&pos); pos.x=pos.y=0;
  cv->CopyPixels(borderWidth,chgt+borderWidth*1,
    pos.wid-borderWidth*2,pos.hgt-chgt-borderWidth*2,
    borderWidth,borderWidth);
  /*cv->CopyPixels(pos->x+borderWidth,pos->y+chgt+borderWidth*1,
    pos->wid-borderWidth*2,pos->hgt-chgt-borderWidth*2,
    pos->x+borderWidth,pos->y+borderWidth);*/
  glReadBuffer(GL_BACK);
  //glDrawBuffer(GL_FRONT);
  // Clear last line
  //printf("  setcolor\n");
  cv->SetColor(col);
  //printf("  rectfill\n");
  cv->Rectfill(borderWidth,pos.hgt-chgt-borderWidth,
    pos.wid-2*borderWidth,chgt);
  cy--;
  //printf("scrollup ret\n");
}
void QMsg::Out(cstring s)
{
  //GLXContext ctx,ctxCV;		// Original and the one from our cv
  //GLXDrawable drawable;
  QGLContext *oldGLC;

  // Because this function may be called from almost anywhere, including
  // graphics calls, it is important NOT to disturb the current context.
  // This means we have to preserve the current context, although this
  // probably WILL cost a lot of time.
  // Consider QMsg::Out() calls from within QCanvas!
  oldGLC=GetCurrentQGLContext();
#ifdef OBSOLETE
  ctx=glXGetCurrentContext();
  drawable=glXGetCurrentDrawable();
#endif
  //printf("QMsg:Out: ctx=%p, drawable=%p\n",ctx,drawable);

  if(cy>=maxCY)
    ScrollUp();
  cv->Select();
  //cv->Text(s,pos->x+cx*cwid,pos->y+cy*chgt);
  cv->SetColor(0,0,0);
  cv->Text(s,cx*cwid+borderWidth,cy*chgt+borderWidth);
  ++cy;

  // Restore the context
  if(oldGLC)oldGLC->Select();
#ifdef OBSOLETE
  ctxCV=glXGetCurrentContext();
  if(ctxCV!=ctx)
  { glXMakeCurrent(app->GetDisplay()->GetXDisplay(),drawable,ctx);
  }
#endif
  /*printf("QMsg:Out: restored from %p to ctx=%p, drawable=%p\n",
    ctxCV,ctx,drawable);*/
}
void QMsg::printf(cstring fmt,...)
{ va_list args;
  char buf[256];

  va_start(args,fmt);
  vsprintf(buf,fmt,args);
  Out(buf);
  va_end(args);
}

