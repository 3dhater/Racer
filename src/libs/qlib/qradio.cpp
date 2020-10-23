/*
 * QRadio - radio buttons
 * 18-06-97: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/radio.h>
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

QRadio::QRadio(QWindow *parent,QRect *ipos,string itext,int igroup)
  : QWindow(parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
{
  //printf("QRadio ctor, text=%p this=%p\n",itext,this);
  //printf("button '%s'\n",itext);
  // Shadow lines
  colText=new QColor(0,0,0);
  font=app->GetSystemFont();
  if(itext)
    text=qstrdup(itext);
  else text=0;
  group=igroup;

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

  //printf("QRadio ctor RET\n");
}

QRadio::~QRadio()
{
}

void QRadio::SetText(string ntext)
{ if(text)qfree(text);
  text=qstrdup(ntext);
}
void QRadio::SetTextColor(QColor *col)
{ colText->SetRGBA(col->GetR(),col->GetG(),col->GetB(),col->GetA());
}
void QRadio::SetState(bool yn)
{ chkState=yn;
}

void QRadio::Paint(QRect *r)
{ QRect rr;
  int sw=4;		// Shadow width/height

  Restore();
  // Paint insides
  //cv->Insides(pos.x,pos.y,BOXSIZE,BOXSIZE);
  //qdbg("inside\n");

#ifdef OBS
  // Paint border
  if(state==ARMED)
    cv->Inline(pos.x,pos.y,BOXSIZE,BOXSIZE);
  else
    cv->Outline(pos.x,pos.y,BOXSIZE,BOXSIZE);
#endif

  QRect pos;
  GetPos(&pos);
  pos.x=0; pos.y=0;
  pos.y+=2;			// Offset for SGI
  cv->SetColor(80,80,80);
  cv->Rectfill(pos.x+4,pos.y+0,4,1);
  cv->Rectfill(pos.x+2,pos.y+1,1,3);
  cv->Rectfill(pos.x+1,pos.y+2,3,1);
  cv->Rectfill(pos.x+0,pos.y+4,1,4);
  cv->Rectfill(pos.x+1,pos.y+9,1,1);
  cv->Rectfill(pos.x+2,pos.y+10,2,1);
  cv->Rectfill(pos.x+4,pos.y+11,5,1);
  cv->Rectfill(pos.x+9,pos.y+1,1,1);
  cv->Rectfill(pos.x+10,pos.y+2,1,2);
  cv->Rectfill(pos.x+11,pos.y+4,1,5);
  cv->Rectfill(pos.x+9,pos.y+9,1,1);
  cv->Rectfill(pos.x+10,pos.y+9,1,1);

  cv->SetColor(0,0,0);
  cv->Rectfill(pos.x+3,pos.y+1,6,1);
  cv->Rectfill(pos.x+2,pos.y+2,1,1);
  cv->Rectfill(pos.x+9,pos.y+2,1,1);
  cv->Rectfill(pos.x+1,pos.y+3,1,6);
  cv->Rectfill(pos.x+2,pos.y+9,1,1);

  cv->SetColor(255,255,255);
  cv->Rectfill(pos.x+4,pos.y+12,5,1);
  cv->Rectfill(pos.x+9,pos.y+11,2,1);
  cv->Rectfill(pos.x+11,pos.y+9,1,2);
  cv->Rectfill(pos.x+12,pos.y+4,1,5);

  cv->SetColor(255,255,255);
  cv->Rectfill(pos.x+4,pos.y+2,5,9);
  cv->Rectfill(pos.x+3,pos.y+3,7,7);
  cv->Rectfill(pos.x+2,pos.y+4,9,5);

  if(chkState!=0)
  { // Hilite dot
    cv->SetColor(80,80,80);
    cv->Rectfill(pos.x+5,pos.y+3,3,7);
    cv->Rectfill(pos.x+3,pos.y+5,7,3);
    cv->SetColor(0,0,0);
    cv->Rectfill(pos.x+4,pos.y+4,5,5);
  }
  pos.y-=2;

 //skip_shadow2:
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
   //skip_text:;
  }
  //qdbg("  paint RET\n");
}

// Events

bool QRadio::EvButtonPress(int button,int x,int y)
{ 
  //qdbg("QRadio::EvButtonPress\n");
  state=ARMED;
  Paint();
  //Focus(TRUE);
  return TRUE;
}

bool QRadio::EvButtonRelease(int button,int x,int y)
{ QEvent e;
  QWindow *w;
  int i;

  //qdbg("QRadio::EvButtonRelease, this=%p\n",this);
  if(state==DISARMED)return FALSE;
  state=DISARMED;
  // Deselect other radio buttons belonging to this group
  for(i=0;;i++)
  { w=app->GetWindowManager()->GetWindowN(i);
    if(!w)break;
    if(!strcmp(w->ClassName(),"radio"))		// RTTI?
    { QRadio *r=(QRadio*)w;
      if(r->GetGroup()==group)
      { r->SetState(0);
        r->Paint();
      }
    }
  }
  // Select this one
  chkState=1;
  Paint();
  //Focus(FALSE);
  // Generate click event
  e.type=eventType;
  e.win=this;
  app->GetWindowManager()->PushEvent(&e);
  return TRUE;
}

bool QRadio::EvKeyPress(int key,int x,int y)
{
  //printf("QRadio keypress $%x @%d,%d\n",key,x,y);
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
void QRadio::ShortCut(int key,int mod)
{
  //printf("shortcut %x %x\n",key,mod);
  scKey=key;
  scMod=mod;
}
void QRadio::SetEventType(int eType)
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

