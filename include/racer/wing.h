// racer/wing.h

#ifndef __RACER_WING_H
#define __RACER_WING_H

class RCar;

class RWing
// A wing, or aero part (spoiler, air dam)
{
 protected:

  // Definition (static input)
  qstring name;            // To identify
  rfloat  span,            // Width of wing
          cord;            // Depth (span*cord = area)
  DVector3 coeff;          // Functionality in 3 dimensions
  DVector3 center;         // Center of pressure application
  rfloat   angle;          // Angle of wing (in radians)
  rfloat   angleOffset;    // A zero degree wing still delivers forces
  // Precalculated state
  rfloat   fixedFactorY,   // span*cord*coeff.y*angle*airDensity precalculated
           fixedFactorZ;   // span*cord*coeff.z*angle*airDensity precalculated

  // Input
  DVector3 airVelocity;    // Relative air velocity at center

  // Output
  DVector3 force;          // Forces in 3 dimensions
  DVector3 torque;         // Torque that the wing feels (may be unused)

  // Parent
  RCar *car;

 public:
  RWing(RCar *car);
  ~RWing();

  // Attribs
  DVector3 *GetForce(){ return &force; }
  DVector3 *GetAirVelocity(){ return &airVelocity; }
  DVector3 *GetTorque(){ return &torque; }

  void Reset();
  void PreCalc();

  bool Load(QInfo *info,cstring path);
  bool LoadState(QFile *f);
  bool SaveState(QFile *f);

  // Graphics
  void   Paint();

  // Physics
  void   CalcForces();
  void   CalcAccelerations();
  void   Integrate();
};

#endif
