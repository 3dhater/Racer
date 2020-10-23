/*
 * QGroup - group rectangle (nothing more)
 * 23-04-97: Created!
 * 08-11-98: X-window, text a little lower.
 * (C) MarketGraph/RVG
 */

#include <qlib/group.h>
#include <qlib/app.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

QGroup::QGroup(QWindow *parent,QRect *ipos,string itext)
  : QWindow(parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
{
  //printf("zgroup ctor, text=%p\n",itext);
  // Text color
  colText=new QColor(0,0,0);
  font=app->GetSystemFont();
  text=itext;
  // No events catched
  Create();
}

QGroup::~QGroup()
{
  if(colText)delete colText;
}

void QGroup::SetText(string ntext)
{ text=ntext;
}

void QGroup::Paint(QRect *r)
{ QRect rr;
  int sw=4;		// Shadow width/height
  int yOffset;		// Offset because of font height

#ifdef ND_NEVER_RESTORE_OR_INVALIDATE_WINDOW_CLIENT_AREA
  // Don't restore when no text is visible; just paint border
  // (faster when refreshing border around QBC for example; no flashing)
  if(text)
    Restore();
#endif

  if(text)
    yOffset=font->GetAscent()/2;
  else
    yOffset=0;

  QRect pos;
  GetPos(&pos); pos.x=pos.y=0;
  cv->Outline(pos.x,pos.y+yOffset,pos.wid,pos.hgt-yOffset);

  // Draw text if any
  if(text)
  { int tx,ty,twid,thgt;
    cv->SetFont(font);
    // Center text in button
    thgt=font->GetHeight();
    twid=font->GetWidth(text);
    tx=pos.x+10;
    //ty=pos.y-font->GetAscent()/2;
    ty=pos.y;
    /*printf("twid=%d, hgt=%d, x=%d, y=%d (pos=%d,%d %dx%d)\n",
      twid,thgt,tx,ty,pos->x,pos->y,pos->wid,pos->hgt);*/
    rr.SetXY(tx-4,ty);
    rr.SetSize(twid+2*4,thgt);
    app->GetWindowManager()->RestoreBackground(this,&rr);
    cv->SetColor(colText);
    cv->Text(text,tx,ty);
  }
}

