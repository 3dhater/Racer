// racer/engine.h
// 30-12-01: Gearbox split from engine into class RGearBox

#ifndef __RACER_ENGINE_H
#define __RACER_ENGINE_H

#include <qlib/curve.h>
#include <racer/driveline.h>

class RWheel;

class REngine : public RDriveLineComp
{
 public:
  enum Flags
  {
    STALLED=1,                          // Engine is turned off
    HAS_STARTER=2,                      // Start engine present?
    START_STALLED=4                     // When reset, don't autostart engine?
    //AUTOCLUTCH_ACTIVE=8                 // Assist on?
  };

 protected:
  // Semi-fixed properties
  RCar    *car;                         // The car to which we belong

  // Static data
  rfloat   size;
  DVector3 position;
  rfloat   mass;
  int      flags;
  rfloat   maxRPM,                      // Hard maximum
           idleRPM,                     // RPM when no throttle/friction
           stallRPM,                    // At which point does the engine stall
           startRPM,                    // At which point does it turn on again?
           autoClutchRPM;               // When to start applying the clutch
  rfloat   friction;                    // Friction coeff of motor itself
  rfloat   brakingCoeff;                // To calculate engine braking
  rfloat   idleThrottle,                // Always open by this much 0..1
           throttleRange;               // Effective throttle range
  QCurve  *crvTorque;                   // RPM -> max normalized engine torque
  rfloat   maxTorque;                   // Factor for normalize torque curve
  rfloat   starterTorque;               // Torque generated by starter

  // Dynamic data (some (tEngine) is stored in RDriveLineComp)
  rfloat   torqueReaction;              // Amount (0..1) at which engine
                                        // torque reaches the body

  // Gfx
  GLUquadricObj *quad;
  
  // Physical attributes
#ifdef OBS
  // Inertia
  rfloat  inertiaEngine,                 // Engine internal rotation
          inertiaDriveShaft;             // Differential
  // Gearbox
  rfloat gearRatio[MAX_GEAR];
  int    gears;
  rfloat endRatio;                       // Final drive ratio
  rfloat gearInertia[MAX_GEAR];          // Rotational inertia per gear
  // Shifting
  int    timeToDeclutch,                 // Auto-shifting time
         timeToClutch;
  rfloat shiftUpRPM,                     // Automatic transmissions
         shiftDownRPM;
#endif

  // State (dynamic output)
  //int    curGear;                        // Selected gear
  //rfloat force;                          // Force put out
  //rfloat torque;                         // Torque sent to drivetrain
  //rfloat clutch;                         // Clutch 0..1 (1=fully locked)
  //int    autoShiftStart;                 // Time of auto shift initiation
  //int    futureGear;                     // Gear to shift to
  //rfloat rotationV;                      // Engine rotation speed
 // rfloat torqueWheels,                   // Torque available for the wheels
         //torqueEngine;

  // Input
  rfloat throttle;
  rfloat brakes;
  
 public:
  REngine(RCar *car);
  ~REngine();

  void Reset();                          // Reset all vars
  void Init();                           // Initialize usage of engine
  void PreCalculate();                   // Precalculate some variables

  // Definition
  bool Load(QInfo *info,cstring path=0);
  bool LoadState(QFile *f);
  bool SaveState(QFile *f);

  // Attribs
  rfloat GetMass(){ return mass; }
  //rfloat GetForce(){ return force; }
  rfloat GetRPM(){ return (GetRotationVel()/(2*PI))*60.0f; }
  void   SetRPM(rfloat rpm);
  rfloat GetTorque(){ return GetEngineTorque(); }
  rfloat GetTorqueReaction(){ return torqueReaction; }
  //rfloat GetRollingFrictionCoeff(){ return rollingFrictionCoeff; }
  bool   IsStalled(){ if(flags&STALLED)return TRUE; return FALSE; }
  void   EnableStall(){ flags|=STALLED; }
  void   DisableStall(){ flags&=~STALLED; }
  bool   HasStarter(){ if(flags&HAS_STARTER)return TRUE; return FALSE; }

/*
  bool   IsAutoClutchActive()
  { if(flags&AUTOCLUTCH_ACTIVE)return TRUE; return FALSE; }
  rfloat GetAutoClutch(){ return autoClutch; }
*/

  // Engine torque generation
  rfloat GetMinTorque(rfloat rpm);
  rfloat GetMaxTorque(rfloat rpm);
  rfloat GetTorqueAtDifferential();
#ifdef OBS
  //rfloat GetEngineInertia(){ return inertiaEngine; }
  rfloat GetGearInertia(){ return gearInertia[curGear]; }
  rfloat GetInertiaAtDifferential();
  rfloat GetInertiaForWheel(RWheel *w);
  rfloat GetTorqueForWheel(RWheel *w);
#endif

#ifdef OBS
  // Gearbox
  int    GetGears(){ return gears; }
  int    GetGear(){ return curGear; }
  rfloat GetGearRatio(int n);
  rfloat GetEndRatio();
  void   SetGear(int n);
#endif
 
  // Input
  void SetInput(int ctlThrottle);
  
  // Graphics
  void Paint();

  // Physics
  void CalcForces();
  void CalcAccelerations();
  void Integrate();
  void OnGfxFrame();

  // Debugging
  void DbgPrint(cstring s);
};

#endif
