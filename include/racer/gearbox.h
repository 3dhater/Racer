// racer/gearbox.h
// 16-01-02: Gear 0 is now neutral. Gear R123 etc start at gear=1.
// (c) 2001, 2002 Dolphinity/RvG

#ifndef __RACER_GEARBOX_H
#define __RACER_GEARBOX_H

#include <qlib/curve.h>
#include <racer/driveline.h>

class RGearBox : public RDriveLineComp
{
 public:
  enum Max
  { MAX_GEAR=10                         // #gears possible
  };
  enum Flags
  {
    AUTOMATIC=1                         // Auto shifting
  };

 protected:
  // Semi-fixed properties
  RCar    *car;                         // The car to which we belong

  // Physical attributes
  int    flags;
  rfloat gearRatio[MAX_GEAR];
  int    gears;
  rfloat gearInertia[MAX_GEAR];          // Rotational inertia per gear
  // Shifting (static)
  int    timeToDeclutch,                 // Auto-shifting time
         timeToClutch;
  rfloat shiftUpRPM,                     // Automatic transmissions
         shiftDownRPM;

  // State (dynamic output)
  int    curGear;                        // Selected gear
  int    autoShiftStart;                 // Time of auto shift initiation
  int    futureGear;                     // Gear to shift to

 public:
  RGearBox(RCar *car);
  ~RGearBox();

  void Reset();                          // Reset all vars

  // Definition
  bool Load(QInfo *info,cstring path=0);
  bool LoadState(QFile *f);
  bool SaveState(QFile *f);

  // Attribs
#ifdef OBS
  // Engine torque generation
  rfloat GetMinTorque(rfloat rpm);
  rfloat GetMaxTorque(rfloat rpm);
  rfloat GetTorqueAtDifferential();
  rfloat GetEngineInertia(){ return inertiaEngine; }
  rfloat GetGearInertia(){ return gearInertia[curGear]; }
  rfloat GetInertiaAtDifferential();
  rfloat GetInertiaForWheel(RWheel *w);
  rfloat GetTorqueForWheel(RWheel *w);
#endif

  int     GetGears(){ return gears; }
  int     GetGear(){ return curGear; }
  rfloat  GetGearRatio(int n);
  //rfloat GetEndRatio();
  void    SetGear(int n);
  cstring GetGearName(int gear);
  bool    IsNeutral(){ if(curGear==0)return TRUE; return FALSE; }
 
  // Input
  void SetInput(int ctlThrottle,int ctlClutch);
  
  // Graphics
  void Paint();

  // Physics
  void CalcForces();
  void Integrate();
  void OnGfxFrame();
};

#endif
