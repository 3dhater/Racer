/*
 * Racer - Select car
 * 21-07-2001: Created! (00:12:41)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/dir.h>
#include <qlib/timer.h>
#include <qlib/profile.h>
#include <d3/mesh.h>
#include <d3/global.h>
#include <raceru/window.h>
DEBUG_ENABLE

#define MAX_CAR       1000

// Turn light off when no car is present?
//#define USE_LIGHT_SWITCH_FX

// Fonts to use
#define FONT_STATS    RMenuGetFont(0)

// Time (ms) to wait for car loading (for interactive selection)
#define DELAY_LOAD    750

struct CarInfo
{
  qstring dirName;             // Directory
  qstring name;
  int     year;
};

static CarInfo carInfo[MAX_CAR];
static int     cars;
static int     curCar;              // Current selection

// Graphics
static bool    fRequestLoad;        // Really load the car?
static RCar   *car;                 // Preview car
static float   carScale;            // So it always fits the display
static DBitMapTexture *texFloor;
static DMesh  *mFloor;

// Animation
static bool fWireFrame;
static QTimer *tmr;

/**************
* Car finding *
**************/
static void LoadCars()
// Search for every car and get info
{
  QDirectory *dir;
  QFileInfo   fi;
  char fname[256],buf[256];
  QInfo *info;
  CarInfo *ci;
  
  cars=0;
  dir=new QDirectory("data/cars");
  while(dir->ReadNext(fname,&fi))
  {
    // Exclude special names
    strcpy(buf,fname); strlwr(buf);
    if(!strcmp(buf,".."))continue;
    if(!strcmp(buf,"."))continue;
    if(!strcmp(buf,"default"))continue;
//qdbg("car? '%s'\n",fname);

    // Try to load info
    sprintf(buf,"data/cars/%s/car.ini",fname);
    if(!QFileExists(buf))continue;
    info=new QInfo(buf);
    ci=&carInfo[cars];
    ci->dirName=fname;
    ci->name=info->GetStringDirect("car.name","<untitled>");
    ci->year=info->GetInt("car.year");
//qdbg("  name '%s'\n",ci->name.cstr());
    QDELETE(info);
    cars++;
  }
  QDELETE(dir);
}

/********
* Paint *
********/
void CarPaint()
{
  float d=992;
  float ya;
  float carHgt=0;
  int   i;
  char  buf[256];
  CarInfo *ci;
  QRect r,r2;

  RMenuPaintGlobal(1);
  
  if(car)
  {
    // Determine car height
    RSuspension *susp=car->GetSuspension(0);
    carHgt=susp->GetLength()-susp->GetPosition()->y+
      car->GetWheel(0)->GetRadius();
#ifdef OBS
qdbg("suspLen=%f, suspPosY=%f, wheelRad=%f\n",
susp->GetLength(),susp->GetPosition()->y,car->GetWheel(0)->GetRadius());
#endif
    // Wheels must fake a surface underneath
    for(i=0;i<car->GetWheels();i++)
    {
//qdbg("wheel%d; y=%f\n",i,car->GetWheel(i)->GetSurfaceInfo()->y);
      car->GetWheel(i)->GetSurfaceInfo()->y=0;
    }
//qdbg("carHgt=%f\n",carHgt);
  }

  QCV->Set3D();
  //QCV->Clear();

  glPushAttrib(GL_ENABLE_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  
  // Car state
  glShadeModel(GL_SMOOTH);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  
  // Get nearer
  glPushMatrix();
  glTranslatef(0.5,-0.5,d);
  
  // Rotate around
  ya=tmr->GetMilliSeconds()/50.0f;
  glRotatef(20,1,0,0);
  glRotatef(ya,0,1,0);
  
  // Specular fun
  QCV->GetGLContext()->SetSeparateSpecularColor(TRUE);

  // Reflection
  glFrontFace(GL_CW);
  glPushMatrix();
  glScalef(carScale,-carScale,carScale);
  glEnable(GL_NORMALIZE);
  glTranslatef(0,carHgt,0);
  if(fWireFrame)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  if(car)car->Paint();
  glPopMatrix();
  glFrontFace(GL_CCW);
  //glScalef(1,1,1);
  if(fWireFrame)glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
 
#define OPT_FLOOR
#ifdef OPT_FLOOR
  // Paint floor
  glDisable(GL_LIGHTING);
  // FerP4 envmapping
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glPushMatrix();
  // Don't rotate the floor?
  glRotatef(-ya,0,1,0);
  glRotatef(-90,1,0,0);
#ifdef USE_LIGHT_SWITCH_FX
  if(!car)
  {
    mFloor->GetPoly(0)->SetOpacity(0.3);
  } else
  {
    mFloor->GetPoly(0)->SetOpacity(1);
  }
#endif
  mFloor->Paint();
  glPopMatrix();
  glEnable(GL_LIGHTING);
#endif
 
#define OPT_UPPER_CAR
#ifdef OPT_UPPER_CAR
  // Move wheels just above floor
  glPushMatrix();
  glScalef(carScale,carScale,carScale);
  if(car)car->PaintShadow();
  glTranslatef(0,carHgt,0);
  glEnable(GL_LIGHTING);
  if(fWireFrame)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  if(car)car->Paint();
  if(fWireFrame)glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glPopMatrix();
#endif
  
  // Restore
  glPopMatrix();
  glPopAttrib();
  // F1 Sauber restore OGL state extra
  //glColor4f(1,1,1,1);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

  QCV->GetGLContext()->SetSeparateSpecularColor(FALSE);

  // Paint info on the car
  //
  QCV->Set2D();
  glDisable(GL_LIGHTING);
  glColor3f(1,1,1);
  ci=&carInfo[curCar];
  r.x=50; r.y=150; r.wid=r.hgt=0;
  RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,ci->name,r2.x,r2.y);
  r.y-=20; RScaleRect(&r,&r2);
  if(!ci->year)strcpy(buf,"Year: unknown");
  else sprintf(buf,"%d",ci->year);
  rrPaintText(FONT_STATS,buf,r2.x,r2.y);
  
#ifdef OBS
  // Paint help
  txt="ENTER to select, ESC to cancel, select with left/right";
  // First get scaled size of text
  r.x=800/2; r.y=r.hgt=0; r.wid=FONT_STATS->GetWidth(txt);
  RScaleRect(&r,&r2);
  r.x=r2.x-r2.wid/2; r.y=40; r.wid=r.hgt=0;
  RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,txt,r2.x,r2.y);
#endif
  
  // Logo on top
  RMenuPaintLogo();

  // GUI
  rrPaintGUI();

  // Show it
  //RMenuSwap();
}

/***************
* Select a car *
***************/
static void FindCurCar()
// Try to find currently selected car (in the ini) in the list of cars
{
  int i;
  qstring car;
  
  RMGR->GetMainInfo()->GetString("dev.car",car);
  for(i=0;i<cars;i++)
  {
//qdbg("car %s, dir=%s?\n",car.cstr(),carInfo[i].dirName.cstr());
    if(!strcmp(carInfo[i].dirName,car))
    {
      curCar=i;
      return;
    }
  }
  // No car found
  curCar=0;
}
static void CarSelect(int n)
{
qdbg("CarSelect(%d)\n",n);
  // Delete old car
  QDELETE(car);

  // Clear the texture cache
  dglobal.texturePool->Clear();
  
  // Select car
  if(n<0||n>=cars)return;
  curCar=n;
  
  // Delay load until the user is idle for a moment (to avoid
  // lengthy times to select a car)
  fRequestLoad=FALSE;
  
  // Start countdown to car loading
  tmr->Reset();
  tmr->Start();
}
static bool LoadCar()
// Load the selected car for real
{
  // Load new car
#ifdef OBS
  qprofiler.Reset();
  qprofiler.Start();
#endif

qdbg("LoadCar: dirName=%s\n",carInfo[curCar].dirName.cstr());
  dglobal.texturePool->Clear();
  dglobal.EnableTexturePool();
  car=new RCar(carInfo[curCar].dirName);
  dglobal.DisableTexturePool();
 
#ifdef OBS
  qprofiler.Report();
#endif

  // Scale model down so it's always nicely visible
  DBox box;
  DVector3 size;
  float    max,newScale;
  car->GetBody()->GetModel()->GetGeode()->GetBoundingBox(&box);
  box.GetSize(&size);
  // Find largest extent
  if(size.x>size.y)max=size.x;
  else             max=size.y;
  if(size.z>max)max=size.z;
qdbg("Max size=%f\n",max);
  // Make sure to only reduce scale so the relative sizes of the cars
  // is visible when scrolling through them.
  if(max<D3_EPSILON)newScale=1;
  else              newScale=4.0/max;
  if(newScale<carScale)
  {
    carScale=newScale;
qdbg("New scale=%.2f\n",carScale);
  }
  return TRUE;
}

/*************
* Work procs *
*************/
static void idlefunc()
{
  // Time to load the car?
  if(fRequestLoad==FALSE)
  {
    if(tmr->GetMilliSeconds()>DELAY_LOAD)
    {
      fRequestLoad=TRUE;
      LoadCar();
    }
  }
  
  // Paint the car and all that
  CarPaint();
  RMenuSwap();
}

static void Do()
{
  int key;
  QEvent *e;

qdbg("Do()\n");

  while(1)
  {
    idlefunc();

    CarPaint();
    RMenuSwap();

    while(QEventPending())
    {
      key=rrInKey();
      // Also listen to GUI
      e=app->GetCurrentEvent();
      if(e->type==QEvent::CLICK)
      {
        if(e->win==bCmd[0])key=QK_LEFT;
        if(e->win==bCmd[1])key=QK_RIGHT;
        if(e->win==bCmd[2])key=QK_ENTER;
        if(e->win==bCmd[3])key=QK_ESC;
      }

      if(key==QK_LEFT)
      {
        // Prev car
        if(curCar>0)
        {
          curCar--;
         do_car:
          CarSelect(curCar);
        }
      } else if(key==QK_RIGHT)
      {
        if(curCar<cars-1)
        {
          curCar++;
          goto do_car;
        }
      } else if(key==QK_W)
      {
        // Toggle wireframe (gadget)
        fWireFrame=!fWireFrame;
      } else if(key==QK_ESC)
      {
        // Cancel car choice
        goto leave;
      } else if(key==QK_ENTER||key==QK_KP_ENTER)
      {
        // Confirm car choice, write back choice in prefs file
  //qdbg("selected car '%s'\n",carInfo[curCar].dirName.cstr());
        RMGR->GetMainInfo()->SetString("dev.car",
          carInfo[curCar].dirName.cstr());
        // Make sure the settings stick
        RMGR->GetMainInfo()->Write();
        
        goto leave;
      }
    }
  }
 leave:;
}

/***********
* High-end *
***********/
void rrSelectCar()
// Select a car from the available ones
{
qdbg("rrSelectCar\n");
  // Take over idle proc
  app->SetIdleProc(idlefunc);
  
  SetupButtons();
QDELETE(lbSel);             // It's not in use for now
  tmr=new QTimer();
 
  // Start with max scale
  carScale=1000;

  // Get floor polygon
  QImage img("data/images/floorlight.tga");
  if(img.IsRead())
    texFloor=new DBitMapTexture(&img);
  mFloor=new DMesh();
  mFloor->DefineFlat2D(10,10);
  QRect r(0,0,img.GetWidth(),img.GetHeight());
  mFloor->GetPoly(0)->DefineTexture(texFloor,&r);
  mFloor->EnableCSG();
  mFloor->GetPoly(0)->SetBlendMode(DPoly::BLEND_SRC_ALPHA);
  
  RMenuSetTitle("Select a car");
  LoadCars();
  FindCurCar();
  CarSelect(curCar);
  Do();
  
  // Free resources
  DestroyButtons();
  app->SetIdleProc(0);
  QDELETE(texFloor);
  QDELETE(mFloor);
  QDELETE(tmr);
  QDELETE(car);
}
