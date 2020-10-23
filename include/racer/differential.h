// racer/differential.h

#ifndef __RACER_DIFFERENTIAL_H
#define __RACER_DIFFERENTIAL_H

#include <racer/driveline.h>

class RCar;
class RWheel;
class REngine;

class RDifferential : public RDriveLineComp
// A differential that has 3 parts; 1 'input', and 2 'outputs'.
// Actually, all parts work together in deciding what happens.
{
 public:
  enum Type
  {
    FREE=0,                // Free/open differential
    VISCOUS=1,             // Viscous locking differential
    SALISBURY=2            // Grand Prix Legends type clutch locking
  };
  enum Flags
  {
    //LINEAR_LOCKING=1       // Viscous diff is of linear (expensive) type
  };

 protected:

  // Definition (static input)
  int    type;
  int    flags;            // Behavior flags
  rfloat lockingCoeff;     // Coefficient for viscous diff
  // Salisbury diff
  rfloat powerAngle,coastAngle;    // Ramp angles
  int    clutches;                 // Number of clutches in use
  rfloat clutchFactor;             // Scaling the effect of the clutches
  rfloat maxBiasRatioPower;        // Resulting max. bias ratio
  rfloat maxBiasRatioCoast;        // Resulting max. bias ratio

  // Input

  // Torque on the 3 parts
  rfloat torqueIn;
  rfloat torqueOut[2];
  rfloat torqueBrakingOut[2];        // Potential braking torque
  // Inertia of objects
  rfloat inertiaIn;
  rfloat inertiaOut[2];
  // Limited slip differentials add a locking torque
  rfloat torqueLock;

  // State
  rfloat velASymmetric;              // Difference in wheel rotation speed
  rfloat torqueBiasRatio;            // Relative difference in reaction T's
  rfloat torqueBiasRatioAbs;         // Always >=1
  int    locked;                     // Mask of locked ends

  // Output

  // Resulting accelerations
  rfloat accIn,
         accASymmetric;
  rfloat accOut[2];
  rfloat rotVdriveShaft;             // Speed of driveshaft (in)

  RCar *car;

 public:
  // Input
  REngine *engine;
  // Output members
  RWheel  *wheel[2];

 public:
  RDifferential(RCar *car);
  ~RDifferential();

  void   Reset();

  bool   Load(QInfo *info,cstring path);
  bool LoadState(QFile *f);
  bool SaveState(QFile *f);

#ifdef OBS
  void   SetTorqueIn(rfloat torque);
  rfloat GetTorqueOut(int n);

  void   SetInertiaIn(rfloat inertia);
  void   SetInertiaOut(int n,rfloat inertia);
#endif

  rfloat GetAccIn() const { return accIn; }
  rfloat GetAccASymmetric() const { return accASymmetric; }
  rfloat GetAccOut(int n) const { return accOut[n]; }
  // Salisbury info
  rfloat GetTorqueBiasRatio(){ return torqueBiasRatio; }
  rfloat GetMaxBiasRatioPower(){ return maxBiasRatioPower; }
  rfloat GetMaxBiasRatioCoast(){ return maxBiasRatioCoast; }
  rfloat GetPowerAngle(){ return powerAngle; }
  rfloat GetCoastAngle(){ return powerAngle; }
  int    GetClutches(){ return clutches; }
  rfloat GetClutchFactor(){ return clutchFactor; }
  // Other info
  rfloat GetRotationalVelocityIn() const { return rotVdriveShaft; }

  void   Lock(int wheel);
  bool   IsLocked(int wheel)
  { if(locked&(1<<wheel))return TRUE; return FALSE; }

  rfloat CalcLockingTorque();
  void   CalcSingleDiffForces(rfloat torqueIn,rfloat inertiaIn);
  void   CalcForces();
  void   Integrate();
};

#endif
