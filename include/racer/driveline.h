// racer/driveline.h

#ifndef __RACER_DRIVELINE_H
#define __RACER_DRIVELINE_H

class RDriveLine;

class RDriveLineComp
// Driveline component base class
{
 protected:
  // Administration
  qstring name;                   // For easier debugging
  RDriveLineComp *parent,         // Bi-directional tree
                 *child[2];
  RDriveLine *driveLine;          // Part of which driveline

 private:
  // Static data
  rfloat inertia;                 // Inertia of this component
  rfloat ratio;                   // Gearing ratio (gearbox/differential)
  rfloat invRatio;                // 1/ratio (optimized multiplication)

  // Semi-static data (recalculated when gear is changed)
  rfloat effectiveInertiaDownStream;  // Inertia of component + children
  rfloat cumulativeRatio;         // Ratio at this point of the driveline

  // Dynamic data
 protected:
  rfloat tReaction,               // Reaction torque
         tBraking,                // Braking torque (also a reaction torque)
         tEngine;                 // Torque from the engine side
  // State
  rfloat rotV,                    // Rotational velocity
         rotA;                    // Rotational acceleration

 public:
  RDriveLineComp();
  virtual ~RDriveLineComp();

  // Attribs
  cstring GetName(){ return name.cstr(); }
  void    SetName(cstring s){ name=s; }
  rfloat GetInertia(){ return inertia; }
  void   SetInertia(rfloat i){ inertia=i; }
  rfloat GetRatio(){ return ratio; }
  void   SetRatio(rfloat r);
  rfloat GetInverseRatio(){ return invRatio; }
  rfloat GetEffectiveInertia(){ return effectiveInertiaDownStream; }
  rfloat GetCumulativeRatio(){ return cumulativeRatio; }
  RDriveLine *GetDriveLine();
  void        SetDriveLine(RDriveLine *dl){ driveLine=dl; }
  RDriveLineComp *GetParent(){ return parent; }
  RDriveLineComp *GetChild(int n);
  rfloat GetReactionTorque(){ return tReaction; }
  rfloat GetBrakingTorque(){ return tBraking; }
  rfloat GetEngineTorque(){ return tEngine; }

  rfloat GetRotationVel(){ return rotV; }
  void   SetRotationVel(rfloat v){ rotV=v; }
  rfloat GetRotationAcc(){ return rotA; }
  void   SetRotationAcc(rfloat a){ rotA=a; }

  // Attaching to other components
  void AddChild(RDriveLineComp *comp);
  void SetParent(RDriveLineComp *comp);

  // Reset for new use
  virtual void Reset();
  virtual bool LoadState(QFile *f);
  virtual bool SaveState(QFile *f);

  // Precalculation
  void CalcEffectiveInertia();
  void CalcCumulativeRatio();

  // Physics
  void CalcReactionForces();
  void CalcEngineForces();
  virtual void CalcForces();
  virtual void CalcAccelerations();
  virtual void Integrate();

  // Debugging
  void DbgPrint(int depth,cstring s);
};

class RDriveLine
// A complete driveline
// Includes the clutch and the handbrakes
{
 private:
  RDriveLineComp *root;           // Root of driveline tree; the engine
  RDriveLineComp *gearbox;        // The gearbox is always there
  RCar *car;

  // Static driveline data
  int diffs;                      // Number of differentials

  // Semi-static driveline data (changes when gear is changed)
  rfloat preClutchInertia,        // Total inertia before clutch (engine)
         postClutchInertia,       // After clutch (gearbox, diffs)
         totalInertia;            // Pre and post clutch inertia

  // Clutch
  rfloat clutchApplication,       // 0..1 (how much clutch is applied)
         clutchLinearity,         // A linear clutch was too easy to stall
         clutchMaxTorque,         // Clutch maximum generated torque (Nm)
         clutchCurrentTorque;     // Clutch current torque (app*max)
  bool   autoClutch;              // Assist takes over clutch?

  // Handbrakes
  rfloat handbrakeApplication;    // 0..1 (control)

  // Dynamic
  bool   prepostLocked;           // Engine is locked to rest of drivetrain?
  rfloat tClutch;                 // Directed clutch torque (+/-clutchCurT)

 public:
  RDriveLine(RCar *car);
  ~RDriveLine();

  // Attribs
  void SetRoot(RDriveLineComp *comp);
  RDriveLineComp *GetRoot(){ return root; }
  RDriveLineComp *GetGearBox(){ return gearbox; }
  void SetGearBox(RDriveLineComp *c){ gearbox=c; }
  bool IsSingleDiff(){ return diffs==1; }
  int  GetDifferentials(){ return diffs; }
  void SetDifferentials(int n){ diffs=n; }
  bool IsPrePostLocked(){ return prepostLocked; }
  void LockPrePost(){ prepostLocked=TRUE; }
  void UnlockPrePost(){ prepostLocked=FALSE; }
  rfloat GetClutchTorque(){ return tClutch; }

  // Precalculation
  void CalcCumulativeRatios();
  void CalcEffectiveInertiae();
  void CalcPreClutchInertia();
  void CalcPostClutchInertia();

  // Clutch
  rfloat GetClutchApplication(){ return clutchApplication; }
  rfloat GetClutchMaxTorque(){ return clutchMaxTorque; }
  rfloat GetClutchCurrentTorque(){ return clutchCurrentTorque; }
  void SetClutchApplication(rfloat app);
  bool IsAutoClutchActive(){ return autoClutch; }
  void EnableAutoClutch(){ autoClutch=TRUE; }
  void DisableAutoClutch(){ autoClutch=FALSE; }

  // Handbrakes
  rfloat GetHandBrakeApplication(){ return handbrakeApplication; }

  // Definition
  bool Load(QInfo *info);
  bool LoadState(QFile *f);
  bool SaveState(QFile *f);
 
  // Input
  void SetInput(int ctlClutch,int ctlHandbrake);

  // Physics
  void Reset();
  void CalcForces();
  void CalcAccelerations();
  void Integrate();

  // Debugging
  void DbgPrint(cstring s);
};

#endif

