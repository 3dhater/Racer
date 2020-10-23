/*
 * QSermouse - serial mouse Microsoft mode implementation
 * 08-02-00: Created! (a bit for Bommen)
 * FUTURE:
 * - Threads take up quite some time, perhaps a select() can be done.
 * BUGS:
 * - GetX()/Y/LMB etc don't seem too thread-safe
 * (C) MG/RVG
 */

#include <qlib/sermouse.h>
#include <qlib/app.h>
#include <qlib/event.h>
#pragma hdrstop
#include <stropts.h>
#include <fcntl.h>
#ifdef OBS
#include <unistd.h>
#include <bstring.h>
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <sys/ioctl.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DEFAULT_SKIP       0

static void MouseStart(void *instance)
{
  QSerMouse *m;
  m=(QSerMouse*)instance;
//qdbg("MouseStart: m=%p\n",m);
  m->Thread();
}

QSerMouse::QSerMouse(int unit,int _flags)
{
  flags=_flags;
  serUnit=unit;
  ser=new QSerial(serUnit,1200);
  
  // Position
  x=y=0;
  b=0;
  eventSkip=DEFAULT_SKIP;
  skip=0;
  
  // Default min/max
  window.x=0; window.y=0;
  window.wid=720; window.hgt=576;
  
  if(flags&MANUAL_HANDLE)
  {
    // Don't start thread
    thread=0;
  } else
  {
    // Generate thread which keeps track of mouse
//qdbg("Sermouse thread!\n");
    thread=new QThread(MouseStart,this);
    thread->Create();
  }
}

QSerMouse::~QSerMouse()
{
  delete ser;
}

void QSerMouse::ClipXY()
// Keep x/y inside window
{
  if(x<window.x)x=window.x;
  if(x>=window.x+window.wid)x=window.x+window.wid-1;
  if(y<window.y)y=window.y;
  if(y>=window.y+window.hgt)y=window.y+window.hgt-1;
}

void QSerMouse::GenerateEvent(int type,int x,int y,int but)
// Create an event, motion or button press/release
{ QEvent e;

  // Does the user want events?
  if(flags&NO_EVENTS)return;

  e.type=type;
  e.x=x; e.y=y;
  e.xRoot=x; e.yRoot=y;
  // To find out which serial mouse generated the event, pass our
  // structure
  e.p=(void*)this;
  // Butt press or release (or 0 in case of motion)
  e.n=but;
  e.win=Q_BC;
  QEventPush(&e);
}

void QSerMouse::HandleCmd(cstring s,int nCharsWaiting)
// Distill what the mouse is telling us
// Brief notes (empirical results):
// - 3 bytes per command (Microsoft mode?)
// - Bit 6 (0x40) notifies start of command
// - s[0] => cclryyxx
//        c=command indicator, l=left mouse button, r=right
//        yy=upper 2 bits of delta y, xx=upper 2 bits of delta x
{
  int dx,dy;
  signed char c;
  
//qdbg("handle %x/%x/%x\n",s[0],s[1],s[2]);
  c=s[0];
  if((c&0x40)!=0x40)
  {
    // Weird middle button stuff
    return;
  }
  
  
  // X
  c=s[1];
  c|=(s[0]&0x3)<<6;
  dx=c;
  
  // Y
  c=s[2];
  c|=(s[0]&0xC)<<4;
  dy=c;
  
//qdbg("d=%d,%d\n",dx,dy);
  // Movements
  x+=dx;
  y+=dy;
  ClipXY();
  if(dx!=0||dy!=0)
  {
    if(nCharsWaiting<3)
    { // Generate event; no mouse data waiting
      skip=0;
     do_event:
      if(!(flags&NO_EVENTS))
      { 
//qdbg("mouse gen's event, skip=%d\n",skip);
        if(nCharsWaiting>=9)
        { // Don't generate event; lots to come, use end of mouse serial
          // events
          //qdbg("skipping ncw>3 ncw=%d\n",nCharsWaiting);
        } else
        { GenerateEvent(QEvent::SM_MOTIONNOTIFY,x,y,0);
          //qdbg("gen event @%d,%d\n",x,y);
        }
      }
    } else
    {
      // Don't generate events for all
      if(skip<=0)
      {
        skip=eventSkip;
        goto do_event;
      } else
      {
        //qdbg("skipping event skip=%d, ncw=%d\n",skip,nCharsWaiting);
        skip--;
      }
    }
  }

  // Buttons
  c=s[0];
  if(c&0x20)
  { if(!(b&LMB))
      GenerateEvent(QEvent::SM_BUTTONPRESS,x,y,1);
    b|=LMB;
  } else
  { if(b&LMB)
      GenerateEvent(QEvent::SM_BUTTONRELEASE,x,y,1);
    b&=~LMB;
  }
  if(c&0x10)
  { if(!(b&RMB))
      GenerateEvent(QEvent::SM_BUTTONPRESS,x,y,3);
    b|=RMB;
  } else
  { if(b&RMB)
      GenerateEvent(QEvent::SM_BUTTONRELEASE,x,y,3);
    b&=~RMB;
  }
}

void QSerMouse::Handle()
{
  int n;
  char c;
  static char cmd[10],ci;
  
  for(n=ser->CharsWaiting();n>0;n--)
  {
    ser->Read(&c,1);
    // Synchronize with mouse (first time in, or when plugging in mice)
    if((c&0x40)==0x40)
    { //HandleCmd(cmd,n);
      ci=0;
     store_c:
      cmd[ci]=c;
      ci++;
      if(ci>=sizeof(cmd))ci--;
      // Microsoft mouse mode uses 3 bytes per event
      if(ci==3)
      { HandleCmd(cmd,n);
        ci=0;
      }
    } else
    { 
      c&=0x3F;
      goto store_c;
    }
  }
}

/**************
* INFORMATION *
**************/
int QSerMouse::GetX()
{
  return x;
}
int QSerMouse::GetY()
{
  return y;
}
void QSerMouse::SetXY(int _x,int _y)
// Set coordinates directly
{
  x=_x; y=_y;
  ClipXY();
}
int QSerMouse::GetButtons()
{
  return b;
}
bool QSerMouse::GetLMB()
{
  if(b&LMB)return TRUE;
  return FALSE;
}
bool QSerMouse::GetMMB()
{
  if(b&MMB)return TRUE;
  return FALSE;
}
bool QSerMouse::GetRMB()
{
  if(b&RMB)return TRUE;
  return FALSE;
}

void QSerMouse::SetMinMax(QRect *r)
{
  window.SetXY(r->x,r->y);
  window.SetSize(r->wid,r->hgt);
  // Clip coordinates now
  ClipXY();
}

void QSerMouse::SetEventSkip(int n)
// Set the #events to skip when lots of events are pending
// This compresses mouse events a bit so you don't get too many
// Still, don't just look at the last X/Y for smooth motions, because
// the thread way of handling the mouse generates events in (small) bursts,
// thereby creating jumps in the mouse X/Y.
{
  eventSkip=n;
}
void QSerMouse::EnableEvents()
// Turn on event generation
{
  flags&=~NO_EVENTS;
}
void QSerMouse::DisableEvents()
// Turn off event generation
{
  flags|=NO_EVENTS;
}

/*********
* THREAD *
*********/
void QSerMouse::Thread()
// The thread which handles the mouse in the background
{
  int n;
  fd_set fdr;

  //QSetThreadPriority(PRI_MEDIUM);
qdbg("QSerMouse:Thread\n");
  while(1)
  {
    // Wait for activity
    //QNap(1);
    //if(skip)qdbg("skip!=0; ncw=%d\n",ser->CharsWaiting());
    skip=0;
    FD_ZERO(&fdr);
    FD_SET(ser->GetFD(),&fdr);
    if(select(ser->GetFD()+1,&fdr,0,0, 0)<0)
      perror("QSerMouse: select");
   retry:
    n=ser->CharsWaiting();
//qdbg("sermouse: %d ser chars\n",n);
    if(n>0)
    { // Handle while things are happening
      Handle();
      //goto retry;
    }
  }
}
