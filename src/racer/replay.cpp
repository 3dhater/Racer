/*
 * Racer - Replay playback
 * 01-11-01: Created!
 * NOTES:
 * - Mainly uses the RReplay class to do the dirty work
 * - It's a bit of a mess when it comes to stopping at the boundaries
 * of the replay (making sure the time stays at those borders).
 * (c) Dolphinity/RvG
 */

#include "main.h"
#include <qlib/debug.h>
DEBUG_ENABLE
#include <racer/replay.h>
#include <raceru/button.h>
#include <qlib/timer.h>

// mrun.cpp import
extern bool fReplay;
void PaintScene(int flags);
void GSwap();

// Timing
static QTimer *tmr;
static int     timeBase;              // Time in replay when tmr time is 0
static float   timeFactor;            // Speed of replay time wrt real time

// Marking
static int mark[2],marks;

// Movie recording
static bool fDumping;                 // Generating screenshot images for a movie

// GUI
#define MAX_CONTROL    11
static RButton *bControl[MAX_CONTROL];
extern DTexFont *texfStat;

/********
* Setup *
********/
void ReplaySetup()
{
  int i,w=32,h=20;
  QRect r,rDisarmed,rArmed;
  cstring sControl[MAX_CONTROL]=
  { "|<","<<","<","Stop",">",">>",">|","Rec","Eject","Mark","Movie" };

  if(RMGR->GetMainInfo()->GetInt("replay.load_standard"))
  {
qdbg("Loading standard replay (test.rpl)\n");
    RMGR->replay->Load("data/replays/test.rpl");
  }

  for(i=0;i<MAX_CONTROL;i++)
  {
    r.x=210+i*(w+10);
    r.y=550;
    r.wid=w;
    r.hgt=h;
    RScaleRect(&r,&r);
    bControl[i]=new RButton(QSHELL,&r,sControl[i],texfStat);
    rDisarmed.x=i*48;
    rDisarmed.y=0;
    rDisarmed.wid=48;
    rDisarmed.hgt=32;
    rArmed.x=i*48;
    rArmed.y=32;
    rArmed.wid=48;
    rArmed.hgt=32;
    bControl[i]->SetTexture(RMGR->texReplay,&rDisarmed,&rArmed);
  }
qdbg("buts, texfStat=%p\n",texfStat);

  if(!tmr)tmr=new QTimer();
  tmr->Reset();
  tmr->Start();

  timeBase=RMGR->replay->GetFirstFrameTime();
  timeFactor=1;
}
void ReplayDestroy()
{
  int i;

  QDELETE(tmr);
  for(i=0;i<MAX_CONTROL;i++)
    QDELETE(bControl[i]);
}

/***********************************
* Time waits for no-one, but Racer *
***********************************/
static int GetReplayTime()
// Returns the time where the replay should be
{
  return (int)(tmr->GetMilliSeconds()*timeFactor+timeBase);
}
static void GotoTime(int t)
// Move replay to time 't'
// If t==0, then the start is searched for (there may not be t=0)
{
  // Check for 't' to exist in the replay, or get the closest value
  //...

  // Go there
  timeBase=t;
  tmr->Reset();
  tmr->Start();
}
static void SetTimeFactor(float f)
{
  // Make sure time doesn't jump by restarting the timer
  timeBase=GetReplayTime();
qdbg("SetTimeFactor: new base=%d\n",timeBase);
  tmr->Reset();
  tmr->Start();
  // Set new time factor
  timeFactor=f;
qdbg("timeFactor=%.2f\n",f);
}

/*************
* Operations *
*************/
static void Play()
// Play is pressed; may switch to slomo
{
qdbg("Play: tf=%.2f\n",timeFactor);
  if(timeFactor==1.0f)SetTimeFactor(0.1f);
  else if(timeFactor==0.1f)SetTimeFactor(1.0);
  else
  {
    // Probably wasn't playing
    SetTimeFactor(1.0);
  }
}
static void Stop()
{
  SetTimeFactor(0);
  tmr->Stop();

//qdbg("Check timeBase (%d) against overlapse %d\n",timeBase,RMGR->replay->GetLastFrameTime());
  // Don't overflow/underflow the clock wrt the replay
  if(timeBase<RMGR->replay->GetFirstFrameTime())
  {
    // Time went on too long in a reverse play
    timeBase=RMGR->replay->GetFirstFrameTime();
  } else if(timeBase>RMGR->replay->GetLastFrameTime())
  {
    // Time went on too long in forward play
//qdbg("timeBase (%d) corrected to %d\n",timeBase,RMGR->replay->GetLastFrameTime());
    timeBase=RMGR->replay->GetLastFrameTime();
  }
}
static void Rewind()
// Faster backwards, or step if stopped
{
  if(timeFactor==0)
  {
    // Step
    timeBase-=100;
    Stop();
    return;
  }

  if(timeFactor==-1.0)SetTimeFactor(-2.0);
  else if(timeFactor==-2.0)SetTimeFactor(-3.0);
  else if(timeFactor==-3.0)SetTimeFactor(-4.0);
  else if(timeFactor==-4.0)SetTimeFactor(-5.0);
  else if(timeFactor==-5.0)SetTimeFactor(-6.0);
  else if(timeFactor==-6.0)SetTimeFactor(-7.0);
  else if(timeFactor==-7.0)SetTimeFactor(-8.0);
  else if(timeFactor==-8.0)SetTimeFactor(-9.0);
  else if(timeFactor==-9.0)SetTimeFactor(-10.0);
  else if(timeFactor==-10.0)SetTimeFactor(-10.0);   // Keep
  else 
  {
    SetTimeFactor(-1.0);
    // Restart skids (they can't be reverse-played)
    RMGR->smm->Clear();
  }
}
static void FastForward()
// Faster, or step if stopped
{
  if(timeFactor==0)
  {
    // Step
    timeBase+=100;
    Stop();
    return;
  }

  if(timeFactor==1.0)SetTimeFactor(2.0);
  else if(timeFactor==2.0)SetTimeFactor(3.0);
  else if(timeFactor==3.0)SetTimeFactor(4.0);
  else if(timeFactor==4.0)SetTimeFactor(5.0);
  else if(timeFactor==5.0)SetTimeFactor(6.0);
  else if(timeFactor==6.0)SetTimeFactor(7.0);
  else if(timeFactor==7.0)SetTimeFactor(8.0);
  else if(timeFactor==8.0)SetTimeFactor(9.0);
  else if(timeFactor==9.0)SetTimeFactor(10.0);
  else if(timeFactor==10.0)SetTimeFactor(10.0);   // Keep
  else SetTimeFactor(1.0);
}
static void SaveReplay()
{
qdbg("Saving replay\n");
  char buf[128],buf2[1024];
  bool r;

  strcpy(buf,"test");
  r=QDlgString("Save replay","Enter replay name",buf,sizeof(buf));
  if(r==FALSE)return;
  sprintf(buf2,"data/replays/%s.rpl",buf);
  // Save subrange if present
#ifdef ND_FUTURE
  // A bug is still present; car-enter commands are missing in the replay, so no car
  // is allocated. This means I have to store the cars and ALWAYS new them, upfront.
  if(marks==2)
    RMGR->replay->Save(buf2,mark[0],mark[1]);
  else
#endif
    RMGR->replay->Save(buf2);
}
static void MarkPoint()
// Set a mark at the current point
{
  if(marks==2)
  {
    // 2 marks were set; clear
    marks=0;
    RCON->printf("Marks cleared.\n");
  } else
  {
    mark[marks]=GetReplayTime();
    if(marks==0)RCON->printf("Mark IN set.\n");
    else        RCON->printf("Mark OUT set.\n");
    marks++;
  }
  // Make sure console is shown
  RCON->Show();
}
static void ExportPics()
// Create image files to create replay movies from.
// Uses mark points if present.
{
qdbg("Export pics replay\n");
  char buf[1024];
  bool r;
  int    tStart,tEnd,tStep,t;
  int    index;
  rfloat freq;
  QBitMap *bm;

  Stop();

  // Get frequency
  strcpy(buf,"15");
 retry:
  r=QDlgString("Save images","Enter movie frequency (in Hz)",buf,sizeof(buf));
  if(r==FALSE)return;
  freq=atof(buf);
  if(freq<0.1)goto retry;

  // Decide start/end time
  if(marks==2)
  {
    // Use marks
    tStart=mark[0];
    tEnd=mark[1];
  } else
  {
    // Do entire replay
    tStart=RMGR->replay->GetFirstFrameTime();
    tEnd=RMGR->replay->GetFirstFrameTime();
  }
  tStep=int(1000.0f/freq);

  // Loop through all the frames; don't use QSHELL->ScreenShot(), but allocate a bitmap
  // just once for some extra performance.
  bm=new QBitMap(32,QSHELL->GetWidth(),QSHELL->GetHeight());
  fDumping=TRUE;
  RCON->Hide();
  for(t=tStart,index=0;t<=tEnd;t+=tStep)
  {
    if(!RMGR->replay->Goto(t))
      break;
    RMGR->scene->OnGfxFrame();
    // Paint the scene, but NOT the GUI (for nicer movies), and no info
    PaintScene(1);
    GSwap();

    // Grab the image and save
    glReadBuffer(GL_FRONT);
    QCV->ReadPixels(bm);
    glReadBuffer(GL_BACK);
    sprintf(buf,"data/dump/replay%05d.tga",index);
    bm->Write(buf,QBMFORMAT_TGA);
    index++;

    // Handle events; notice this gets a bit complicated; the mrun.cpp's
    // event handler will see events, set fReplay to FALSE if ESC is pressed,
    // and other events are routed to ReplayEvent().
    app->Run1();
    if(!fReplay)
    { // ESC was pressed; break
      fReplay=TRUE;                 // But don't leave replay mode
      break;
    }
  }
  // Free resources
  QDELETE(bm);
  fDumping=FALSE;
}
void ReplayOp(int cmd)
// One of the buttons is pressed
{
  switch(cmd)
  {
    case 0 : GotoTime(0); break;
    case 1 : Rewind(); break;
    case 2 : SetTimeFactor(-1); break;
    case 3 : Stop(); break;
    case 4 : Play(); break;
    case 5 : FastForward(); break;
    //case 6 : GotoEnd(); break;
    case 7 : SaveReplay(); break;
    case 8 : Stop(); SaveReplay(); break;
    case 9 : MarkPoint(); break;
    case 10: ExportPics(); break;
    default: break;
  }
}

/*********
* Events *
*********/
bool ReplayEvent(QEvent *e)
{
  int i;

  // While dumping (ExportPics()), don't listen to anything
  if(fDumping)return FALSE;

  if(e->type==QEvent::CLICK)
  {
    for(i=0;i<MAX_CONTROL;i++)
    {
      if(e->win==bControl[i])
        ReplayOp(i);
    }
  } else if(e->type==QEvent::KEYPRESS)
  {
    // Shortcuts
    if(e->n==QK_HOME)ReplayOp(0);
    else if(e->n==QK_S)SaveReplay();
    else if(e->n==QK_SPACE)Stop();
    else if(e->n==QK_P)Play();
    else if(e->n==QK_LEFT)ReplayOp(1);
    else if(e->n==QK_RIGHT)ReplayOp(5);
    else if(e->n==QK_M)ReplayOp(9);
    //else if(e->n==QK_X)ReplayOp(8);
  }
  return FALSE;
}

static void Reset()
{
  // Clear marks
  marks=0;
}

void Replay()
{
  int t;

//qdbg("Replay mode\n");
//qdbg("  first frame: %p, currentFrame=%p\n",
//RMGR->replay->GetFirstFrame(),RMGR->replay->GetCurrentFrame());

  RMGR->musicMgr->PlayRandom(RMusicManager::REPLAY);

  fReplay=TRUE;
  Reset();
  app->SetIdleProc(0);
  ReplaySetup();

  while(fReplay)
  {
    // Events (keep the pipe empty)
    // Note that mrun.cpp passes any events through to ReplayEvent()
    while(app->Run1()!=2);

    // Goto the current situation
    //if(tmr->IsRunning())
    {
      t=GetReplayTime();
//qdbg("Goto t=%d (base=%d, factor=%.2f)\n",t,timeBase,timeFactor);
      if(!RMGR->replay->Goto(t))
      {
        // Could not go there; probably the time is out of range.
        Stop();
      }
    }

    // Update cameras (not really the right place probably)
    RMGR->scene->OnGfxFrame();

//qdbg("Paint\n");
    // Paint the situation
    PaintScene(0);
    // GUI
    rrPaintGUI();
    GSwap();
  }

//qdbg("End of replay; return to normal\n");

  // Restore situation (car states)
  RMGR->smm->EndAllStrips();
  RMGR->replay->Goto(RMGR->replay->GetLastFrameTime()-1);
  RMGR->smm->EndAllStrips();

  ReplayDestroy();
}

