// d3/fps.h - Framerate measurement

#ifndef __D3_FPS_H
#define __D3_FPS_H

#include <qlib/timer.h>
#include <d3/types.h>

class DFPS
// Calculates frames per second
{
 public:
  enum Methods
  {
    LAG_DIV_2		// Use time interval, dividing by 2 on every lapse
  };

 protected:
  QTimer *tmr;
  int     framesRendered,
          lastRenderTime;
  int     method;
  int     timeInterval;

 public:
  DFPS(int method=LAG_DIV_2);
  ~DFPS();

  // Attribs
  dfloat GetFPS();

  // Operations
  void FrameRendered();
  void Reset();          // Use when instance hasn't been used for a while
};

#endif
