/*
 * DFPS - framerate measurement
 * 06-01-2001: Created! (12:54:21)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/fps.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "DFPS"

DFPS::DFPS(int _method)
 : method(_method), framesRendered(0), lastRenderTime(0)
{
  DBG_C("ctor")
  DBG_ARG_I(_method)
  
  tmr=new QTimer();
  if(!tmr)qerr("DFPS ctor: Can't create timer");
  // Start for convenience; first few fps's may be inaccurate
  // but will even out soon
  tmr->Start();
  
  // Use time interval for LAG_DIV_2
  timeInterval=1000;
}
DFPS::~DFPS()
{
  QDELETE(tmr);
}

/**************
* Information *
**************/
dfloat DFPS::GetFPS()
// Returns frames per second
// Depending on the method, the returned value may be averaged over
// a time interval, so it will remain relatively steady.
{
  //int t=tmr->GetMilliSeconds();
  int t=lastRenderTime;
  dfloat fps;
  
  // Safety first
  if(!t)return 0.0f;
  
  // Calculate frames over the last time interval
  fps=((dfloat)(framesRendered*100000/t))/100.0f;
//qdbg("framesRendered=%d, t=%4d, fps=%.2f\n",framesRendered,t,fps);
  
  // Check for time adjustments (so we always keep a relatively
  // accurate fps over not too much time)
  if(method==LAG_DIV_2)
  {
    // Check for lapse of time interval
    if(t>=timeInterval)
    {
//qdbg("DFPS; retreat from t=%d\n",t);
      // Go back HALF the time interval, and assume that approximately
      // half of the frames rendered were in half that interval
      tmr->AdjustMilliSeconds(-timeInterval/2);
      framesRendered/=2;
    }
  }
  return fps;
}

/*******************
* Frames coming in *
*******************/
void DFPS::FrameRendered()
// Call this when a frame has been rendered
{
  int newTime;
  framesRendered++;
  // Remember time at which this frame was rendered.
  // This is done to avoid drift in the FPS because of time moving in
  // an unequal ratio wrt frames.
  newTime=tmr->GetMilliSeconds();
//qdbg("  FPS; last frame took %d ms\n",newTime-lastRenderTime);
  lastRenderTime=newTime;
}

/********
* Admin *
********/
void DFPS::Reset()
// Restart timing
{
  tmr->Reset();
  lastRenderTime=0;
  framesRendered=0;
  // Restart timer
  tmr->Start();
}
