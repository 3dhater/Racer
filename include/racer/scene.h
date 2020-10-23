// racer/scene.h

#ifndef __RACER_SCENE_H
#define __RACER_SCENE_H

#ifndef __RACER_CAR_H
#include <racer/car.h>
#endif
#ifndef __RACER_ENV_H
#include <racer/env.h>
#endif
#ifndef __RACER_TRACKV_H
#include <racer/trackv.h>
#endif
#include <d3/culler.h>

class RDriver;

class RScene
{
 public:
  enum Max
  {
    MAX_CAR=50,			// Enough for Nascar
    MAX_DRIVER=2,               // Drivers behind 1 client
    MAX_LAP=1000		// Enough for Le Mans 24hr?
  };
  enum CamModes
  {
    CAM_MODE_CAR=0,             // Car-relative views
    CAM_MODE_TRACK=1,           // Track view
    CAM_MODE_MAX
  };
  enum Flags
  {
    CLEAR_BLUR=1                // Don't clear fully; sort of motion blur
  };

 public:
  REnvironment *env;
  RCar         *car[MAX_CAR];
  int           cars;
  rfloat        carSphereRadius[MAX_CAR];    // Bounding sphere size
  RTrack       *track;
  RDriver      *driver[MAX_DRIVER];
  int           drivers;

  int  flags;

  // FX
  int  blurAlpha;

  // Timing
  int  curTimeLine[MAX_CAR];
  int  lapTime[MAX_CAR][MAX_LAP];
  int  bestLapTime[MAX_CAR];
  int  tlTime[MAX_CAR][RTrack::MAX_TIMELINE];
  int  curLap[MAX_CAR];		// -1 = before 1st lap

  // Timed events
  int  timeNextFX;
  int  fxInterval;              // #ms between each FX call
  int  timeNextReplayFrame;
  int  replayInterval;

  // Camera
  int   camMode;                // CAM_MODE_xxx
  int   curCam;                 // Car or track camera index
  int   curCamGroup;            // Track camera group
  int   lastCam[CAM_MODE_MAX];  // For switching modes
  RCar *camCar;                 // Car in focus

 public:
  RScene();
  ~RScene();

  bool LoadState(QFile *f);
  bool SaveState(QFile *f);

  void Reset();
  void ResetTime();

  // Cars
  void  RemoveCars();
  bool  AddCar(RCar *car);
  int   GetCars(){ return cars; }
  RCar *GetCar(int n){ return car[n]; }

  // Track
  void SetTrack(RTrack *trk);

  // Drivers
  int   GetDrivers(){ return drivers; }
  RDriver *GetDriver(int n){ return driver[n]; }
  void     AddDriver(RDriver *driver);

  // Painting
  void SetView(int x,int y,int wid,int hgt);
  void SetDefaultState();
  void Clear();
  void Set2D();
  void Paint();

  // Timing
  int  GetCurrentLapTime(RCar *car);
  int  GetBestLapTime(RCar *car);
  void TimeLineCrossed(RCar *car);
  void ProcessFX();
  void ProcessReplay();

  // Cameras
  void     DetectCamera();
  void     GoCamera();
  int      GetCamMode(){ return camMode; }
  int      GetCamGroup(){ return curCamGroup; }
  int      GetCam(){ return curCam; }
  RCamera *GetCamCar(){ if(camCar)return camCar->GetCamera(curCam); return 0; }
  rfloat   GetCamAngle();
  void     GetCameraOrigin(DVector3 *vec);
  void SetCamMode(int camMode);
  void SetNextCamMode();
  void SetCam(int cam);
  void SetCamGroup(int cam);
  void SetCamCar(RCar *car);

  // During racing
  void Animate();
  void OnGfxFrame();
  void Update();

  void ResetRace();
};

#endif
