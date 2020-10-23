// racer/manager.h

#ifndef __RACER_MANAGER_H
#define __RACER_MANAGER_H

#include <racer/scene.h>
#include <racer/audio.h>
#include <racer/gfx.h>
#include <racer/controls.h>
#include <racer/profile.h>
#include <racer/time.h>
#include <racer/trackf.h>
#include <racer/trackv.h>
#include <racer/replay.h>
#include <racer/network.h>
#include <racer/console.h>
#include <racer/skidmark.h>
#include <racer/chair.h>
#include <racer/music.h>
#include <qlib/info.h>
#include <d3/texfont.h>
#include <d3/particle.h>
#ifdef RR_ODE
#include <ode/ode.h>
#endif

class RManager
// The top class, managing the Racer universe
{
 public:
  enum Flags
  {
    // User settings
    NO_CONTROLS=1,		// Don't manage controls
    NO_AUDIO=2,			// Don't manage audio
    NO_TRACK_LIGHTING=4,        // Don't light track geodes
    NO_ENV_MAPPING=8,           // No environment mapping on models (perf.)
    NO_SKY=0x10,                // No sky drawing
    ENABLE_FOG=0x20,            // Fog (graphics)
    // Assist flags
    ASSIST_AUTOCLUTCH=0x40,     // Press clutch when RPM is low
    ASSIST_AUTOSHIFT=0x80       // Automatic
  };
  enum DevFlags
  {
    NO_SUSPENSION=0x1,		// Don't do suspension torques
    NO_GRAVITY_BODY=2,		// Don't pull body down
    SHOW_TIRE_FORCES=4,         // Paint tire forces
    APPLY_FRICTION_CIRCLE=8,    // Reduce forces?
    NO_CONTROLLER_UPDATES=16,   // Don't look at the controls
    NO_AI=32,                   // Forget the AI
    WHEEL_LOCK_ADJUST=0x40,     // Adjust force direction when wheel is locking
    USE_SLIP2FC=0x80,           // Reduce friction when slipping
    SHOW_AXIS_SYSTEM=0x100,     // Show some axes?
    FRAME_LINE=0x200,           // Line on debug output for every step?
    DRAW_EVERY_STEP=0x400,      // Draw every integration step? (slomo)
    SHOW_SPLINE_TRIS=0x800,     // Draw spline triangles?
    NO_TRACK_COLLISION=0x1000,  // Don't check track-car collisions?
    SHOW_CAR_BBOX=0x2000,       // Show car bounding boxes?
    CHECK_GL_ERRORS=0x4000,     // Check OpenGL errors every frame?
    NO_SPLINES=0x8000,          // Don't use spline surfaces (to compare)
    USE_SUSP_AS_LOAD=0x10000,   // Paul Harrington's idea; use suspension force
    NO_LATERAL_FORCES=0x20000,  // Zero out lateral tire forces
    NO_AABB_TREE=0x40000,       // Don't use the AABBTree
    XXX
  };
  enum FrictionCircleMethod
  {
    FC_GENTA,                   // Genta (page 52?)
    FC_VECTOR,                  // Vector reduction to fit circle
    FC_VOID,                    // Don't touch forces (for testing)
    FC_SLIPVECTOR               // Gregor's method of combined Pacejka
  };

  // Members that make up the environment
  RScene    *scene;
  RConsole  *console;
  RControls *controls;
  RAudio    *audio;
  RTime     *time;
  RProfile  *profile;
  RChair    *chair;

  // Replays
  DBitMapTexture *texReplay;    // Replay buttons
  RReplay   *replay;            // Generic replay for the whole scene
  RReplay   *replayGhost;       // Ghost car replay
  RReplay   *replayLast;        // Last lap replay

  // Multiplayer
  RNetwork  *network;

  // Track types
  RTrackFlat *trackFlat;
  RTrackVRML *trackVRML;
  RTrack     *track;
  RSkidMarkManager *smm;

  // Physics
#ifdef RR_ODE
  dWorldID odeWorld;            // The world (for rigid bodies)
  dSpaceID odeSpace;            // The space (for geometrical bodies)
  dJointGroupID odeJointGroup;  // For contact points
#endif

  // Default graphics
  DTexFont   *fontDefault;
  DBitMapTexture *texEnv;       // Default environment map

  // Other graphics
  DParticleManager *pm;
  DParticleGroup   *pgSmoke;
  DParticleGroup   *pgSpark;
  DBitMapTexture   *texSmoke;
  DBitMapTexture   *texSpark;

  // Audio
  RMusicManager *musicMgr;

  // Settings
  QInfo     *infoDebug;
  QInfo     *infoMain;
  QInfo     *infoGfx;
  //QInfo     *infoAudio;         // Audio settings

  // Preferences
  int    flags;                 // User settable flags
  int    resWidth,resHeight;    // Resolution (fast access)
  rfloat lowSpeed;              // What is considered low speed?
  rfloat dampSRreversal;        // Damping of longitudinal speed reversal
  int    maxSimTimePerFrame;    // To guard against slow machines
  rfloat maxForce,maxTorque;    // Avoid sudden extreme physics hits
  int    frictionCircleMethod;  // Type of friction circle applying
  rfloat ffDampingThreshold;    // When to stop damping Mz (squared!)
  // Graphics preferences
  rfloat visibility;            // Meters to look ahead
  rfloat fogDensity;            // Note flag ENABLE_FOG to enable fog
  rfloat fogStart,fogEnd;       // Fog start/end in meters
  int    fogHint;               // 0=dont_care, 1=fastest, 2=nicest
  int    fogMode;               // GL_EXP, GL_EXP2, GL_LINEAR...
  float  fogColor[4];           // RGBA fog color
  // Networking preferences
  int    timePerNetworkUpdate;  // #ms to wait for car update messages

  // Debugging
  int    devFlags;              // Development options
  int    slomoTime;             // #ms of pausing per frame

 public:
  RManager();
  ~RManager();

  // Precreation
  void EnableAudio();
  void DisableAudio();
  void EnableControls();
  void DisableControls();

  bool Create();
  void Destroy();

  // State saving (race save game for example, but mostly debugging)
  bool LoadState(cstring fName=0);
  bool SaveState(cstring fName=0);

  // Attribs
  QInfo *GetMainInfo(){ return infoMain; }
  QInfo *GetDebugInfo(){ return infoDebug; }
  QInfo *GetGfxInfo(){ return infoGfx; }
  RTrack *GetTrack(){ return track; }
  RTrackVRML *GetTrackVRML(){ return trackVRML; }
  RNetwork *GetNetwork(){ return network; }
  rfloat GetFFDampingThreshold(){ return ffDampingThreshold; }

  // Flags testing
  bool IsEnabled(int mask){ if(flags&mask)return TRUE; return FALSE; }
  bool IsDevEnabled(int mask){ if(devFlags&mask)return TRUE; return FALSE; }

  // Scene/track/car
  bool LoadTrack(cstring trackName,RTrackLoadCB cbFunc=0);

  // Graphics
  DBitMapTexture *GetEnvironmentTexture(){ return texEnv; }

  // Simulation
  void SimulateUntilNow();
  void ResetRace();
};

// 1 global Racer variable
extern RManager *__rmgr;

// Macro to get at the global variable
#define RMGR	(__rmgr)
#define RCON    (RMGR->console)

// Timestep macro; you may want to fix this value, increasing
// performance but limiting timestep playing by the user
//#define RR_FIXED_TIMESTEP
#ifdef RR_FIXED_TIMESTEP
#define RR_TIMESTEP       0.01f
#else
#define RR_TIMESTEP       (RMGR->time->GetSpan())
#endif

#endif
