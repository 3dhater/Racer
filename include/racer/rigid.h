// racer/rigid.h - Rigid body (physics)
// 17-11-01: Begun to introduce ODE (Russell Smith)

#ifndef __RACER_RIGID_H
#define __RACER_RIGID_H

#include <d3/types.h>
#include <d3/bbox.h>
#include <d3/quat.h>
#include <d3/matrix.h>
#include <qlib/file.h>
#ifdef RR_ODE
#include <ode/ode.h>
#endif

class RRigidBody
// A rigid body class based on the state:
// - linear position (DVector3)
// - linear velocity (DVector3)
// - rotational position (DQuaternion) (orientation)
// - rotational velocity (DVector3)
// Based on Richard Chaney's source code and documentation
{
 private:
  // 'mass' is private, since 'oneOverMass' must always match,
  // so no direct access is allowed
  rfloat      mass,oneOverMass;

 protected:
  // Linear state
  DVector3 linPos,		// Linear position in world coordinates
           linVel,		// Velocity in world coordinates
           linVelBC;            // Velocity in body coordinates (convenience)
  // Rotational state
  DQuaternion qRotPos;		// The orientation is stored in a quat
  DMatrix3    mRotPos;		// Matrix from quat; converts body -> world
  DVector3    rotVel;		// Rotation velocity in BODY coordinates
  // Other state info
  DMatrix3    inertiaTensor;	// Only diagonal contains numbers

#ifdef RR_ODE
  dBodyID odeBody;
  dMass   odeMass;
  // Collision detection help
  dGeomID odeGeom;              // Bounding box (OBB) of car
#endif

  // Runge-Kutta 4th order integration (future)
  DVector3 linPos0,linVel0,rotVel0;
  DQuaternion qRotPos0;
  DMatrix3 rotPos0;
  DVector3 linVel1,rotVel1,linAcc1,rotAcc1;
  DVector3 linVel2,rotVel2,linAcc2,rotAcc2;
  DVector3 linVel3,rotVel3,linAcc3,rotAcc3;

  // Integration steps
  static rfloat h,h2,h4,h6;

  // Current total force and torque
  DVector3 totalForce,totalTorque;

  // Precalculated
  rfloat IyyMinusIzzOverIxx,IzzMinusIxxOverIyy,IxxMinusIyyOverIzz;
  rfloat oneOverIxx,oneOverIyy,oneOverIzz;

 public:
  RRigidBody();
  virtual ~RRigidBody();

  virtual bool LoadState(QFile *f);
  virtual bool SaveState(QFile *f);

  // Attributes
  void   SetMass(rfloat m);
  rfloat GetMass(){ return mass; }
  rfloat GetOneOverMass(){ return oneOverMass; }
  void   SetInertia(rfloat x,rfloat y,rfloat z);
  void   SetMassInertia(rfloat m,rfloat I11,rfloat I22,rfloat I33);
  static void SetTimeStep(rfloat h);

  void   CalcMatrixFromQuat();

#ifdef RR_ODE
  dBodyID      GetODEBody(){ return odeBody; }
  dMass        GetODEMass(){ return odeMass; }
  dGeomID      GetODEGeom(){ return odeGeom; }
#endif
  DVector3    *GetLinPos(){ return &linPos; }
  DVector3    *GetLinVel(){ return &linVel; }
  DVector3    *GetLinVelBC(){ return &linVelBC; }
  DVector3    *GetRotVel(){ return &rotVel; }
  DMatrix3    *GetRotPosM(){ return &mRotPos; }
  DQuaternion *GetRotPosQ(){ return &qRotPos; }
  DMatrix3    *GetInertia(){ return &inertiaTensor; }
  DVector3    *GetTotalTorque(){ return &totalTorque; }

  // Forces
  void AddWorldForceAtBodyPos(DVector3 *force,DVector3 *pos);
  void AddBodyForceAtBodyPos(DVector3 *force,DVector3 *pos);

  // Torques
  void AddBodyTorque(DVector3 *torque);
  void AddWorldTorque(DVector3 *torque);

#ifdef OBS_CHANEY
  // Direct torques
  void AddBodyTorque(DVector3 *torque);
  void AddWorldTorque(DVector3 *torque);
  // Forces with a point of application
  void AddWorldWorldForce(DVector3 *force,DVector3 *pos);
  void AddWorldBodyForce(DVector3 *force,DVector3 *pos);
  void AddBodyBodyForce(DVector3 *force,DVector3 *pos);
#endif

  // Coordinate system conversions
  void ConvertBodyToWorldPos(DVector3 *bodyPos,DVector3 *worldPos);
  void ConvertBodyToWorldOrientation(DVector3 *bodyOr,DVector3 *worldOr);
  void ConvertWorldToBodyPos(DVector3 *worldPos,DVector3 *bodyPos);
  void ConvertWorldToBodyOrientation(DVector3 *worldOr,DVector3 *bodyOr);

  // Coordinates system conversions for velocities
  void CalcWorldVelForBodyPos(const DVector3 *bodyPos,DVector3 *worldVel);
  void CalcBodyVelForBodyPos(const DVector3 *bodyPos,DVector3 *worldVel);

#ifdef FUTURE
  void CalcWorldPos(DVector3 *bodyPos,DVector3 *worldPos);
  void CalcBodyPos(DVector3 *worldPos,DVector3 *bodyPos);
  void CalcBodyWorldVel(DVector3 *bodyPos,DVector3 *worldVel);
  void CalcWorldWorldVel(DVector3 *worldPos,DVector3 *worldVel);
  void CalcWorldPosVel(DVector3 *bodyPos,DVector3 *worldPos,
    DVector3 *worldVel);
#endif

  // Integration
  void IntegrateInit();
  void IntegrateEuler();
  void PostIntegrate();

  // Debugging helpers
  rfloat GetEnergy();
  void   GetMomentum(DVector3 *linear,DVector3 *rotational);
};

#endif
