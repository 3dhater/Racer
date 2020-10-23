// racer/pacejka.h

#ifndef __RACER_PACEJKA_H
#define __RACER_PACEJKA_H

#include <d3/types.h>

class RPacejka
// A Pacejka calculation engine
// Uses SAE axis system for reference
// (SAE X=Racer Z, SAE Y=Racer X, SAE Z=Racer -Y)
{
 public:
  // Fx longitudinal force
  rfloat a0,a1,a2,a3,a4,a5,a6,a7,a8,
         a9,a10,a111,a112,a12,a13;
  // Fy lateral force
  rfloat b0,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10;
  // Mz aligning moment
  rfloat c0,c1,c2,c3,c4,c5,c6,
         c7,c8,c9,c10,c11,c12,c13,c14,c15,c16,c17;

 protected:
  // Input parameters
  rfloat camber,                // Angle of tire vs. surface (in degrees!)
         sideSlip,              // Slip angle (in degrees!)
         slipPercentage,        // Percentage slip ratio (in %)
         Fz;                    // Normal force (in kN!)
  // Output
  rfloat Fx,Fy,Mz;              // Longitudinal/lateral/aligning moment
  rfloat longStiffness,         // Longitudinal tire stiffness
         latStiffness;          // Lateral or cornering stiffness
  DVector3 maxForce;            // Max. available tire force (friction ellipse)

 public:
  RPacejka();
  ~RPacejka();

  bool  Load(QInfo *info,cstring path);

  // Attribs
  void SetCamber(rfloat _camber){ camber=_camber*RR_RAD2DEG; }
  void SetSlipAngle(rfloat sa){ sideSlip=sa*RR_RAD2DEG; }
  void SetSlipRatio(rfloat sr){ slipPercentage=sr*100.0f; }
  void SetNormalForce(rfloat force){ Fz=force/1000.0f; }

  // Physics
  void   Calculate();
 protected:
  rfloat CalcFx();
  rfloat CalcFy();
  rfloat CalcMz();
 public:
  rfloat GetFx(){ return Fx; }
  rfloat GetFy(){ return Fy; }
  rfloat GetMz(){ return Mz; }
  // Adjust (used in combined slip)
  void   SetFx(rfloat v){ Fx=v; }
  void   SetFy(rfloat v){ Fy=v; }
  void   SetMz(rfloat v){ Mz=v; }

  // Extras
  rfloat GetMaxLongForce(){ return maxForce.x; }
  rfloat GetMaxLatForce(){ return maxForce.y; }
  // Adjust (used for surface and quick-adjust modifications)
  void   SetMaxLongForce(rfloat v){ maxForce.x=v; }
  void   SetMaxLatForce(rfloat v){ maxForce.x=v; }
  rfloat GetLongitudinalStiffness(){ return longStiffness; }
  rfloat GetCorneringStiffness(){ return latStiffness; }
};

#endif
