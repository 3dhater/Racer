/*
 * QVRDisplays - Vos & Ramselaar Lights interface
 * NOTES:
 * - V&R interface for lamps (and latches)
 * - Still works when serial port can't be opened; just silently
 * 14-03-2000: Created! (17:32:00)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <qlib/vrdisplays.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define STX 2
#define ETX 3

// Speed of interface
#define BPS    9600

QVRDisplays::QVRDisplays(int iCount,int unit)
// Construct displays and open serial connection for 'iCount' displays
{
  int i;
  
  count=iCount;
  if(count>MAX_DISPLAY)
  { qwarn("QVRDisplays: count(%d) > MAX(%d), clipped to max displays",
      count,MAX_DISPLAY);
    count=MAX_DISPLAY;
  }
  // Clear displays
  for(i=0;i<count;i++)
    state[i]=' ';
  ser=new QSerial(unit,BPS,QSerial::WRITEONLY);
  if(!ser->IsOpen())
  { qerr("QVRDisplays: can't open serial unit %d",unit);
    delete ser; ser=0;
  }
}

QVRDisplays::~QVRDisplays()
{
  if(ser){ delete ser; ser=0; }
}

int QVRDisplays::GetState(int display)
// Get state of a display
{
  if(display>count)return OFF;
  return state[display];
}

void QVRDisplays::SetState(int display,int newState)
// Set state of a display
// Note that 'newState' is an ASCII char
// Use space (0x20, 32) to clear a display.
{
  if(display>count)return;
  state[display]=newState;
}
void QVRDisplays::SetStates(int newState)
// Set state of all displays
{
  int i;
  for(i=0;i<count;i++)
    state[i]=newState;
}

void QVRDisplays::Update()
// Send out lamp state to interface
{ char buf[256],*d,c;
  int i;
  
  if(!ser)return;

  d=buf;
  *d++=STX;
  for(i=0;i<count;i++)
  { *d++=state[i];
  }
  *d++=ETX; *d=0;
  ser->Write(buf);
  
#ifdef ND_LINGO
void GUpdateDisplays()
// Update hardware
{
  char buf[16*2+2];
  int  i;
  static int cache[2]={ -1,-1 };                // Minimize display changes
  for(i=0;i<2*16+2;i++)
    buf[i]=0;
  if(cache[0]==m->jackpot[0]&&
     cache[1]==m->jackpot[1])
  { qdbg("> Update displays cached; skip\n");
    return;
  }
  // STX/ETX
  buf[0]=2;
  buf[2*16+1]=3;
  if(m->jackpot[0]!=-1)
    sprintf(&buf[1],"%5d",m->jackpot[0]);
  if(m->jackpot[1]!=-1)
    sprintf(&buf[17],"%5d",m->jackpot[1]);
  for(i=0;i<2*16+2;i++)
    if(buf[i]==0)buf[i]=' ';
  serDsp->Write(buf,2*16+2);
  QHexDump(buf,2*16+2);
  // Cache this update
  cache[0]=m->jackpot[0];
  cache[1]=m->jackpot[1];
}
#endif

}

/********
* DEBUG *
********/
void QVRDisplays::DbgShowState()
// Show state on stderr
{
  int i;
  qdbg("QVRDisplays state: ");
  for(i=0;i<count;i++)
  { if(i>0)qdbg("-");
    qdbg("%c",GetState(i));
  }
  qdbg("\n");
}

