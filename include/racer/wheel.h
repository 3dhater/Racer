// rwheel.h

#ifndef __RWHEEL_H
#define __RWHEEL_H

#ifdef RR_GFX_OGL
#include <GL/glu.h>
#endif
#include <d3/types.h>
#include <d3/bbox.h>
#include <qlib/curve.h>
#include <racer/audio.h>
#include <racer/differential.h>
#include <racer/model.h>
#include <racer/track.h>
#include <racer/pacejka.h>
#include <racer/driveline.h>

class RCar;
class RSuspension;

class RWheel : public RDriveLineComp
{
  enum PropFlags
  {
    STEERING=1,                         // This wheel is used for steering
    POWERED=2,                          // Linked to engine (OBSOLETE)
    HANDBRAKES=4                        // Connected to the handbrakes
  };
  enum StateFlags
  {
    ON_SURFACE=1,                       // Wheel is touching surface?
    ATTACHED=2,                         // Attached to suspension?
    LOW_SPEED=4                         // Wheel is turning slow
  };
  enum Flags
  {
    DRAW_BBOX=1                 // Paint bounding box?
  };

 protected:
  // Semi-fixed properties
  RCar    *car;                         // The car to which we belong
  int      wheelIndex;                  // Number of this wheel
  int      flags;
  RSuspension *susp;                    // Link to its suspension
  RDifferential *differential;          // Attach to this differential
  int            differentialSide;      // Which output from the diff
  //RAudioProducer *rapSkid;              // Skidding sound
  rfloat   mass;                        // Mass of wheel+tyre
  rfloat   radius,
           width;
  rfloat   rollingCoeff;                // Rolling resistance coefficient
  rfloat   toe;                         // Pre-rotating the wheel
  rfloat   ackermanFactor;              // Scaling the steering angle

  // Skidding
  rfloat   skidLongStart,               // Slip ratio
           skidLongMax,
           skidLongStartNeg,
           skidLongMaxNeg,
           skidLatStart,                // Based on slip velocity (!)
           skidLatMax;
  int      skidStrip;                   // Graphical strip; -1=none

  // Spring values
  rfloat   tireRate;			// Spring vertical rate of tire
  // SAE 950311
  rfloat   dampingCoefficientLat,       // Amount of damping at low speed
           dampingCoefficientLong;
  rfloat   dampingSpeed;                // Speed at which to start damping
  // Braking
  rfloat   maxBrakingTorque;		// Max. force when braking 100%
  rfloat   brakingFactor;		// For example .58 for fronts, .42 back
  rfloat   lock;                        // Max angle of heading (radians)

  // Pre-calculated
  rfloat   distCGM;                     // Distance to center of geometry
  rfloat   angleCGM;                    // Angle (around car Y) of wheel
  rfloat   distCM;                      // Distance to center of mass
  DVector3 offsetWC;                    // Position offset wrt suspension
  float    frictionDefault;             // Friction to default surface
  int      propFlags;
  QCurve  *crvSlipTraction,             // Traction curve for slip ratio
          *crvSlipBraking;              // Braking curve
  QCurve  *crvLatForce;                 // Slip angle -> normalized lat. force
  QCurve  *crvSlip2FC;                  // Effect of slip vel on frict. circle
  rfloat   slip2FCVelFactor;            // Scaling of 'crvSlip2FC' X axis
  RPacejka pacejka;                     // Pacejka calculations
  rfloat   optimalSR,optimalSA;         // Optimal slip ratio/angle (in rad)
  rfloat   tanOptimalSA;                // tan(optimalSA)

  // State
  int      stateFlags;
  rfloat   lowSpeedFactor;              // How slow it is spinning
  rfloat   slipAngle;                   // Vel. vs. wheel dir in radians
  rfloat   slipRatio;                   // Ratio of wheel speed vs. velocity
  // SAE 950311 differential slipAngle/Ratio extra variables
  rfloat   tanSlipAngle;                // From this, 'slipAngle' is derived
  rfloat   relaxationLengthLat;         // 'b' in SAE 950311
  rfloat   lastU;                       // To detect switching of long. speed
  rfloat   signU;                       // Sign of longitudinal speed
  rfloat   differentialSlipRatio;       // SR calculated by differential eq.
  rfloat   relaxationLengthLong;        // 'B' in SAE 950311
  rfloat   dampingFactorLong;           // Damping at low speed
  rfloat   dampingFactorLat;            // Damping laterally
  // Combined slip
  rfloat   csSlipLen;                   // Combined slip vector length

  // Other state vars
  //rfloat   load;                        // Load on the wheel (N)
  rfloat   radiusLoaded;                // Rl = radius of loaded wheel to gnd
  rfloat   aligningTorque;              // Mz in the SAE system
  RSurfaceInfo surfaceInfo;		// Info on surface we're above

  // Dynamics
  DVector3 torqueTC;                    // 3-dimensions of torque
  DVector3 torqueBrakingTC;             // Total braking torque (incl. rr)
  DVector3 torqueRollingTC;             // Rolling resistance
  DVector3 torqueFeedbackTC;            // Feedback (road reaction)
  DVector3 forceRoadTC;                 // Because of slip ratio/angle
  DVector3 forceDampingTC;              // Damping if low-speed (SAE950311)
  DVector3 forceBrakingTC;		// Brakes
  DVector3 forceBodyCC;                 // Force on body (for acc.)
  DVector3 forceGravityCC;		// Gravity that pulls wheel down
  rfloat   velMagnitude;                // Length of slip velocity vector (obs)
  DVector3 velWheelCC,                  // Real velocity of wheel
           velWheelTC;                  // Velocity in tire coords
  DVector3 posContactWC;		// Position of contact patch in WC
  DVector3 slipVectorCC,                // Diff of tire vs wheel velocity
           slipVectorTC;                // In tire coords
  rfloat   slipVectorLength;            // Magnitude of slipVector?C

  // Translation
  DVector3 position;                    // Position in CC wrt the body (!)
  DVector3 velocity,                    // Velocity in wheel coords
           acceleration;                // For the suspension
  // Rotation
  DVector3 rotation;                    // Current rotation
  DVector3 rotationV,rotationA;         // Rotation speed and acceleration

  // Status
  DVector3 slipVector;                  // Diff. of theorial v vs ACTUAL v
  rfloat   frictionCoeff;               // Current friction coefficient
  rfloat   skidFactor;                  // How much skidding? 0..1

  // End forces
  DVector3 forceVerticalCC;             // Result of suspension+gravity
  
  // Graphics
  int slices;
  GLUquadricObj *quad;
  RModel        *model;
  DBoundingBox  *bbox;          // For debugging/creation
  RModel        *modelBrake;            // Static brake model (doesn't rotate)
  DMaterial     *matBrake;              // Material to show heat

 public:
  DVector3 devPoint;			// A point in world coordinates (dev)

 public:
  RWheel(RCar *car);
  ~RWheel();

  void Init();
  void Destroy();
  
  bool Load(QInfo *info,cstring path);
  bool LoadState(QFile *f);
  bool SaveState(QFile *f);

  void Reset();
  
  // Attribs
  void  DisableBoundingBox();
  void  EnableBoundingBox();
  bool  IsSteering(){ if(propFlags&STEERING)return TRUE; return FALSE; }
  bool  IsPowered(){ if(propFlags&POWERED)return TRUE; return FALSE; }
  bool  IsHandbraked(){ if(propFlags&HANDBRAKES)return TRUE; return FALSE; }
  bool  IsOnSurface(){ if(stateFlags&ON_SURFACE)return TRUE; return FALSE; }
  bool  IsLowSpeed(){ if(stateFlags&LOW_SPEED)return TRUE; return FALSE; }
  bool  IsAttached(){ if(stateFlags&ATTACHED)return TRUE; return FALSE; }
  void  EnableHandbrakes() { propFlags|=HANDBRAKES; }
  void  DisableHandbrakes(){ propFlags&=~HANDBRAKES; }
 
  void           SetSuspension(RSuspension *s){ susp=s; }
  RSuspension   *GetSuspension(){ return susp; }
  // Differential support
  void SetDifferential(RDifferential *diff,int side)
  { differential=diff; differentialSide=side; }
  RDifferential *GetDifferential(){ return differential; }
  int GetDifferentialSide(){ return differentialSide; }

  int    GetWheelIndex(){ return wheelIndex; }
  void   SetWheelIndex(int n);
  rfloat GetX(){ return position.GetX(); }
  rfloat GetY(){ return position.GetY(); }
  rfloat GetZ(){ return position.GetZ(); }
  rfloat GetMass(){ return mass; }
  rfloat GetLock(){ return lock; }
  rfloat GetSlipAngle(){ return slipAngle; }
  rfloat GetSlipRatio(){ return slipRatio; }
  rfloat GetHeading(){ return rotation.GetY(); }
  rfloat GetRotation(){ return rotation.GetX(); }
  rfloat GetRotationV(){ return rotationV.GetX(); }
  // A bit of an awkard way to get to the rotation vector itself
  DVector3 *GetRotationVector(){ return &rotation; }
  rfloat GetRadius(){ return radius; }
  rfloat GetAligningTorque(){ return aligningTorque; }
  rfloat GetSkidFactor(){ return skidFactor; }

  RModel   *GetModel(){ return model; }
  //DVector3 *GetRotationalInertia(){ return &inertia; }
  DVector3 *GetPosition(){ return &position; }
  DVector3 *GetVelocity(){ return &velocity; }
  DVector3 *GetAcceleration(){ return &acceleration; }
  DVector3 *GetSlipVectorCC(){ return &slipVectorCC; }
  DVector3 *GetPosContactWC(){ return &posContactWC; }
  // Forces
  DVector3 *GetForceRoadTC(){ return &forceRoadTC; }
  DVector3 *GetForceBodyCC(){ return &forceBodyCC; }
  DVector3 *GetTorqueTC(){ return &torqueTC; }
  // Feedback torque is the road reaction torque
  DVector3 *GetTorqueFeedbackTC(){ return &torqueFeedbackTC; }
  DVector3 *GetTorqueBrakingTC(){ return &torqueBrakingTC; }
  DVector3 *GetTorqueRollingTC(){ return &torqueRollingTC; }

  RSurfaceInfo *GetSurfaceInfo(){ return &surfaceInfo; }
  rfloat GetMassAbove();
  rfloat GetForceNormal();
  
  void SetHeading(float heading);
  
  void Paint();
  
  // Physics
  void CalcSlipRatio(DVector3 *velWheelCC);
  void CalcSlipAngle();
  void CalcSlipVelocity();
  void CalcLoad();
  void CalcPacejka();
  void CalcDamping();
  void CalcWheelPosWC();
  void PreAnimate();
  
  void CalcSlipVector();
  void CalcFrictionCoefficient();
  void CalcTorqueForces(rfloat torque);
  void CalcSuspForces();
  void CalcSlipForces();
  void CapFrictionCircle();
  void CalcForces();
  // Applying
  void CalcWheelAngAcc();
  void CalcBodyForce();
  void CalcAccelerations();
  
  void AdjustCoupling();
  
  void Integrate();
  void OnGfxFrame();
  void ProcessFX();
  
  // Coordinate systems
  void ConvertCarToTireOrientation(DVector3 *from,DVector3 *to);
  void ConvertCarToTireCoords(DVector3 *from,DVector3 *to);
  void ConvertTireToCarOrientation(DVector3 *from,DVector3 *to);
  void ConvertTireToCarCoords(DVector3 *from,DVector3 *to);
};

#endif
