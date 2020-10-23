/*
 * Modeler - generating 3D models for Racer
 * (C) 2000 Dolphinity/RVG
 */

#include "main.h"
#pragma hdrstop
//#include <qlib/dxsbuf.h>
//#include <vector>
#include <qlib/debug.h>
DEBUG_ENABLE
#include <qlib/profile.h>

#define INI_FILE	"tracked.ini"

//#define APP_TITLE       "Racer TrackEd v0.1"

// Import
void Run();

// Global
QInfo *info;
QProfiler qprofiler;

void main(int argc,char **argv)
{
  QRect rShell(0,0,800,600);
  QRect rBC(32,32,512,512);
  QFullScreenSettings *fss;
  //QTitleBar *title;

  info=new QInfo(INI_FILE);

  rShell.wid=info->GetInt("resolution.width",640);
  rShell.hgt=info->GetInt("resolution.height",480);
  rBC.wid=info->GetInt("view.width",512);
  rBC.hgt=info->GetInt("view.height",512);

#ifndef WIN32
  if(argc>1)
  {
    // Set last track used
    info->SetString("track.lasttrack",argv[1]);
  }
#endif

  app=new QApp("tracked");
  //app->PrefNoBC();
  app->PrefNoStates();
  app->PrefBCRect(&rBC);
  app->PrefUIRect(&rShell);
  if(info->GetInt("resolution.fullscreen",0))
    app->PrefFullScreen();
  fss=app->GetFullScreenSettings();
  fss->width=rShell.wid;
  fss->height=rShell.hgt;
  fss->bits=info->GetInt("resolution.bits",16);

  app->Create();
  Q_BC->GetQXWindow()->SetCursor(app->GetShellCursor());
  //title=new QTitleBar(QSHELL,APP_TITLE);
//Dev();

  Run();
}
