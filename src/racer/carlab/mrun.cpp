/*
 * Racer - Carlab
 * 01-12-00: Created!
 * NOTES:
 * - Modifying existing and creating new cars
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <d3/global.h>
#include <qlib/timer.h>
#include <qlib/dxjoy.h>
#include <GL/glu.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define DRW        Q_BC
#define CAR_NAME  "devtest"
//#define CONTROLS_FILE   "controls.ini"

// Use some default things to get the car running
// at something near-real life.
#define TIME_PER_INTEGRATION    10

// Maximum time to emulate per gfx frame
#define MAX_SIMTIME_PER_FRAME 100

#define DEBUG_INI  "debug.ini"

RCar         *car;
RTrackFlat   *trackFlat;

QTimer *tmr;

// GUI
enum { MAX_MODIFY=2 };
QButton *butModify[MAX_MODIFY];

// Controller info
int ctlSteer;
int ctlThrottle;
int ctlBrakes;

// Hack for rcontrol
char keyState[256];

static int  tSimTime;             // Simulated time in milliseconds
static bool fAnimatePhysics=TRUE;

// Editing
int curCam,
    curWheel,
    curSusp;
int curMode,curSubMode;

cstring sMode[]={ "Drive","Camera" };
cstring sSubMode[]={ "Offset","Angle" };

void exitfunc()
{
  int i;

  for(i=0;i<MAX_MODIFY;i++)QDELETE(butModify[i]);
  if(RMGR)delete RMGR;
#ifdef Q_USE_FMOD
  // Should not be needed
  FSOUND_Close();
#endif
}

/*********
* EVENTS *
*********/
static void MapXYtoCtl(int x,int y)
// Convert X/Y coords to controller input
{
  x-=DRW->GetWidth()/2;
  y-=DRW->GetHeight()/2;
#ifndef WIN32
  //if(!(rdbg->flags&RF_NO_STEERING))
  { 
    // Mouse
    ctlSteer=-x*3;
  }
  {
    // Combined axis simulation
    if(y<0)
    { ctlThrottle=-y*4;
      ctlBrakes=0;
      if(ctlThrottle>1000)
        ctlThrottle=1000;
    } else
    { ctlThrottle=0;
      ctlBrakes=y*4;
      if(ctlBrakes>1000)
        ctlBrakes=1000;
    }
  }
#endif
}

bool event(QEvent *e)
{
  static int  butsDown;
  static int  dragX,dragY;
  RCamera *cam;
  int i;

#ifdef OBS
if(e->type!=QEvent::MOTIONNOTIFY)
qdbg("event %d, n=%d\n",e->type,e->n);
#endif

  if(PartsEvent(e))return TRUE;
  
  cam=car->GetCamera(curCam);
  
  if(e->type==QEvent::MOTIONNOTIFY)
  {
    if(e->win!=Q_BC)return FALSE;
    
    if(curMode==MODE_DRIVE)
    {
      // Only move when the mouse is down; it's a drag otherwise
      if((butsDown&1)==1)
        MapXYtoCtl(e->x,e->y);
    } else if(curMode==MODE_CAM)
    {
      // Edit the camera
      if(butsDown&1)
      {
        if(curSubMode==SUBMODE_OFFSET)
        {
          cam->offset.x+=((rfloat)(e->x-dragX))*1./640.;
          cam->offset.y+=((rfloat)(dragY-e->y))*1./480.;
        } else
        {
          cam->angle.y+=((float)(e->x-dragX))*100./576.;
          cam->angle.x+=((float)(e->y-dragY))*100./720.;
        }
      } else if(butsDown&4)
      {
        if(curSubMode==SUBMODE_OFFSET)
        {
          cam->offset.z+=(e->x-dragX)*.005;
        } else
        {
          cam->angle.z+=(e->x-dragX)*50./576.;
        }
      }
      dragX=e->x;
      dragY=e->y;
    }
  } else if(e->type==QEvent::BUTTONPRESS)
  {
    butsDown|=(1<<(e->n-1));
    dragX=e->x; dragY=e->y;
  } else if(e->type==QEvent::BUTTONRELEASE)
  {
    butsDown&=~(1<<(e->n-1));
  } else if(e->type==QEvent::KEYPRESS)
  {
    if(e->n==QK_ESC)
      app->Exit(0);
    else if(e->n==QK_A)
      Analyse();
  } else if(e->type==QEvent::CLICK)
  {
    for(i=0;i<MAX_MODIFY;i++)
    {
      if(e->win==butModify[i])
      { switch(i)
        { case 0: Analyse(); break;
          case 1: ModifyWeightRatio(); break;
        }
      }
    }
  }

  return FALSE;
}

/***********
* HANDLING *
***********/
void SetupViewport(int w,int h)
{
  float fov;

  //fov=RMGR->scene->GetCamAngle();
  fov=car->GetCamera(curCam)->GetLensAngle();
//qdbg("fov=%f\n",fov);
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(fov,(GLfloat)w/(GLfloat)h,0.1,1000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

QDraw *GGetDrawable()
{
  return DRW;
}

QCanvas *GGetCV()
{
  return DRW->GetCanvas();
}
void GSwap()
{ DRW->Swap();
}

static void Camera()
{
  car->GetCamera(curCam)->Go();
}

/*************
* CONTROLLER *
*************/
static void PaintVar(cstring name,int value,int *x,int *y)
{
  QCanvas *cv=GGetCV();
  char buf[80];
  
  sprintf(buf,"%s: %d",name,value);
  cv->Text(buf,*x,*y);
  *y+=16;
}
static void PaintVar(cstring name,double value,int *x,int *y)
{
  QCanvas *cv=GGetCV();
  char buf[80];
  
  sprintf(buf,"%s: %.3f",name,value);
  cv->Text(buf,*x,*y);
  *y+=16;
}
static void PaintVar(cstring name,cstring value,int *x,int *y)
{
  QCanvas *cv=GGetCV();
  char buf[80];
  
  sprintf(buf,"%s: %s",name,value);
  cv->Text(buf,*x,*y);
  *y+=16;
}
#define _PV(s,v) PaintVar(s,v,&x,&y)
static void PaintControllerStatus()
// Show different vars
{
  QCanvas *cv=GGetCV();
  char buf[80];
  int  x,y;
  
  cv->Set3D();
  cv->Set2D();
  x=20; y=20;
  cv->SetFont(app->GetSystemFont());
  _PV("Simulated time (s)",tSimTime/1000);
  _PV("Current part",partName[curPart]);
  _PV("Mouse mode",sMode[curMode]);
  _PV("Submode",sSubMode[curSubMode]);
}

/*******
* IDLE *
*******/
void UpdateControllers()
{
  RMGR->controls->Update();
#ifdef WIN32
  // Add both steering controls
  ctlSteer=RMGR->controls->control[RControls::T_STEER_LEFT]->value-
           RMGR->controls->control[RControls::T_STEER_RIGHT]->value;
  ctlThrottle=RMGR->controls->control[RControls::T_THROTTLE]->value;
  ctlBrakes=RMGR->controls->control[RControls::T_BRAKES]->value;
#endif
}

void idlefunc()
{
  DVector3 *v;
 
#ifdef linux
// OpenGL bug on Linux/nVidia/Geforce2MX
glFlush();
#endif
  // Don't refresh while window is disabled. This happens
  // when a dialog gets on top, and redrawing doesn't work
  // correctly on a lot of buggy drivers on Win98.
  if(!Q_BC->IsEnabled())return;

  // Update controller status
  UpdateControllers();

  // Physics
  if(fAnimatePhysics)
    RMGR->SimulateUntilNow();
  
  GGetCV()->Select();
  SetupViewport(DRW->GetWidth(),DRW->GetHeight());
  GGetCV()->SetFont(app->GetSystemFont());
  glClearColor(.1,.4,.1,0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glEnable(GL_CULL_FACE);
  //glCullFace(GL_FRONT);
  //glFrontFace(GL_CW);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  float lightDir[3]={ 0,-.1,-1 };
  //float lightPos[4]={ 1,1,1,0 };
  float lightPos[4]={ 0.01,.01,-1000,0 };
  //glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,lightDir);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_FLAT);
  
  // Camera
  Camera();
  glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
  //glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,lightDir);

  v=car->GetPosition();
  RGfxAxes(v->x,v->y+.01,v->z,1);
  //glTranslatef(0,-1.66,-5.0);
  //glRotatef(15,1,0,0);
#ifdef ND_MOVING_CAM
  static float vx=.05;
  static float y=2.5;
  y+=vx; if(y>5||y<1)vx=-vx;
  gluLookAt(y*2-6,y /*1.66*/,-5.0, 0,0,0, 0,1,0);
#endif
    
  car->SetDefaultMaterial();
  RMGR->scene->Paint();

#ifdef OBS
  // Cars
  car->Paint();
#endif

  // Info
  PaintControllerStatus();
  
  GSwap();
}

void SetupUI()
{
  int   i;
  QRect r;
  cstring modStr[]={ "(A)nalyse","Modify weight ratio" };

  // Modify
  r.x=Q_BC->GetX()+Q_BC->GetWidth()+10;
  r.y=Q_BC->GetY()+Q_BC->GetHeight()+10;
  r.wid=150; r.hgt=30;
  for(i=0;i<MAX_MODIFY;i++)
  {
    butModify[i]=new QButton(QSHELL,&r,modStr[i]);
    r.y+=r.hgt+5;
  }
}
void Setup()
{
  float v;
  qstring s;
  
  SetupUI();

  // Create Racer manager
  new RManager();
  RMGR->Create();

  // Create a scene
  RMGR->scene->env=new REnvironment();
  RMGR->LoadTrack(info->GetStringDirect("misc.track"));
#ifdef OBS
  trackFlat=new RTrackFlat();
  trackFlat->SetName("devflat");
  if(!trackFlat->Load())
  { qerr("Can't load 'devflat' track");
    app->Exit(0);
  }
  RMGR->scene->SetTrack(trackFlat);
#endif
  
  // Load last used car
  info->GetString("misc.lastcar",s,"devtest");
  car=new RCar(s);
  car->Warp(&RMGR->track->gridPos[0]);
  tmr=new QTimer();

  // Add car to scene for painting
  RMGR->scene->AddCar(car);

  // Start debug info
  ctlSteer=RMGR->infoDebug->GetInt("car.steer");
  ctlThrottle=RMGR->infoDebug->GetInt("engine.throttle");
  ctlBrakes=RMGR->infoDebug->GetInt("engine.brakes");
  
  DRW->GetCanvas()->SetMode(QCanvas::DOUBLEBUF);

  PartsSetup();
  
  // Get window up
  app->RunPoll();
  app->SetIdleProc(idlefunc);
  app->SetExitProc(exitfunc);
  app->SetEventProc(event);

  // Now that the window is up, we can continue with some things
  RMGR->controls->LoadConfig();
}

void Run()
{
#ifdef OBS
DBG_TRACE_ON()
#endif

  Q_BC->GetQXWindow()->SetCursor(app->GetShellCursor());
  
  Setup();
  app->Run();
}
