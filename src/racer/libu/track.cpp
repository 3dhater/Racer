/*
 * RacerU - tracks
 * 27-02-01: Created!
 * (c) Dolphinity/Ruud van Gaal
 */

#include <racer/racer.h>
#include <raceru/all.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <qlib/progress.h>
DEBUG_ENABLE

// Progress bar for loading
static QProgress *prg;

static bool cbProgress(int cur,int total,cstring text)
{
//qdbg("cbprg(%d,%d,%d)\n",cur,total);
  prg->SetProgress(cur,total);
  // Note that the fullscreen texture may have upset
  // the drawing canvas, and we have to explicitly set
  // a 2D mode in the GUI canvas
  QCV->Set3D();
  QCV->Set2D();
  prg->GetCanvas()->SetMode(QCanvas::SINGLEBUF);
  //prg->GetQXWindow()->GetGLContext()->Select();
  //prg->GetQXWindow()->GetGLContext()->DrawBuffer(GL_FRONT);
  //prg->GetQXWindow()->GetGLContext()->Clear(GL_COLOR_BUFFER_BIT);
  prg->Paint();
  // Some Win32 OpenGL drivers draw a little too late
  glFinish();
  return TRUE;
}

static void rrScaleRect(const QRect *rIn,QRect *rOut)
// Mainly taken from menu.cpp
{
  float scaleX,scaleY;
  scaleX=QSHELL->GetWidth()/800.0f;
  scaleY=QSHELL->GetHeight()/600.0f;
  rOut->x=(int)(rIn->x*scaleX);
  rOut->y=(int)(rIn->y*scaleY);
  rOut->wid=(int)(rIn->wid*scaleX);
  rOut->hgt=(int)(rIn->hgt*scaleY);
}
bool rrTrackLoad(cstring tname)
// Load a track, and keep a progress bar
{
  QRect r(200,570,400,20),r2;

  rrScaleRect(&r,&r2);
  prg=new QProgress(QSHELL,&r2);

//qdbg("prg=%p, QSHELL=%p\n",prg,QSHELL);

  if(!RMGR->LoadTrack(tname,cbProgress))
  { QDELETE(prg);
    return FALSE;
  }

  QDELETE(prg);

  // Check validity of track
  if(RMGR->GetTrack()->GetTimeLines()<2)
  { qerr("Track '%s' must have at least 2 timelines",tname);
    // Delete track
    //...
    return FALSE;
  }
  if(RMGR->GetTrack()->GetGridPosCount()<2)
  { qerr("Track '%s' must have at least 2 grid positions",tname);
    // Delete track
    //...
    return FALSE;
  }
  if(RMGR->GetTrack()->GetTrackCamCount()<1)
  { qerr("Track '%s' must have at least 1 track camera",tname);
    // Delete track
    //...
    return FALSE;
  }

  return TRUE;
}
