/*
 * QVRLights - Vos & Ramselaar Lights interface
 * NOTES:
 * - V&R interface for lamps (and latches)
 * - Still works when serial port can't be opened; just silently
 * 14-03-2000: Created! (17:04:35)
 * 25-07-01: Added support for sharing serial port (with QButtons for ex.)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <qlib/vrlights.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define STX 2
#define ETX 3

// Speed of interface
#define BPS    9600

QVRLights::QVRLights(int iCount,int unit,QSerial *shareSer)
// Construct lights and open serial connection for 'iCount' lights
{
  int i;
  
  flags=0;
  count=iCount;
  if(count>MAX_LIGHTS)
  { qwarn("QVRLights: count(%d) > MAX(%d), clipped to max lights",
      count,MAX_LIGHTS);
    count=MAX_LIGHTS;
  }
  for(i=0;i<count;i++)
    state[i]=OFF;
  if(shareSer)
  {
    // Use the already existing serial port
    ser=shareSer;
  } else
  {
    // Open our own
    flags|=PRIVATE_SER;
    ser=new QSerial(unit,BPS /*,QSerial::WRITEONLY*/);
    if(!ser->IsOpen())
    { qerr("QVRLights: can't open serial unit %d",unit);
      delete ser; ser=0;
    }
  }
}

QVRLights::~QVRLights()
{
  if(flags&PRIVATE_SER)
  { QDELETE(ser);
  }
}

int QVRLights::GetState(int light)
// Get state of a light <b>OFF</b> or <b>ON</b>
{
  if(light>count)return OFF;
  return state[light];
}

void QVRLights::SetState(int light,int newState)
// Set state of a light
{
  if(light>count)return;
  state[light]=newState;
}
void QVRLights::SetStates(int newState)
// Set state of all lights
{
  int i;
  for(i=0;i<count;i++)
    state[i]=newState;
}

void QVRLights::Update()
// Send out lamp state to interface
{ char buf[256],*d,c;
  int i;
  
  QASSERT_V(ser);	// No serial port was opened
  
  d=buf; *d++=STX;
  for(i=0;i<(count+3)/4;i++)
  { c=0x30;
    if(state[i*4+0])c|=1;
    if(state[i*4+1])c|=2;
    if(state[i*4+2])c|=4;
    if(state[i*4+3])c|=8;
    *d++=c;
  }
  *d++=3;                       // ETX
  *d=0;
  ser->Write(buf);
}

/********
* DEBUG *
********/
void QVRLights::DbgShowState()
// Show state on stderr
{
  int i;
  qdbg("QVRLights state: ");
  for(i=0;i<count;i++)
  { qdbg("%d",GetState(i));
  }
  qdbg("\n");
}

