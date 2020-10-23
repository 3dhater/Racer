/*
 * RManager
 * 05-10-00: Created!
 * 17-11-01: Introduction of ODE for the rigid bodies.
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/app.h>
#include <d3/global.h>
DEBUG_ENABLE

// Option to keep retiming after initial catch-up. This means that
// we look at the current time, then calculate the physics up to that point;
// if OPT_KEEP_RETIMING is defined, the program will then see how far
// the current time has advanced, and keep on calculating until the program
// catches up with real time.
// Turning it off may improve framerate a little, but reduce responsiveness
// just a little too. Default behavior for up to v0.4.8 was OPT_KEEP_RETIMING.
//#define OPT_KEEP_RETIMING

#define DEBUG_FILE "debug.ini"		// Debug settings
#define MAIN_FILE  "racer.ini"          // Global flags
#define GFX_FILE   "gfx.ini"            // Graphics settings
//#define AUDIO_FILE "audio.ini"          // Audio settings (obsolete since v0.5)

// Default dump filename
#define DEFAULT_DUMP_NAME "racer.sav"

// Racer's one and only global variable, accessable through RMGR
RManager *__rmgr;

RManager::RManager()
{
  if(__rmgr)
  { qerr("RManager created more than once");
    return;
  }
  __rmgr=this;

  flags=0;
  devFlags=0;
  scene=0;
  trackFlat=0;
  trackVRML=0;
  network=0;
  controls=0;
  console=0;
  profile=0;
  audio=0;
  time=0;
  fontDefault=0;
  texEnv=0;
  frictionCircleMethod=0;
  smm=0;
  pm=0;
  texSmoke=0;
  texSpark=0;
  pgSmoke=pgSpark=0;
  chair=0;
  fogDensity=1;
  fogStart=0; fogEnd=1;
  fogHint=GL_DONT_CARE;
  fogMode=GL_EXP;
  infoDebug=0;
  replay=replayGhost=replayLast=0;
  texReplay=0;
#ifdef RR_ODE
  odeWorld=0;
  odeSpace=0;
  odeJointGroup=0;
#endif
  infoMain=new QInfo(RFindFile(MAIN_FILE));
  infoGfx=new QInfo(RFindFile(GFX_FILE));
  
  // Load preferences
  lowSpeed=infoMain->GetFloat("car.low_speed");
  dampSRreversal=infoMain->GetFloat("car.damp_sr_reversal");
  resWidth=infoGfx->GetInt("resolution.width");
  resHeight=infoGfx->GetInt("resolution.height");
  maxSimTimePerFrame=infoMain->GetInt("timing.max_sim_time_per_frame");
  timePerNetworkUpdate=infoMain->GetInt("multiplayer.time_per_update",1000);
  ffDampingThreshold=infoMain->GetFloat("ini.ff_damping_threshold");
  // The damping is squared here to avoid square roots while racing.
  ffDampingThreshold*=ffDampingThreshold;
//qdbg("RMGR: main file='%s'\n",RFindFile(MAIN_FILE));
}
RManager::~RManager()
{
  Destroy();
}

void RManager::Destroy()
{
  // Delete network first to avoid problems in multithreading
  QDELETE(network);
  QDELETE(scene);
  QDELETE(controls);
  QDELETE(audio);
  QDELETE(time);
  QDELETE(trackFlat);
  QDELETE(trackVRML);
  QDELETE(profile);
  QDELETE(fontDefault);
  QDELETE(texEnv);
  QDELETE(infoDebug);
  QDELETE(infoMain);
  QDELETE(infoGfx);
  QDELETE(musicMgr);
  QDELETE(console);
  QDELETE(smm);
  QDELETE(pm);
  QDELETE(texSmoke);
  QDELETE(texSpark);
  QDELETE(pgSmoke);
  QDELETE(pgSpark);
  QDELETE(chair);
  QDELETE(replay);
  QDELETE(replayGhost);
  QDELETE(replayLast);
  QDELETE(texReplay);
#ifdef RR_ODE
  if(odeWorld)dWorldDestroy(odeWorld);
  if(odeSpace)dSpaceDestroy(odeSpace);
  if(odeJointGroup)dJointGroupDestroy(odeJointGroup);
#endif
}

/***********
* Creation *
***********/
void RManager::EnableAudio(){ flags&=~NO_AUDIO; }
void RManager::DisableAudio(){ flags|=NO_AUDIO; }
void RManager::EnableControls(){ flags&=~NO_CONTROLS; }
void RManager::DisableControls(){ flags|=NO_CONTROLS; }

bool RManager::Create()
// Create the managed items, so you can start using the Racer universe
// Assumes a window is already up.
{
  char buf[256];
  qstring s;

  console=new RConsole();
  console->printf("Console activated.");

  // Info files
  infoDebug=new QInfo(RFindFile(DEBUG_FILE));

  time=new RTime();
  
  // Normal flags (user may set these to influence things)
  if(!infoGfx->GetInt("fx.track_lighting",1))
    flags|=NO_TRACK_LIGHTING;
  if(!infoGfx->GetInt("envmap.enable"))
    flags|=NO_ENV_MAPPING;
  if(!infoGfx->GetInt("fx.sky_enable",1))
    flags|=NO_SKY;
  // Fog
  if(infoGfx->GetInt("fx.fog",1))
    flags|=ENABLE_FOG;
  visibility=infoGfx->GetFloat("fx.visibility");
  switch(infoGfx->GetInt("fx.fog_hint"))
  {
    case 0: fogHint=GL_DONT_CARE; break;
    case 1: fogHint=GL_FASTEST; break;
    case 2: fogHint=GL_NICEST; break;
    default: fogHint=GL_DONT_CARE;
      qwarn("gfx.ini's fog_hint out of range 0..2"); break;
  }

  // Development options (not normally changed by regular users)
  if(infoDebug->GetInt("stats.show_tire_forces"))
    devFlags|=SHOW_TIRE_FORCES;
  if(infoDebug->GetInt("stats.show_axis_system"))
    devFlags|=SHOW_AXIS_SYSTEM;
  if(infoDebug->GetInt("stats.frame_line"))
    devFlags|=FRAME_LINE;
  if(infoDebug->GetInt("gfx.show_spline_tris"))
    devFlags|=SHOW_SPLINE_TRIS;
  if(infoDebug->GetInt("gfx.show_car_bbox"))
    devFlags|=SHOW_CAR_BBOX;
  if(infoDebug->GetInt("gfx.check_opengl_errors"))
    devFlags|=CHECK_GL_ERRORS;
  if(infoDebug->GetInt("car.no_splines"))
    devFlags|=NO_SPLINES;
  if(infoDebug->GetInt("car.use_susp_as_load"))
    devFlags|=USE_SUSP_AS_LOAD;
  if(!infoDebug->GetInt("car.suspension",1))
    devFlags|=NO_SUSPENSION;
  if(!infoDebug->GetInt("car.gravity_body",1))
    devFlags|=NO_GRAVITY_BODY;
  if(!infoDebug->GetInt("car.lateral_tire_forces",1))
    devFlags|=NO_LATERAL_FORCES;
  if(!infoDebug->GetInt("car.use_aabb_tree",1))
    devFlags|=NO_AABB_TREE;

  // Timing
  if(infoDebug->GetInt("timing.draw_every_step"))
    devFlags|=DRAW_EVERY_STEP;
  slomoTime=infoDebug->GetInt("timing.slomo");
  time->SetSpan(infoDebug->GetInt("timing.integration_time"));

  if(!infoDebug->GetInt("controls.update",1))
    devFlags|=NO_CONTROLLER_UPDATES;
  if(!infoDebug->GetInt("ai.enable",1))
    devFlags|=NO_AI;

  // Car physics
  if(infoDebug->GetInt("car.apply_friction_circle"))
    devFlags|=APPLY_FRICTION_CIRCLE;
  if(infoDebug->GetInt("car.use_slip2fc"))
    devFlags|=USE_SLIP2FC;
  if(infoDebug->GetInt("car.wheel_lock_adjust"))
    devFlags|=WHEEL_LOCK_ADJUST;
  if(!infoDebug->GetInt("car.track_collision",1))
    devFlags|=NO_TRACK_COLLISION;
  maxForce=infoDebug->GetFloat("car.max_force");
  maxTorque=infoDebug->GetFloat("car.max_torque");
  frictionCircleMethod=infoDebug->GetInt("car.friction_circle_method");
 
  // Car assists
  if(infoMain->GetInt("assist.auto_clutch"))
    flags|=ASSIST_AUTOCLUTCH;
  if(infoMain->GetInt("assist.auto_shift"))
    flags|=ASSIST_AUTOSHIFT;

  // Physics
#ifdef RR_ODE
  odeWorld=dWorldCreate();
  // Create Hash or Simple space. As not much is tested inside ODE,
  // a simple space will do.
  odeSpace=dSimpleSpaceCreate();
  odeJointGroup=dJointGroupCreate(1000);
#endif

  // Graphics
  int   fMipMap,fTriLinear;
  float maxAnisotropy;
  fMipMap=infoGfx->GetInt("filter.mipmap");
  fTriLinear=infoGfx->GetInt("filter.trilinear");
  maxAnisotropy=infoGfx->GetFloat("filter.max_anisotropy",1);
  dglobal.prefs.maxFilter=GL_LINEAR;
  // Mipmapping/trilinear behavior change thanks to LaRoche, 17-10-01
  switch(fMipMap)
  {
    case 0:
      if(fTriLinear)dglobal.prefs.minFilter=GL_LINEAR;
      else          dglobal.prefs.minFilter=GL_NEAREST;
      break;
    case 1:
      // Simple mipmapping
      if(fTriLinear)dglobal.prefs.minFilter=GL_NEAREST_MIPMAP_LINEAR;
      else dglobal.prefs.minFilter=GL_NEAREST_MIPMAP_NEAREST;
      break;
    case 2:
      if(fTriLinear)dglobal.prefs.minFilter=GL_LINEAR_MIPMAP_LINEAR;
      else dglobal.prefs.minFilter=GL_LINEAR_MIPMAP_NEAREST;
      break;
  }
  // Set the max anisotropy and hope no-one will change
  // it during rendering.
  QCV->GetGLContext()->SetMaxAnisotropy(maxAnisotropy);

  // More graphics
  smm=new RSkidMarkManager();
  pm=new DParticleManager();

  // Add smoke particle group
  {
    QImage img("data/images/smoke.tga");
    QCV->Select();
    texSmoke=new DBitMapTexture(&img);
    pgSmoke=new DParticleGroup(DParticleGroup::SMOKE,75);
    pgSmoke->SetTexture(texSmoke);
    pm->AddGroup(pgSmoke);
  }

  // Network
  network=new RNetwork();
  if(network->IsEnabled())
    network->Create();
  // Exotic controllers
  chair=new RChair();

  // Replays
  replay=new RReplay();
  replay->Create();
  replayGhost=new RReplay();
  replayGhost->Create();
  // Auto-load ghost replay? (mainly just a debugging test)
  infoMain->GetString("replay.auto_load",s);
  if(!s.IsEmpty())replayGhost->Load(s);
  replayLast=new RReplay();
  replayLast->Create();
  // Load replay buttons (careful on the resources)
  {
    QImage img("data/images/repbuts.tga");
    texReplay=new DBitMapTexture(&img);
  }

  // Maintained objects
  scene=new RScene();
  if(!(flags&NO_CONTROLS))
    controls=new RControls();

  // Audio
  if(!(flags&NO_AUDIO))
  {
    audio=new RAudio();
    musicMgr=new RMusicManager();
    musicMgr->Create();
  }

  profile=new RProfile();

  // Create default graphics objects
  fontDefault=new DTexFont("data/fonts/euro16");
  if(IsEnabled(NO_ENV_MAPPING))
    goto skip_env_map;
  infoGfx->GetString("envmap.default_texture",buf);
  if(buf[0])
  { QImage *img;
//qdbg("RMGR:Create(); envmap '%s'\n",buf);
    img=new QImage(RFindFile(buf,"data/images"));
    if(!img->IsRead())
    { QDELETE(img);
     warn_no_img:
      qwarn("RMGR: Env mapping is enabled, but no valid image (%s) was given",
        buf);
    } else
    {
      // Create texture in rendering window
      QCV->Select();
      texEnv=new DBitMapTexture(img);
      // Use this texture as a default, if the user wants it
      dglobal.SetDefaultEnvironmentMap(texEnv);
      QDELETE(img);
    }
  } else if(!IsEnabled(NO_ENV_MAPPING))
  { goto warn_no_img;
  }
 skip_env_map:

  return TRUE;
}

/*******
* Dump *
*******/
bool RManager::LoadState(cstring fName)
{
  if(!fName)fName=DEFAULT_DUMP_NAME;

  // Open dump file
  QFile f(fName);
  if(!f.IsOpen())return FALSE;

  // Dump state
  if(!time->LoadState(&f))return FALSE;
  if(!scene->LoadState(&f))return FALSE;

  return TRUE;
}
bool RManager::SaveState(cstring fName)
{
  if(!fName)fName=DEFAULT_DUMP_NAME;

  // Open dump file
  QFile f(fName,QFile::WRITE);
  if(!f.IsOpen())return FALSE;

  // Dump state
  if(!time->SaveState(&f))return FALSE;
  if(!scene->SaveState(&f))return FALSE;

  return TRUE;
}

/*********
* TRACKS *
*********/
bool RManager::LoadTrack(cstring trackName,RTrackLoadCB cbFunc)
// Loads a track and sets it into the scene. You may specify a progress
// callback function, which is called for every loaded model.
// Returns FALSE in case of failure
{
  RTrack *trackNew;
  bool r=TRUE;                // Return value
  
  // Make sure textures go into the right graphics context
  QCV->Select();
  
  // Attempt to find track to get its type
  trackNew=new RTrack();
  trackNew->SetName(trackName);
  trackNew->Load();
//qdbg("pre1\n");QDELETE(trackNew);return FALSE;
//qdbg("RMGR:Track(%s); type=%s\n",trackName,trackNew->GetType());

//qdbg("prekill\n");QDELETE(trackNew);return FALSE;

  // Delete old track
  //...
  
  // Load new track
  if(!strcmp(trackNew->GetType(),"vrml"))
  {
//qdbg("Loading VRML track\n");
    trackVRML=new RTrackVRML();
    trackVRML->SetName(trackName);
    if(cbFunc)
      trackVRML->SetLoadCallback(cbFunc);
    if(!trackVRML->Load())
    {
      r=FALSE;
    }
    trackVRML->SetLoadCallback(0);
    track=trackVRML;
    // Set track in scene
//qdbg("Set scene track (scene=%p)\n",scene);
    scene->SetTrack(RMGR->trackVRML);
#ifdef OBS_NO_MORE_FLAT_SUPPORTED
  } else if(!strcmp(trackNew->GetType(),"flat"))
  {
qdbg("Loading FLAT track\n");
    trackFlat=new RTrackFlat();
    trackFlat->SetName(trackName);
    trackFlat->Load();
    track=trackFlat;
    scene->SetTrack(RMGR->trackFlat);
#endif
  } else
  {
    qwarn("RManager:LoadTrack(%s); unsupported track type (%s)",
      trackName,trackNew->GetType());
    r=FALSE;
  }
//qdbg("Delete trackNew (%p)\n",trackNew);
  delete trackNew;
  //QDELETE(trackNew);
//qdbg("Return %d\n",r);
  return r;
}

/**************
* Integration *
**************/
void RManager::SimulateUntilNow()
// Perform simulation until the time 'now' is reached
// Moved from 'mrun.cpp' into this class on 19-5-01
{
  int    lastTime;
  int    curTime,diffTime,maxTime;

  // Calculate time from last to current situation
  time->Update();
  curTime=time->GetRealTime();
  lastTime=time->GetLastSimTime();
  diffTime=curTime-lastTime;
  maxTime=lastTime+maxSimTimePerFrame;

#ifdef OBS
qdbg("curT=%d, lastT=%d, diffT=%d, maxT=%d, span=%f/%d\n",
 curTime,lastTime,diffTime,maxTime,time->GetSpan(),time->GetSpanMS());
#endif
  // Frequency of simulation is constant; the drawing of gfx
  // frames is NOT
  if(diffTime>maxSimTimePerFrame)
  {
    //diffTime=maxSimTimePerFrame;
    curTime=maxTime;
    //runFlags|=RUNF_SLOMO;
    //curTime=lastTime+maxSimTimePerFrame;
//qdbg("Warning: SLOMO\n");
  } else
  {
    //runFlags&=~RUNF_SLOMO;
  }
  
  // Calculate physics until we are at the last realtime checkpoint
  int integrations=0;
//qdbg("controller input: %d %d %d\n",ctlSteer,ctlThrottle,ctlBrakes);
#ifdef OBS
  car->SetInput(ctlSteer,ctlThrottle,ctlBrakes,ctlClutch);
#endif
  while(time->GetSimTime()<curTime)
  {
    integrations++;
    if(IsDevEnabled(FRAME_LINE))
      qdbg("-----------------------\n");
    
    // Run all cars
//qdbg("RMGR: scene Animate\n");
    scene->Animate();

    // At specific time intervals, run other time-related processes
    scene->ProcessFX();
    scene->ProcessReplay();
    if(chair->IsTimeForUpdate())
      chair->Update();

    // Keep total SIMULATED time (in case of slomo, this is NOT
    // the same as real time)
//qdbg("RMGR: add Sim Time\n");
    time->AddSimTime(time->GetSpanMS());
    
    // If we want to draw every frame, don't continue calculating
    if(IsDevEnabled(DRAW_EVERY_STEP))
      break;
    
#ifdef OPT_KEEP_RETIMING
    // Keep on pushing to realtime
//qdbg("RMGR: push until realtime\n");
    curTime=time->GetRealTime();
    // Don't go beyond time limit per frame
    if(curTime>maxTime)
      curTime=maxTime;
#endif
  }
 
  // Run audio system
//qdbg("RMGR: audio\n");
  if(integrations)
  { if(IsDevEnabled(DRAW_EVERY_STEP))
    { // Debug does a bit more audio to get a better feel
      audio->Run(RR_TIMESTEP*integrations*5);
    } else
    { // Actual sim rate
      audio->Run(RR_TIMESTEP*integrations*2);
    }
  }
 
  // Run music
  if(musicMgr->NextSongRequested())
  {
    // Play the next song; this couldn't be done directly in the class itself,
    // so RManager picks up the job.
    musicMgr->PlayNext();
  }

  // Update other sim stuff that are out of the integration loop
//qdbg("RMGR: scene->OnGfxFrame\n");
  scene->OnGfxFrame();
  // Particles
  pm->Update(((float)(curTime-lastTime))/1000.0f);

  if(network->IsEnabled())
    network->Handle();

  // We are now with the simulation near the actual real time
//qdbg("RMGR: setLastSimTime\n");
  time->SetLastSimTime();
//qdbg("RMGR:SimulateUntilNow() RET\n");
}

/************
* Resetting *
************/
void RManager::ResetRace()
// Do a reset/restart of the race
{
  // Reset time
  RMGR->time->Reset();
  RMGR->time->Start();
  if(scene)
    scene->ResetRace();
}

