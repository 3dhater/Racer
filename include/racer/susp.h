// rsusp.h

#ifndef __RSUSP_H
#define __RSUSP_H

#include <GL/glu.h>
#include <d3/geode.h>

class RCar;

class RSuspension
{
  enum Flags
  {
    DRAW_BBOX=1                 // Paint bounding box?
  };

 protected:
  // Fixed properties
  RCar    *car;
  int      flags;

  // Physical attribs
  DVector3 position;                    // Top relative position to car
  DVector3 rollCenter;                  // Static rest roll center
  // Spring
  rfloat   restLength,                  // Unsprung length of entire suspension
           minLength,                   // Minimal physical length
           maxLength;                   // Maximal physical length
  rfloat   k;                           // Spring constant/wheel rate (N/m)
  // Damper
  rfloat   bumpRate,                    // Nominal compression rate (N/m/s)
           reboundRate;
  rfloat   radius;                      // Radius of cylinder
  // Anti-rollbar; directly incorporated here, since the structure
  // is so simple.
  RSuspension *arbOtherSusp;            // Suspension on the other side
  rfloat       arbRate;                 // N/m

  // State
  rfloat   length;                      // Current length
  DVector3 pistonVelocity;              // Speed at which piston is moving
  DVector3 forceBody,                   // Force that body is putting on us
           forceWheel;                  // Force that wheels puts on us
  DVector3 forceSpring,                 // Total force of the spring
           forceDamper,                 // Damping force of the damper
           forceARB;                    // Anti-roll bar

  // Graphics
  int            slices;
  GLUquadricObj *quad;
  RModel        *model;
  DBoundingBox  *bbox;

 public:
  RSuspension(RCar *car);
  ~RSuspension();
  
  // Attribs
  rfloat    GetLength(){ return length; }
  rfloat    GetMinLength(){ return minLength; }
  rfloat    GetMaxLength(){ return maxLength; }
  rfloat    GetRestLength(){ return restLength; }
  rfloat    GetK(){ return k; }
  rfloat    GetBumpRate(){ return bumpRate; }
  rfloat    GetReboundRate(){ return reboundRate; }
  DVector3 *GetPosition(){ return &position; }
  DVector3 *GetRollCenter(){ return &rollCenter; }
  DVector3 *GetForceBody(){ return &forceBody; }
  DVector3 *GetForceWheel(){ return &forceWheel; }
  DVector3 *GetForceSpring(){ return &forceSpring; }
  DVector3 *GetForceDamper(){ return &forceDamper; }
  DVector3 *GetForceARB(){ return &forceARB; }
  // XYZ relative to geometrical center
  rfloat    GetX(){ return position.GetX(); }
  rfloat    GetY(){ return position.GetY(); }
  rfloat    GetZ(){ return position.GetZ(); }
  // Set
  void      SetLength(rfloat l){ length=l; }

  // Anti-rollbar
  RSuspension *GetARBOtherSuspension(){ return arbOtherSusp; }
  rfloat       GetARBRate(){ return arbRate; }
  void         SetARBOtherSuspension(RSuspension *s){ arbOtherSusp=s; }
  void         SetARBRate(rfloat rate){ arbRate=rate; }

  void  Destroy();

  void  Init();
  bool  Load(QInfo *info,cstring path);
  bool  LoadState(QFile *f);
  bool  SaveState(QFile *f);
  void  Reset();

  void  DisableBoundingBox();
  void  EnableBoundingBox();
  void  Paint();
  
  // Physics
  void  PreAnimate();
  void  CalcForces();
  void  CalcAccelerations();
  void  Integrate();
  
  bool  ApplyWheelTranslation(DVector3 *translation,DVector3 *velocity);
};

#endif
