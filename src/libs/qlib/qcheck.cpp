/*
 * QCheck - check buttons
 * 18-06-97: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/check.h>
#include <qlib/canvas.h>
#include <qlib/event.h>
#include <qlib/app.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define QBSW		4		// Shadow size

#define BOXSIZE		16		// Checkbox size
#define BOXSPACE	4		// Space between box & text

QCheck::QCheck(QWindow *parent,QRect *ipos,string itext)
  : QWindow(parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
{
  //printf("QCheck ctor, text=%p this=%p\n",itext,this);
  //printf("button '%s'\n",itext);
  // Shadow lines
  colText=new QColor(0,0,0);
  font=app->GetSystemFont();
  if(itext)
    text=qstrdup(itext);
  else text=0;

  // Size check so it fits text
  if(text)
  {
    Size(font->GetWidth(text)+BOXSPACE+BOXSIZE,font->GetHeight(text));
    //pos.wid=font->GetWidth(text)+BOXSPACE+BOXSIZE;
    //pos.hgt=font->GetHeight(text);
    //if(pos.hgt<BOXSIZE)pos.hgt=BOXSIZE;
  }
  state=DISARMED;
  chkState=0;

  // No shortcut key
  scKey=0; scMod=0;
  eventType=QEVENT_CLICK;

  Create();
  // Take minimal amount of events
  // Include keystrokes because a shortcut key may be assigned to each button
  Catch(CF_BUTTONPRESS|CF_BUTTONRELEASE|CF_KEYPRESS);

  //printf("QCheck ctor RET\n");
}

QCheck::~QCheck()
{
}

void QCheck::SetText(string ntext)
{ if(text)qfree(text);
  text=qstrdup(ntext);
  Invalidate();
}
void QCheck::SetTextColor(QColor *col)
{ colText->SetRGBA(col->GetR(),col->GetG(),col->GetB(),col->GetA());
  Invalidate();
}
void QCheck::SetState(bool yn)
{ chkState=yn;
  Invalidate();
}

void QCheck::Paint(QRect *r)
{ QRect rr;
  int sw=4;		// Shadow width/height

  Restore();
  // Paint insides
  QRect pos;
  GetPos(&pos);
  pos.x=pos.y=0;

  cv->Insides(pos.x,pos.y,BOXSIZE,BOXSIZE);
  //qdbg("inside\n");

  // Paint border
  if(state==ARMED)
    cv->Inline(pos.x,pos.y,BOXSIZE,BOXSIZE);
  else
    cv->Outline(pos.x,pos.y,BOXSIZE,BOXSIZE);

  if(chkState)
  { // Checkmark
    cv->SetColor(0,0,0);
    //cv->Rectfill(pos.x+4,pos.y+4,2,BOXSIZE-8);
    cv->Rectfill(pos.x+4,pos.y+6,2,6);
    cv->Rectfill(pos.x+6,pos.y+9,1,2);
    cv->Rectfill(pos.x+7,pos.y+8,1,2);
    cv->Rectfill(pos.x+8,pos.y+7,1,2);
    cv->Rectfill(pos.x+9,pos.y+6,1,2);
    cv->Rectfill(pos.x+10,pos.y+5,1,2);
    cv->Rectfill(pos.x+11,pos.y+4,1,2);
  }
 skip_shadow2:
  // Draw text if any
  if(text)
  { int tx,ty,twid,thgt;
    cv->SetColor(colText);
    cv->SetFont(font);
    // Align just outside rect
    thgt=font->GetHeight();
    twid=font->GetWidth(text);
    tx=pos.x+BOXSIZE+BOXSPACE;
    ty=(pos.hgt-thgt)/2+pos.y;
    /*printf("twid=%d, hgt=%d, x=%d, y=%d (pos=%d,%d %dx%d)\n",
      twid,thgt,tx,ty,pos.x,pos.y,pos.wid,pos.hgt);*/
    cv->Text(text,tx,ty);
   skip_text:;
  }
  //qdbg("  paint RET\n");
}

// Events

bool QCheck::EvButtonPress(int button,int x,int y)
{ 
  //qdbg("QCheck::EvButtonPress\n");
  state=ARMED;
  Paint();
  app->GetWindowManager()->BeginMouseCapture(this);
  //Focus(TRUE);
  return TRUE;
}

bool QCheck::EvButtonRelease(int button,int x,int y)
{ QEvent e;
  //qdbg("QCheck::EvButtonRelease, this=%p\n",this);
  if(state==DISARMED)return FALSE;
  state=DISARMED;
  chkState^=1;
  Paint();
  app->GetWindowManager()->EndMouseCapture();
  //Focus(FALSE);
  // Generate click event
  e.type=eventType;
  e.win=this;
  app->GetWindowManager()->PushEvent(&e);
  return TRUE;
}

bool QCheck::EvKeyPress(int key,int x,int y)
{
  //printf("QCheck keypress $%x @%d,%d\n",key,x,y);
  if(scKey==key)
  { // Simulate button press
    EvButtonPress(1,x,y);
    QNap(CLK_TCK/50);		// Make sure it shows
    EvButtonRelease(1,x,y);
    return TRUE;		// Eat the event
  }
  return FALSE;
}

// Behavior
void QCheck::ShortCut(int key,int mod)
{
  //printf("shortcut %x %x\n",key,mod);
  scKey=key;
  scMod=mod;
}
void QCheck::SetEventType(int eType)
// If eType==0, normal click events are generated
// Otherwise, special 'eType' events are generated
// eType must be a user event
{
  if(eType==0)
  { eventType=QEVENT_CLICK;
  } else
  { QASSERT_V(eType>=QEVENT_USER);		// Button event type
    eventType=eType;
  }
}

