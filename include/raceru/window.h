// raceru/window.h

#ifndef __RACERU_WINDOW_H
#define __RACERU_WINDOW_H

#include <qlib/types.h>

// GUI animation
class RAnimTimer
{
 protected:
  int baseTime,               // Time at which anim was started
      duration,               // Length of anim
      delay,                  // Before starting the anim
      maxVal;                 // Value to reach over time
  int flags;

  enum Flags
  {
    FINISHED=1
  };

 public:
  RAnimTimer(int defaultVal=0);
  ~RAnimTimer();

  // Attribs
  int  IsFinished(){ if(flags&FINISHED)return TRUE; return FALSE; }

  // Envelopes
  void Trigger(int maxVal,int duration,int delay=0,int flags=0);
  int  GetValue();
};

// Flat functions
void rrPaintGUI();

#endif

