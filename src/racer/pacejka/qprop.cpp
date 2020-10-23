/*
 * QProp - proportional gadget (generated or imagery)
 * 07-04-97: Created!
 * 18-07-97: Support for user-defined jumping. (finetuning)
 * 28-02-98: dsp[] is now a percentage shown. Horizontal props support
 * BUGS:
 * - After calling SetRange(), you have to recheck cur[] and dsp[], which
 *   may be out of range.
 * (C) MarketGraph/RVG
 */

#include <qlib/prop.h>
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

// Win95 style?
#define STYLE_W95

#define QBSW	4		// Shadow size

QProp::QProp(QWindow *parent,QRect *ipos,int _type,int _propFlags)
  : QWindow(parent,ipos->x,ipos->y,ipos->wid,ipos->hgt)
// Construct a proportional control
// '_type' can be QProp::VERTICAL or QProp::HORIZONTAL
// Use QProp::SHOW_VALUE for '_propFlags' if you want the current
// position (value) to be displayed in the control (often handy)
{
  //printf("QProp ctor\n");
  state=IDLE;
  propFlags=_propFlags;

  type=_type;
  // Default ranges
  min[HORIZONTAL]=0; max[HORIZONTAL]=100;
  min[VERTICAL]=0; max[VERTICAL]=100;
  cur[0]=cur[1]=0;
  dsp[0]=dsp[1]=50;
  jmp[0]=jmp[1]=dsp[0]/2;
  // Take minimal amount of events
  Catch(CF_BUTTONPRESS|CF_BUTTONRELEASE|CF_MOTIONNOTIFY);
  CompressExpose();
  CompressMotion();
  Create();

  //printf("QProp ctor RET\n");
}

QProp::~QProp()
{
}

void QProp::EnableShowValue()
// Enables displaying the position on the control
{
  propFlags|=SHOW_VALUE;
}
void QProp::DisableShowValue()
// Disables displaying the position on the control
{
  propFlags&=~SHOW_VALUE;
}

int QProp::GetDir(int dir)
// Deduce direction to act upon based on dir:
// dir=-1; deduce what the user wants
//      0: HOR, 1: VERT (as the const's)
{ if(dir==-1)
  { // Deduce from type
    switch(type)
    { case HORIZONTAL:
      case VERTICAL  :
        dir=type; break;
      default:
        dir=HORIZONTAL;		// We must choose one
        break;
    }
  }
  QASSERT_0(dir==HORIZONTAL||dir==VERTICAL);
  return dir;
}

int QProp::GetPosition(int dir)
// Returns the current position (within the control's range)
{ dir=GetDir(dir);
  return cur[dir];
}

void QProp::SetRange(int nmin,int nmax,int dir)
// Set logical range
// For example: SetRange(1,10) if you have 10 items starting at 1.
{ 
  dir=GetDir(dir);
  min[dir]=nmin;
  if(nmax<nmin)nmax=nmin;
  max[dir]=nmax;
  // Rethink dsp[] and cur[] (jmp[])
  //...
  Invalidate();
}
void QProp::SetDisplayed(int n,int outOf,int dir)
// Set the number of items displayed. For example, you have a text onscreen,
// of which 14 lines are visible out of 20 in the whole text.
// Call SetDisplayed(14,20) to set the size of the scrollbar.
{ QRect pos;
  dir=GetDir(dir);
  //if(n<min[dir])n=min[dir];
  //if(n>max[dir])n=max[dir];
  GetPos(&pos);
  //n=n*pos.wid/outOf;
  // Percentage
  if(outOf)
    n=n*100/outOf;
  else
    n=100;
  // Validate
  //if(n>pos.wid-8)n=pos.wid-8;
  if(n>100)n=100; else if(n<10)n=10;		// Handle must be touchable
  dsp[dir]=n;
  // Rethink jump amount
  jmp[dir]=n/2;
}
void QProp::SetPosition(int pos,int dir)
// Set the position (inside the given range)
// The control will be repainted.
{ dir=GetDir(dir);
  if(pos<min[dir])pos=min[dir];
  if(pos>max[dir])pos=max[dir];
  cur[dir]=pos;
  Invalidate();
}
void QProp::SetJump(int n,int dir)
// Set the jump distance if the user clicks outside the drag box.
// This should have a sort of 'page jump' feel.
// It may be 1 though if you have only a few items.
{ dir=GetDir(dir);
  if(n<0)n=-n;
  if(n>max[dir])n=max[dir];
  jmp[dir]=n;
}

void QProp::CalcWindowBox(QRect *r)
// Calculate, from cur[], dsp[] etc. the results window coordinates
// of the box INSIDE the prop gadget (the DRAG box)
{
  int dw,dh,dx,dy;
  int bw;
  // Default fills all
  QRect pos;
  GetPos(&pos);
  pos.x=0; pos.y=0;		// Local coordinate system
#ifdef STYLE_W95
  // Calc inside dimensions
  dh=pos.hgt;
  dw=pos.wid;
  dx=0; dy=0;
  bw=0;				// No border
#else
  // Calc inside dimensions
  dh=pos.hgt-2*4;
  dw=pos.wid-2*4;
  dx=4; dy=4;			// 20-2-98; own XWindow
  bw=4;				// Fat border
#endif

  if(type==VERTICAL||type==FREE)
  { 
#ifdef OBS
    dh=dsp[VERTICAL]*dh/(max[VERTICAL]-min[VERTICAL]);
#else
    dh=dsp[VERTICAL]*dh/100;		// Percentage
#endif
    // Move Y; mind the space taken by the 'displayed' part (dh pixels)
    if(max[VERTICAL]==min[VERTICAL])dy=0;
    else dy=cur[VERTICAL]*(pos.hgt-2*bw-dh)/(max[VERTICAL]-min[VERTICAL]);
    dy+=pos.y+bw;
  }
  if(type==HORIZONTAL||type==FREE)
  { 
#ifdef OBS
    dw=dsp[HORIZONTAL]*dw/(max[HORIZONTAL]-min[HORIZONTAL]);
#else
    dw=dsp[HORIZONTAL]*dw/100;
#endif
    // Move X; mind the space taken by the 'displayed' part (dw pixels)
    if(max[HORIZONTAL]==min[HORIZONTAL])dx=0;
    else dx=cur[HORIZONTAL]*(pos.wid-2*bw-dw)/(max[HORIZONTAL]-min[HORIZONTAL]);
    dx+=pos.x+bw;
  }
//qdbg("QProp: dxywh=%d,%d %dx%d\n",dx,dy,dw,dh);
  r->x=dx; r->y=dy;
  r->wid=dw; r->hgt=dh;
}

/********
* PAINT *
********/
void QProp::Paint(QRect *r)
{ QRect rr;
  char buf[40];

  //Restore();
  QRect pos;
  GetPos(&pos);

#ifdef OBS_OLD_STYLE
  // Paint insides
  //cv->Insides(pos.x+2,pos.y+2, pos.wid-2*2,pos.hgt-2*2);
  cv->Insides(2,2, pos.wid-2*2,pos.hgt-2*2);

  // Paint border
  //cv->Outline(pos.x,pos.y,pos.wid,pos.hgt);
  cv->Outline(0,0,pos.wid,pos.hgt);
#endif

  // Paint box itself
  cv->SetColor(QApp::PX_LTGRAY);
  cv->Rectfill(0,0,pos.wid,pos.hgt);

  // Inside drag box
  CalcWindowBox(&rr);
  cv->Insides(rr.x+2,rr.y+2,rr.wid-4,rr.hgt-4);
  cv->Outline(rr.x,rr.y,rr.wid,rr.hgt);

  // Show value?
  if(propFlags&SHOW_VALUE)
  { if(type==VERTICAL||type==HORIZONTAL)
    {
      sprintf(buf,"%d",GetPosition());
      SetText(buf);
    }
  }

  // Paint text
  if(!text.IsEmpty())
  {
    cv->SetFont(app->GetSystemFont());
    cv->SetColor(0,0,0);
    cv->Text(text,rr.x+rr.wid/2-cv->GetFont()->GetWidth(text)/2,
      rr.y+(rr.hgt-cv->GetFont()->GetHeight())/2);
  }
//qdbg("  paint RET\n");
}

/*********
* EVENTS *
*********/
bool QProp::EvButtonPress(int button,int x,int y)
{ QRect r;
  //qdbg("QProp::EvButtonPress\n");
  if(button!=1)return FALSE;
  QRect pos;
  GetPos(&pos);
  pos.x=pos.y=0;
  CalcWindowBox(&r);
  if(r.Contains(x,y))
  { state=DRAGGING;
    dragX=x; dragY=y;
    QWM->BeginMouseCapture(this);
    //qdbg("  drag!\n");
    //Focus(TRUE);
  } else
  { // Jump
    //qdbg("  jump!\n");
    if(type==VERTICAL||type==FREE)
    { 
      //if(y<r.y)cur[VERTICAL]-=dsp[VERTICAL]/2;
      //else     cur[VERTICAL]+=dsp[VERTICAL]/2;
      if(y<r.y)cur[VERTICAL]-=jmp[VERTICAL];
      else     cur[VERTICAL]+=jmp[VERTICAL];
      if(cur[VERTICAL]<min[VERTICAL])cur[VERTICAL]=min[VERTICAL];
      if(cur[VERTICAL]>max[VERTICAL])cur[VERTICAL]=max[VERTICAL];
    }
    if(type==HORIZONTAL||type==FREE)
    { //if(x<r.x)cur[HORIZONTAL]-=dsp[HORIZONTAL]/2;
      //else     cur[HORIZONTAL]+=dsp[HORIZONTAL]/2;
      if(x<r.x)cur[HORIZONTAL]-=jmp[HORIZONTAL];
      else     cur[HORIZONTAL]+=jmp[HORIZONTAL];
      if(cur[HORIZONTAL]<min[HORIZONTAL])cur[HORIZONTAL]=min[HORIZONTAL];
      if(cur[HORIZONTAL]>max[HORIZONTAL])cur[HORIZONTAL]=max[HORIZONTAL];
    }
    // Generate event
    state=DRAGGING;			// Fool ButtonRel (dragging)
    EvButtonRelease(button,x,y);
    Paint();
  }
  return TRUE;
}

bool QProp::EvButtonRelease(int button,int x,int y)
{ QEvent e;
  //qdbg("QProp::EvButtonRelease, this=%p\n",this);
  if(button!=1)return FALSE;
  if(state==IDLE)return FALSE;
  if(state==DRAGGING)
  { QWM->EndMouseCapture();
  }

  state=IDLE;
  Paint();
  //Focus(FALSE);
  // Generate click event to signify end of dragging
  e.type=QEvent::CLICK;
  // For single-direction props, pass the new position
  if(type==VERTICAL||type==HORIZONTAL)e.n=cur[type];
  else                                e.n=0;
  e.win=this;
  //app->GetWindowManager()->PushEvent(&e);
  PushEvent(&e);
  return TRUE;
}

bool QProp::EvMotionNotify(int x,int y)
{ int dx,dy;
  int w,h;
  int newp[2],			// Pixel values
      newY,newX;		// Logical (range) values
  QRect r,r2;

  if(state!=DRAGGING)return FALSE;

  QRect pos;
  GetPos(&pos);
  pos.x=pos.y=0;

  //qdbg("QProp motion @%d,%d\n",x,y);
  dx=x-dragX; dy=y-dragY;
  CalcWindowBox(&r);
  // Calculate new pixel location
  newp[VERTICAL]=r.y+dy;
  newp[HORIZONTAL]=r.x+dx;
  // Calculate back to a range value (min..max) (logical)
  //h=pos.hgt-2*4;		// Space of drag box
  h=pos.hgt-2*0;		// Space of drag box
  h-=r.hgt;			// Deduct space used by dsp[]
  //w=pos.wid-2*4;
  w=pos.wid-2*0;
  w-=r.wid;
  newY=newp[VERTICAL]-(pos.y+0);	// Distance to top
  newX=newp[HORIZONTAL]-(pos.x+0);	// Distance to left
  // Calc logical range from pixel offset
  newY=newY*(max[VERTICAL]-min[VERTICAL])/(h?h:1);
  newX=newX*(max[HORIZONTAL]-min[HORIZONTAL])/(w?w:1);
  cur[VERTICAL]=newY;
  cur[HORIZONTAL]=newX;
//qdbg("QProp: curV=%d\n",cur[VERTICAL]);
//qdbg("min/max=%d,%d\n",min[VERTICAL],max[VERTICAL]);
  // Impose limits
  if(cur[VERTICAL]<min[VERTICAL])cur[VERTICAL]=min[VERTICAL];
  if(cur[VERTICAL]>max[VERTICAL])cur[VERTICAL]=max[VERTICAL];
  if(cur[HORIZONTAL]<min[HORIZONTAL])cur[HORIZONTAL]=min[HORIZONTAL];
  if(cur[HORIZONTAL]>max[HORIZONTAL])cur[HORIZONTAL]=max[HORIZONTAL];
  CalcWindowBox(&r2);
  if(r.x!=r2.x||r.y!=r2.y)
  { Paint();

    // Generate click event; notice the drag
    QEvent e;
    e.type=QEvent::CHANGE;
    // For single-direction props, pass the new position
    if(type==VERTICAL||type==HORIZONTAL)e.n=cur[type];
    else                                e.n=0;
    e.win=this;
    //app->GetWindowManager()->PushEvent(&e);
    PushEvent(&e);
  }

  // Better way of dragging is to take the pixel result
  // This way, we never lose the 'grabbing' point
  //dragX=x; dragY=y;
  dragX=dragX+r2.x-r.x; dragY=dragY+r2.y-r.y;

  return FALSE;
}

