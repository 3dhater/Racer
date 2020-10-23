// rbody.h

#ifndef __RBODY_H
#define __RBODY_H

#include <racer/model.h>
#include <racer/rigid.h>
#include <d3/bbox.h>
#include <d3/obb.h>

class RCar;

class RBody : public RRigidBody
// The vehicle's body; no wings, no wheels, just the tub
// This is the focal point of the 'car'
{
 protected:
  enum Flags
  {
    DRAW_BBOX=1			// Paint bounding box?
  };

  RCar *car;
  int   flags;

  // Physical
  DVector3 size;           // Size of body (stub gfx)
  rfloat cockpitZ,cockpitLength;   // Room for cockpit (for RSteer)
  rfloat coeffRestitution;     // Bounciness
  DVector3 aeroCenter,         // Drag center relative to CG
           aeroCoeff;          // Drag coefficients (OpenGL axis system)
  rfloat   aeroArea;           // Drag area

  // State
  DOBB     obb;                // Bounding box for collisions
  DAABB    aabb;               // Bounding box for track tree collisions

  // Dynamics; forces
  DVector3 force;              // Force at body?
  DVector3 forceGravityWC;     // Gravity force at CM in world coords
  DVector3 forceGravityCC;     // Gravity in car coords
  DVector3 forceDragCC;        // Drag in car coords

  // Gfx
  GLUquadricObj *quad;
  RModel        *model,
                *modelIncar;    // Other model for incar views
  DBoundingBox  *bbox;		// For debugging/creation
  RModel        *modelBraking[2]; // 2 sides
  RModel        *modelLight[2];   // Front lights

  // Debugging
  DVector3 vCollision;          // A point of body collision

 public:
  RBody(RCar *car);
  ~RBody();
  
  void Destroy();
  void Reset();

  bool Load(QInfo *info,cstring path=0);
  bool LoadState(QFile *f);
  bool SaveState(QFile *f);
 
  // Attribs
  rfloat    GetCoeffRestitution(){ return coeffRestitution; }
  DOBB     *GetOBB(){ return &obb; }
  DAABB    *GetAABB(){ return &aabb; }
  DVector3 *GetForceGravityWC(){ return &forceGravityWC; }
  DVector3 *GetForceGravityCC(){ return &forceGravityCC; }
  // Map through old Get...()
  DVector3 *GetPosition(){ return GetLinPos(); }
  DVector3 *GetVelocity(){ return GetLinVel(); }
  DVector3 *GetSize(){ return &size; }
  RModel   *GetModel(){ return model; }
  // Debugging state
  DVector3 *GetCollisionPoint(){ return &vCollision; }

  // Gfx
  void  DisableBoundingBox();
  void  EnableBoundingBox();
  void  PaintSetup();
  void  Paint();
  void  Integrate();
  
  // Physics
  void  PreAnimate();
  void  CalcForces();
  void  CalcAccelerations();
  void  ApplyRotations();
  void  DetectCollisions();

  // Debugging
  bool  DbgCheck(cstring s);
};

#endif
