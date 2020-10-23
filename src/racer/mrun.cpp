/*
 * Racer - main test exe
 * 05-08-00: Created!
 * 13-08-01: Some networking support. Cars created while already in the game.
 * NOTES:
 * (c) DolphinityRvG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/global.h>
#include <d3/fps.h>
#include <d3/texfont.h>
#include <qlib/timer.h>
#include <qlib/dxjoy.h>
#include <nlib/global.h>
#include "intro.h"
#include <GL/glu.h>
#ifdef Q_USE_SDL_VIDEO
#include <SDL/SDL.h>
#endif
#include <qlib/profile.h>
#include <stdarg.h>
DEBUG_ENABLE

#define DRW        QSHELL
#define DEBUG_INI  "debug.ini"

// Green dots to indicate where the wheels are?
//#define DO_DEBUG_POINTS

// Green dots to indicate collision points
//#define DO_DEBUG_COLLISION_POINTS

// Profile the simulation?
#define USE_PROFILING

#ifdef USE_PROFILING
#define PROFILE(n)     RMGR->profile->SwitchTo(n)
#else
#define PROFILE(n)
#endif

// Graphics
DFPS     *fps;
DTexFont *texfStat;

enum GfxFlags
{
  GFXF_FPS=1
};
int     gfxFlags=GFXF_FPS;

enum RunFlags
{
  RUNF_SLOMO=1,
  RUNF_PAUSE=2
};
int     runFlags;

// The client's car
RCar         *car;
// Some AI car; testing only
RCar         *carAI;

// Debugging
RDebugInfo *rdbg;
//QProfiler qprofiler;

// Controller info
int ctlSteer,ctlThrottle,ctlBrakes,ctlClutch,ctlHandbrake;

// Camera
int curCam=5;

struct Statistics
{
  double fps;                // Frames per second
  int    frame;              // Painted frame number
};

struct Prefs
{
  bool bReflections;         // Draw reflection of car?
  int  framesMax;            // After this #frames, auto exit program
};

static Statistics rStats;
static Prefs      rPrefs;
static int  tSimTime;             // Simulated time in milliseconds
static bool isRunning=TRUE;
static int  tCheckCarTime;        // Time to enter car into the game
bool fViewReplay;                 // Are we just going to watch a replay?

// Information
enum InfoPages
{
  INFO_NONE,                      // Turned off
  INFO_TIRE,                      // Tire forces
  INFO_TIMING,                    // Laptimes
  INFO_SURFACE,                   // Surface height, name etc.
  INFO_CONTROLS,                  // Joystick positions
  INFO_NETWORK,                   // Network info
  INFO_PROFILE,                   // CPU usage
  INFO_SUSP,                      // Suspension forces (including ARB)
  INFO_MAX
};
cstring infoPageName[INFO_MAX]=
{
  0,"Tires","Timing","Surface","Controls","Network","Profile","Suspension"
};
int infoPage;

// GUI interfacing for all modules
static int key;
static bool fMenu;                // Flag for event handler
static bool fRequestMenu;         // Want to go back to the menu
bool fReplay;                     // We're in replay mode

// Proto
void idlefunc();

/*******
* Exit *
*******/
void exitfunc()
{
qdbg("exitfunc()\n");

  // Stop networking
  qnShutdown();

  // Try not to lose statistics
  if(RMGR)
    if(RMGR->scene)
      RMGR->scene->Update();

  QDELETE(RMGR);
  QDELETE(texfStat);
  QFREE(rdbg);

  // Free 3rd party libraries
#ifdef Q_USE_SDL_VIDEO
  // Close SDL
//qdbg("SDL_Quit\n");
  SDL_Quit();
#endif

#ifdef Q_USE_FMOD
  // Close FMOD
  FSOUND_Close();
#endif

  // Check for everything to be freed
  QDebugListObjects();
}

/**********
* Pausing *
**********/
void PauseOn()
// Stop the simulation
{
  RMGR->time->Stop();
  // Nice time to update your file
  RMGR->scene->Update();
  runFlags|=RUNF_PAUSE;
  isRunning=FALSE;
}
void PauseOff()
// Start the simulation again
{
  RMGR->time->Start();
  runFlags&=~RUNF_PAUSE;
  isRunning=TRUE;
}

/**********
* Pausing *
**********/
static void StartReplay()
// Prepare the replay, start it, and exit
{
  // Don't replay while replaying
  if(fReplay)return;

  // Hold the time!
  PauseOn();
  Replay();
  // Restore racing
  app->SetIdleProc(idlefunc);
}

/*************
* Screenshot *
*************/
static string GetNextScreenShotFile(cstring prefix,string d)
// Automatically numbers screenshot pictures so a non-existing
// file can be created
// 'prefix' might be 'bc' for example.
// Filename is stored 'd', which must be some 40 chars minimal.
{
  int i;
  for(i=1;;i++)
  {
    sprintf(d,"data/dump/%s%03d.tga",prefix,i);
    if(!QFileExists(d))break;
  }
  return d;
}
static void ScreenShot()
{
  char buf[256];
  string s=GetNextScreenShotFile("screenshot",buf);
  DRW->ScreenShot(s);
}

/*********
* EVENTS *
*********/
char keyState[256];
bool event(QEvent *e)
{
  PROFILE(RProfile::PROF_EVENTS);

  if(fMenu)
  {
    if(e->type==QEvent::KEYPRESS)
    {
      // Pass on keys
      key=e->n;
    }
    return FALSE;
  } else if(fReplay)
  {
    if(e->type==QEvent::KEYPRESS)
    {
      if(e->n==QK_ESC)
      {
        // Leave replay mode
        fReplay=FALSE;
        return TRUE;
      }
    }
    // Pass it all on to the replay module
    if(ReplayEvent(e))
      return TRUE;
  }
  
  // Keystate keeping
  if(e->type==QEvent::KEYPRESS)keyState[e->n&255]=1;
  else if(e->type==QEvent::KEYRELEASE)keyState[e->n&255]=0;
#ifdef OBS
  if(e->type==QEvent::KEYPRESS)qdbg("key %d on\n",e->n&255);
  else if(e->type==QEvent::KEYRELEASE)qdbg("key %d off\n",e->n&255);
#endif

  // Racing keys
  if(e->type==QEvent::KEYPRESS)
  {
//qdbg("event; key %x\n",e->n);
    if(fReplay)goto skip_esc;

    if(e->n==QK_ESC)
    {
      // Wish a menu
      fRequestMenu=TRUE;
      RMGR->time->Stop();
      return TRUE;
    } else if(e->n==(QK_SHIFT|QK_ESC))
    {
      app->Exit(0);
    }
   skip_esc:

    // Camera selection
    if(e->n==QK_1)
    { curCam=0;
     do_cam:
      RMGR->scene->SetCam(curCam);
    } else if(e->n==QK_2)
    { curCam=1; goto do_cam;
    } else if(e->n==QK_3)
    { curCam=2; goto do_cam;
    } else if(e->n==QK_4)
    { curCam=3; goto do_cam;
    } else if(e->n==QK_5)
    { curCam=4; goto do_cam;
    } else if(e->n==QK_6)
    { curCam=5; goto do_cam;
    } else if(e->n==QK_7)
    { curCam=6; goto do_cam;
    } else if(e->n==QK_8)
    { curCam=7; goto do_cam;
    } else if(e->n==QK_9)
    { curCam=8; goto do_cam;
    } else if(e->n==QK_0)
    { curCam=9; goto do_cam;
    }
    if(e->n==QK_C)
    {
      // Camera mode
      RMGR->scene->SetNextCamMode();
    }
    
    if(e->n==(QK_CTRL|QK_F))
    {
      // Toggle FPS
      if(!(gfxFlags&GFXF_FPS))
      { // Reset FPS counter
        fps->Reset();
      }
      gfxFlags^=GFXF_FPS;
    } else if(e->n==QK_GRAVE)
    {
      if(RCON->IsHidden())RCON->Show();
      else                RCON->Hide();
    } else if(e->n==QK_F2)
    { 
      StartReplay();
    } else if(e->n==QK_F12)
    { 
      ScreenShot();
    } else if(e->n==(QK_SHIFT|QK_R))
    {
      RMGR->ResetRace();
#ifdef OBS
      // Move car to first grid location (pole position)
      if(car)
        car->Warp(&RMGR->track->gridPos[0]);
      // Avoid super time by resetting time as well
      RMGR->time->Reset();
      RMGR->time->Start();
#endif
    } else if(e->n==(QK_SHIFT|QK_P))
    {
      // Move car to first pit location
      if(car)
        car->Warp(&RMGR->track->pitPos[0]);
    } else if(e->n==(QK_CTRL|QK_L))
    {
qdbg("Load state\n");
      RMGR->LoadState();
    } else if(e->n==(QK_CTRL|QK_S))
    {
qdbg("Save state\n");
      RMGR->SaveState();
    }
    
    // Pause
    if(e->n==QK_P)
    {
      if(isRunning)PauseOn();
      else         PauseOff();
//RMGR->scene->GetDriver(0)->Update();
    }
    // Toggles
    if(e->n==(QK_CTRL|QK_I))
    {
      infoPage=(infoPage+1)%INFO_MAX;
    }
  }

  // Return to other stuff
  PROFILE(RProfile::PROF_OTHER);

  return FALSE;
}
int rrInKey()
// Returns key or -1 if none
// Not really like normal event-driven programming, but so handy
{
  key=-1;
  app->Run1();
  return key;
}

/***********
* Viewport *
***********/
void SetupViewport(int w,int h,rfloat fov)
{
  static bool fSetup=FALSE;
  static DMatrix4 matPrj;
  //static rfloat visibility;
  
#ifdef FUTURE_CAM_ANGLE_PROBLEMS
  if(fSetup)
  {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(matPrj.GetM());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    return;
  }
#endif
  
#ifdef FUTURE_VIEWS
  QInfo *info=RMGR->GetGfxInfo();
#endif
  QRect r;
  
  // Calculate projection
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
//RMGR->visibility=100000;
  //gluPerspective(fov,(GLfloat)w/(GLfloat)h,0.1f,RMGR->visibility);
  GetCurrentQGLContext()->Perspective(
    fov,(GLfloat)w/(GLfloat)h,0.1f,RMGR->visibility);
  // Remember for later
  //glGetFloatv(GL_PROJECTION_MATRIX,matPrj.GetM());
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
#ifdef FUTURE
  r.x=info->GetInt("view.render.x");
  r.y=info->GetInt("view.render.y");
  r.wid=info->GetInt("view.render.wid");
  r.hgt=info->GetInt("view.render.hgt");
  glViewport(r.x,r.y,r.wid,r.hgt);
  glScissor(r.x,r.y,r.wid,r.hgt);
  glEnable(GL_SCISSOR_TEST);
#endif
  GetCurrentQGLContext()->Viewport(0,0,w,h);

  fSetup=TRUE;
}

/***********
* Painting *
***********/
QDraw *GGetDrawable()
{
  return DRW;
}
QCanvas *GGetCV()
{
  return DRW->GetCanvas();
}
void GSwap()
{
//#define MOTION_BLUR_TEST
#ifdef MOTION_BLUR_TEST
  // On Linux/Windows, this is SOOOOOOO slow you just can't believe it. :(
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_PACK_SKIP_PIXELS,0);
  glPixelStorei(GL_PACK_SKIP_ROWS,0);
  glRasterPos2i(0,0);
  glDisable(GL_DITHER);
  glEnable(GL_BLEND);
  glBlendFunc(GL_CONSTANT_COLOR_EXT,GL_ONE_MINUS_CONSTANT_COLOR_EXT);
  glBlendColorEXT(1,1,1,0.1);
  glReadBuffer(GL_BACK);
  glDrawBuffer(GL_FRONT);
  glReadBuffer(GL_FRONT);
  glDrawBuffer(GL_BACK);
  //glWriteBuffer(GL_FRONT);
  //glDisable(GL_BLEND);
  glCopyPixels(0,0,800,600,GL_COLOR);
  glEnable(GL_DITHER);
  glDrawBuffer(GL_BACK);
  DRW->Swap();
#else
  DRW->Swap();
#endif
}

/*********
* Status *
*********/
static void PaintStr(cstring name,int *x,int *y)
{
  texfStat->Paint(name,*x,*y);
  *y-=25;
}
static void PaintVar(cstring name,cstring value,int *x,int *y)
{
  char buf[80];
  
  sprintf(buf,"%s: %s",name,value);
  texfStat->Paint(buf,*x,*y);
  *y-=15;
}
static void PaintVar(cstring name,int value,int *x,int *y)
{
  //QCanvas *cv=GGetCV();
  char buf[80];
  
  sprintf(buf,"%s: %d",name,value);
  //cv->Text(buf,*x,*y);
  texfStat->Paint(buf,*x,*y);
  *y-=15;
}
static void PaintVar(cstring name,double value,int *x,int *y)
{
  //QCanvas *cv=GGetCV();
  char buf[80];
  
  sprintf(buf,"%s: %.3f",name,value);
  //cv->Text(buf,*x,*y);
  texfStat->Paint(buf,*x,*y);
  *y-=15;
}
#define _PV(s,v) PaintVar(s,v,&x,&y)
static void PaintControllerStatus()
// Show different vars
// Assumes 2D view has been set up
{
  //QCanvas *cv=GGetCV();
  char buf[80];
  int  x,y;
  int  i;
 
  x=20; y=DRW->GetHeight()-20;
  glDisable(GL_CULL_FACE);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  glDisable(GL_BLEND);
  
  // Display any important messages
  if(runFlags&RUNF_SLOMO)
  {
    texfStat->Paint("SLOMO",DRW->GetWidth()/2-50,y-25);
  }
  if(fReplay)
  {
    texfStat->Paint("REPLAY",DRW->GetWidth()/2-50,y);
  } else if(runFlags&RUNF_PAUSE)
  {
    texfStat->Paint("PAUSED",DRW->GetWidth()/2-50,y);
  }
  
  // FPS
  fps->FrameRendered();
  if(gfxFlags&GFXF_FPS)
  {
    _PV("FPS",fps->GetFPS());
  }

  // Always handy until it's standard in the views
  if(car)
    _PV("Gear",car->GetGearBox()->GetGearName(car->GetGearBox()->GetGear()));

  // Selected info page; title
  y-=10;
  if(infoPage!=INFO_NONE)
    PaintStr(infoPageName[infoPage],&x,&y);

  if(infoPage==INFO_SURFACE)
  {
    if(car){

    DMaterial *mat;
    RSurfaceType *st;

    mat=car->GetWheel(0)->GetSurfaceInfo()->material;
    _PV("surf0",mat?mat->name.cstr():"---");
    mat=car->GetWheel(1)->GetSurfaceInfo()->material;
    _PV("surf1",mat?mat->name.cstr():"---");
    mat=car->GetWheel(2)->GetSurfaceInfo()->material;
    _PV("surf2",mat?mat->name.cstr():"---");
    mat=car->GetWheel(3)->GetSurfaceInfo()->material;
    _PV("surf3",mat?mat->name.cstr():"---");

    st=car->GetWheel(0)->GetSurfaceInfo()->surfaceType;
//qdbg("st=%p\n",st);
    _PV("friction0",st?st->frictionFactor:0);
    _PV("rolling0",st?st->rollingResistanceFactor:0);

    // UV coordinates
    _PV("u0",car->GetWheel(0)->GetSurfaceInfo()->u);
    _PV("u1",car->GetWheel(1)->GetSurfaceInfo()->u);
    _PV("u2",car->GetWheel(2)->GetSurfaceInfo()->u);
    _PV("u3",car->GetWheel(3)->GetSurfaceInfo()->u);
  
    _PV("v0",car->GetWheel(0)->GetSurfaceInfo()->v);
    _PV("v1",car->GetWheel(1)->GetSurfaceInfo()->v);
    _PV("v2",car->GetWheel(2)->GetSurfaceInfo()->v);
    _PV("v3",car->GetWheel(3)->GetSurfaceInfo()->v);
  
    _PV("Y0",car->GetWheel(0)->GetSurfaceInfo()->y);
    _PV("Y1",car->GetWheel(1)->GetSurfaceInfo()->y);
    _PV("Y2",car->GetWheel(2)->GetSurfaceInfo()->y);
    _PV("Y3",car->GetWheel(3)->GetSurfaceInfo()->y);
    }
  } else if(infoPage==INFO_TIMING)
  {
    _PV("Real time (s)",RMGR->time->GetRealTime()/1000);
    _PV("Simulated time (s)",RMGR->time->GetSimTime()/1000);
    if(car)
      _PV("Vel (km/h)",(3.6f*car->GetVelocity()->Length())); // m/s => km/h
    _PV("Lap",RMGR->scene->curLap[0]);
    _PV("Timeline index",RMGR->scene->curTimeLine[0]);
    _PV("1st section time",RMGR->scene->tlTime[0][0]);
    _PV("2nd section time",RMGR->scene->tlTime[0][1]);

    // Replay info
    char *firstFrame,*currentFrame,*buffer;
    buffer=(char*)RMGR->replay->GetBuffer();
    firstFrame=(char*)RMGR->replay->GetFirstFrame();
    currentFrame=(char*)RMGR->replay->GetCurrentFrame();
    _PV("Replay first offset",firstFrame-buffer);
    _PV("Replay current offset",currentFrame-buffer);
    _PV("Replay memory usage (Kb)",RMGR->replay->GetReplaySize()/1024);
    _PV("Replay first frame time",RMGR->replay->GetFrameTime(firstFrame));
    _PV("Replay total time (s)",
      float(RMGR->replay->GetFrameTime(currentFrame)-
            RMGR->replay->GetFrameTime(firstFrame))/1000.0);
  } else if(infoPage==INFO_TIRE)
  {
    if(car){
    _PV("wheel(0)-Mz",car->GetWheel(0)->GetAligningTorque());
    _PV("wheel(1)-Mz",car->GetWheel(1)->GetAligningTorque());
    _PV("Filtered Mz",car->GetMzFilter()->GetValue());
    //_PV("Joy0 B1",RMGR->controls->joy[0]->button[1]);
    //_PV("Car heading",(float)(car->GetHeading()*RAD_DEG_FACTOR));
    // Slip ratios and angles
    _PV("SR0",car->GetWheel(0)->GetSlipRatio());
    _PV("SR1",car->GetWheel(1)->GetSlipRatio());
    _PV("SR2",car->GetWheel(2)->GetSlipRatio());
    _PV("SR3",car->GetWheel(3)->GetSlipRatio());
    _PV("F-lon-0",car->GetWheel(0)->GetForceRoadTC()->z);
    _PV("F-lon-1",car->GetWheel(1)->GetForceRoadTC()->z);
    _PV("F-lon-2",car->GetWheel(2)->GetForceRoadTC()->z);
    _PV("F-lon-3",car->GetWheel(3)->GetForceRoadTC()->z);
    _PV("SA0",car->GetWheel(0)->GetSlipAngle()*RR_RAD2DEG);
    _PV("SA1",car->GetWheel(1)->GetSlipAngle()*RR_RAD2DEG);
    _PV("SA2",car->GetWheel(2)->GetSlipAngle()*RR_RAD2DEG);
    _PV("SA3",car->GetWheel(3)->GetSlipAngle()*RR_RAD2DEG);
    _PV("F-lat-0",car->GetWheel(0)->GetForceRoadTC()->x);
    _PV("F-lat-1",car->GetWheel(1)->GetForceRoadTC()->x);
    _PV("F-lat-2",car->GetWheel(2)->GetForceRoadTC()->x);
    _PV("F-lat-3",car->GetWheel(3)->GetForceRoadTC()->x);
    _PV("Skid0",car->GetWheel(0)->GetSkidFactor());
    _PV("Skid1",car->GetWheel(1)->GetSkidFactor());
    _PV("Skid2",car->GetWheel(2)->GetSkidFactor());
    _PV("Skid3",car->GetWheel(3)->GetSkidFactor());
    _PV("SlipV0",car->GetWheel(0)->GetSlipVectorCC()->Length());
    _PV("SlipV1",car->GetWheel(1)->GetSlipVectorCC()->Length());
    _PV("SlipV2",car->GetWheel(2)->GetSlipVectorCC()->Length());
    _PV("SlipV3",car->GetWheel(3)->GetSlipVectorCC()->Length());
    _PV("Load0",car->GetWheel(0)->GetForceRoadTC()->y);
    _PV("Load1",car->GetWheel(1)->GetForceRoadTC()->y);
    _PV("Load2",car->GetWheel(2)->GetForceRoadTC()->y);
    _PV("Load3",car->GetWheel(3)->GetForceRoadTC()->y);
    if(car->GetWings())
    {
      // Show downforce for all wings
      for(i=0;i<car->GetWings();i++)
      {
        sprintf(buf,"Wing%d downforce",i);
        _PV(buf,car->GetWing(0)->GetForce()->y);
        sprintf(buf,"Wing%d drag",i);
        _PV(buf,car->GetWing(0)->GetForce()->z);
      }
    }
    }
  } else if(infoPage==INFO_CONTROLS)
  {
//#ifdef ND_CRASHES_LINUX
    // Joystick watch
    if(RMGR->controls->joy[0]!=0&&
       RMGR->controls->joy[0]!=RR_CONTROLLER_MOUSE&&
       RMGR->controls->joy[0]!=RR_CONTROLLER_KEYBOARD)
    {
//qdbg("joy0=%p\n",RMGR->controls->joy[0]);
      _PV("Joy0 X",RMGR->controls->joy[0]->x);
      _PV("Joy0 Y",RMGR->controls->joy[0]->y);
      _PV("Joy0 Z",RMGR->controls->joy[0]->z);
      _PV("Joy0 Rx",RMGR->controls->joy[0]->rx);
      _PV("Joy0 Ry",RMGR->controls->joy[0]->ry);
      _PV("Joy0 Rz",RMGR->controls->joy[0]->rz);
      // Show POV as well.
    }
    // Controls watching (the virtual controls)
    _PV("Ctl SteerLeft",
      RMGR->controls->control[RControls::T_STEER_LEFT]->value);
    _PV("Ctl SteerRight",
      RMGR->controls->control[RControls::T_STEER_RIGHT]->value);
    _PV("Ctl Throttle",RMGR->controls->control[RControls::T_THROTTLE]->value);
    _PV("Ctl Brakes",RMGR->controls->control[RControls::T_BRAKES]->value);
    _PV("Ctl Clutch",RMGR->controls->control[RControls::T_CLUTCH]->value);
    _PV("Ctl Handbrake",RMGR->controls->control[RControls::T_HANDBRAKE]->value);
    _PV("Ctl Starter",RMGR->controls->control[RControls::T_STARTER]->value);
    _PV("Ctl Horn",RMGR->controls->control[RControls::T_HORN]->value);
    _PV("Ctl ShiftUp",RMGR->controls->control[RControls::T_SHIFTUP]->value);
    _PV("Ctl ShiftDown",
      RMGR->controls->control[RControls::T_SHIFTDOWN]->value);
    //_PV("steer",ctlSteer);
    // Must move throttle/brake to the car object, since assists may
    // juggle with both. (like the clutch)
    _PV("throttle%",ctlThrottle/10.0f);
    _PV("brakes%",ctlBrakes/10.0f);
    //_PV("clutch%",ctlClutch/10.0f);
    if(car)
    {
      _PV("clutch%",car->GetDriveLine()->GetClutchApplication()*100.0f);
      _PV("Engine stalled",car->GetEngine()->IsStalled());
      _PV("Clutch torque",car->GetDriveLine()->GetClutchCurrentTorque());
      _PV("Engine rotV",car->GetEngine()->GetRotationVel());
      _PV("Nrm Gearbox rotV",car->GetGearBox()->GetRotationVel()*
         car->GetGearBox()->GetRatio());
      //_PV("Gearbox rotV",car->GetGearBox()->GetRotationVel());
      _PV("PrePost locked",car->GetDriveLine()->IsPrePostLocked());
      //_PV("Reaction T at engine",car->GetEngine()->GetReactionTorque());
      //_PV("Braking T at engine",car->GetEngine()->GetBrakingTorque());
    }
  //if(car)_PV("rpm",car->GetEngine()->GetRPM());
//#endif
  } else if(infoPage==INFO_PROFILE)
  {
    RMGR->profile->Update();
    _PV("CPU% physics",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_PHYSICS));
    _PV("CPU% CD:wheels-splines",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_CD_WHEEL_SPLINE));
    _PV("CPU% CD:wheels-track",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_CD_WHEEL_TRACK));
    _PV("CPU% CD:car-track",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_CD_CARTRACK));
    _PV("CPU% CD:objects-objects",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_CD_OBJECTS));
    _PV("CPU% graphics",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_GRAPHICS));
    _PV("CPU% swap",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_SWAP));
    _PV("CPU% controls",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_CONTROLS));
    _PV("CPU% events",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_EVENTS));
    _PV("CPU% other",
      RMGR->profile->GetTimePercentageFor(RProfile::PROF_OTHER));
  } else if(infoPage==INFO_NETWORK)
  {
    _PV("Enabled",RMGR->network->IsEnabled());
    _PV("Server",RMGR->network->GetServerName());
    _PV("ClientID",RMGR->network->GetClientID());
    _PV("Clients",RMGR->network->clients);
    for(i=0;i<RMGR->network->clients;i++)
    {
      sprintf(buf,"Client%d",i);
      _PV(buf,RMGR->network->client[i].addr.ToString());
    }
    _PV("Port",RMGR->network->GetPort());
    _PV("Cars",RMGR->scene->GetCars());
  } else if(infoPage==INFO_SUSP)
  {
    if(car)
    {
      for(i=0;i<car->GetWheels();i++)
      {
        sprintf(buf,"S%d Length",i);
        _PV(buf,car->GetSuspension(i)->GetLength());
      }
      for(i=0;i<car->GetWheels();i++)
      { if(car->GetSuspension(i)->GetARBOtherSuspension())
        { sprintf(buf,"S%d spring force",i);
          _PV(buf,car->GetSuspension(i)->GetForceSpring()->y);
        }
      }
      for(i=0;i<car->GetWheels();i++)
      { if(car->GetSuspension(i)->GetARBOtherSuspension())
        { sprintf(buf,"S%d damper force",i);
          _PV(buf,car->GetSuspension(i)->GetForceDamper()->y);
        }
      }
      for(i=0;i<car->GetWheels();i++)
      { if(car->GetSuspension(i)->GetARBOtherSuspension())
        { sprintf(buf,"S%d ARB force",i);
          _PV(buf,car->GetSuspension(i)->GetForceARB()->y);
        }
      }
#ifdef ND_RATE_IS_CONSTANT
      for(i=0;i<car->GetWheels();i++)
      { if(car->GetSuspension(i)->GetARBOtherSuspension())
        { sprintf(buf,"S%d ARB rate",i);
          _PV(buf,car->GetSuspension(i)->GetARBRate());
        }
      }
#endif
    }
  }
}

/**************
* Integration *
**************/
void UpdateControllers()
{
  rfloat Mz,kerbLoad,kerbVelocity;
  int i;
  RWheel *w;
  RSurfaceType *st;

//qdbg("UpdateControllers(); car=%p\n",car);

  // Any car being controlled?
  if(!car)return;

  if(RMGR->IsDevEnabled(RManager::NO_CONTROLLER_UPDATES))
    goto skip_read;
 
  // Get aligning moment of all steering wheels
  Mz=0;
  kerbLoad=0;
  for(i=0;i<car->GetWheels();i++)
  {
    w=car->GetWheel(i);
    st=w->GetSurfaceInfo()->surfaceType;
    // Aligning moment
    if(w->IsSteering())
      Mz+=w->GetAligningTorque();
    // Kerb characteristics
    if(st!=0&&st->baseType==RSurfaceType::KERB)
    {
      // This wheel is driving on a kerb; add the load
      // at the wheel
      kerbLoad+=w->GetForceRoadTC()->y;
      //kerbVelocity=
    }
  }
  if(kerbLoad>0)
  {
    // Found kerbs, calculate velocity
    kerbVelocity=car->GetVelocityTach();
  } else
  {
    kerbVelocity=0;
  }

  // Filter the FF value at low speed
  if(car->GetBody()->GetLinVel()->LengthSquared()<
     RMGR->GetFFDampingThreshold())
  {
    car->GetMzFilter()->MoveTo(Mz);
  } else
  {
    // Set directly; should be fine now
    car->GetMzFilter()->Set(Mz);
  }

  // Pass on to the controllers
  RMGR->controls->SetFFSteerResponse(car->GetMzFilter()->GetValue(),
    kerbLoad,kerbVelocity);

  // Read the new set of controller data
  RMGR->controls->Update();
  // Add both steering controls to get steering result
  ctlSteer=RMGR->controls->control[RControls::T_STEER_LEFT]->value-
           RMGR->controls->control[RControls::T_STEER_RIGHT]->value;
  ctlThrottle=RMGR->controls->control[RControls::T_THROTTLE]->value;
  ctlBrakes=RMGR->controls->control[RControls::T_BRAKES]->value;
  ctlClutch=RMGR->controls->control[RControls::T_CLUTCH]->value;
  ctlHandbrake=RMGR->controls->control[RControls::T_HANDBRAKE]->value;
//qdbg("throttle=%d, brakes=%d, steer=%d\n",ctlThrottle,ctlBrakes,ctlSteer);
 skip_read:
  // Pass controller info on to the player car
//qdbg("  pass controller data\n");
  car->SetInput(ctlSteer,ctlThrottle,ctlBrakes);
  car->GetDriveLine()->SetInput(ctlClutch,ctlHandbrake);
}

void PaintScene(int flags=0)
// Clear the view, paint the whole lot
// flags&1 => don't paint info (FPS/console/info, all 2D stuff)
{
  DVector3 *v;


  GGetCV()->Select();
  SetupViewport(DRW->GetWidth(),DRW->GetHeight(),
    RMGR->scene->GetCamAngle());
#ifdef OBS
DMatrix4 m;
glGetFloatv(GL_MODELVIEW_MATRIX,m.GetM());
m.DbgPrint("modelview_1");
#endif
  GGetCV()->SetFont(app->GetSystemFont());

  if(RMGR->IsDevEnabled(RManager::CHECK_GL_ERRORS))
  {
//qdbg("check GL errors\n");
    QShowGLErrors("idleFunc");
  }

  RMGR->scene->Clear();
  
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  //float lightDir[3]={ 0,-.1,-1 };
  //float lightPos[4]={ 1,1,1,0 };
  float lightPos[4]={ -1,1,1,0 };
  //glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,lightDir);
  glEnable(GL_DEPTH_TEST);
  
  // Move to the correct viewing position/rotation
  RMGR->scene->GoCamera();
  
  glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
  //glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,lightDir);

  if(RMGR->IsDevEnabled(RManager::SHOW_AXIS_SYSTEM)&&car)
  {
    v=car->GetPosition();
    RGfxAxes(v->x,v->y+.01,v->z,3);
  }
  //glTranslatef(0,-1.66,-5.0);
  //glRotatef(15,1,0,0);
#ifdef ND_MOVING_CAM
  static float vx=.05;
  static float y=2.5;
  y+=vx; if(y>5||y<1)vx=-vx;
  gluLookAt(y*2-6,y /*1.66*/,-5.0, 0,0,0, 0,1,0);
#endif
    
  RMGR->scene->Paint();
  
//qdbg("  idle: until geometry\n");
  // Unlit geometry coming up
  glShadeModel(GL_FLAT);
  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
 
#ifdef ND_NO_REFLECTIONS_LIKE_THIS
  if(rPrefs.bReflections)
  { // Reflections
    glFrontFace(GL_CW);
    glPushMatrix();
    glScalef(1,-1,1);
    //glPixelZoom(1,-1);
    car->Paint();
    glPopMatrix();
    glFrontFace(GL_CCW);
    //glPixelZoom(1,1);
    //glScalef(1,1,1);
  }
#endif
  
#ifdef OBS_NO_PROJECTED_SHADOW_LIKE_THIS
  // Shadows
  //glFrontFace(GL_CW);
  glPushMatrix();
  glScalef(1,0,1);
  car->Paint();
  glPopMatrix();
  //glFrontFace(GL_CCW);
#endif
  
  // Floor
//QTRACE("idlefunc: RMGR->scene=%p\n",RMGR->scene);
  if(rPrefs.bReflections)
  {
#if defined(__sgi)
    glBlendFunc(GL_ONE,GL_CONSTANT_ALPHA_EXT);
    glBlendColorEXT(.5,.5,.5,.5);
    glEnable(GL_BLEND);
#else
    glDisable(GL_BLEND);
#endif
  } else
  {
    glDisable(GL_BLEND);
  }
  
#ifdef DO_DEBUG_POINTS
  if(car)
  {
    // Paint dev debug test points in hard color
    glColor3f(0,1,0);
    glPointSize(2);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_POINTS);
    for(int i=0;i<car->GetWheels();i++)
    {
      glVertex3f(car->GetWheel(i)->devPoint.x,
                 car->GetWheel(i)->devPoint.y,
                 car->GetWheel(i)->devPoint.z);
car->GetWheel(i)->devPoint.DbgPrint("wheel devPt");
    }
    glEnd();
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
  }
#endif
  
#ifdef DO_DEBUG_COLLISION_POINTS
  if(car)
  {
    // Paint dev debug collision points in hard color
    glColor3f(0,1,0);
    glPointSize(2);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_POINTS);
    {
      DVector3 *v=car->GetBody()->GetCollisionPoint();
      glVertex3f(v->x,v->y,v->z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
  }
#endif
  
#ifdef OBS_IN_RSCENE
  // Paint view (cockpit elements) of current car/camera
  // Set 2D view
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glOrtho(0,DRW->GetWidth(),0,DRW->GetHeight(),-1,1);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  if(car)
    car->GetViews()->GetView(0)->Paint();
#endif
 
  if(flags&1)goto skip_2d;

  // Info
//qdbg("  idle: paint ctrlstatus\n");
  PaintControllerStatus();
 
  RCON->Paint();
 skip_2d:;
}

void idlefunc()
{
  //RTrack *track=RMGR->GetTrack();
 
  // Don't run when a dialog is up (PC driver bugs workaround)
  if(!DRW->IsEnabled())return;

  PROFILE(RProfile::PROF_CONTROLS);

  // Pick up a car as soon as possible
  if(!car)
  {
    // Any car available yet?
    if(RMGR->scene->GetCars()>0)
      car=RMGR->scene->GetCar(0);
  }

  if(RMGR->time->GetRealTime()>tCheckCarTime)
  {
    // Check cars
    if(!car)
    {
      if(RMGR->scene->GetCars()==0)
      {
        // Time to get our car in the game
        cstring carName;
        carName=RMGR->GetMainInfo()->GetStringDirect("dev.car","devtest");
        RMGR->network->GetGlobalMessage()->OutEnterCar(carName,0);
        // Wait a while to let command get through (avoid sending out
        // multiple requests for a car)
        tCheckCarTime+=1000;
      }
    }
  }

  // Update controller status
  UpdateControllers();

  // Physics
  PROFILE(RProfile::PROF_PHYSICS);
  // Animate all things upto the current time
  if(isRunning)
    RMGR->SimulateUntilNow();
//qdbg("time %d\n",RMGR->time->GetLastSimTime());
  // Don't paint when not running physics
  //if(!isRunning)return;
  
  PROFILE(RProfile::PROF_GRAPHICS);
  PaintScene();

  PROFILE(RProfile::PROF_SWAP);
  GSwap();

  PROFILE(RProfile::PROF_OTHER);
  if(rdbg->flags&RF_SLOMO)
  { QNap(rdbg->slomo);
  }
 
/*
qdbg("Profile count 0: %d (d3FindTriObb)\n",RMGR->profile->GetCount(0));
qdbg("Profile count 1: %d (RWhee:PreAnimate)\n",RMGR->profile->GetCount(1));
qdbg("Profile count 2: %d (CDGeodeTriOBB)\n",RMGR->profile->GetCount(2));
*/

  if(++rStats.frame==rPrefs.framesMax)
    app->Exit(0);
}

void Setup(bool first)
// Create Racer's manager (if first=TRUE), and show the menu.
{
  int     v;
  //cstring name;
  qstring carName;
  
  if(!first)
  {
    // Prepare 2nd menu
    DRW->GetCanvas()->SetMode(QCanvas::DOUBLEBUF);
    goto seconds;
  }
  
  // Start networking
  qnInit();

  // Get window up
  DRW->GetCanvas()->SetMode(QCanvas::DOUBLEBUF);
  app->RunPoll();

//printf("GL extensions: %s\n",glGetString(GL_EXTENSIONS));

  // Create Racer manager
  new RManager();
  RMGR->Create();
  RMGR->scene->env=new REnvironment();

  // Install procs for inkey and exit support
  //app->SetIdleProc(idlefunc);
  app->SetExitProc(exitfunc);
  app->SetEventProc(event);
  
  // Intro
  if(RMGR->GetMainInfo()->GetInt("gadgets.intro"))
    RacerIntro();
  
  // Debugging
  rdbg=(RDebugInfo*)qcalloc(sizeof(RDebugInfo));
  // Old, but still used; slomo
  v=RMGR->infoDebug->GetInt("timing.slomo");
  if(v)
  { rdbg->slomo=v;
    rdbg->flags|=RF_SLOMO;
  }
  if(RMGR->infoDebug->GetInt("controls.steering")==FALSE)
    rdbg->flags|=RF_NO_STEERING;
  if(RMGR->infoDebug->GetInt("stats.wheelinfo")==TRUE)
    rdbg->flags|=RF_WHEEL_INFO;
  
  // Prefs
  rPrefs.bReflections=RMGR->infoDebug->GetInt("gfx.reflections");
  rPrefs.framesMax=RMGR->infoDebug->GetInt("timing.frames_max");
  infoPage=RMGR->infoMain->GetInt("stats.default_info");
  if(infoPage>=INFO_MAX)infoPage=INFO_MAX-1;
  if(RMGR->infoMain->GetInt("console.active"))
    RCON->Show();
  else
    RCON->Hide();
  // Default filtering of textures
  //dglobal.prefs.minFilter=GL_LINEAR_MIPMAP_LINEAR;

 seconds:
  // Reread some preferences which may have been changed since the last
  // race.
  RMGR->network->ReadPrefs();

  // Run the menu
  if(RMGR->GetMainInfo()->GetInt("gadgets.menu"))
  {
    // Start the menu
    fMenu=TRUE;
    app->SetIdleProc(0);
    RMenuCreate();
    RMenuRun();
    RMenuDestroy();
  }
  
  // Time to race!
  if(!RMGR->GetMainInfo()->GetInt("gadgets.game"))
    app->Exit(0);

  // Reinstall procs after menu
  fMenu=FALSE;
  app->SetIdleProc(idlefunc);

  // Display 'loading...' picture
  rrFullScreenTexture("data/images/racer512.tga");
  
  // Create a scene
  RMGR->scene->Reset();
  
  // Load a track; first clear the texture pool to
  // avoid name conflicts when selecting another track.
  dglobal.texturePool->Clear();
  if(!rrTrackLoad(RMGR->infoMain->GetStringDirect("ini.track")))
    app->Exit(0);

  if(RMGR->network->IsClient())
  {
    // Connect to the server
    if(!RMGR->network->ConnectToServer())
    {
      RMSG("Can't connect to server '%s'",RMGR->network->GetServerName());
      goto fail;
    }
    // Retrieve all cars on the server
    RMGR->network->GetGlobalMessage()->OutRequestCars();
  }

  // Create a driver
  RDriver *drv;
  drv=new RDriver("default");
  drv->SetTrackName(RMGR->infoMain->GetStringDirect("ini.track"));
  drv->Load();
  RMGR->scene->AddDriver(drv);
  
#ifdef OBS_LATER
  // Move cars to initial positions
  car->Warp(&RMGR->track->gridPos[0]);
  if(carAI)carAI->Warp(&RMGR->track->gridPos[1]);
#endif
 
  // Start debug info
  ctlSteer=RMGR->infoDebug->GetInt("car.steer");
  ctlThrottle=RMGR->infoDebug->GetInt("controls.throttle");
  ctlBrakes=RMGR->infoDebug->GetInt("controls.brakes");
  ctlClutch=RMGR->infoDebug->GetInt("controls.clutch",1000);
  tCheckCarTime=0;
  
  DRW->GetCanvas()->SetMode(QCanvas::DOUBLEBUF);

  // Now that the window is up, we can continue with some things
  RMGR->controls->LoadConfig();
  
#ifdef OBS_LATER
  // Now that we have a car, focus on it
  RMGR->scene->SetCam(curCam);
  RMGR->scene->SetCamCar(car);

  // Auto load last saved state? (debugging)
  if(RMGR->infoDebug->GetInt("state.auto_load"))
    RMGR->LoadState();
#endif
  
  // Graphics
  if(first)
  {
    fps=new DFPS();
    QCV->Select();
    texfStat=new DTexFont("data/fonts/euro16");
  }

  // Reset the replay
  RMGR->replay->Reset();

  // Start profiling
  RMGR->profile->Reset();
  
  // Start the clock
  RMGR->time->Reset();
  RMGR->time->Start();

  // Music
  RMGR->musicMgr->PlayRandom(RMusicManager::RACE);

  // Ok
  return;

 fail:
  // Not OK
  fRequestMenu=TRUE;
  //app->SetIdleProc(0);
  //app->SetEventProc(0);
  return;
}

void Run()
{
  char buf[256];

  Setup(TRUE);
  // Keep coming back to the menu
  while(1)
  {
    while(!fRequestMenu)
    {
      app->Run1();

      if(fViewReplay)
      {
        // Load the replay and start
        fViewReplay=FALSE;
        sprintf(buf,"data/replays/%s.rpl",
          RMGR->GetMainInfo()->GetStringDirect("ini.replay"));
        RMGR->replay->Load(buf);
        StartReplay();
        // Don't go on racing (well, interesting thought...)
        fRequestMenu=TRUE;
      }
    }
qdbg("Back in Run()\n");
    fRequestMenu=FALSE;
  
    // Disconnect from server
    if(RMGR->network)
      RMGR->network->Disconnect();

    // Redo?
    if(!RMGR->GetMainInfo()->GetInt("gadgets.menu"))
      break;

qdbg("Update\n");

    // Nice time to update your file
    RMGR->scene->Update();

qdbg("Reset()\n");
    // Release some resources (cars)
    RMGR->scene->Reset();
    car=0;
qdbg("  network clients=%d\n",RMGR->network->clients);

qdbg("Re-setup\n");
    //exitfunc();
    Setup(FALSE);
  }
}

