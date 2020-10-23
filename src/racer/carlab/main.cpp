/*
 * Carlab - modifying cars
 * (C) 2000 Dolphinity/RVG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/dxsbuf.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#define APP_TITLE       "Racer CarLab v1.2"

// Import
void Run();

// Global
QInfo *info;

void main(int argc,char **argv)
{
  QRect rShell(0,0,800,600);
  QRect rBC(32,48,640,480);
  QFullScreenSettings *fss;
  QTitleBar *title;

  info=new QInfo("carlab.ini");
  rShell.wid=info->GetInt("resolution.width",640);
  rShell.hgt=info->GetInt("resolution.height",480);

  app=new QApp("carlab");
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
  title=new QTitleBar(QSHELL,APP_TITLE);

  Run();
}

