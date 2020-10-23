/*
 * Racer - create environment to run in
 * (C) 2000,2001 Dolphinity/RVG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
#include <stdlib.h>

#ifdef Q_USE_SDL_VIDEO
#include <SDL/SDL.h>
#endif //linux

#ifdef Q_USE_FMOD
#ifdef linux
#define PLATFORM_LINUX
#include <fmod/wincompat.h>
#endif 
#include <fmod/fmod.h>
#endif //FMOD 

#include <qlib/dxsbuf.h>
DEBUG_ENABLE

QDXSound *dxSound;

// Import
void Run();

int main(int argc,char **argv)
{
  QRect rShell(0,0,800,600);
  QRect rBC(0,0,20,20);
  QFullScreenSettings *fss;
  QInfo *infoGfx;

#ifdef Q_USE_SDL_VIDEO
// open SDL library
  if(SDL_Init(SDL_INIT_VIDEO)<0)
  {
    qerr("main.cpp: Couldn't initialize SDL video: %s\n",SDL_GetError());
    exit(2);
  }
  //fprintf(stderr, "SDL video initialized.\n"); 
  atexit(SDL_Quit); 
#endif //Q_USE_SDL_VIDEO
  
  infoGfx=new QInfo("gfx.ini");
  rShell.wid=infoGfx->GetInt("resolution.width",640);
  rShell.hgt=infoGfx->GetInt("resolution.height",480);

  app=new QApp("racer");
  app->PrefNoBC();
  app->PrefBCRect(&rBC);
  app->PrefUIRect(&rShell);
  app->PrefNoStates();
  if(infoGfx->GetInt("window.manage"))
    app->PrefWM();
  if(infoGfx->GetInt("resolution.fullscreen"))
  {
    app->PrefFullScreen();
  }
  fss=app->GetFullScreenSettings();
  fss->width=rShell.wid;
  fss->height=rShell.hgt;
  fss->bits=infoGfx->GetInt("resolution.bits",16);

  QDELETE(infoGfx);

  app->Create();
  Run();
  app->Exit(0);

  // We'll never get here, but let's please the ANSI people
  return 0;
}
