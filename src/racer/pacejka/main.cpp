/*
 * Modeler - generating 3D models for Racer
 * (C) 2000 Dolphinity/RVG
 */

#include "main.h"
#pragma hdrstop
//#include <qlib/dxsbuf.h>
#include <qlib/debug.h>
DEBUG_ENABLE

// INI files in order of trying
#define INI_FILE1	"pacejka.ini"
#define INI_FILE2	"HOME:bin/modeler.ini"

#define APP_TITLE       "Racer Pacejka Player v1.0 - (c) Dolphinity 2001"

// Import
void Run();

// Export
cstring appTitle=APP_TITLE;
QTitleBar *title;

// Global
QInfo *info;

void main(int argc,char **argv)
{
  QRect rShell(0,0,700,500);
  QRect rBC(32,32,512,512);
  QFullScreenSettings *fss;

  if(QFileExists(INI_FILE1))
    info=new QInfo(INI_FILE1);
  else
    info=new QInfo(INI_FILE2);

  rShell.wid=info->GetInt("resolution.width",640);
  rShell.hgt=info->GetInt("resolution.height",480);
  rBC.wid=info->GetInt("view.width",512);
  rBC.hgt=info->GetInt("view.height",512);

  app=new QApp("modeler");
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
  title=new QTitleBar(QSHELL,APP_TITLE);

  Run();
}
