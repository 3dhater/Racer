/*
 * QEdit - edit control (string gadget)
 * 24-05-97: Created! (from qbutton.cpp)
 * 02-09-97: Begun to support multiline text
 * 06-09-99: Range support, scrolling etc. for REAL editor stuff
 * 20-06-01: Readonly edit controls support.
 * NOTES:
 * - Not really meant for large edits (>100K), but since processors are fast,
 * it's not too bad.
 * - It's big but very useful by now
 * FUTURE:
 * - InsertString() can be much faster (for pastes)
 * BUGS:
 * - TAB control is the responsibility of QWinMgr, not QEdit
 * - TAB offset (offx) in PaintCursor() is NYI
 * (C) MarketGraph/RVG
 */

#include <qlib/edit.h>
#include <qlib/canvas.h>
#include <qlib/event.h>
#include <qlib/app.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <qlib/keys.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

#define QBSW	4		// Shadow size

#define BORDER_SIZE	4	// Size to leave around edges

// USEX is to paint using X GC calls
//#define USEX

QEdit::QEdit(QWindow *parent,QRect *ipos,int maxlen,cstring itext,int iflags)
  : QWindow(parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
{
  int i;
//qdbg("QEdit ctor, text=%p this=%p\n",itext,this);
  //printf("button '%s'\n",itext);

  colText=new QColor(0,0,0);
  font=app->GetSystemFont();
  //SetFont(app->GetSystemFont());
  flags=iflags;
  for(i=0;i<MAX_VIEW_LINES;i++)
  { lineStart[i]=0;
    lineRefresh[i]=0;
  }

  /*printf("QEdit: %d,%d %dx%d; %d\n",
    ipos->x,ipos->y,ipos->wid,ipos->hgt,font->GetHeight());*/
  // Autosize height to match font
  if(!(flags&MULTILINE))
  { Size(ipos->wid,font->GetHeight()+2*BORDER_SIZE);
    linesInView=1;
  }
#ifdef USEX
  PrefDefaultVisual();
#endif
  Create();
#ifdef USEX
  cv->UseX();
#endif

  // Allocate contents
  maxChar=maxlen;
  text=(string)qcalloc(maxlen);
  if(itext)strncpy(text,itext,maxlen-1);

  state=INACTIVE;
  cx=cy=0;
  cxWanted=0;

  //QRect pos;
  //GetXPos(&pos);
  SetFont(font);
  textTop=text;
//qdbg("QEdit ctor: font=%p, linesInView=%p\n",font,linesInView);

  rangeStart=rangeEnd=0;
  rangeDirection=0;

  marker=0;

  buffer=0;
  bufferSize=0;
  eFlags=0;

  SetTabStop();

  // Make sure lineStart[] is ok
  FormatView();

  // Don't supply CF_KEYPRESS because of shortcut problem in qwinmgr.cpp
  // (we would get the same key twice)
  //Catch(CF_BUTTONPRESS|CF_KEYPRESS);
  Catch(0);
  CompressExpose();

  // Add window to application (Z)
  //app->GetWindowManager()->AddWindow(this);
  //printf("QEdit ctor RET\n");
}

QEdit::~QEdit()
{
  if(text)qfree(text);
  if(colText)delete colText;
  if(buffer)qfree(buffer);
}

/**********
* Attribs *
**********/
void QEdit::SetFont(QFont *nfont)
{
  font=nfont;
  // Recalculate some settings
  QRect pos;
  GetPos(&pos);
  linesInView=(pos.hgt-2*BORDER_SIZE)/font->GetHeight();
//qdbg("QEdit:SetFont: pos.hgt=%d, fontHgt=%d\n",pos.hgt,font->GetHeight());
  if(linesInView<1)linesInView=1;
  if(linesInView>MAX_VIEW_LINES)linesInView=MAX_VIEW_LINES;
}
void QEdit::SetText(string ntext)
{ 
  if(!ntext)ntext="";
  strncpy(text,ntext,maxChar-1);
  Paint();
}
int QEdit::GetInt()
{ return Eval(GetText());
}
void QEdit::SetTextColor(QColor *col)
{ colText->SetRGBA(col->GetR(),col->GetG(),col->GetB(),col->GetA());
}
void QEdit::NoEnterPropagation()
// Specify that the edit control eats ENTER events, instead of passing them on
// to buttons for example.
// This is useful to avoid situations, like in Lingo, where a button with
// QK_ENTER shortcut and an edit control are both active at the same time,
// thus pressing ENTER in the edit control
// also activates the button, which is normally NOT what you want.
// Default is to propagate ENTERs (useful for dialogs)
// ENTER propagation is NEVER on for multiline edit controls.
{
  flags|=NO_ENTER_PROPAGATE;
}

bool QEdit::IsMultiLine()
// Returns TRUE if control is multiline
{
  if(flags&MULTILINE)return TRUE;
  return FALSE;
}

/*************
* FORMATTING *
*************/
static void pss(cstring s)
{ char buf[256];
  strncpy(buf,s,80);
  buf[30]=0;
  qdbg("'%s'",buf);
}
static string FindLine(string s,int line)
// Find text of line 'line'
{ int i;
  for(i=0;i<line;i++)
  { for(;*s!=10;s++)
    { if(*s==0)return s;
    }
    s++;
  }
  return s;
}
static cstring ExtractToPreviousLine(cstring text,cstring topText)
// The address of the previous line of data is returned.
// The address won't go beyond 'topText'
{
  int i;
  string d=0;

  //if(maxLen==-1)maxLen=999999;
  //for(i=0;i<maxLen-1;i++)
  if(text>topText)
    text--;
  if(text>topText)
    text--;
  while(1)
  {
    if(text<topText)
    { text++; break;
    }
    if(*text==10)
    { // Skip newline for next paint
      text++; break;
    }
    if(d)*d++=*text;
    text--;
  }
  if(d)*d=0;
  return text;
}
static cstring ExtractViewLine(cstring text,string d=0,int maxLen=-1,int *pLen=0)
// Text is copied to 'd' (if d!=0)
// The address of the next line of data is returned.
// If 'pLen'!=0, then *plen is filled with the extracted line length
{
  int i,len=0;
//qdbg("EVL: text=%p\n",text); //pss(text);
//printf("text[%s]\n",text);

  if(maxLen==-1)maxLen=999999;
  for(i=0;i<maxLen-1;i++)
  {
    if(*text==0)break;
    if(*text==10)
    { // Skip newline for next paint
      text++; break;
    }
    if(d)*d++=*text;
    len++;
    text++;
  }
  if(d)*d=0;
  if(pLen)*pLen=len;
//qdbg("EVL: textRet=%p\n",text);
  return text;
}
static void PaintCursor(QCanvas *cv,QRect *pos,string text,int cx,int cy,
  QFont *font)
// Draw a cursor-thing
{ int offx,offy,i;
  char c,*s;
  if(cx==0)offx=0;
  else
  { // Find X position of cursor
    //qdbg("  find x; text=%p, cx=%d, font=%p\n",text,cx,font);
    s=FindLine(text,cy);
    if(!s)offx=0;
    else
    {
      offx=font->GetWidth(s,cx);
      // Add TABs to offsets...
      //...
    }
  }
  // Find Y position of cursor
  offy=BORDER_SIZE+font->GetHeight()*cy;
  cy=0;
  // Draw cursor
  //qdbg("  draw\n");
#ifdef USEX
  cv->SetColor(QApp::PX_RED);
#else
  cv->SetColor(255,80,80);
#endif
  QRect r(pos->x+offx+4,pos->y+offy,2,font->GetHeight());
  cv->Rectfill(&r);
}

void QEdit::FormatView()
{
  int i;
  string s;
  s=textTop;
//qdbg("QEdit:FormatView()\n");
  for(i=0;i<linesInView;i++)
  {
    lineStart[i]=s;
//qdbg("  line%d='%p'\n",i,s);
    s=(string)ExtractViewLine(s,0,-1,&lineLen[i]);
  }
}
void QEdit::RefreshView()
// Refresh all lines
{ int i;
  for(i=0;i<linesInView;i++)
    lineRefresh[i]=1;
}
void QEdit::RefreshLine(int y)
// Refresh per line
{
  if(y<0||y>linesInView)return;
  lineRefresh[y]=1;
}
void QEdit::RefreshSmart()
// Refresh only the lines that have been touched
{
  int i;
  bool drawCursor=FALSE;

  FormatView();
#ifdef USEX
  cv->SetColor(QApp::PX_BLACK);
#else
  cv->SetColor(colText);
#endif
  for(i=0;i<linesInView;i++)
    if(lineRefresh[i])
    {
      PaintLine(i);
      lineRefresh[i]=0;
      drawCursor=TRUE;
    }
  //if(drawCursor==TRUE&&state==ACTIVE&&RangeActive()==FALSE)
  if(drawCursor==TRUE&&state==ACTIVE&&IsEnabled()==TRUE)
  { QRect pos;
    GetXPos(&pos); pos.x=pos.y=0;
    PaintCursor(cv,&pos,textTop,cx,cy,font);
  }
}

static int LineLength(cstring s)
{
  int i;
  for(i=0;;i++,s++)
  { if(*s==10||*s==0)return i;
  }
} 
static int CalcLines(cstring s,int max=9999999)
// Returns #lines from string 's', including current line
// If 'max' is reached, it returns 'max' (for small counts
// within the view)
{ int i;
  for(i=1;*s;s++)
    if(*s==10)
    { if(++i>=max)break;
    }
  return i;
}
static cstring ConvertTabs(cstring s)
// Returns string in which all TABs in 's' have been
// outlined (using tabsize=8) into spaces (for cv->Text() to paint)
// Used in QEdit::PainLine()
// Notes: returned string is never longer than 256 characters.
{ static char buf[256];
  int i,len,n;
  char *d;

  for(d=buf,len=0;len<256-8;s++)	// Safety margin
  {
    if(*s==0)
    { *d=0; break;
    } else if(*s==9)
    { // TAB
      n=8-(len&7);
      len+=n;
      for(i=0;i<n;i++)
      { *d++=' ';
      }
    } else
    { *d++=*s;
      len++;
    }
  }
  return buf;
}

cstring QEdit::PaintLine(int cy /*,cstring text*/)
// Paint 1 line of text
// 'y' is the view Y coordinate
// 'text' is the text start
// Text is drawn until EOF (0) or newline (10) or wrap end (NYI)
// Assumptions: canvas font is set
{ char buf[256];
  char *d;
  int   i,llen;
  QRect pos,r,rRange;
  int   left,right;
  cstring txt=lineStart[cy],nextTxt;

  nextTxt=ExtractViewLine(txt,buf,256,&llen);
//qdbg("QEdit:PaintLine(%d,%s)\n",cy,buf);
  
  GetXPos(&pos);
  r.x=BORDER_SIZE; r.y=BORDER_SIZE+cy*font->GetHeight();
  r.wid=pos.wid-2*BORDER_SIZE;
  r.hgt=font->GetHeight();
  if(rangeStart==0)
  { // No range
#ifdef USEX
    // Paint normal background
    cv->SetColor(QApp::PX_LTGRAY);
    cv->Rectfill(&r);
#else
    cv->SetColor(QApp::PX_MDGRAY);
    //cv->SetColor(166,166,166);
    cv->Rectfill(&r);
#endif
  } else
  { // Check ranges
//qdbg("check text [%p,%p] in range [%p,%p] (ccp=%p)\n",txt-text,txt+llen+1-text,rangeStart-text,rangeEnd-text,CurrentCharPtr()-text);
#ifdef USEX
    // Paint normal background
    cv->SetColor(QApp::PX_LTGRAY);
    cv->Rectfill(&r);
#else
    cv->SetColor(QApp::PX_MDGRAY);
    //cv->SetColor(166,166,166);
    cv->Rectfill(&r);
#endif
    if(rangeStart>txt+llen)goto no_range;
    if(rangeEnd<txt)goto no_range;
    // Left range indicator
    left=rangeStart-txt;
    if(left<0)left=0;
    //rRange.x=r.x+left*font->GetWidth(' ');
//qdbg("  find left: left=%d, gw=%d\n",left,font->GetWidth(lineStart[cy],left));
    rRange.x=r.x+font->GetWidth(lineStart[cy],left);
    rRange.y=r.y;
    rRange.hgt=r.hgt;
    right=rangeEnd-txt;
    if(right>llen)
    { rRange.wid=r.wid-rRange.x;	// Fill upto right border
    } else
    { //rRange.wid=(right-left)*font->GetWidth(' ');
      // Proportional font support
//qdbg("  cx=%d, right=%d\n",cx,right);
      rRange.wid=font->GetWidth(lineStart[cy]+left,right-left);
      //rRange.wid=font->GetWidth(lineStart[cy],right)-rRange.x-r.x;
    }
    if(rRange.wid>0)
    {
#ifdef USEX
      cv->SetColor(QApp::PX_WHITE);
      cv->Rectfill(&rRange);
#else
      cv->SetColor(255,255,255);
      cv->Rectfill(&rRange);
#endif
    }
   no_range:;
//qdbg("Range: left=%d, right=%d, wid=%d\n",left,right,rRange.wid);
  }
  //cv->Insides(x,y,pos.wid-2*BORDER_SIZE,font->GetHeight());
#ifdef USEX
  cv->SetColor(QApp::PX_BLACK);
#else
  cv->SetColor(colText);
#endif
  cv->Text(ConvertTabs(buf),r.x,r.y);
  return nextTxt;
}

void QEdit::Paint(QRect *r)
{ QRect rr;
  int sw=4;		// Shadow width/height
  int y;

  if(!IsVisible())return;

  // Paint insides (intelligent; only the border chunks)
  QRect pos;
  GetXPos(&pos);
  pos.x=pos.y=0;
  cv->Insides(pos.x+2,pos.y+2,pos.wid-4,2,FALSE);
  cv->Insides(pos.x+2,pos.y+2,2,pos.hgt-4,FALSE);
  cv->Insides(pos.x+pos.wid-4,pos.y+2,2,pos.hgt-4,FALSE);
  cv->Insides(pos.x+2,pos.y+pos.hgt-font->GetHeight()-1,
    pos.wid-4,font->GetHeight(),FALSE);

  // Paint border
  cv->Inline(pos.x,pos.y,pos.wid,pos.hgt);

  // Clear all lines at once
#ifdef USEX
  cv->SetColor(QApp::PX_BLACK);
#else
  cv->SetColor(colText);
#endif
  // Draw text if any
  FormatView();
  cv->SetFont(font);
  for(y=0;y<linesInView;y++)
  {
    PaintLine(y);
  }

  // Draw visible lines of text
  //if(state==ACTIVE&&RangeActive()==FALSE)
  if(state==ACTIVE&&IsEnabled()==TRUE)
  {
    PaintCursor(cv,&pos,textTop,cx,cy,font);
  }
//qdbg("  paint RET\n");
}

/*********
* RANGES *
*********/
void QEdit::RangeClear()
// Cancels the range selection (and marks lines to be refreshed)
{
//qdbg("QEdit:RangeClear()\n");
  if(rangeStart==0)return;
  rangeStart=rangeEnd=0;
  rangeDirection=0;
  // Refresh lines that contain part of the range
  RefreshView();
}
bool QEdit::RangeActive()
{ if(rangeStart==0)return FALSE;
  if(rangeStart==rangeEnd)return FALSE;
  return TRUE;
}
void QEdit::RangeSetStart(char *p)
{
//qdbg("RangeSetStart(%p)\n",p);
  rangeStart=p;
  // Range direction not yet known (set in RangeSetEnd)
  rangeDirection=0;
}
void QEdit::RangeSetEnd(char *p)
// Set end of range. Keeps range direction updated to deal
// with ranges moving from one side to the other (of the cursor)
// BUGS: with PageDown/Up it doesn't set the right range
{
//qdbg("RangeSetEnd(%p); dir=%d\n",p,rangeDirection);
  if(p<rangeStart)
  { // Marking to the left of the start
    if(!rangeEnd)
      rangeEnd=rangeStart;
    if(rangeDirection==1)
    { // Changing from right to left range
      rangeEnd=rangeStart;
    }
    rangeStart=p;
    rangeDirection=-1;
  } else if(p<=rangeEnd&&rangeDirection==-1)
  { // Making backward selection smaller
    rangeStart=p;
  } else
  { // End to the right
    if(rangeDirection==-1)
    { // Changing from left to right range
      rangeStart=rangeEnd;
    }
    rangeEnd=p;
    rangeDirection=1;
  }
}
bool QEdit::IsInRange(cstring p)
{
  if(rangeStart==0)return FALSE;
  if(p>=rangeStart&&p<=rangeEnd)return TRUE;
  return FALSE;
}
bool QEdit::RangeDelete()
// Delete the current range
{
  int len;

  if(!RangeActive())return FALSE;
  // Cut range
  len=rangeEnd-rangeStart;
  if(len<=0)return FALSE;
  Goto(rangeStart);
  DeleteChars(len);
  RangeClear();
  return TRUE;
}
void QEdit::SelectAll()
// Select all text (highlight it)
// Doesn't paint.
{
  RangeSetStart(text);
  RangeSetEnd(text+strlen(text));
  // Set cursor at end of selection
  Goto(rangeEnd);
  Invalidate();
}
bool QEdit::Find(cstring s,int flags)
// Find a piece of text and go there
// See enum FindFlags (in qlib/edit.h) for 'flags'
{
  char fbuf[MAX_FIND_LEN];
  int  i,flen;

  // Protect against bad find strings
  if(strlen(s)>=MAX_FIND_LEN)
  { qwarn("QEdit:Find(); find string too big");
    return FALSE;
  }

  // Empty string is never found
  if(!*s)return FALSE;

  // Generate actual search string
  if(flags&CASE_DEPENDENT)
  { strcpy(fbuf,s);
  } else
  { cstring sc;
    for(sc=s,i=0;*sc;sc++,i++)
      fbuf[i]=toupper(*sc);
    fbuf[i]=0;
  }
  flen=strlen(fbuf);
//qdbg("find '%s'\n",fbuf);

  // Search from here on (just one char further actually)
  s=CurrentCharPtr();
  if(flags&FIND_BACKWARDS)
  { if(s>text)s--;
  } else
  { if(*s)s++;
  }
  if(flags&CASE_DEPENDENT)
  { qwarn("QEdit: Find case dependent NYI");
  } else
  { // Case independent
    while(1)
    {
      if(flags&FIND_BACKWARDS)
      { if(s<text)break;
      } else
      { if(!*s)break;
      }
      if(toupper(*s)==fbuf[0])
      { // Test from here
        for(i=1;i<flen;i++)
        { if(toupper(s[i])!=fbuf[i])
            goto not_here;
        }
        // Found it!
//qdbg("found!\n");
        Goto((char*)s);
        //RefreshSmart();
        FormatView(); RefreshView(); RefreshSmart();
        return TRUE;
       not_here:;
      }
      if(flags&FIND_BACKWARDS)s--;
      else                    s++;
    }
  }
  return FALSE;
}

/*********
* BUFFER *
*********/
void QEdit::Cut()
{
  if(!RangeActive())return;
  Copy();
  RangeDelete();
}
void QEdit::Copy()
{
  int len;
  len=rangeEnd-rangeStart;
  if(len<=0)return;
  if(bufferSize<len+1)
  { if(buffer)qfree(buffer);
    bufferSize=len+1;
    buffer=(char*)qcalloc(bufferSize);
  }
  strncpy(buffer,rangeStart,len);
  buffer[len]=0;
}
void QEdit::Paste()
{
  if(RangeActive())RangeDelete();
  if(buffer)
  {
//qdbg("Paste '%s'\n",buffer);
    InsertString(buffer);
  }
}


//
// Events
//
bool QEdit::EvSetFocus()
{
//qdbg("QEdit::EvSetFocus\n");
  state=ACTIVE;
  //Paint();
  if(!(flags&MULTILINE))
  { // Select all text
    RangeSetStart(text);
    RangeSetEnd(text+strlen(text));
    RefreshSmart();
  }
  return QWindow::EvSetFocus();
}
bool QEdit::EvLoseFocus()
// Focus is lost, give signal that we have touched the control
// to the app (the text may have remained unchanged)
{
//qdbg("QEdit::EvLoseFocus\n");
  state=INACTIVE;
  RangeClear();
  //RefreshLine(cy);
  //RefreshSmart();

  //Invalidate();
#ifdef OBS
  // Generate message this window lost focus
  QEvent e;
  e.type=QEvent::LOSEFOCUS;
  e.win=this;
  PushEvent(&e);
#endif

  return QWindow::EvLoseFocus();
}

char *QEdit::MouseXYtoCursorXY(int mx,int my,int *cx,int *cy)
// Translates mouse coordinates to cursor coordinates
// Returns the text ptr in the edit control's text
{
  int wid;
  string s;

  QRect pos;
  GetXPos(&pos);
  pos.x=pos.y=0;

  // Relative coords
  mx-=pos.x; my-=pos.y;
  // Find cx/cy; first cy
  my-=2;
  my/=font->GetHeight();
  // Do not exceed #lines in view
  if(my>=linesInView)
    my=linesInView-1;
  // Do not exceed file length
  if(my>=CalcLines(text))
    my=CalcLines(text)-1;
  *cy=my;
  // Track cx down
  wid=0; mx-=2;
  s=FindLine(textTop,*cy);
  for(*cx=0;*s!=0&&*s!=10;(*cx)++)
  { wid+=font->GetWidth(*s);
    if(wid>mx)break;
    s++;
  }
  return s;
}

bool QEdit::EvMotionNotify(int x,int y)
// Mouse moves
{
  int cx,cy;
  string s;

  if(eFlags&DRAGGING)
  {
//qdbg("drag!\n");
    s=MouseXYtoCursorXY(x,y,&cx,&cy);
    if(s)
    { //qdbg("drag to %p\n",s);
      if(!RangeActive())
        RangeSetStart(CurrentCharPtr());
      RangeSetEnd(s);
      Goto(s);
      Invalidate();
    }
    return TRUE;
  }
  return FALSE;
}

bool QEdit::EvButtonPress(int button,int x,int y)
{ //int wid; 
  //string s;
  //qdbg("QEdit::EvButtonPress\n");

  // Select with left mouse button
  if(button!=1)return FALSE;

  if(!QWM->SetKeyboardFocus(this))return FALSE;

  RefreshLine(cy);
  MouseXYtoCursorXY(x,y,&cx,&cy);

  RangeClear();
  eFlags|=DRAGGING;
  RefreshLine(cy);
  RefreshSmart();
  QWM->BeginMouseCapture(this);
  return TRUE;
}
bool QEdit::EvButtonRelease(int button,int x,int y)
{
  eFlags&=~DRAGGING;
  QWM->EndMouseCapture();
  return TRUE;
}

/**************
* INFORMATION *
**************/
int QEdit::GetCX()
{ return cx;
}
int QEdit::GetCY()
// Returns row in which the cursor is located (first row = 0)
// Not too fast, so don't call TOO often.
{
  char *s,*c;
  int   n=0;

  c=CurrentCharPtr();
  for(s=text;s<c;s++)
  { if(*s==10)n++;
  }
  return n;
}

/***********
* MOVEMENT *
***********/
void QEdit::CheckCX()
// Checks and corrects if necessary CX to not exceed the line length
{
  string s;
  s=lineStart[cy];
  if(cx>LineLength(s))
  {
    // Line is too short; cap
    cx=LineLength(s);
  } else
  {
    // Line is big enough
    // Remember cursor location
    //cxWanted=cx;
  }
}
void QEdit::CheckCY()
// Checks and corrects if necessary CY to not exceed the file length
{
  string s;
 retry:
  s=lineStart[cy];
  if(s==0||*s==0)
  { if(cy>0)
    { cy--;
      goto retry;
    }
  }
}

void QEdit::Goto(cstring p)
// Move cursor to a pointer location
// Scrolls view to a different location if necessary
{
  int i;

  // Assert
  if(p<text){ qerr("QEdit:Goto() internal error; p<text"); return; }

  if(!(flags&MULTILINE))
  { // Always in only line
    cy=0; cx=p-text;
    cxWanted=cx;
    return;
  }

 retry:
  // Check if its onscreen
  if(p<textTop)
  { // Move back
    ScrollUp();
    goto retry;
  } else
  { // Find line in which to go
    for(i=1;i<linesInView;i++)
    { if(lineStart[i]>p)
      { // Cursor in previous line
        cy=i-1;
        cx=p-lineStart[cy];
        return;
      }
    }
    // Scroll down to find line
    if(CalcLines(textTop,linesInView+1)<linesInView+1)
    { // Don't scroll further than this
      CheckCY(); CheckCX();
      return;
    }
    if(!ScrollDown())
    { // Can't scroll any further
      /*qerr("QEdit:Goto() can't find p=%p in text=%p-%p\n",
        p,text,text+strlen(text));*/
      return;
    }
    goto retry;
  }
}
bool QEdit::CursorDown()
{
  int linesFromTop;
  linesFromTop=CalcLines(textTop);
//RefreshView();
  if(cy<linesFromTop-1)
  {
    // Check display
    if(cy>=linesInView-1)
    { if(flags&SCROLLING)
      { // See if more lines are available
        if(linesFromTop>linesInView)
        { // Set top text to one line further up
          ScrollDown();
          goto ok;
        }
      }
      return FALSE;
    }
    RefreshLine(cy);
    cy++;
   ok:
    RefreshLine(cy);
    // Attempt to go to wanted X coordinate
    if(cx<cxWanted)
    {
      // Try and go to the right as far as possible
      cx=cxWanted;
    } else
    {
      // Remember this as the desired X location
      cxWanted=cx;
    }
    CheckCX();
  } else return FALSE;
  return TRUE;
}
bool QEdit::CursorUp()
{ string s;
  if(cy>0)
  { RefreshLine(cy);
    cy--;
    RefreshLine(cy);
   do_cx:
    // Attempt to go to wanted X coordinate
    if(cx<cxWanted)
    {
      // Try and go to the right as far as possible
      cx=cxWanted;
    } else
    {
      // Remember this as the desired X location
      cxWanted=cx;
    }
    CheckCX();
  } else if(textTop>text)
  { // Some lines are above this one
    ScrollUp();
    goto do_cx;
  } else return FALSE;
  return TRUE;
}
bool QEdit::CursorRight()
{
  //qdbg("QEdit:CursorRt\n");
  if(lineStart[cy]-text+cx<strlen(text))
  { 
    RefreshLine(cy);
    if(CurrentChar()==10)
    { // Line feed; proceed to next line
      if(cy>=linesInView-1)
      { ScrollDown();
      } else
      { cy++;
        RefreshLine(cy);
      }
      cx=0;
    } else cx++;
    // Remember this is the desired X coordinate
    cxWanted=cx;
  } else return FALSE;
  //qdbg("QEdit:CursorRt RET\n");
  return TRUE;
}
bool QEdit::CursorLeft()
{ string s1,s2;
//qdbg("CursorLeft\n");
  if(cx>0)
  { cx--;
    RefreshLine(cy);
  } else if(cy>0||textTop>text)		// Any lines above current line?
  { RefreshLine(cy);
    //if(text[cabs]==10)
    { if(cy==0)
      { // Need to scroll up
        ScrollUp();
      } else cy--;
      RefreshLine(cy);
      // Find length of this line
      s1=FindLine(textTop,cy);
//qdbg("s1=");pss(s1);
      s2=FindLine(textTop,cy+1);
//qdbg("s2=");pss(s1);
      cx=(s2-s1)-1;
//qdbg("cx=%d\n",cx);
      //cx=3;
      //printf("line1: '%s', 2: '%s', diff %d\n",s1,s2,cx);
    }// else cx--;
  } else return FALSE;
  // Remember this is the desired X coordinate
  cxWanted=cx;
  return TRUE;
}
void QEdit::CursorHome()
{
  // Always set desired X to left
  cxWanted=0;
  if(cx==0)return;
  cx=0;
  RefreshLine(cy);
}
void QEdit::CursorEnd()
{ string s;
  int len;
  s=lineStart[cy];
  if(!s)return;
  len=LineLength(s);
  if(cx<len)
  { // Move to end
    //cabs+=len-cx;
    cx+=len-cx;
    RefreshLine(cy);
  }
  // Set wanted X to far right
  cxWanted=99;
}
bool QEdit::ScrollDown()
// Move top to a next line
// Returns FALSE if it can't scroll any further
{
  char *tt;
  tt=(string)ExtractViewLine(textTop);
  if(tt==textTop)
  { // No change; cannot scroll further
    return FALSE;
  }
  textTop=tt;
  RefreshView();
  FormatView();
  return TRUE;
}
bool QEdit::ScrollUp()
// Move top to a previous line
{
  char *tt;
  tt=(string)ExtractToPreviousLine(textTop,text);
  if(tt==textTop)
  { // No change; cannot scroll further
    return FALSE;
  }
  textTop=tt;
  RefreshView();
  FormatView();
  return TRUE;
}

/**********
* EDITING *
**********/
char QEdit::CurrentChar()
// Returns char at cursor position
{
  return *(lineStart[cy]+cx);
}
char *QEdit::CurrentCharPtr()
// Returns ptr to char at cursor position
{
  return lineStart[cy]+cx;
}

bool QEdit::InsertChar(char c)
// Inserts a char into the current cursor position
// Returns FALSE in case of any error
{
  int i;
  char *pc,*s;

  if(flags&READONLY)return FALSE;
  
//qdbg("QEdit:InsertChar(%d)\n",c);
//qdbg(" text=%p, strlen=%d, maxChar=%d\n",text,strlen(text),maxChar);
//qdbg(" lineStart[0]=%p\n",lineStart[0]);

  if(strlen(text)<maxChar-1)
  { // Insert char
    //qdbg("chk len\n");
    if(c==10)
    { // Check for overflow of display
      if(CalcLines(textTop)>=linesInView)
      { if(!(flags&SCROLLING))
          return FALSE;
      }
    }
    //len=strlen(text);
    pc=CurrentCharPtr();
//qdbg("inschr: len=%d, text='%s', cx=%d, c=%d\n",len,text,cx,c);
//qdbg("move\n");
    for(s=text+strlen(text)+1;s>pc;s--)
      *s=*(s-1);
    //for(i=len+1;i>cabs;i--)
      //text[i]=text[i-1];
    *pc=c;
    //text[cabs]=c;
//qdbg("add\n");
  }
  Changed();
  return TRUE;
}
bool QEdit::InsertString(cstring s,bool reformat)
// Insert string at the current cursor position
// Moves the cursor along
// If 'reformat' is TRUE, the display is reformatted after insertion, and
// the cursor is moved to the new position. This is avoided by some macros
// which do nifty text insertions.
// Q: Does it work with LF's?
// FUTURE: Insert yourself instead of calling InsertChar(), EXCEPT
// when !flags&SCROLLING (mustn't overflow), or !MULTILINE. Check maxBuffer!
{
  int i,len;
  len=strlen(s);
  for(i=len-1;i>=0;i--)
  { if(!InsertChar(s[i]))
      return FALSE;
  }
  if(reformat)
  { FormatView();
    for(i=0;i<len;i++)
    { //if(s[i]!=10)
        CursorRight();
    }
  }
  return TRUE;
}

bool QEdit::DeleteChar()
// Delete char at current position
// Returns FALSE if char was not deleted.
{
  return DeleteChars(1);
}
bool QEdit::DeleteChars(int n)
// Delete n chars at current cursor position
// Returns FALSE if chars were not deleted.
{
  char *pc,*s;
  int   len;

  if(flags&READONLY)return FALSE;
  
  pc=CurrentCharPtr();
  if(!*pc)return FALSE;
  len=strlen(text);
  for(s=pc;s<text+len-n;s++)
    *s=*(s+n);
  *s=0;
  Changed();
  return TRUE;
}

bool QEdit::IsChanged()
{
  if(eFlags&CHANGED)return TRUE;
  return FALSE;
}
void QEdit::Changed()
{
  eFlags|=CHANGED;
}

//static int IndentPoint(cstring s)
int QEdit::IndentPoint(cstring s)
// Return number of spaces to indent, given previous line 's'
{
  int i;
  for(i=0;;i++,s++)
  { if(*s==0)break;
    if(*s==' ')continue;
    if(*s=='{'||*s=='#')
    { // Indent beyond the open brace/pound
      return i+2;
    }
    break;
  }
  return i;
}
static bool IsWordChar(char c)
// Is used to skip words
{
  if(c>='a'&&c<='z')return TRUE;
  if(c>='@'&&c<='Z')return TRUE;
  if(c=='_')return TRUE;
  if(c>='0'&&c<='9')return TRUE;
  return FALSE;
}
static bool IsFunctionLine(cstring s)
// Attempts to determine if this string is a function
// prototypes, like 'void func(int a)'
{
  if(s[0]==' ')return FALSE;
  // Include, define, ifdef, endif etc
  if(s[0]=='#')return FALSE;
  // Function start/end
  if(s[0]=='{')return FALSE;
  if(s[0]=='}')return FALSE;
  // Comments
  if(s[0]=='/')return FALSE;
  if(s[0]=='*')return FALSE;
  // Empty lines
  if(s[0]==10)return FALSE;
  // Debug statements
  if(!strncmp(s,"qdbg",4))return FALSE;
  if(!strncmp(s,"DEBUG_",6))return FALSE;
  return TRUE;
}

bool QEdit::EvKeyPress(int key,int x,int y)
// Implements a lot of keystrokes
// Derived class (like QCEdit) may beat us to it and assign more special
// functions to keys.
{ bool refresh=FALSE;
  bool rangePost=FALSE;		// Do range postprocessing?
  char c;
  int i,len;
  int mods,ukey;		// Shift modifiers, unmodified key
  int autoIndentLen;

  if(state==INACTIVE)goto do_default;

//qdbg("QEdit(this=%p) keypress $%x @%d,%d\n",this,key,x,y);
//return FALSE;

  // Check keys; order is important!
  mods=QK_Modifiers(key);
  ukey=QK_Key(key);

  if(ukey==_QK_LSHIFT||ukey==_QK_RSHIFT||
     ukey==_QK_LCTRL||ukey==_QK_RCTRL||
     ukey==_QK_LALT||ukey==_QK_RALT)
  { // Skip modifier keys; they conflict with things like auto-outdent
    goto do_default;
  }

  if(!IsMultiLine())
  { // Don't eat certain keys for single-line edit controls
    if(ukey==QK_PAGEUP)return FALSE;
    if(ukey==QK_PAGEDOWN)return FALSE;
  }

//qdbg("  1\n");
  // Range operations
  if(ukey==QK_LEFT||ukey==QK_RIGHT||ukey==QK_UP||ukey==QK_DOWN||
     ukey==QK_HOME||ukey==QK_END||ukey==QK_PAGEUP||ukey==QK_PAGEDOWN)
  { if(mods&QK_SHIFT)
    { // Start range if not already started
      if(!RangeActive())
      { RangeSetStart(CurrentCharPtr());
      }
      rangePost=TRUE;
    } else
    { if(RangeActive())
      { // While in a range, left/right goes to the bounds of the range
        // Other keys cancel the range and act on the key in the usual way
        if(ukey==QK_LEFT||ukey==QK_RIGHT)
        {
          // Goto left side of range
          //Goto(ukey==QK_LEFT?GetRangeStart():GetRangeEnd());
          RangeClear();
          refresh=TRUE;
          // Continue processing this key
          //goto skip_keys;
        }
        // Cancel range, and process key
        RangeClear();
      }
    }
  }
  // Ukeys and normal keys
  if(key==QK_ENTER||key==QK_KP_ENTER)
  {
    if(flags&MULTILINE)
    { // Insert a LF; don't escape control
      c=10;
      // If we add a } right away, outdent it
      eFlags|=IF_BRACE_OUTDENT;
      goto ins_char;
    }
   leaveit:
    //QWM->SetKeyboardFocus(0);
    //state=INACTIVE;
#ifdef OBS
    Focus(FALSE);
    state=INACTIVE;
    Paint();
#endif
    // Notify app this window was used
    QEvent e;
    e.type=QEVENT_CLICK;
    e.win=this;
    e.n=key;
    PushEvent(&e);
    if(flags&NO_ENTER_PROPAGATE)
    { // Eat ENTER so no other windows see it
      return TRUE;
    } else
    { // Leave event for any OK button (dialogs mainly)
      goto do_default;
    }
  } else if(key==(QK_CTRL|QK_END))
  { // To end
    while(ScrollDown());
    for(i=0;i<linesInView-1;i++)ScrollUp();
    cy=linesInView-1; cx=0;
    CheckCY();
    refresh=TRUE;
  } else if(key==(QK_CTRL|QK_HOME))
  { // To end
    while(ScrollUp());
    RefreshLine(cy);
    cy=cx=0;
    RefreshLine(cy);
    refresh=TRUE;
  } else if(key==QK_TAB)
  { //goto leaveit;
    //c=9; goto ins_char;
    // Move to next control
    goto do_default;
  } else if(key==(QK_SHIFT|QK_TAB))
  { goto do_default;
  } else if(ukey==QK_UP)
  { if(mods&QK_CTRL)
    { while(CursorUp())
      {
        if(IsFunctionLine(lineStart[cy]))
        {
          cx=cxWanted=0;
          break;
        }
      }
    } else
    { CursorUp();
    }
    refresh=TRUE;
  } else if(ukey==QK_DOWN)
  { if(mods&QK_CTRL)
    { while(CursorDown())
      {
        if(IsFunctionLine(lineStart[cy]))
        {
          cx=cxWanted=0;
          break;
        }
      }
    } else
    { CursorDown();
    }
    refresh=TRUE;
  } else if(ukey==QK_HOME)
  { CursorHome();
    refresh=TRUE;
  } else if(ukey==QK_END)
  { CursorEnd();
    refresh=TRUE;
  } else if(ukey==QK_LEFT)
  { if(mods&QK_CTRL)
    { // Move to back of previous word
      while(CursorLeft())
      {
        c=CurrentChar();
        if(IsWordChar(c))
        { break;
        }
      }
      // Find start of that word
      while(CursorLeft())
      {
        c=CurrentChar();
        if(!IsWordChar(c))
        { CursorRight();
          break;
        }
      }
    } else
    { CursorLeft();
    }
    refresh=TRUE;
  } else if(ukey==QK_RIGHT)
  { if(mods&QK_CTRL)
    { // Move to end of current word
      do
      {
        c=CurrentChar();
        if(!IsWordChar(c))
        { break;
        }
      } while(CursorRight());
      // Find start of next word
      while(CursorRight())
      {
        c=CurrentChar();
        if(IsWordChar(c))
          break;
      }
    } else
    { CursorRight();
    }
    refresh=TRUE;
  } else if(ukey==QK_PAGEDOWN)
  { if(cy==linesInView-1)
    { for(i=0;i<linesInView-2;i++)
        CursorDown();
    } else
    { RefreshLine(cy);
      cy=linesInView-1;
      CheckCY();
      CheckCX();
      RefreshLine(cy);
    }
    refresh=TRUE;
  } else if(ukey==QK_PAGEUP)
  { if(cy==0)
    { for(i=0;i<linesInView-2;i++)
        CursorUp();
    } else
    { RefreshLine(cy);
      cy=0;
      CheckCX();
      RefreshLine(cy);
    }
    refresh=TRUE;
  } else if(key==QK_BS)
  {
    if(RangeActive())
    { RangeDelete();
    } else
    { if(CursorLeft())
      { DeleteChar();
      }
    }
    RefreshView(); refresh=TRUE;
    //RefreshSmart();
    //return TRUE;
  } else if(key==(QK_CTRL|QK_X))
  { Cut(); refresh=TRUE;
  } else if(key==(QK_CTRL|QK_C))
  { Copy(); refresh=TRUE;
  } else if(key==(QK_CTRL|QK_V))
  { Paste(); RefreshView(); refresh=TRUE;
  }
  if(key==QK_DEL)
  { if(RangeActive())
      RangeDelete();
    else
    { DeleteChar();
    }
    RefreshView();
    refresh=TRUE;
  }

  // Normal text characters
  c=QKeyToASCII(key);
  if(c>=32 /*&&c<=127*/)
  {
    // Auto-outdenting
    if(eFlags&IF_BRACE_OUTDENT)
    { if(c=='}')
      { if(cx>=2)
        { CursorLeft(); CursorLeft(); DeleteChar(); DeleteChar();
        }
      } else if(c=='#')
      { // Outdent to start of line 
        while(cx>0){ CursorLeft(); DeleteChar(); }
      }
    }
   ins_char:
    //refresh=TRUE;
    //qdbg("inschar\n");
    if(RangeActive())RangeDelete();

    autoIndentLen=0;
    if(c==10)
    { if(eFlags&AUTOINDENT)
      { // See how far to indent
        autoIndentLen=IndentPoint(lineStart[cy]);
#ifdef ND_REFINE_THIS
        // Don't indent further than cursor X position (shifts are annoying)
        if(autoIndentLen>cx)autoIndentLen=cx;
#endif
      }
    }
    if(InsertChar(c))
      CursorRight();
    if(autoIndentLen)
    { // Indent begins; reformat view (lines have changed)
      FormatView();
//qdbg("ch inserted\n");RefreshSmart();QNap(100);
      // Insert enough spaces
      for(i=0;i<autoIndentLen;i++)
      { if(InsertChar(' '))
          CursorRight();
//qdbg("indent space inserted\n");RefreshSmart();QNap(100);
      }
    }

    if(c==10)RefreshView();		// Refresh all if new line
    refresh=TRUE;
  }

 skip_keys:
  // Handle automatic brace outdenting
//qdbg("IF_BRACE_OUTDENT=%d, c=%d\n",eFlags&IF_BRACE_OUTDENT,c);
  if(c!=10)
  { eFlags&=~IF_BRACE_OUTDENT;
  }

  // Post-range operations
  if(rangePost)
  { RangeSetEnd(CurrentCharPtr());
    // Refresh entire view for big selectors
    if(ukey==QK_PAGEUP||ukey==QK_PAGEDOWN)
      RefreshView();
    //if(ukey==QK_DOWN)RefreshView();
  }

  // Show changes in view
  if(refresh)
  { //Paint();
    RefreshSmart();
    //if(key==QK_LEFT)qdbg("QEdit: keypress LEFT; return TRUE\n");
    // Keep key from moving up the hierarchy
    return TRUE;
  }
 do_default:
  return QWindow::EvKeyPress(key,x,y);
}

