// racer/camera.h

#ifndef __RACER_CAMERA_H
#define __RACER_CAMERA_H

#include <qlib/vector.h>
#include <racer/types.h>

class RCar;

class RCamera
// A (car) camera definition
{
 public:
  RCar    *car;         // The car
  DVector3 offset;      // Offset to car center (world coordinates)
  DVector3 angle;       // Pitch/yaw/bank (roll)
  rfloat   lensAngle;   // Angle of lens in degrees; typically 65 (FOV)
  int      flags;
  int      view;        // Desired view
  qstring  name;        // Name of this camera setting
  int      index;	// Camera index

  // Dynamic variables
  DVector3 angleOffset;	// Animating the angle

 public:
  enum Flags
  {
    FOLLOW_YAW=1,       // Follow car's yaw
    FOLLOW_PITCH=2,     // Follow car's pitch
    FOLLOW_ROLL=4,
    ANIM_YAW=8,         // Auto-roll yaw
    ANIM_PITCH=16,
    ANIM_ROLL=32,
    INCAR=0x40,         // Camera is INSIDE the car (uses other model)
    NOCAR=0x80,         // Camera completely doesn't show the car
                        // Mutually exclusive with INCAR
    NO_WHEELS=0x100,    // Camera doesn't show the wheels
    FLAGS_xxx
  };

 public:
  RCamera(RCar *car);
  ~RCamera();

  bool Load(QInfo *info);

  DVector3 *GetOffset(){ return &offset; }
  DVector3 *GetAngle(){ return &angle; }
  rfloat    GetLensAngle(){ return lensAngle; }
  int       GetFlags(){ return flags; }
  int       GetView(){ return view; }
  void      GetOrigin(DVector3 *v) const;

  bool IsInCar(){ if(flags&INCAR)return TRUE; return FALSE; }

  void SetFlags(int n);
  void SetIndex(int n);

  // Graphics usage
  void Go();
};

#endif
