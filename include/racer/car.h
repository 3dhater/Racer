// racer/car.h

#ifndef __RACER_CAR_H
#define __RACER_CAR_H

#include <qlib/info.h>
#include <qlib/vector.h>
#include <qlib/archive.h>
#include <racer/wheel.h>
#include <racer/susp.h>
#include <racer/wing.h>
#include <racer/differential.h>
#include <racer/steer.h>
#include <racer/engine.h>
#include <racer/gearbox.h>
#include <racer/body.h>
#include <racer/audio.h>
#include <racer/camera.h>
#include <racer/view.h>
#include <racer/network.h>
#include <racer/filter.h>
#include <racer/driveline.h>

class RCar : public QObject
// A car consisting of parts
// May need a RBody to represent the car's body instead of directly here
{
 public:
  enum Max
  {
    MAX_WHEEL=8,
    MAX_DIFFERENTIAL=3,                 // Enough for a 4WD Jeep
    MAX_WING=10,                        // Front, rear and perhaps others
    MAX_CAMERA=10			// Max #camera's around car
  };
  enum Flags
  {
    AI=1,                               // Car is driven by robot
    NETWORK=2                           // Car is a network car; no physics!
  };
  // Where to warp (see Warp())
  enum WarpDestination
  {
    WARP_GRID,
    WARP_PIT
  };

 protected:
  // Index in the scene
  int index;
  // Objects that make the car a car
  RWheel      *wheel[MAX_WHEEL];
  int          wheels;
  RSuspension *susp[MAX_WHEEL];
  int          wings;
  RWing       *wing[MAX_WING];
  // Steering wheel, etc
  RSteer  *steer;
  REngine *engine;
  RGearBox *gearbox;
  RDifferential  *differential[MAX_DIFFERENTIAL];
  int             differentials;
  RBody          *body;
  RDriveLine     *driveLine;
  RAudioProducer *rapEngine,
                 *rapGearShift,
                 *rapGearMiss,
                 *rapStarter,
                 *rapSkid;
  int             flags;
  RRobot         *robot;                // AI driver

  // Cameras around car
  RCamera *camera[MAX_CAMERA];
  int      curCamera;
  // Views for use in the cameras
  RViews  *views;

  // State (dynamic)
  DVector3 cg;			// Center of gravity position

  // Input
  int   ctlThrottle,ctlBrakes;
  RFilter  *ffMz;                 // Filter aligning moment

  // Graphics
  DTexture *texShadow;            // Global shadow texture
  rfloat    shadowWid,shadowLen;  // Shadow size
  DVector3  shadowOffset;         // Offset wrt car position

  // Timing
  DVector3  timingPosBC,          // Point of car that determines timing
            timingPosPreWC,       // Position of car before timestep
            timingPosPostWC;      // After (check timeline crossing)

  // Collision detection
  DAABBNode *enclosingNode;       // Node deepest in the track's tree that
                                  // still contains the entire car
  rfloat aabbWid,aabbHgt,aabbDep; // Size of AABB approximation of car

  // Import
  QInfo *info,*infoDefault;
  qstring carDir;               // The directory path for the car files
  qstring carName;              // Real car name
  qstring carShortName;         // Car directory name (just the last part)
  QArchive *archive;            // Packed file

  // Network (multiplayer)
  int nextNetworkUpdate;        // When to send our generic state to the server
  // Time at which this car's state was last sent to client <x>.
  // This is recorded per client as cars NOT in the vicinity of a client
  // will be updated less, as determined by class RNetwork.
  int lastClientUpdate[RNetwork::MAX_CLIENT];

 public:
  RCar(cstring carName=0);
  ~RCar();
 
  bool LoadState(QFile *f);
  bool SaveState(QFile *f);

  void Reset();

  // Attribs
  QInfo       *GetInfo(){ return info; }
  cstring      GetDir(){ return carDir; }
  cstring      GetName(){ return carName; }
  cstring      GetShortName(){ return carShortName; }
  QArchive    *GetArchive(){ return archive; }

  RCamera       *GetCamera(int n){ return camera[n]; }
  RCamera       *GetCurrentCamera(){ return camera[curCamera]; }
  RViews        *GetViews(){ return views; }
  RSteer        *GetSteer(){ return steer; }
  REngine       *GetEngine(){ return engine; }
  RDriveLine    *GetDriveLine(){ return driveLine; }
  RGearBox      *GetGearBox(){ return gearbox; }
  RWheel        *GetWheel(int n){ return wheel[n]; }
  RSuspension   *GetSuspension(int n){ return susp[n]; }
  int            GetWheels(){ return wheels; }
  RWing         *GetWing(int n){ return wing[n]; }
  int            GetWings(){ return wings; }
  RDifferential *GetDifferential(int n){ return differential[n]; }
  int            GetDifferentials(){ return differentials; }
  void           AddDifferential(RDifferential *diff);
  RBody         *GetBody(){ return body; }
  RFilter       *GetMzFilter(){ return ffMz; }
  // Audio
  RAudioProducer *GetRAPEngine(){ return rapEngine; }
  RAudioProducer *GetRAPSkid(){ return rapSkid; }
  RAudioProducer *GetRAPGear(){ return rapGearShift; }
  RAudioProducer *GetRAPGearMiss(){ return rapGearMiss; }
  RAudioProducer *GetRAPStarter(){ return rapStarter; }

  // Controls
  int          GetThrottle(){ return ctlThrottle; }
  int          GetBrakes(){ return ctlBrakes; }

  // Attribs
  rfloat       GetMass();
  int          GetIndex(){ return index; }
  void         SetIndex(int n){ index=n; }
  DAABBNode   *GetEnclosingNode(){ return enclosingNode; }

  // AI
  bool         IsAI(){ if(flags&AI)return TRUE; return FALSE; }
  void         EnableAI();
  void         DisableAI();

  // Network support
  bool         IsNetworkCar(){ if(flags&NETWORK)return TRUE; return FALSE; }
  void         SetNetworkCar(bool yn);

  rfloat    GetVelocityTach();
  // Convenience function to get at car data, which actually
  // is data inside the body
#ifdef OBS_QUAT
  rfloat    GetHeading(){ return body->GetRotation()->y; }
#endif
  DVector3 *GetPosition(){ return body->GetPosition(); }
  DVector3 *GetVelocity(){ return body->GetVelocity(); }
#ifdef OBS_QUAT
  DVector3 *GetAcceleration(){ return body->GetAcceleration(); }
#endif
  rfloat    GetCGHeight(){ return cg.y; }

  void Warp(RCarPos *pos);
  void Warp(int where,int index);

  bool Load();
  void PreCalcDriveLine();

  // Paint
  void PaintShadow();
  void Paint();
  
  // Controls
  void SetInput(int ctlSteer,int ctlThrottle,int ctlBrakes);
  inline int  GetBraking(){ return ctlBrakes; }

  // Animation/physics
  void PreAnimate();
  void Animate();
  void PostIntegrate();
  void OnGfxFrame();
  void ApplySteeringToWheels();
  void ApplyAcceleration();
  void ApplyRotations();
  void DetectCollisions();
  void ProcessFX();
  void ProcessReplay();
  
  // Helpers
  void ConvertCarToWorldCoords(DVector3 *from,DVector3 *to);
  void ConvertWorldToCarCoords(DVector3 *from,DVector3 *to);
  void ConvertCarToWorldOrientation(DVector3 *from,DVector3 *to);
  void ConvertWorldToCarOrientation(DVector3 *from,DVector3 *to);
  void SetDefaultMaterial();
};

#endif
