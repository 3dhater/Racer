/*
 * RTime
 * 05-10-00: Created!
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

RTime::RTime()
{
  // Default time span
  span=0.01;
  spanMS=10;
  tmr=new QTimer();
  Reset();
}
RTime::~RTime()
{
  QDELETE(tmr);
}

/**********
* Attribs *
**********/
void RTime::AddSimTime(int msecs)
// Another 'msecs' of time was simulated
{
  curSimTime+=msecs;
}
#ifdef OBS_INLINED
void RTime::SetLastSimTime(int t)
// 't' is the time to which the sim has been simulated
// This is set after a number of integrations to remember the last
// time for the next graphics frame (calculating the distance in time to 'now')
{
  lastSimTime=t;
}
#endif
void RTime::SetSpan(int ms)
// Set integration step time in milliseconds
{
  spanMS=ms;
  span=((rfloat)ms)/1000.0f;
}

/*******
* Dump *
*******/
bool RTime::LoadState(QFile *f)
{
  f->Read(&curRealTime,sizeof(curRealTime));
  f->Read(&curSimTime,sizeof(curSimTime));
  f->Read(&lastSimTime,sizeof(lastSimTime));
  
  // Make (volatile) timer contain the right time
  bool wasTimeRunning=tmr->IsRunning();
  int  t;
  tmr->Stop();
  t=tmr->GetMilliSeconds();
#ifdef OBS
qdbg("RTime:Load(); curSimTime=%d, curRealTime=%d, t=%d\n",
 curSimTime,curRealTime,t);
#endif
  tmr->AdjustMilliSeconds(curRealTime-t);
  if(wasTimeRunning)
  { // Restart timer, as it was running before the load
    tmr->Start();
  }
  
  return TRUE;
}
bool RTime::SaveState(QFile *f)
{
  f->Write(&curRealTime,sizeof(curRealTime));
  f->Write(&curSimTime,sizeof(curSimTime));
  f->Write(&lastSimTime,sizeof(lastSimTime));
  return TRUE;
}

/*************
* Start/stop *
*************/
void RTime::Start()
// Start the real timer
{
  tmr->Start();
}
void RTime::Stop()
// Stop the real timer
{
  tmr->Stop();
}
void RTime::Reset()
// Reset the real timer (to 0)
{
  tmr->Reset();
  curRealTime=0;
  curSimTime=0;
  lastSimTime=0;
}

/*********
* Update *
*********/
void RTime::Update()
// Update the real timing
{
  curRealTime=tmr->GetMilliSeconds();
}
