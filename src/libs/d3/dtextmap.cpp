/*
 * DTextMap - creating texture texturemaps
 * 22-09-99: Created! (detached from zmesh.cpp)
 * NOTES:
 * - This source code is certainly not elegant or clean.
 * - The text generated here is stretched and aligned horizontally
 * and vertically to fit the desired space a bit.
 * FUTURE:
 * - Paint text directly in a pbuffer, or pdmbuffer
 * (C) MG/RVG
 */

// SPECIAL:
// - Style; full outline
// - Line spacing; a little less than the font height

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/textmap.h>
#include <qlib/app.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// Must have even readpixels or it fails
#define BUG_O2_READPIXELS

#undef  DBG_CLASS
#define DBG_CLASS "DTextMap"

/************
* TEXT WRAP *
************/
static QCanvas *tcv;
typedef bool (*SPLITCBFUNC)(string start,string end);

static string strSkipWhiteSpace(string s)
// Skips spaces, tabs, newlines
{ for(;*s;s++)
    if(*s!=' '&&*s!=9&&*s!=10&&*s!=13)break;
  return s;
}
static string strSkipSpaces(string s)
// Skips spaces
{ for(;*s;s++)
    if(*s!=' ')break;
  return s;
}

static int CharWidth(char c)
{ return (int)tcv->GetFont()->GetWidth(c);
}
static void TextSplitter(string txt,int wid,SPLITCBFUNC cb)
// Splits the given text in lines
{
  string s,lw,e;
  int len;

//printf("split(%s)\n",txt);
  s=txt;
  // Find line
 next_line:
  len=0;
  e=s; lw=s;
  if(*e==0)
  { // Last line
    if(s!=e)
    {do_line:
      if(s==lw)lw=e;                // Don't keep hanging on indefinetely
      if(!cb(s,lw))return;
      s=strSkipWhiteSpace(lw);
      if(*s==0)return;              // End of text
      goto next_line;
    }
  } else
  {do_char:
    // End of text?
    if(*e==0)
    { lw=e; goto do_line;
    }
    // Line feed?
    if(*e==10)
    { lw=e; goto do_line;
    }
    len+=CharWidth(*e);
    if(*e==' ')lw=e;            // Mark end of word
    if(len<=wid)
    { e++;
      goto do_char;
    }
    // Line end
    goto do_line;
  }
}
#define MAX_SPLIT       10

static int tx,ty,twid,thgt;
static string tStart[MAX_SPLIT],tEnd[MAX_SPLIT];
static int tLines;

//static bool PaintFunc(string start,string end)
bool DTextMap::PaintFunc(string start,string end)
{
  char c;
  int  lineWid,align;

  c=*end; *end=0;
//printf("'%s'\n",start);fflush(stdout);

  // Align horizontally
  align=alignH;
  lineWid=tcv->GetFont()->GetWidth(start);
  if(align==LEFT)
  { tx=0;
    // Leave space for style shading
    //tx+=2;
  } else if(align==RIGHT)
  { tx=(twid-lineWid);
  } else
  { // Center
    tx=(twid-lineWid)/2;
  }
//qdbg("PaintFunc: lineWid=%d, twid=%d, tx=%d, start=%s\n",
//lineWid,twid,tx,start);

  // Paint text in current style
//qdbg("PaintFunc(%s) @%d,%d\n",start,tx,ty);
  //tcv->SetColor(25,25,25);
  int d=1;
#ifdef ND_SHADE
  tcv->SetColor(75,75,75);
  tcv->Text(start,tx+d,ty+d);
  tcv->Text(start,tx-d,ty+d);
  tcv->Text(start,tx+d,ty-d);
  tcv->Text(start,tx-d,ty-d);
  tcv->Text(start,tx-d,ty);
  tcv->Text(start,tx+d,ty);
  tcv->Text(start,tx,ty-d);
  tcv->Text(start,tx,ty+d);
#endif
  //tcv->SetColor(255,255,255);
  if(textStyle==0)
  { // Use default; white text
    tcv->SetColor(255,255,255);
    tcv->Text(start,tx,ty);
  } else
  { // Use text style
    QTextStyle *oldStyle=tcv->GetTextStyle();
    tcv->SetTextStyle(textStyle);
    tcv->Text(start,tx,ty);
    tcv->SetTextStyle(oldStyle);
  }

  // Advance to next line; use some line height?
  //ty+=tcv->GetFont()->GetHeight();
  *end=c;
  return TRUE;
}
static bool StoreFunc(string start,string end)
{ if(tLines==MAX_SPLIT)
  { qerr("Text split generates too many lines"); return FALSE; }
  tStart[tLines]=start;
  tEnd[tLines]=end;
  tLines++;
  return TRUE;
}
void DTextMap::TextSplit(string s)
// Paint word-wrapped text
// Centers X and Y currently
{ int lineHgt,textHgt,lineSpacingY;
  int i,align;
  QCanvas *cv=app->GetBC()->GetCanvas();
  int x,y,wid,hgt;

  x=y=0;
  wid=curWid; hgt=curHgt;

  // Init globalized vars
 retry_lines:
  tcv=cv;
  tx=x; ty=y; twid=wid; thgt=hgt;
  tLines=0;
  // First split all lines (to center Y)
//qdbg("DTM:TextSplit; try '%s' wid=%d, hgt=%d\n",s,wid,hgt);
  TextSplitter(s,wid,StoreFunc);
  if(tLines>maxLines)
  { // Too many lines, adjust width and try again
qwarn("DTM:TextSplit; too many lines; wid=%d, tLines=%d",wid,tLines);
    wid+=20;
    if(wid<2048)
      goto retry_lines;
    // Otherwise it is hopeless!
  }
  curWid=wid;

//qdbg("DTM:TextSplit; ret\n");
#ifdef ND_SPLIT
  // Align Y
  lineSpacingY=lineSpacing;
  lineHgt=cv->GetFont()->GetHeight()+lineSpacingY;
  textHgt=tLines*lineHgt-lineSpacingY;
  align=alignV;
  if(align==TOP)
  { ty=0;
  } else if(align==BOTTOM)
  { ty=thgt-textHgt;
  } else
  { // Center
    ty=thgt/2-textHgt/2;
  }

  for(i=0;i<tLines;i++)
  {
    PaintFunc(tStart[i],tEnd[i]);
    ty+=lineHgt;
  }
#endif
}
void DTextMap::TextPaint(string s)
// Paint word-wrapped text
// Centers X and Y currently
{
  DBG_C("TextPaint");
  DBG_ARG_S(s);

  int lineHgt,textHgt,lineSpacingY;
  int i,align;
  QCanvas *cv=app->GetBC()->GetCanvas();
  int x,y,wid,hgt;

#ifdef ND_SPLIT
  x=y=0;
  wid=curWid; hgt=curHgt;

  // Init globalized vars
 retry_lines:
  tcv=cv;
  tx=x; ty=y; twid=wid; thgt=hgt;
  tLines=0;
  // First split all lines (to center Y)
  TextSplitter(s,wid,StoreFunc);
  if(tLines>maxLines)
  { // Too many lines, adjust width and try again
    wid+=20;
    if(wid<2048)
      goto retry_lines;
    // Otherwise it is hopeless!
  }
#endif

  // Align Y
  lineSpacingY=lineSpacing;
  lineHgt=cv->GetFont()->GetHeight()+lineSpacingY;
  textHgt=tLines*lineHgt-lineSpacingY;
  align=alignV;
  if(align==TOP)
  { ty=0;
  } else if(align==BOTTOM)
  { ty=thgt-textHgt;
  } else
  { // Center
    ty=thgt/2-textHgt/2;
  }

  for(i=0;i<tLines;i++)
  {
    PaintFunc(tStart[i],tEnd[i]);
    ty+=lineHgt;
  }
}

/***********
* ZTEXTMAP *
***********/
DTextMap::DTextMap(QFont *_font,int _wid,int _hgt)
// Keeper of a bitmap to use for text textures
{
  DBG_C("ctor");

  flags=0;
  alignH=alignV=CENTER;
  bm=0;
  font=_font;
  if(!font)font=app->GetSystemFont();
  wid=_wid; hgt=_hgt;
  curWid=curHgt=0;
  textStyle=0;
  lineSpacing=0;
  // No practical limit to line count
  maxLines=9999;
}
DTextMap::~DTextMap()
{
  DBG_C("dtor");
  QDELETE(bm);
}

/********
* STYLE *
********/
void DTextMap::SetTextStyle(QTextStyle *style)
{
  textStyle=style;
}
void DTextMap::SetWrap(bool yn)
// Turn on/off text wrapping
{
  if(yn)flags|=WRAP;
  else  flags&=~WRAP;
}
void DTextMap::SetMaxLines(int n)
// Set the maximum number of lines the text may be split into
// This is the logical vertical equivalent of a maximum width.
{
  maxLines=n;
}
void DTextMap::SetAlign(int h,int v)
// Set text alignment (LEFT/RIGHT/CENTER, TOP/BOTTOM/CENTER)
{
  alignH=h;
  alignV=v;
}
void DTextMap::SetSize(int w,int h)
{
  wid=w;
  hgt=h;
}
void DTextMap::SetLineSpacing(int n)
// 'n' may be negative (lines placed near eachother)
{
  lineSpacing=n;
}
void DTextMap::SetFont(QFont *nFont)
// Set the font
{
  font=nFont;
}

/**************
* INFORMATION *
**************/
void DTextMap::GetRect(QRect *r)
// Returns size of actually used bitmap
// This may be larger than originally requested, to fit the text
{
  r->x=0; r->y=0;
  r->wid=curWid; r->hgt=curHgt;
}

void DTextMap::GetSize(QSize *size)
{
  size->wid=curWid;
  size->hgt=curHgt;
}

/***************
* SETTING TEXT *
***************/
#define PR glFinish()

void DTextMap::SetText(cstring s)
// Generate bitmap with the given text (and style)
// Note that the bitmap will be sized to fit the text; the given size
// is a MINIMAL requirement. This way big texts can fit on smaller polygons
// (the texture resizes the text automatically). Seems better than creating
// a font with smaller ResX (it is anti-aliased now).
// FUTURE: Generate directly into pbuffer or something (NOT efficient now)
{
  DBG_C("SetText");
  DBG_ARG_S(s);

  QRect r;
  int   texWid,texHgt;
  QCanvas *cvText;
  QBitMap *bmBack;
//proftime_t t;

//PR;profStart(&t);

//qdbg("DTextMap::SetText(%s)\n",s);
//QShowGLErrors("ZMesh::SetText ENTER");

  // Determine size of text space; grow bitmap beyond required size if needed
  //twid=curVar->mesh->GetPoly(curPoly)->GetWidth();
  //thgt=curVar->mesh->GetPoly(curPoly)->GetHeight();
  if(flags&WRAP)
  { // Use preferred width and split lines in that area
    curWid=wid;
  } else
  { // Look at the width of the text and take the maximum line width
    curWid=font->GetWidth(s);
  }
  if(curWid<wid)curWid=wid;
  curHgt=font->GetHeight();
  if(curHgt<hgt)curHgt=hgt;
  
  // Prepare canvas
  cvText=QCV;
  QTRACE_PRINT_L(3,"cvText=%p",cvText);
  cvText->SetFont(font);

  // Split lines and update curWid/curHgt if necessary
qdbg("DTextMap:SetText(%s)\n",s);
  TextSplit((string)s);
//qdbg("  curWid=%d, curHgt=%d\n",curWid,curHgt);

  texWid=QNearestPowerOf2(curWid);
#ifdef BUG_O2_READPIXELS
  if(texWid&1)texWid++;
#endif
  texHgt=QNearestPowerOf2(curHgt);


//PR;profReport(&t,"dtm:init");

  // Restore settings
//#ifdef OBS
  cvText->Set3D();
  cvText->Set2D();
//PR;profReport(&t,"dtm:set2d");
//#endif
  //cvText->GetGLContext()->Disable(GL_TEXTURE_2D);

  QDELETE(bm);
  bm=new QBitMap(32,texWid,texHgt);
  // Backup
  bmBack=new QBitMap(32,texWid,texHgt);
//PR;profReport(&t,"dtm:bm and bmBack");
  cvText->ReadPixels(bmBack,0,0,texWid,texHgt);
//QShowGLErrors("ZMesh::SetText RP");
//PR;profReport(&t,"dtm:readpix");

//qdbg("DTextMap: prefer %dx%d, current %dx%d\n",wid,hgt,curWid,curHgt);

  // Split into lines and paint
  r.x=r.y=0; r.wid=curWid; r.hgt=curHgt;
  glClearColor(0,0,0,0);
  cvText->Clear(&r);
//PR;profReport(&t,"dtm:clear");
  TextPaint((string)s);
//QShowGLErrors("ZMesh::SetText TextPaint");
//PR;profReport(&t,"dtm:textpaint");

  // Read generated bitmap
  QColor tCol(0,0,0);
  cvText->ReadPixels(bm,0,0,curWid,curHgt);
//PR;profReport(&t,"dtm:readpix2");
  //cvText->ReadPixels(bm,0,0,texWid,texHgt);
  bm->CreateTransparency(&tCol);
//PR;profReport(&t,"dtm:createtransp");

  // Restore original
  cvText->Blit(bmBack,0,0,texWid,texHgt,0,0);
//QShowGLErrors("ZMesh::SetText Blit");
//PR;profReport(&t,"dtm:blit org");

#ifdef ND_FOR_CALLER
  // Install texture for this poly
  QDELETE(curVar->texBM);
  curVar->mesh->GetPoly(curPoly)->Enable(DPoly::BLEND);
  curVar->texBM=new DBitMapTexture(bmText);
  curVar->mesh->GetPoly(curPoly)->DefineTexture(curVar->texBM,&r);
  QDELETE(bmText);
#endif

  QDELETE(bmBack);
//QShowGLErrors("ZMesh::SetText RET");
//PR;profReport(&t,"dtm:cleanup");
}
