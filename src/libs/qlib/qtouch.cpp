/*
 * QTouch - touchscreen basic support
 * 23-05-00: Created! (for Kunstkoppen/VARA)
 * FUTURE:
 * - Threads take up quite some time, perhaps a select() can be done.
 * BUGS:
 * - GetX()/Y/LMB etc don't seem too thread-safe
 * (C) MG/RVG
 */

#include <qlib/touch.h>
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
  QTouch *m;
  m=(QTouch*)instance;
//qdbg("MouseStart: m=%p\n",m);
  m->Thread();
}

QTouch::QTouch(int unit,int _type,int _flags)
{
  flags=_flags;
  type=_type;
  serUnit=unit;
  ser=new QSerial(serUnit,9600);
  
  // Position
  x=y=0;
  b=0;
  eventSkip=DEFAULT_SKIP;
  skip=0;
  
  // Default min/max
  window.x=0; window.y=0;
  window.wid=720; window.hgt=576;
  // Default calibration
  calibration.x=0; calibration.y=0;
  calibration.wid=1000; calibration.hgt=1000;
  
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

QTouch::~QTouch()
{
  delete ser;
}

void QTouch::ClipXY()
// Keep x/y inside window
{
  if(x<window.x)x=window.x;
  if(x>=window.x+window.wid)x=window.x+window.wid-1;
  if(y<window.y)y=window.y;
  if(y>=window.y+window.hgt)y=window.y+window.hgt-1;
}

void QTouch::GenerateEvent(int type,int x,int y,int but)
// Create an event, motion or button press/release
{ QEvent e;
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

void QTouch::HandleCmd(cstring s,int nCharsWaiting)
// A packet has arrived
// For serial connections, the lead-in bytte and checksum byte
// have already been stripped.
{
  int dx,dy;
  signed char c;
  
#ifdef OBS
qdbg("handle %x/%x/%x/%x/%x/%x/%x/%x\n",
  s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7]);
#endif

  if(s[0]=='T')
  { // Touch
    if(flags&INITIAL_TOUCHES)
    {
      // Check for 1st touch (much less hits)
      if(!(s[1]&1))
      { // Not an initial touch; ignore this
        return;
      }
    }
    if(s[1]&2)
    { // Stream touch; don't count
      //return;
    }
    x=(s[3]<<8)|s[2];
    y=(s[5]<<8)|s[4];
    z=(s[7]<<8)|s[6];
//qdbg("raw x=%d, y=%d, z=%d\n",x,y,z);
    
    // Scale the coordinates into the calibration window
    x-=calibration.x;
    y-=calibration.y;
    // Scale the coordinates to the size of the calibration area
    x=x*window.wid/calibration.wid;
    y=y*window.hgt/calibration.hgt;
    // Mirroring
    if(flags&FLIP_X)
    { x=window.wid-1-x;
    }
    if(flags&FLIP_Y)
    { y=window.hgt-1-y;
    }
    // Offset into dest window
    x+=window.x;
    y+=window.y;
    // Make sure the clipping area isn't crossed
    ClipXY();
//qdbg("scaled x=%d, y=%d, z=%d\n",x,y,z);
    
    // Event
    if(!(flags&NO_EVENTS))
    { GenerateEvent(QEvent::TS_MOTIONNOTIFY,x,y,z);
    }
  }
  
#ifdef OBS
  // Event generation?
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
#endif
}

void QTouch::Handle()
// Input data available, read and process
{
  int n;
  char c;
  unsigned char sum=0;
  static char cmd[50],ci;
  
  for(n=ser->CharsWaiting();n>0;n--)
  {
    ser->Read(&c,1);
//qdbg("read char %x\n",c);
    // Serial
    // Sync with start of packet
    if(ci==0)
    {
      if(c=='U')
      { // Start of packet
        sum=0xAA+'U';
        goto store_c;
      }
    } else
    { // In middle of packet
     store_c:
      cmd[ci]=c;
      ci++;
      // Don't overflow packet buffer
      if(ci>=sizeof(cmd))
      { ci--;
        // Bad packet probably; restart
        ci=0;
      }
      // Packet = 10 serial chars
      if(ci==10)
      { HandleCmd(&cmd[1],n);
        ci=0;
      }
    }
  }
}

/**************
* INFORMATION *
**************/
int QTouch::GetX()
{
  return x;
}
int QTouch::GetY()
{
  return y;
}
int QTouch::GetButtons()
{
  return b;
}
bool QTouch::GetLMB()
{
  if(b&LMB)return TRUE;
  return FALSE;
}
bool QTouch::GetMMB()
{
  if(b&MMB)return TRUE;
  return FALSE;
}
bool QTouch::GetRMB()
{
  if(b&RMB)return TRUE;
  return FALSE;
}

void QTouch::SetMinMax(QRect *r)
{
  window.SetXY(r->x,r->y);
  window.SetSize(r->wid,r->hgt);
  // Clip coordinates now
  ClipXY();
}
void QTouch::SetCalibration(QRect *r)
// Define minimal and maximal values that you get on the screen edges
// This means the edges of the UNDERLYING video screen, not the edges
// of the touchscreen.
{
  calibration.SetXY(r->x,r->y);
  calibration.SetSize(r->wid,r->hgt);
}

void QTouch::SetEventSkip(int n)
// Set the #events to skip when lots of events are pending
// This compresses mouse events a bit so you don't get too many
// Still, don't just look at the last X/Y for smooth motions, because
// the thread way of handling the mouse generates events in (small) bursts,
// thereby creating jumps in the mouse X/Y.
{
  eventSkip=n;
}
void QTouch::EnableEvents()
// Turn on event generation
{
  flags&=~NO_EVENTS;
}
void QTouch::DisableEvents()
// Turn off event generation
{
  flags|=NO_EVENTS;
}


/*********
* THREAD *
*********/
void QTouch::Thread()
// The thread which handles the mouse in the background
{
  int n;
  fd_set fdr;

  //QSetThreadPriority(PRI_MEDIUM);
qdbg("QTouch:Thread\n");
  while(1)
  {
    // Wait for activity
    //QNap(1);
    //if(skip)qdbg("skip!=0; ncw=%d\n",ser->CharsWaiting());
    skip=0;
    FD_ZERO(&fdr);
    FD_SET(ser->GetFD(),&fdr);
    if(select(ser->GetFD()+1,&fdr,0,0, 0)<0)
      perror("QTouch: select");
   retry:
    n=ser->CharsWaiting();
//qdbg("touch: %d ser chars\n",n);
    if(n>0)
    { // Handle while things are happening
      Handle();
      //goto retry;
    }
  }
}
