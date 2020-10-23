// racer/ai.h

#ifndef __RACER_AI_H
#define __RACER_AI_H

class RWayPoint
// Tracks contain waypoint for the AI
{
 public:
  enum Flags
  {
    SELECTED=1                    // Hilites waypoint (for editing)
  };
  int      flags;
  DVector3 pos,                   // Target point to drive through
           posDir;                // Directional vertex
  DVector3 left,                  // Left/right side of road (perpendicular to
           right;                 // the driving direction)
  dfloat   lon;                   // Estimated longitudinal position

 public:
  RWayPoint()
  { pos.SetToZero(); posDir.SetToZero();
    left.SetToZero(); right.SetToZero();
    lon=0;
    flags=0;
  }
};

//
// Driver AI classes
//

class RWayPointKnowledge
// Info that the driver learns about a track waypoint
{
 public:
  dfloat speed;                   // Desired vehicle speed

 public:
  RWayPointKnowledge()
  {
    speed=0;
  }
};

class RCar;

class RRobot
// An articifical driver
{
 protected:
  int curWayPoint;                // Which waypoint next?
  RCar *car;

 public:
  RRobot(RCar *car);
  ~RRobot();

  RCar *GetCar(){ return car; }

  void Decide();
};

#endif
