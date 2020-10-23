/*
 * RScene - a scene contains the track and vehicles
 * 05-10-00: Created!
 * BUGS:
 * - Doesn't keep a bound for the #laps (lapTime[])
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/global.h>
DEBUG_ENABLE

// Local module trace
//#define LTRACE

// Paint wheel 3D world positions? (visual check)
//#define DO_DEBUG_POINTS

#undef  DBG_CLASS
#define DBG_CLASS "RScene"

RScene::RScene()
{
  DBG_C("ctor")

  int i;

  flags=0;
  env=0;
  track=0;
  cars=0;
  camMode=CAM_MODE_CAR;
  curCam=0;
  curCamGroup=0;
  camCar=0;
  drivers=0;
  fxInterval=RMGR->GetGfxInfo()->GetInt("fx.interval",100);
  replayInterval=RMGR->GetMainInfo()->GetInt("replay.interval",100);
  // Blurring; keep blurAlpha ready in case of replays with blur effects
  if(RMGR->GetGfxInfo()->GetInt("fx.blur_enable"))
    flags|=CLEAR_BLUR;
  blurAlpha=RMGR->GetGfxInfo()->GetInt("fx.blur_alpha");

  for(i=0;i<MAX_DRIVER;i++)
    driver[i]=0;
  for(i=0;i<MAX_CAR;i++)
    car[i]=0;
  for(i=0;i<CAM_MODE_MAX;i++)
  {
    lastCam[i]=0;
  }
  Reset();
}

RScene::~RScene()
{
  DBG_C("dtor")

  Reset();
 
  // Track is deleted by RMGR, don't delete here
  QDELETE(env);
  RemoveCars();
}

/********
* Reset *
********/
void RScene::Reset()
// Prepare for filling information into this scene
{
  int i;
  
  RemoveCars();
  ResetTime();
  
  // Delete the drivers
  for(i=0;i<drivers;i++)
    QDELETE(driver[i]);
  drivers=0;

  // Reset others
  track=0;
  curCam=0;
  camMode=CAM_MODE_CAR;
  camCar=0;

  // Reset network
  if(RMGR->network)
    RMGR->network->ResetClients();

  // Reset graphics; clear all cached textures
  // because cars have been deleted
  dglobal.texturePool->Clear();
}
void RScene::ResetTime()
// Reset timing statistics
{
  int i,j;

  for(i=0;i<MAX_CAR;i++)
  {
    curTimeLine[i]=0;
    bestLapTime[i]=0;
    for(j=0;j<MAX_LAP;j++)
    {
      lapTime[i][j]=0;
    }
    for(j=0;j<RTrackVRML::MAX_TIMELINE;j++)
    {
      tlTime[i][j]=0;
    }
    // No lap started yet
    curLap[i]=-1;
  }

  // FX timing
  timeNextFX=0;
  timeNextReplayFrame=0;

  // Chair timing
  RMGR->chair->nextUpdateTime=0;
}

/*******
* Dump *
*******/
bool RScene::LoadState(QFile *f)
{
  int i;
  f->Read(&cars,sizeof(cars));
  f->Read(&curTimeLine,sizeof(curTimeLine));
  f->Read(&lapTime,sizeof(lapTime));
  f->Read(&bestLapTime,sizeof(bestLapTime));
  f->Read(&tlTime,sizeof(tlTime));
  f->Read(&curLap,sizeof(curLap));
  f->Read(&camMode,sizeof(camMode));
  f->Read(&curCam,sizeof(curCam));
  f->Read(&curCamGroup,sizeof(curCamGroup));
  f->Read(lastCam,sizeof(lastCam));
  for(i=0;i<cars;i++)
  {
    if(!car[i]->LoadState(f))return FALSE;
  }
  return TRUE;
}
bool RScene::SaveState(QFile *f)
{
  int i;
  f->Write(&cars,sizeof(cars));
  f->Write(&curTimeLine,sizeof(curTimeLine));
  f->Write(&lapTime,sizeof(lapTime));
  f->Write(&bestLapTime,sizeof(bestLapTime));
  f->Write(&tlTime,sizeof(tlTime));
  f->Write(&curLap,sizeof(curLap));
  f->Write(&camMode,sizeof(camMode));
  f->Write(&curCam,sizeof(curCam));
  f->Write(&curCamGroup,sizeof(curCamGroup));
  f->Write(lastCam,sizeof(lastCam));
  for(i=0;i<cars;i++)
  {
    if(!car[i]->SaveState(f))return FALSE;
  }
  return TRUE;
}

/*******
* CARS *
*******/
void RScene::RemoveCars()
{
  int i;

//qdbg("RScene:RemoveCars(); %d cars\n",cars);
  for(i=0;i<cars;i++)
    QDELETE(car[i]);
  cars=0;
}
bool RScene::AddCar(RCar *newCar)
{
  DBG_C("AddCar")
  DBG_ARG_P(newCar)
 
  if(cars==MAX_CAR)
  { qwarn("Too many cars in scene");
    return FALSE;
  }
  car[cars]=newCar;
  // Make sure the car knows its place in the scene
  car[cars]->SetIndex(cars);
  // Get approximate car bounding sphere size, based on body
  DBox box;
//qdbg("newCar: model=%p\n",newCar->GetBody()->GetModel());
//qdbg("newCar: geode=%p\n",newCar->GetBody()->GetModel()->GetGeode());
  newCar->GetBody()->GetModel()->GetGeode()->GetBoundingBox(&box);
  carSphereRadius[cars]=box.GetRadius();
 
  if(cars==0)
  {
    // This is the only car; focus on it
    SetCam(0);
    SetCamCar(newCar);
  }

  // Another car
  cars++;
  return TRUE;
}

/********
* TRACK *
********/
void RScene::SetTrack(RTrack *trk)
// Set a track (mostly a derived class of RTrack)
{
  DBG_C("SetTrack")
  DBG_ARG_P(trk)

  track=trk;
}

/**********
* Drivers *
**********/
void RScene::AddDriver(RDriver *newDriver)
// Add a new driver behind the racing client
// The driver structure is NOT copied, so keep it alive.
{
  if(drivers==MAX_DRIVER)
  {
    qwarn("RScene:AddDriver() max reached (%d)",MAX_DRIVER);
    return;
  }
  driver[drivers]=newDriver;
  drivers++;
}

/***********
* Painting *
***********/
void RScene::SetView(int x,int y,int wid,int hgt)
// Set scene viewport (in the current graphics context)
{
  DBG_C("SetView")
  DBG_ARG_I(x)
  DBG_ARG_I(y)
  DBG_ARG_I(wid)
  DBG_ARG_I(hgt)

#ifdef RR_GFX_OGL
  glViewport(x,y,wid,hgt);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(65.0,(GLfloat)wid/(GLfloat)hgt,1.0,1000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
#endif
}
void RScene::Set2D()
// Setup 2D painting. This is called in Paint()
// after painting the track & cars, and only 2D objects
// (like instruments, console and others) are expected.
{
  // Switch to 2D
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glOrtho(0,QSHELL->GetWidth(),0,QSHELL->GetHeight(),-1,1);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glColor4f(1,1,1,1);
  glDisable(GL_LIGHTING);
}

void RScene::SetDefaultState()
// Set some OpenGL state variable that we like to use as a default.
// Actually using this function should be avoided.
{
#ifdef RR_GFX_OGL
  glClearColor(.2,.3,.4,0);
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_FLAT);
#endif
}

void RScene::Clear()
// Do a default clear of the view
// Note this may be skipped if you know you're going to paint all over the viewport
{
  if(flags&CLEAR_BLUR)
  {
    // Use blur
    // Switch to 2D; be careful because the projection stack may be small (2)
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    //glOrtho(0,QSHELL->GetWidth(),0,QSHELL->GetHeight(),-1,1);
    glOrtho(0,1,0,1,-1,1);
    //glDisable(GL_TEXTURE_GEN_S);
    //glDisable(GL_TEXTURE_GEN_T);
    glColor4f(track->clearColor[0],track->clearColor[1],
      track->clearColor[2],blurAlpha); //,track->clearColor[3]);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE);
    glEnable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glBegin(GL_QUADS);
      glVertex2f(0,0);
      glVertex2f(1,0);
      glVertex2f(1,1);
      glVertex2f(0,1);
    glEnd();

    glClear(GL_DEPTH_BUFFER_BIT);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
  } else
  {
    // Full clear
    // Set the right clearing color
    glClearColor(track->clearColor[0],track->clearColor[1],
      track->clearColor[2],track->clearColor[3]);
    // Note that on Linux/Geforce2MX400, clearing color and depth buffer
    // makes almost no difference compared to just the depth buffer.
    //glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
  }
}
void RScene::Paint()
// Paint track & cars
{
  DBG_C("Paint")

  int i;
  bool visible[MAX_CAR];

#ifdef OBS
  {
    static int roll;
    glRotatef(roll,0,1,0);
    roll++;
  }
#endif

  // Paint the track
  glShadeModel(GL_FLAT);
  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  //car[0]->SetDefaultMaterial();
//qdbg("RScene:Paint(); track\n");

  // Fog handling
  if(RMGR->IsEnabled(RManager::ENABLE_FOG))
  {
#ifdef OBS
    //GLfloat density;
    GLfloat fogColor[4];
    
    // Use the same color for the fog as the track's clear color
   
    for(i=0;i<3;i++)
    {
      fogColor[i]=track->clearColor[i];
    }
#endif
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE,RMGR->fogMode);
    glFogfv(GL_FOG_COLOR,RMGR->fogColor);
    glFogf(GL_FOG_DENSITY,RMGR->fogDensity);
    glFogf(GL_FOG_START,RMGR->fogStart);
    glFogf(GL_FOG_END,RMGR->fogEnd);
    glHint(GL_FOG_HINT,RMGR->fogHint);

    // nVidia radial fog extension; put this in QGLContext
#ifdef GL_NV_fog_distance
    if(QCV->GetGLContext()->IsExtensionSupported("GL_NV_fog_distance"))
    {
//qdbg("Fog extension (NV_radial)\n");
      // Set fog distance mode; GL_EYE_RADIAL_NV, GL_EYE_PLANE or
      // GL_EYE_PLANE_ABSOLUTE_NV.
      glFogi(GL_FOG_DISTANCE_MODE_NV,GL_EYE_RADIAL_NV);
    }
#endif

//qdbg("Fog: start %f, end %f\n",RMGR->fogStart,RMGR->fogEnd);
//qdbg("Fog color %.2f,%.2f,%.2f\n",RMGR->fogColor[0],RMGR->fogColor[1],
//RMGR->fogColor[2]);
  }

  // No separate specular for the track
  QCV->GetGLContext()->SetSeparateSpecularColor(FALSE);

  if(track)
  {
    // Try to get colors a bit nicer; the future will just use a shader engine
    rfloat white[3]={ 1.0f,1.0f,1.0f };
    rfloat ambient[3]={ 0.2f,0.2f,0.2f };
    rfloat black[3]={ 0.0f,0.0f,0.0f };
    glColor3fv(white);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,white);
    glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
    glMaterialfv(GL_FRONT,GL_SPECULAR,white);
    glMaterialfv(GL_FRONT,GL_EMISSION,black);
    glMaterialf(GL_FRONT,GL_SHININESS,50.0f);

    track->Paint();
//track->PaintHidden();
  }

  // Paint skid marks
  RMGR->smm->Paint();

  // Deduce sun position (if any) while we're in 3D
  track->sun->CalcWindowPosition();

  // Specular fun
  QCV->GetGLContext()->SetSeparateSpecularColor(TRUE);

  // Debugging track paints
//qdbg("RScene:Paint(); spline tris?\n");
  if(RMGR->IsDevEnabled(RManager::SHOW_SPLINE_TRIS))
    RMGR->GetTrackVRML()->PaintHiddenSplineTris();
  
  // Paint the cars (lit)
//qdbg("RScene:Paint(); cars prep\n");
  glDisable(GL_LIGHTING);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
#ifdef ND_BATCH_DOESNOT_WORK_HERE
  dglobal.EnableBatchRendering();
#endif
  // Paint all car shadows first; note these are drawn OUTSIDE
  // of the batch.
//qdbg("Paint cars\n");
  for(i=0;i<cars;i++)
  {
    // Loan track's culler to decide whether car is inside the viewport
    if(RMGR->GetTrackVRML()->GetCuller()->SphereInFrustum(
         car[i]->GetPosition(),carSphereRadius[i])!=DCuller::OUTSIDE)
    {
//qdbg("Paint car %d (is visible)\n",i);
      visible[i]=TRUE;
      car[i]->PaintShadow();
    } else
    {
      visible[i]=FALSE;
    }
  }
//qdbg("RScene:Paint(); car models\n");
  // Next, render all body models
  // These are lit, smoothed
  glEnable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  for(i=0;i<cars;i++)
  {
    if(visible[i])
    {
      car[i]->Paint();
    }
  }
#ifdef ND_BATCH_DOESNOT_WORK_HERE
  dglobal.DisableBatchRendering();
//qdbg("RScene:Paint(); batch flush\n");
  // Actually push the graphics to the graphics pipeline
  glEnable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  dglobal.GetBatch()->Flush();
//qdbg("RScene:Paint(); RET\n");
#endif

  // Paint particles (all transparent, so last)
  RMGR->pm->Render();

  // Paint debug point in 3D
#ifdef DO_DEBUG_POINTS
  for(i=0;i<cars;i++)
  {
    // Paint dev debug test points in hard color
    glColor3f(0,1,0);
    glPointSize(2);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_POINTS);
    for(int j=0;j<car[i]->GetWheels();j++)
    {
      glVertex3f(car[i]->GetWheel(j)->devPoint.x,
                 car[i]->GetWheel(j)->devPoint.y,
                 car[i]->GetWheel(j)->devPoint.z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
  }
#endif

  // Only 2D things coming up after this
  Set2D();

  // Paint the sun in 2D
  track->sun->Paint();

  // Paint the 2D instruments (view 0 or 2)
  if(camCar)
  {
    // Only those instruments that aren't already displayed in 3D
    RCamera *cam=camCar->GetCamera(curCam);
    if(camMode==CAM_MODE_TRACK)
    {
      // Always view 0 (2D)
      camCar->GetViews()->GetView(0)->Paint2D();
    } else if(cam->IsInCar())
    {
      // Try view 2 for 2D dials that aren't visible in view 1
      if(camCar->GetViews()->GetView(2))
        camCar->GetViews()->GetView(2)->Paint2D();
    } else
    {
      // Always draw the default 2D dials
      camCar->GetViews()->GetView(0)->Paint2D();
    }
  }
}

/********************
* Timing statistics *
********************/
void RScene::TimeLineCrossed(RCar *car)
// A car has crossed its current timeline (checkpoint).
// Adjust statistics for this car.
{
  int i,n=car->GetIndex();
  int lap;
  
  // Check for bad tracks
  if(!track->GetTimeLines())return;
  
  // Store intermediate time
  if(curTimeLine[n]==0)
  {
    // Start/finish!
    if(curLap[n]==-1)
    {
      // Just starting the first lap
      curLap[n]++;
      // Store lap start time in lap time variable
      lapTime[n][0]=RMGR->time->GetSimTime();
    } else
    {
      // Finished a lap
      lap=curLap[n];
      // The start time of this lap was stored in curLapTime[...]
      lapTime[n][lap]=RMGR->time->GetSimTime()-lapTime[n][lap];
      
      // Check for all-time best laps for driver
      driver[0]->GetStats()->CheckTime(lapTime[n][lap],car);
      // Check for all-time best laps for track
      track->GetStats()->CheckTime(lapTime[n][lap],car);
      
      // Check for current session best lap
      if(bestLapTime[n]==0||lapTime[n][lap]<bestLapTime[n])
      {
        // You've got a best lap!
        bestLapTime[n]=lapTime[n][lap];
      }
      // Store start time of next lap
      lapTime[n][lap+1]=RMGR->time->GetSimTime();
      // Reset intermediate times
      for(i=0;i<RTrack::MAX_TIMELINE;i++)
      {
        tlTime[n][i]=0;
      }
      curLap[n]++;
    }
  }
  
  // Store intermediate time
  if(curTimeLine[n]==0)
  {
    // First line crossed (start/finish)
    tlTime[n][curTimeLine[n]]=RMGR->time->GetSimTime()-lapTime[n][curLap[n]];
  } else
  {
    // Somewhere on the track
    tlTime[n][curTimeLine[n]]=
      RMGR->time->GetSimTime()-tlTime[n][curTimeLine[n]-1];
  }

  // Switch to next checkpoint timeline
  curTimeLine[n]=(curTimeLine[n]+1)%track->GetTimeLines();
}
int RScene::GetCurrentLapTime(RCar *car)
// Returns live current lap time for the car
// This is updated as the car is driving the lap
{
  int n=car->GetIndex();
  
  // Doing a lap already?
  if(curLap[n]==-1)return 0;
  // Calculate the time the car is busy in the current lap
  return RMGR->time->GetSimTime()-lapTime[n][curLap[n]];
}
int RScene::GetBestLapTime(RCar *car)
// Returns best lap time for the car, or 0 for none.
{
  return bestLapTime[car->GetIndex()];
}

/**********
* Cameras *
**********/
void RScene::DetectCamera()
// Check whether the current camera is still ok
{
  int i,n,newCam;
  rfloat d;
  //DVector3 *carPos;

  if(camMode==CAM_MODE_CAR)
  {
    // Camera is selected by user; no need to detect
    return;
  }
  n=track->GetTrackCamCount();
  if(!camCar)
  {
    // Make sure a car has the focus
    if(!cars)return;
    SetCamCar(car[0]);
  }
  //carPos=camCar->GetPosition();

  // Try next cam
  if(curCam<n-1)newCam=curCam+1;
  else          newCam=0;
#ifdef OBS
qdbg("RScene:DetectCam: curCam=%d, newCam=%d\n",curCam,newCam);
d=track->GetTrackCam(curCam)->pos.DistanceTo(carPos);
qdbg("Cur cam dist=%f (r=%f)\n",d,track->GetTrackCam(curCam)->radius);
d=track->GetTrackCam(newCam)->pos.DistanceTo(carPos);
qdbg("  Next cam dist=%f\n",d);
#endif
  track->GetTrackCam(newCam)->SetCar(camCar);
  if(track->GetTrackCam(newCam)->IsInRange())
  {
    // Switch to next cam
//qdbg("next cam!\n");
    SetCam(newCam);
    return;
  }
  // Current camera still valid?
  if(track->GetTrackCam(curCam)->IsInRange())
  {
    // Keep the cam (avoid flipflopping between cams)
    return;
  }
  // Try previous cam (perhaps the driver is driving in the wrong direction)
  if(curCam>0)newCam=curCam-1;
  else        newCam=n-1;
  track->GetTrackCam(newCam)->SetCar(camCar);
  if(track->GetTrackCam(newCam)->IsInRange())
  {
    // Switch to previous cam
//qdbg("prev cam!\n");
    SetCam(newCam);
    return;
  }
  // Check whether current camera is still quite valid
  if(track->GetTrackCam(curCam)->IsFarAway())
  {
//qdbg("Car is far away!\n");
    // Find the closest camera to stick a bit to the car
    rfloat min=99999;
    int    closestCam=0;
    for(i=0;i<n;i++)
    {
      d=track->GetTrackCam(i)->pos.SquaredDistanceTo(camCar->GetPosition());
      if(d<min)
      {
        min=d;
        closestCam=i;
      }
    }
    SetCam(closestCam);
  }
  // else no change in camera
//qdbg("curCam=%d\n",curCam);
}

void RScene::GoCamera()
// Move the graphics observation point to the current camera
{
#ifdef OBS
qdbg("RScene:GoCamera()\n");
qdbg("  camCar=%p, camMode=%d\n",camCar,camMode);
#endif
  if(!camCar)
  {
    // Default view is track camera 0, to its first preview cube
    // that you set in TrackEd, to get SOME view
    // (no car may be on the track)
    track->GetTrackCam(0)->Go();
    return;
  }

  switch(camMode)
  {
    case CAM_MODE_CAR  : camCar->GetCamera(curCam)->Go(); break;
    case CAM_MODE_TRACK: track->GetTrackCam(curCam)->Go(); break;
  }
}
void RScene::SetCamMode(int mode)
{
  // Store the current camera for later
  lastCam[camMode]=curCam;
  camMode=mode;
  // Retrieve last selected camera
  SetCam(lastCam[camMode]);
}
void RScene::SetNextCamMode()
// Loops through the possible camera modes
{
  SetCamMode((camMode+1)%CAM_MODE_MAX);
}
void RScene::SetCam(int cam)
{
  curCam=cam;
}
void RScene::SetCamGroup(int group)
{
  curCamGroup=group;
}
void RScene::SetCamCar(RCar *car)
// Sets the car to focus on
{
  int i,n;

//qdbg("RSc:SCC: camCar=%p\n",camCar);
  camCar=car;
  n=track->GetTrackCamCount();
//qdbg("  n=%d\n",n);
  for(i=0;i<n;i++)
  {
    track->GetTrackCam(i)->SetCar(camCar);
  }
}

rfloat RScene::GetCamAngle()
// Returns FOV angle for current camera
{
  if(camMode==CAM_MODE_CAR)
  {
    // Currently fixed
    if(camCar)
      return camCar->GetCamera(curCam)->GetLensAngle();
  } else if(camMode==CAM_MODE_TRACK)
  {
    return track->GetTrackCam(curCam)->GetZoom();
  }
  // Default for the rest (no car on the track yet)
  //qerr("RScene:GetCamAngle(); no car or track mode");
  return 50.0f;
}
void RScene::GetCameraOrigin(DVector3 *vec)
// Returns the origin of the current viewing source
{
  if(camMode==CAM_MODE_CAR)
  {
    if(!camCar)return;
//qdbg("RSc:GCO: camCar=%p\n",camCar);
    RCamera *cam=camCar->GetCamera(curCam);
//qdbg("RSc:GCO: cam=%p\n",cam);
    if(cam)cam->GetOrigin(vec);
  } else if(camMode==CAM_MODE_TRACK)
  {
    // Use last calculate position (which may still
    // be (0,0,0) before the first integration)
    //track->GetTrackCam(curCam)->GetOrigin(vec);
    *vec=*track->GetTrackCam(curCam)->GetCurPos();
  } else
  { vec->SetToZero();
  }
}

/*********
* Racing *
*********/
void RScene::OnGfxFrame()
// Do things for the next graphics frame
{
  int i;
  
  for(i=0;i<cars;i++)
  {
    car[i]->OnGfxFrame();
  }
  if(camMode==CAM_MODE_TRACK)
  {
    DetectCamera();
    // Calculate zoom, distance etc.
    track->GetTrackCam(curCam)->CalcState();
  }

  // 3D sound
#ifdef Q_USE_FMOD
  DVector3 org;
  static float v0[3]={ 0,0,0 };

  GetCameraOrigin(&org);
  //FSOUND_3D_Listener_SetDopplerFactor(10);
  FSOUND_3D_Listener_SetRolloffFactor(
    RMGR->GetMainInfo()->GetFloat("audio.rolloff_factor"));
  FSOUND_3D_Listener_SetAttributes(&org.x,v0, 0,0,1, 0,1,0);
  FSOUND_3D_Update();
#endif

}
void RScene::Update()
// A moment of pause is underway; update any info you want
// stored on disk.
{
  int i;
  
  // Update any statistics
  if(track)
    track->Update();
  for(i=0;i<drivers;i++)
    driver[i]->Update();
}

/**********
* Animate *
**********/
void RScene::Animate()
// Animate all objects in the scene
{
  int i;
  for(i=0;i<cars;i++)
  {
    car[i]->Animate();
  }
#ifdef RR_ODE
  // Perform an integration step on all bodies.
  // After this, make sure the rigid bodies from Racer are synchronised.
  dWorldStep(RMGR->odeWorld,RR_TIMESTEP);
  // Clear all contact point generated during collision detection.
  dJointGroupEmpty(RMGR->odeJointGroup);
#endif

  // Post integration (synchronise); mostly for ODE development (conversion)
  for(i=0;i<cars;i++)
    car[i]->PostIntegrate();
}

/************
* Resetting *
************/
void RScene::ResetRace()
// Do a reset on all cars
{
  int i;

#ifdef LTRACE
qdbg("RScene:ResetRace()\n");
#endif

  // Put cars back on grid positions
  for(i=0;i<cars;i++)
  {
    car[i]->Warp(RCar::WARP_GRID,i);
#ifdef LTRACE
car[i]->GetEngine()->DbgPrint("RScene:ResetRace");
#endif

  }
  // Kill all laptimes and restart other timing
  ResetTime();
}

/*****
* FX *
*****/
void RScene::ProcessFX()
// Examine the FX
{
  int i;

  if(RMGR->time->GetSimTime()<timeNextFX)
  {
    // Don't overdo the FX
    return;
  }

  // Check the FX
  timeNextFX+=fxInterval;

  // Put cars back on grid positions
  for(i=0;i<cars;i++)
  {
    car[i]->ProcessFX();
  }
}

/**********
* Replays *
**********/
void RScene::ProcessReplay()
// Examine the replay manager
{
  int i;

  // Is it time yet to store the situation for the replay?
  if(RMGR->time->GetSimTime()<timeNextReplayFrame)
    return;

  // Calculate next time
  timeNextReplayFrame+=replayInterval;

  // Tell cars to store a replay frame
  for(i=0;i<cars;i++)
  {
    car[i]->ProcessReplay();
  }

  // Store this replay frame
  RMGR->replay->EndFrame();

  // Store time for the future replay frame for statistics
  RMGR->replay->BeginFrame(timeNextReplayFrame);
}

