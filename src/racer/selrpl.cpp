/*
 * Racer - Select replay
 * 15-12-01: Created!
 * NOTES:
 * - Based heavily on seltrk.cpp
 * FUTURE:
 * - In FindCurReplay(), retrieve the last replay and set the listbox (lbSel)
 * to that item (and check for overflow and that the item is visible).
 * (C) Dolphinity/RvG
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

#define MAX_RPL       1000

// Rotate track preview mesh?
//#define USE_ROTATE

// Fonts to use
#define FONT_STATS    RMenuGetFont(0)

// Time (ms) to wait for track loading (for interactive selection)
#define DELAY_LOAD    100

struct RplInfo
{
  qstring name;                // Directory
  qstring version;             // Version string
  qstring track;               // Which track?
  int     interval,            // Replay freq
          cars;                // Number of cars present
};

extern bool fViewReplay;

// GUI
static DTexFont *font[2];           // From menu.cpp

static RplInfo rplInfo[MAX_RPL];
static int     replays;
static int     curRpl;              // Current selection
//static RStats  stats;               // Best laps and such

// Graphics
static DBitMapTexture *texBgr;

static void LocalSetupButtons()
{
  QRect r;
  int i;

  for(i=0;i<2;i++)
    font[i]=RMenuGetFont(i);

  // Resize listbox
  QDELETE(lbSel);
  r.wid=(int)RScaleX(700);
  r.hgt=(int)RScaleX(420);
  r.x=(int)RScaleX(50);
  r.y=(int)RScaleY(100);
  lbSel=new RListBox(QSHELL,&r,0,font[0]);

  // Modify button text (which is "Select")
  bCmd[2]->SetText("View");
}

/*****************
* Replay finding *
*****************/
static void LoadReplays()
// Search for every replay and get info
{
  QDirectory *dir;
  QFileInfo   fi;
  char fname[256],buf[256];
  //QInfo *info;
  RplInfo *ci;
  RReplayHeader hdr;
  char *s;

  replays=0;
  dir=new QDirectory("data/replays");
  while(dir->ReadNext(fname,&fi))
  {
    // Exclude special names
    if(!strcmp(fname,".."))continue;
    if(!strcmp(fname,"."))continue;
//qdbg("car? '%s'\n",fname);

    // Try to load info; if it failed, there's something wrong with
    // the replay.
    sprintf(buf,"data/replays/%s",fname);
    if(!RMGR->replay->LoadHeader(buf,&hdr))
    {
      qdbg("Can't load '%s'\n",buf);
      continue;
    }
    ci=&rplInfo[replays];
    // Cut off the .rpl
    strcpy(buf,fname);
    s=(string)QFileExtension(buf);
    if(s)*s=0;
    ci->name=buf;

    sprintf(buf,"v%d.%d",hdr.versionMajor,hdr.versionMinor);
    ci->version=buf;
    ci->track=hdr.trackName;
    ci->interval=hdr.interval;
    ci->cars=hdr.cars;

    sprintf(buf,"%s - %s - %dHz, %d cars (%s)",ci->name.cstr(),
      ci->track.cstr(),ci->interval,ci->cars,
      ci->version.cstr());
    lbSel->AddString(buf);
    replays++;
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
void RplPaint()
{
  //int   dy,x0;
  //char  buf[256];
  //RplInfo *ci;
  QRect r,r2;
 
  RMenuPaintGlobal(0);
 
#ifdef ND_REPLAY_INFO
  //
  // Paint info on the replay
  //
  x0=50; dy=20;
  QCV->Set2D();
  glDisable(GL_LIGHTING);
  glColor3f(1,1,1);
  ci=&rplInfo[curRpl];
  r.x=x0; r.y=130; r.wid=r.hgt=0;
  RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,ci->name,r2.x,r2.y);
  r.y-=dy; RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,ci->year,r2.x,r2.y);
  r.y-=dy; RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,ci->length,r2.x,r2.y);
  r.y-=dy; RScaleRect(&r,&r2);
  rrPaintText(FONT_STATS,ci->creator,r2.x,r2.y);
#endif

  // Finally the GUI
  rrPaintGUI();
  // Logo on top
  RMenuPaintLogo();
}

/*****************
* Select a track *
*****************/
static void FindCurReplay()
// Find the last played replay.
{
  // No 'last replay' is stored yet
  curRpl=0;
}
static bool LoadReplay()
// Load the selected track specs for real
{
  // Load the replay header
qdbg("LoadReplay: name=%s\n",rplInfo[curRpl].name.cstr());
  
  return TRUE;
}
static void RplSelect(int n)
// Select a replay
{
qdbg("RplSelect(%d)\n",n);
  if(n<0||n>=replays)return;
  curRpl=n;
  lbSel->Select(curRpl);
  LoadReplay();
}

/*************
* Work procs *
*************/
static void idlefunc()
{
}

static void Do()
{
  int key;
  QEvent *e;

  while(1)
  {
    RplPaint();
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
          if(lbSel->GetSelectedItem()!=curRpl)
          {
            curRpl=lbSel->GetSelectedItem();
            goto do_trk;
          }
        }
      }
if(key!=-1)qdbg("key=%d, QK_ESC=%d\n",key,QK_ESC);
      if(key==QK_LEFT)
      {
        // Prev car
        if(curRpl>0)
        {
          curRpl--;
         do_trk:
          RplSelect(curRpl);
        }
      } else if(key==QK_RIGHT)
      {
        if(curRpl<replays-1)
        {
          curRpl++;
          goto do_trk;
        }
      } else if(key==QK_ESC)
      {
        // Cancel choice
        goto leave;
      } else if(key==QK_ENTER||key==QK_KP_ENTER)
      {
        // Confirm car choice, write back choice in prefs file
qdbg("selected trk '%s'\n",rplInfo[curRpl].name.cstr());
        RMGR->GetMainInfo()->SetString("ini.track",
          rplInfo[curRpl].track.cstr());
        RMGR->GetMainInfo()->SetString("ini.replay",
          rplInfo[curRpl].name.cstr());
        // Make sure the settings stick
        RMGR->GetMainInfo()->Write();
        // Notify main menu we want a replay
        fViewReplay=TRUE;
        goto leave;
      }
    }
  }
 leave:;
}

/***********
* High-end *
***********/
bool rrSelectReplay()
// Select a car from the available ones
{
qdbg("rrSelectReplay\n");

  // Init variables
  fViewReplay=FALSE;               // No replay to be viewed

  // Take over idle proc
  app->SetIdleProc(idlefunc);
 
  SetupButtons();
  LocalSetupButtons();

  RMenuSetTitle("View replay");
  LoadReplays();
  FindCurReplay();
  RplSelect(curRpl);
  Do();
  
  // Free resources
  DestroyButtons();
  app->SetIdleProc(0);

  return fViewReplay;
}
