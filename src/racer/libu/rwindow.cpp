/*
 * RWindow - Racer GUI support (animation mostly)
 * 04-11-01: Created!
 * NOTES:
 * - An attempt to get a 3D polygon-type button, instead of the regular
 * QLib buttons, but in the same framework to avoid coding a 2nd GUI.
 * FUTURE:
 * - Allow spline interpolation instead of linear for smoother flows
 * (c) Dolphinity/RvG
 */

#include <raceru/window.h>
#include <qlib/app.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

static QTimer *tmr;

/******************
* Animation timer *
******************/
RAnimTimer::RAnimTimer(int defaultVal)
  : baseTime(0),duration(0),delay(0),maxVal(defaultVal),flags(FINISHED)
{
  // Make sure a timer is present and running
  if(!tmr)
  {
    tmr=new QTimer();
    tmr->Start();
  }
}
RAnimTimer::~RAnimTimer()
{
}

void RAnimTimer::Trigger(int _maxVal,int _duration,int _delay,int _flags)
// Trigger a value rising from 0..maxVal (inclusive!)
{
  maxVal=_maxVal;
  duration=_duration;
  delay=_delay;
  // Take over allowed flags
  flags|=(_flags&0);

  baseTime=tmr->GetMilliSeconds();
  flags&=~FINISHED;
}
int RAnimTimer::GetValue()
// Returns current value for the envelope, based on the current time.
// Cache this value if you use it multiple times close together; it is dynamic.
{
  int t;

  // Speed things up when the effect is done
  if(IsFinished())return maxVal;

  t=tmr->GetMilliSeconds()-baseTime-delay;
  if(t>duration)
  {
    // Done
    flags|=FINISHED;
    return maxVal;
  }
  // Are will still living in front of the delay?
  if(t<0)return 0;

  // Linear interpolation (do spline?)
  return t*maxVal/duration;
}

void rrPaintGUI()
// Flat function to paint the entire GUI (from scratch)
// Do not call while driving; it may cost some graphics time
{
  int i,n;
  QWindow *w;

  // Set correct 3D view
  QCV->GetGLContext()->Setup3DWindow();
  // Make sure state is ok
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  //glEnable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_FOG);
  //glEnable(GL_BLEND);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  //glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glColor4f(1,1,1,1);

  n=QWM->GetNoofWindows();
  for(i=0;i<n;i++)
  {
    w=QWM->GetWindowN(i);
    // Avoid drawing the shell; this will paint a background
    if(w!=QSHELL&&w->IsVisible())
      w->Paint();
  }
}

