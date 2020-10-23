// rsteer.h

#ifndef __RSTEER_H
#define __RSTEER_H

#include <racer/car.h>

class RSteer
{
  RCar *car;

  // Physical
  float lock;              // Maximum angle (radians)
  DVector3 position;
  float radius;
  float xa;                // Rotation towards the driver's face
  
  // State (output)
  float angle;             // -lock .. +lock
  
  // Gfx
  GLUquadricObj *quad;
  RModel        *model;
  
  // Input
  int  axisInput;          // Controller position (axis)
  
 public:
  RSteer(RCar *car);
  ~RSteer();
  
  bool Load(QInfo *info,cstring path=0);
  bool LoadState(QFile *f);
  bool SaveState(QFile *f);
  
  float GetAngle(){ return angle; }
  float GetLock(){ return lock; }
  void  SetAngle(float _angle){ angle=_angle; }      // For replays

  void  SetInput(int axisPos);
  
  void  Paint();
  void  Integrate();
};

#endif

