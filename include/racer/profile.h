// rprofile.h

#ifndef __RACER_PROFILE_H
#define __RACER_PROFILE_H

#include <qlib/timer.h>

class RProfile
// Profiling where we spend our time
{
 public:
  enum Parts
  // Where we spend our time
  {
    PROF_PHYSICS,		// Profiling the physics engine
    PROF_CD_WHEEL_SPLINE,       // Wheel spline collision detection
    PROF_CD_WHEEL_TRACK,        // Other wheel-track collisions (rest contacts)
    PROF_CD_CARTRACK,           // Car-track collisions
    PROF_CD_OBJECTS,            // Car-car and all object-object collisions
    PROF_GRAPHICS,		// How long to draw
    PROF_SWAP,			// How long the swapping takes
    PROF_EVENTS,		// Event processing (OS)
    PROF_CONTROLS,		// Controller updating
    PROF_OTHER,			// Other areas
    PROF_MAX			// Number of parts
  };
  enum Counters
  {
    PROF_EV_xxx
  };
  enum Max
  {
    PROF_MAX_COUNTER=100
  };

 protected: 
  // Timers
  int timeUsed[PROF_MAX];
  int totalTime;
  QTimer *tmr;			// To time the things
  int part;
  // Counters
  int count[PROF_MAX_COUNTER];

 public:
  RProfile();
  ~RProfile();
  
  // Attribs
  int  GetPart(){ return part; }
  int  GetCount(int n){ return count[n]; }
  void AddCount(int n){ count[n]++; }

  // Profiling
  void Reset();
  void Update();
  void SwitchTo(int part);
  int    GetTimeFor(int part);
  rfloat GetTimePercentageFor(int part);
  int    GetTotalTime(){ return totalTime; }
};

#endif

