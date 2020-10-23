/*
 * QProgress - progress bar
 * 23-10-98: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/progress.h>
#include <qlib/canvas.h>
#include <qlib/event.h>
#include <qlib/app.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define QBSW	4		// Shadow size

// USEX controls whether we use X GC or OpenGL to draw the button
// (OpenGL ctx switching is slow on O2)
//#define USEX

QProgress::QProgress(QWindow *parent,QRect *ipos,string itext)
  : QWindow(parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
{
  colShadow1=new QColor(40,40,40);
  colShadow2=new QColor(70,70,70);
  colText=new QColor(0,0,0);
  font=app->GetSystemFont();
  if(itext)
    text=qstrdup(itext);
  else text=0;

  align=CENTER;

  // Default is empty bar
  cur=0; total=1;

  // Take minimal amount of events
  Catch(0);
  CompressExpose();

#ifdef USEX
  PrefDefaultVisual();
#endif
  Create();
#ifdef USEX
  cv->UseX();
#endif

  // Add window to application (Z)
  //app->GetWindowManager()->AddWindow(this);
  //printf("zbutton ctor RET\n");
}

QProgress::~QProgress()
{
  QDELETE(colShadow1);
  QDELETE(colShadow2);
  QDELETE(colText);
  QFREE(text);
}

void QProgress::SetText(cstring ntext)
{ if(text)qfree(text);
  text=qstrdup(ntext);
}
void QProgress::SetTextColor(QColor *col)
{ colText->SetRGBA(col->GetR(),col->GetG(),col->GetB(),col->GetA());
}

void QProgress::SetProgress(int current,int tot,bool paint)
{
  cur=current;
  total=tot;
  // Normalize to avoid big numbers drift over the 32-bit edge
  while(cur>1000||total>1000){ cur/=10; total/=10; }
  if(paint)Paint();
}

void QProgress::Paint(QRect *r)
{ QRect rr;
  int sw=0;		// Shadow width/height

  if(!IsVisible())return;

  //printf("QB: restore\n");
  Restore();
  QRect pos;
  GetPos(&pos);
  pos.x=pos.y=0;		// Local

  cv->Inline(pos.x,pos.y,pos.wid,pos.hgt);
  //cv->Insides(pos.x+2,pos.y+2,
    //pos.wid-2*2-sw,pos.hgt-2*2-sw);

  // Paint border
  //if(app->GetWindowManager()->GetFocus()==this)
    //cv->Inline(pos.x,pos.y,pos.wid-sw,pos.hgt-sw);
  //else
    //cv->Outline(pos.x,pos.y,pos.wid-sw,pos.hgt-sw);

  // Draw progress
  int curwid,pwid;		// Current width in pixels, progress width

  // Calc width to draw filled (safely)
  pwid=pos.wid-2*4;		// Inside
  if(!total)curwid=pwid;
  else      curwid=cur*pwid/total;
  if(curwid<0)curwid=0; else if(curwid>pwid)curwid=pwid;

  // Draw progress
#ifdef USEX
  cv->SetColor(QApp::PX_LTGRAY);
#else
  cv->SetColor(140,140,140);
#endif
  cv->Rectfill(2,2,pos.wid-4,pos.hgt-2*2);
  if(curwid>0)
  {
#ifdef USEX
    cv->SetColor(QApp::PX_WHITE);
#else
    cv->SetColor(255,255,255);
#endif
    cv->Rectfill(4,4,curwid,pos.hgt-2*4);
  }

 do_text:
  // Draw text if any
  //qdbg("  draw text\n");
  if(text!=0)
  { int tx,ty,twid,thgt;
#ifdef USEX
    cv->SetColor(QApp::PX_BLACK);
#else
    cv->SetColor(colText);
#endif
    //qdbg("  Setfont\n");
    if(font)
    { int a;
      cv->SetFont(font);
      // Center text in button
      thgt=font->GetHeight();
      twid=font->GetWidth(text);
      if(align==CENTER)
      { tx=(pos.wid-sw-twid)/2+pos.x;
        a=QCanvas::ALIGN_CENTERH|QCanvas::ALIGN_CENTERV;
      } else if(align==LEFT)
      { tx=pos.x+4;
        a=QCanvas::ALIGN_CENTERV;
      } else if(align==RIGHT)
      { tx=pos.x+pos.wid-4-twid-sw;
        a=QCanvas::ALIGN_CENTERV;
      }
      ty=(pos.hgt-thgt)/2+pos.y /*+font->GetAscent()*/ ;
      /*printf("twid=%d, hgt=%d, x=%d, y=%d (pos=%d,%d %dx%d)\n",
        twid,thgt,tx,ty,pos.x,pos.y,pos.wid,pos.hgt);*/
#ifdef OLD
      cv->Text(text,tx,ty);
#else
      // New version; may have multiple lines of text
      rr.x=pos.x; rr.y=pos.y;
      rr.wid=pos.wid-sw; rr.hgt=pos.hgt-sw;
      //if(!a)r.y=ty;		// No centering? Center Y at least
      cv->TextML(text,&rr,a);
      //cv->Text(text,r.x,r.y);
#endif
    }
   skip_text:;
  }
}

