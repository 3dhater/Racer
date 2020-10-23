/*
 * Racer - Select track
 * 21-07-2001: Created! (00:12:41)
 * NOTES:
 * - Based heavily on selcar.cpp
 * (C) MarketGraph/RvG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/dir.h>
#include <qlib/timer.h>
#include <d3/mesh.h>
#include <raceru/button.h>
#include <raceru/listbox.h>
#include <raceru/window.h>
DEBUG_ENABLE

#define MAX_TRK       1000

// Rotate track preview mesh?
//#define USE_ROTATE

// Fonts to use
#define FONT_STATS    RMenuGetFont(0)

// Time (ms) to wait for track loading (for interactive selection)
#define DELAY_LOAD    100

struct TrkInfo
{
  qstring dirName;             // Directory
  qstring name;
  qstring year;
  qstring length;
  qstring creator;
};

// GUI
RButton *bCmd[4];
RListBox *lbSel;
extern DTexFont *font[2];           // From menu.cpp

static TrkInfo trkInfo[MAX_TRK];
static int     tracks;
static int     curTrk;              // Current selection
static RStats  stats;               // Best laps and such

// Graphics
static bool    fRequestLoad;        // Really load the track?
static DBitMapTexture *texFloor,*texPreview;
static QImage *imgPreview;
static DMesh  *mFloor,*mPreview;
static float   trkScale;

// Animation
static bool fWireFrame;
static QTimer *tmr;

void SetupButtons()
// Creates command buttons
{
  int i;
  QRect r;
  cstring sCmd[4]={ "Previous","Next","Select","Cancel" };

  for(i=0;i<4;i++)
  {
    r.wid=(int)RScaleX(100);
    r.hgt=int(RScaleX(30));
    r.y=int(RScaleY(550));
    r.x=int(RScaleX(170)+i*r.wid*120/100);
qdbg("cmdBut[%d]\n",i);
    bCmd[i]=new RButton(QSHELL,&r,sCmd[i],font[0]);
  }

  r.wid=RScaleX(250);
  r.hgt=RScaleX(300);
  r.y=RScaleY(230);
  r.x=RScaleX(550);
  lbSel=new RListBox(QSHELL,&r,0,font[0]);
}
void DestroyButtons()
// Creates command buttons
{
  int i;
  for(i=0;i<4;i++)
    QDELETE(bCmd[i]);
  QDELETE(lbSel);
}

/****************
* Track finding *
****************/
static void LoadTracks()
// Search for every track and get info
{
  QDirectory *dir;
  QFileInfo   fi;
  char fname[256],buf[256];
  QInfo *info;
  TrkInfo *ci;
  
  tracks=0;
  dir=new QDirectory("data/tracks");
  while(dir->ReadNext(fname,&fi))
  {
    // Exclude special names
    if(!strcmp(fname,".."))continue;
    if(!strcmp(fname,"."))continue;
    if(!strcmp(fname,"default"))continue;
//qdbg("car? '%s'\n",fname);

    // Try to load info
    sprintf(buf,"data/tracks/%s/track.ini",fname);
    if(!QFileExists(buf))continue;
    info=new QInfo(buf);
    ci=&trkInfo[tracks];
    ci->dirName=fname;
    ci->name=info->GetStringDirect("track.name","<untitled>");
    ci->year=info->GetStringDirect("track.year","<unknown year>");
    ci->length=info->GetStringDirect("track.length","<unknown length>");
    ci->creator=info->GetStringDirect("track.creator","<unknown creator>");
//qdbg("  name '%s'\n",ci->name.cstr());
    QDELETE(info);
    lbSel->AddString(ci->name);
    tracks++;
  }
  QDELETE(dir);
}

/********
* Paint *
********/
static void TimeToBuf(int time,string s)
{
  if(!time)sprintf(s,"---");
  else
    sprintf(s,"%d:%02d:%03d",time/60000,(time/1000)%60,time%1000);
}
static void DateToBuf(int t,string s)
// Convert time() date
{
  time_t tm;
  if(!t)
  {
    s[0]=0;
    return;
  }
  tm=t;
  strftime(s,256,"%d-%m-%y",localtime(&tm));
}
void TrkPaint()
{
  float d=994;
  float ya;
  int   i,dy,x0;
  char  buf[256];
  TrkInfo *ci;
  QRect r,r2;
  cstring txt;
  
  RMenuPaintGlobal(1);
 
  QCV->Set3D();

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
#ifdef USE_ROTATE
  ya=tmr->GetMilliSeconds()/50.0f;
#else
  ya=-15;
#endif
  glRotatef(15,1,0,0);
  glRotatef(ya,0,1,0);
  
  // Reflection
  glFrontFace(GL_CW);
  glPushMatrix();
  glScalef(trkScale,-trkScale,trkScale);
  //glTranslatef(0,0,0);
  if(fWireFrame)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  //if(car)car->Paint();
  if(mPreview)mPreview->Paint();
  glPopMatrix();
  glFrontFace(GL_CCW);
  if(fWireFrame)glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  
  // Paint floor
  glDisable(GL_LIGHTING);
  glPushMatrix();
  // Don't rotate the floor?
  glRotatef(-ya,0,1,0);
  glRotatef(-90,1,0,0);
  if(!tracks)
  {
    // Dark floor
    mFloor->GetPoly(0)->SetOpacity(0.3);
  } else
  {
    mFloor->GetPoly(0)->SetOpacity(1);
  }
  mFloor->Paint();
  glPopMatrix();
  glDisable(GL_LIGHTING);
  
  // Move wheels just above floor
  glPushMatrix();
  glScalef(trkScale,trkScale,trkScale);
  //if(car)car->PaintShadow();
  //glTranslatef(0,carHgt,0);
  if(fWireFrame)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  if(mPreview)mPreview->Paint();
  //if(car)car->Paint();
  if(fWireFrame)glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glPopMatrix();
 
  // Restore
  glPopMatrix();
  glPopAttrib();
 
  // Paint info on the track
  //
  x0=50; dy=20;
  QCV->Set2D();
  glDisable(GL_LIGHTING);
  glColor3f(1,1,1);
  ci=&trkInfo[curTrk];
  r.x=x0; r.y=130; r.wid=r.hgt=0;
  RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,ci->name,r2.x,r2.y);
  r.y-=dy; RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,ci->year,r2.x,r2.y);
  r.y-=dy; RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,ci->length,r2.x,r2.y);
  r.y-=dy; RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,ci->creator,r2.x,r2.y);
  
  // Paint stats
  //r.y=450;
  r.y=390;
  r.x=x0; RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,"Best laps",r2.x,r2.y);
  r.y-=dy*3/2;
  for(i=0;i<10&&i<RStats::MAX_BEST_LAP;i++)
  {
    r.x=x0; RScaleRect(&r,&r2);
    TimeToBuf(stats.lapTime[i],buf);
    rrPaintText(FONT_STATS,buf,r2.x,r2.y);
    r.x+=100; RScaleRect(&r,&r2);
    DateToBuf(stats.date[i],buf);
    rrPaintText(FONT_STATS,buf,r2.x,r2.y);
    r.x+=100; RScaleRect(&r,&r2);
    rrPaintText(FONT_STATS,stats.car[i],r2.x,r2.y);
    
    r.y-=dy;
  }
  
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

#ifdef OBS
  // Show it
  RMenuSwap();
#endif
}

/*****************
* Select a track *
*****************/
static void FindCurTrack()
// Try to find currently selected track (in the ini) in the list of tracks
{
  int i;
  qstring trk;
  
  RMGR->GetMainInfo()->GetString("ini.track",trk);
  for(i=0;i<tracks;i++)
  {
qdbg("track %s, dir=%s?\n",trk.cstr(),trkInfo[i].dirName.cstr());
    if(!strcmp(trkInfo[i].dirName,trk))
    {
      curTrk=i;
      return;
    }
  }
  // No track found
  curTrk=0;
}
static bool LoadTrack()
// Load the selected track specs for real
{
  char buf[256];
  
  // Load new track
qdbg("LoadTrack: dirName=%s\n",trkInfo[curTrk].dirName.cstr());
  
  // Read best laps
  {
    QInfo *info;
    sprintf(buf,"data/tracks/%s/bestlaps.ini",trkInfo[curTrk].dirName.cstr());
    info=new QInfo(buf);
    stats.Load(info,"results");
    QDELETE(info);
  }
  
  // Get preview image
  QDELETE(texPreview);
  sprintf(buf,"data/tracks/%s/preview.tga",trkInfo[curTrk].dirName.cstr());
  imgPreview=new QImage(buf);
  if(imgPreview->IsRead())
    texPreview=new DBitMapTexture(imgPreview);
  else
  {
    // Use stub image
    QDELETE(imgPreview);
    imgPreview=new QImage("data/images/trackstub.tga");
    texPreview=new DBitMapTexture(imgPreview);
  }

  QDELETE(mPreview);
  mPreview=new DMesh();
  mPreview->DefineFlat2D(3,2);
  mPreview->Move(0,1,0);
  QRect r(0,0,imgPreview->GetWidth(),imgPreview->GetHeight());
  mPreview->GetPoly(0)->DefineTexture(texPreview,&r);
  mPreview->EnableCSG();
  //mPreview->GetPoly(0)->SetBlendMode(DPoly::BLEND_SRC_ALPHA);
 
  // No need for image anymore
  QDELETE(imgPreview);

  // Anim in
  QTimer tmr;
  tmr.Start();
  while(1)
  {
    trkScale=((float)tmr.GetMilliSeconds())/200.0f;
    if(trkScale>1.0)trkScale=1.0;
    TrkPaint();
    RMenuSwap();
    if(trkScale>=1.0)break;
  }
  return TRUE;
}
static void TrkSelect(int n)
{
qdbg("TrkSelect(%d)\n",n);
  // Check 'n'
  if(n<0||n>=tracks)return;
  curTrk=n;
  lbSel->Select(curTrk);

  // Delay load until the user is idle for a moment (to avoid
  // lengthy times to select a track)
  fRequestLoad=FALSE;
  
  // Load NOW
  LoadTrack();

#ifdef OBS
  // Start countdown to track loading
  tmr->Reset();
  tmr->Start();
#endif
}

/*************
* Work procs *
*************/
static void idlefunc()
{
#ifdef OBS
  // Time to load the car?
  if(fRequestLoad==FALSE)
  {
    if(tmr->GetMilliSeconds()>DELAY_LOAD)
    {
      fRequestLoad=TRUE;
      LoadTrack();
    }
  }
  
  // Paint the car and all that
  TrkPaint();
#endif
}

static void Do()
{
  int key;
  QEvent *e;

qdbg("Do()\n");

  //LoadTrack();

  while(1)
  {
    TrkPaint();
    RMenuSwap();

    while(QEventPending())
    {
      key=rrInKey();
      // Also listen to GUI
      e=app->GetCurrentEvent();
      if(e->type==QEvent::CLICK)
      {
        if(e->win==bCmd[0])key=QK_LEFT;
        else if(e->win==bCmd[1])key=QK_RIGHT;
        else if(e->win==bCmd[2])key=QK_ENTER;
        else if(e->win==bCmd[3])key=QK_ESC;
        else if(e->win==lbSel)
        {
          if(lbSel->GetSelectedItem()!=curTrk)
          {
            curTrk=lbSel->GetSelectedItem();
            goto do_trk;
          }
        }
      }
if(key!=-1)qdbg("key=%d, QK_ESC=%d\n",key,QK_ESC);
      if(key==QK_LEFT)
      {
        // Prev car
        if(curTrk>0)
        {
          curTrk--;
         do_trk:
          TrkSelect(curTrk);
        }
      } else if(key==QK_RIGHT)
      {
        if(curTrk<tracks-1)
        {
          curTrk++;
          goto do_trk;
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
qdbg("selected trk '%s'\n",trkInfo[curTrk].dirName.cstr());
        RMGR->GetMainInfo()->SetString("ini.track",
          trkInfo[curTrk].dirName.cstr());
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
void rrSelectTrk()
// Select a car from the available ones
{
qdbg("rrSelectTrk\n");
  // Take over idle proc
  app->SetIdleProc(idlefunc);
 
  SetupButtons();
  tmr=new QTimer();
  
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
  
  RMenuSetTitle("Select a track");
  LoadTracks();
  FindCurTrack();
  TrkSelect(curTrk);
  Do();
  
  // Free resources
  DestroyButtons();
  app->SetIdleProc(0);
  QDELETE(texFloor);
  QDELETE(mFloor);
  QDELETE(texPreview);
  QDELETE(mPreview);
  QDELETE(tmr);
}
