/*
 * RProfile - profiling the code
 * 22-10-00: Created!
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Some processes may be very fast; we don't have fast CPUs for nothing!
// Millisecond accuracy may then be inaccurate for processes like
// collision detection (where caching is used).
// It all comes to the accuracy of the system timer ofcourse.
#define USE_MICROSECOND_ACCURACY

RProfile::RProfile()
{
  int i;

  for(i=0;i<PROF_MAX;i++)
    timeUsed[i]=0;
  part=PROF_OTHER;
  tmr=new QTimer();
  tmr->Start();
  totalTime=0;
}

RProfile::~RProfile()
{
  QDELETE(tmr);
}

void RProfile::Reset()
{
  int i;

  tmr->Reset();
  totalTime=0;
  for(i=0;i<PROF_MAX;i++)
    timeUsed[i]=0;
  for(i=0;i<PROF_MAX_COUNTER;i++)
    count[i]=0;
  tmr->Start();
}

void RProfile::Update()
// Look at timer and update the current figures
{
  int i,t,newTotalTime;

#ifdef USE_MICROSECOND_ACCURACY
  newTotalTime=tmr->GetMicroSeconds();
#else
  newTotalTime=tmr->GetMilliSeconds();
#endif

  // Auto reset to keep timing current
#ifdef USE_MICROSECOND_ACCURACY
  if(newTotalTime>1000000)
#else
  if(newTotalTime>1000)
#endif
  {
    // Reset timing counts
    for(i=0;i<PROF_MAX;i++)
      timeUsed[i]=0;
    totalTime=0;
    tmr->Reset();
    tmr->Start();
    return;
  }

  // Get time spent
  t=newTotalTime-totalTime;
  totalTime=newTotalTime;

  // Add to current part
  timeUsed[part]+=t;
}

void RProfile::SwitchTo(int newPart)
// Switch to other part to time
{
  Update();
  // Set new part
  part=newPart;
}

int RProfile::GetTimeFor(int part)
// Returns time in milliseconds for part 'part'
// Call Update() BEFORE calling this function (not done here for efficiency
// of retrieving profile numbers)
{
#ifdef USE_MICROSECOND_ACCURACY
  return timeUsed[part]/1000;
#else
  return timeUsed[part];
#endif
}

rfloat RProfile::GetTimePercentageFor(int part)
// Convenience function to return the time spent in the part as a percentage
// Call Update() BEFORE calling this function (not done here for efficiency
// of retrieving profile numbers)
{
  if(!totalTime)return 0;
//qdbg("timeUsed[%d]=%d, totalTime=%d\n",part,timeUsed[part],totalTime);
  return rfloat(timeUsed[part])*100.0f/rfloat(totalTime);
}

