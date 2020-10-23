/*
 * Modeler - generating 3D models for Racer
 * (C) 2000 Dolphinity/RVG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
//#include <qlib/dxsbuf.h>
DEBUG_ENABLE

// INI files in order of trying
#define INI_FILE1	"modeler.ini"
#define INI_FILE2	"HOME:bin/modeler.ini"

#define APP_TITLE       "Racer Modeler v1.5 - (c) Dolphinity 2000,2001"

// Import
void Run();

// Global
QInfo *info;

void main(int argc,char **argv)
{
  QRect rShell(0,0,800,600);
  QRect rBC(32,32,512,512);
  QFullScreenSettings *fss;
  QTitleBar *title;

/*char buf[256];
printf("cwd='%s'\n",getcwd(buf,sizeof(buf)));
chdir("modeler");
printf("cwd='%s'\n",getcwd(buf,sizeof(buf)));*/

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
