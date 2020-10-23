/*
 * QText.cpp - text gel (behaves much like a bob)
 * 24-12-96: Created!
 * 26-12-96: Derived from QBob
 * NOTES:
 * - Perhaps it will be derived from QBob in the future.
 * BUGS:
 * - On O2, glReadPixels() with odd width seems to bug (RED channel is 0)
 *   Workaround: make sure ReadPixels is done with even width
 * - Italic fonts use probably algorithmic slants; their widths don't match
 *   with what is painted. May cross bitmap limits and dump core.
 * (C) MarketGraph/RVG
 */

#include <stdio.h>
#include <string.h>
#include <qlib/text.h>
#include <qlib/point.h>
#include <GL/gl.h>
#include <unistd.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Clear this when the bug is fixed
#define O2_TEXT_BUG

#define QTEXT_DEPTH	32	// Let's give it depth

static int marginX,marginY;	// Extra pixel margins
static int offsetX,offsetY;	// Pixel offset

// Global settings (should be static member functions)
void QTextSetMargins(int x,int y)
// Defines an extra size to allocate when generating texts
// This way, you can reserve space for shadows, anti-aliasing artifacts,
// an possible a scroll-function that clears its own track
{
  marginX=x;
  marginY=y;
}
void QTextSetOffset(int x,int y)
// Defines the offset of painted texts
// Use this for texts that are going to be outlined
// You could also use a shift/scroll filter and only use the margins
{
  offsetX=x;
  offsetY=y;
}
void QTextSetDefaults()
{ QTextSetMargins(0,0);
  QTextSetOffset(0,0);
}

QText::QText(QCanvas *_cv,QFont *_font,cstring _text,int flags)
  : QBob(0,flags|QGELF_DBLBUF|QGELF_BLEND)
  //: QGel(flags|QGELF_DBLBUF)
{
  //bob=0;
  bm=0;
  text=0;
  cv=_cv;
  font=_font;
  twid=thgt=0;
  col=new QColor(255,255,255);	// White is default color
  colg=0;

  QASSERT_V(cv);
  QASSERT_V(font);
  if(_text)SetText(_text,QBM_AA_ALG_9POINT);
}
QText::~QText()
{ //if(bob)delete bob;
  if(bm)delete bm;
  if(text)qfree((void*)text);
  if(colg)delete colg;
  if(col)delete col;
}

void QText::SetColor(QColor *ncol,QColor *ncolg)
// BUGS: Doesn't change color right away; call SetText()
{ //if(col)delete col;
  col->SetRGBA(ncol->GetR(),ncol->GetG(),ncol->GetB(),ncol->GetA());
  if(ncolg==0)
  { if(colg)delete colg;
    colg=0;
  } else
  { if(colg)
      colg->SetRGBA(ncolg->GetR(),ncolg->GetG(),ncolg->GetB(),ncolg->GetA());
    else
      colg=new QColor(ncolg->GetRGBA());
  }
}
void QText::SetFont(QFont *_font)
// Font will be active at next SetText()
{ font=_font;
}

/*********************
* CREATE TEXT BITMAP *
*********************/
void QText::SetText(cstring _text,int aaType)
// Should use some offscreen bitmap in the future if possible
{ QFont *oldFont;
  QRect r;
  QColor tCol(0);		// Black
  QBitMap *backbm;		// Backing store
  int ox,oy;
  cstring oldText;

  //qdbg("QText::SetText(%s:%p)\n",_text,_text);
  // First copy new text, then delete old (in case of
  // memory overlaps)
  oldText=text;			// In case text==_text
  text=qstrdup(_text);
  if(oldText)
  { //qdbg("QText::SetText: qfree(%s:%p)\n",text,text);
    qfree((void*)oldText);
    //qdbg("  (QText) _text=%s:%p\n",_text,_text);
  }
  //qdbg("QText::SetText(%s) after qstrdup\n",_text);
  //text=_text;
  // Allocate bitmap to contain the text
  twid=font->GetWidth(text);
  thgt=font->GetHeight();		// (text) ? Saves some pixels
  twid+=marginX;
  thgt+=marginY;
#ifdef O2_TEXT_BUG
  // Make sure width is even; bug on O2 glReadPixels()
  if(twid&1)twid++;
#endif
  if(!bm)
    bm=new QBitMap(QTEXT_DEPTH,twid,thgt);
  else
    bm->Alloc(QTEXT_DEPTH,twid,thgt);	// Reallocate (and clear)
  backbm=new QBitMap(QTEXT_DEPTH,twid,thgt);
  //printf("QText: bitmap %dx%d\n",twid,thgt);

  cv->ReadPixels(backbm,0,0,twid,thgt);
#ifdef NOTDEF
  // O2 glReadPixels() bonanza
  // Backup pixels that are about to be destroyed
  glFinish();
  glDisable(GL_BLEND);
  glDisable(GL_DITHER);
  cv->Disable(QCANVASF_BLEND);
  cv->ReadPixels(backbm,100,100,twid,thgt);
  printf("QText:SetText: blit\n");
  char *p=backbm->GetBuffer();
  for(int i=0;i<40;i++)printf("%02x ",*p++);
  printf("\n");
cv->Blit(backbm,100,100,twid,thgt,0,0);
return;
#endif

  // Paint in the hidden buffer; top lefthand corner; clear beforehand
  oldFont=cv->GetFont();
  cv->SetFont(font);
  cv->Blend(FALSE);
  //cv->Disable(QCANVASF_BLEND);
//glDrawBuffer(GL_FRONT);
  r.x=0; r.y=0; r.wid=twid; r.hgt=thgt;
  cv->Clear(&r);
  // 11-04-97: No need to lift by Ascent anymore
  //cv->Text(text,0,font->GetAscent());
  ox=offsetX; oy=offsetY;
  // Check offset; text must remain in bitmap
  if(ox>marginX)
  { qwarn("QText X offset (%d) is larger than margin (%d)!",ox,marginX);
    ox=marginX-1;
  }
  if(oy>marginY)
  { qwarn("QText Y offset (%d) is larger than margin (%d)!",oy,marginY);
    oy=marginY-1;
  }
  // Use specified color
  //cv->SetColor(255,255,255);
  cv->SetColor(col);
  cv->Text(text,ox,oy);
//sginap(100);
  //cv->Text(text,100,100);
  // Now get the bitmap before anyone sees it
  //char *p=bm->GetBuffer();
  //for(int i=0;i<twid*thgt*4;i++)*p++=i;
  cv->ReadPixels(bm,0,0,twid,thgt);
  // Copy back what was there before
  //glReadBuffer(GL_FRONT);
  //cv->CopyPixels(0,0,twid,thgt,0,0);
  cv->Blit(backbm,0,0,twid,thgt,0,0);
  // Create mask that masks out all black pixels
  bm->CreateTransparency(&tCol);
  if(flags&QGELF_AA)
    bm->CreateAA(aaType);

  // Be like a bob for this bitmap
  if(bmFiltered){ delete bmFiltered; bmFiltered=0; }
  sbm=bm;
  //sx=0; sy=0; swid=twid; shgt=thgt;
  SetSourceLocation(0,0,twid,thgt);		// Does more than just let

  delete backbm;

  if(oldFont)
    cv->SetFont(oldFont);
  MarkDirty();
//printf("  QText SetText ret\n");
}

