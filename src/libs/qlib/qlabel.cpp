/*
 * QLabel - a user interface label (may be image or text)
 * 11-04-97: Created!
 * 04-06-97: Now allocated memory to hold text
 * 21-09-99: Multiline support.
 * BUGS:
 * - Paint() doesn't clip all the time
 * (C) MarketGraph/RVG
 */

#include <qlib/label.h>
#include <qlib/app.h>
#include <qlib/image.h>
//#include <GL/gl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <qlib/debug.h>
DEBUG_ENABLE

static int noofLines(cstring s)
// Count number of lines in text 's'
{ int count;
  for(count=1;*s;s++)
    if(*s==10)count++;
  return count;
}

QLabel::QLabel(QWindow *parent,QRect *ipos,cstring itext,int flags)
  : QWindow (parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
{
  // Default is black text
  colText=new QColor(0,0,0);
  // Default font is Q system font
  font=app->GetSystemFont();
  bm=0;
  img=0;

  if(itext)text=qstrdup(itext);
  else     text=0;
  if(flags&IMAGE)
  { // itext is an image instead of a direct text
    img=new QImage(text);
    if(!img->IsRead())
    { bm=new QBitMap(32,10,10);
    } else
    { bm=img;
    }
#ifdef OBS
    // Resize label
    pos.wid=bm->GetWidth();
    pos.hgt=bm->GetHeight();
#endif
    Size(bm->GetWidth(),bm->GetHeight());
    //qerr("QLabel must resize itself!\n");
  } else
  { // Resize label to fit text
    if(text)
    { 
#ifdef OBS
      pos.wid=font->GetWidth(text);
      pos.hgt=font->GetHeight(text);
#endif
      Size(font->GetWidth(text),font->GetHeight(text)*noofLines(text));
      //qerr("QLabel must resize itself (text)!\n");
    }
  }
  Create();

  // A label doesn't catch anything
  Catch(CF_BUTTONPRESS);
}

QLabel::~QLabel()
{
  if(text)qfree(text);
  if(colText)delete colText;
  if(img)delete img;
}

void QLabel::SetFont(QFont *nfont)
{
  font=nfont;
  if(text)
    Size(font->GetWidth(text),font->GetHeight(text));
}

void QLabel::SetText(cstring ntext)
// Doesn't repaint
{
  if(text)qfree(text);
  if(ntext)
  { text=qstrdup(ntext);
    // Resize label to fit text
#ifdef OBS
    pos->wid=font->GetWidth(text);
    pos->hgt=font->GetHeight(text);
#endif
    Size(font->GetWidth(text),font->GetHeight(text));
    //qerr("QLabel:SetText doesn't resize yet NYI");
  }
  else text=0;
}

void QLabel::Paint(QRect *r)
{
  if(!IsVisible())return;

  Restore();
  QRect pos;
  GetPos(&pos);
  pos.x=pos.y=0;
  if(bm)
  { // Label is a bitmap
    // Should clip for max; picture size or window size (==label size)
    if(flags&IMAGE_NOBLEND)cv->Blend(FALSE);
    else                   cv->Blend(TRUE);
    cv->Blend(TRUE);
//if(flags&IMAGE_NOBLEND)qdbg("no blend"); else qdbg("BLEND lab!\n");
    cv->Blit(bm,pos.x,pos.y);
  } else
  { // Label is a text
    cv->SetColor(colText);
    cv->SetFont(font);
    //cv->Text(text,pos.x,pos.y);
    cv->TextML(text,&pos,0);
  }
}


bool QLabel::EvButtonPress(int button,int x,int y)
{ QEvent e;
  // Generate click event
  e.type=QEVENT_CLICK;
  e.win=this;
  app->GetWindowManager()->PushEvent(&e);
  return TRUE;
}

