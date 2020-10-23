// racer/chair.h

#ifndef __RACER_CHAIR_H
#define __RACER_CHAIR_H

#ifndef __NLIB_COMMON_H
#include <nlib/common.h>
#endif

class RChair
{
  enum Flags
  {
    ENABLE=1            // Use
  };
  int flags;
  QNSocket  *socket;
  QNAddress  address;
  QNChannel *chan;               // Unused currently
  qstring    host;               // Host to send to
  int        port;

  // Collected data
  DVector3 bodyAcc;              // Acceleration of body (g-forces)
  DMatrix3 bodyOr;               // Orientation matrix of the body

  int timePerUpdate;             // #ms between each update

 public:
  int nextUpdateTime;            // When is the next update to happen?
  enum DataID
  // Which data is to come? (with fixed length data chunk)
  {
    ID_ACC=1,                    // Acceleration (float acc[3], or DVector3)
    ID_OR=2                      // Orientation 3x3 matrix (DMatrix3)
  };

 public:
  RChair();
  ~RChair();

  void Enable();
  void Disable();

  bool IsEnabled(){ if(flags&ENABLE)return TRUE; return FALSE; }
  bool IsTimeForUpdate();

  void Update();
};

#endif

