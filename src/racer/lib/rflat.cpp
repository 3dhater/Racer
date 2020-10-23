/*
 * Racer - flat functions
 * 06-12-00: Created!
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
// stdlib is needed by some Linux installations
#include <stdlib.h>
DEBUG_ENABLE

cstring RFindFile(cstring fname,cstring prefDir,cstring fallBackDir)
// Find the file in a number of places
// The environment var "RACER" may be used to find Racer's base directory
// (mainly used for development)
// 'fallBackDir' is used in case the file isn't found in 'prefDir'.
// Use 'fallBackDir' for example to load samples; first in the car's
// directory (prefDir), then in a global 'fallBackDir'.
// Returns the filename as it SHOULD be, if not found.
{
  static char buf[256];
  char *env;

  // Try preferred directory
  sprintf(buf,"%s/%s",prefDir,fname);
  if(QFileExists(buf))
    goto do_ok;

  // Try fallback directory
  if(fallBackDir)
  {
    sprintf(buf,"%s/%s",fallBackDir,fname);
    if(QFileExists(buf))
      goto do_ok;
  }

  // Try preffered directory in relation to $RACER
  env=getenv("RACER");
  if(env)
  {
    sprintf(buf,"%s/%s/%s",env,prefDir,fname);
    if(QFileExists(buf))
      goto do_ok;
  }

  // Not found anywhere, return filename as it SHOULD be
  if(env)
  {
    // We have an env var; base it on that
    sprintf(buf,"%s/%s/%s",env,prefDir,fname);
    goto do_ok;
  } else
  {
    // Joe user PC probably; assume we're in Racer's home directory
    sprintf(buf,"%s/%s",prefDir,fname);
    goto do_ok;
  }
 do_ok:
//qdbg("RFindFile(%s,%s)=%s\n",fname,prefDir,buf);
  return buf;
}

cstring RFindDir(cstring prefDir)
// Returns the directory full path
// The environment var "RACER" may be used to find Racer's base directory
// (mainly used for development)
{
  static char buf[256];
  char *env;

  // Try preffered directory in relation to $RACER
  env=getenv("RACER");
  if(env)
  {
    // We have an env var; base it on that
    sprintf(buf,"%s/%s",env,prefDir);
  } else
  {
    // Joe user PC probably; assume we're in Racer's home directory
    sprintf(buf,"%s",prefDir);
  }
  return buf;
}

/**********
* Scaling *
**********/
static bool fScaleSetup;
static float scaleX,scaleY;

void RScaleSetup()
// Determine scaling in use based on a 800x600 default size
{
  if(fScaleSetup)return;
  if(!QSHELL)return;
  scaleX=QSHELL->GetWidth()/800.0f;
  scaleY=QSHELL->GetHeight()/600.0f;
}
void RScaleXY(int x,int y,int *px,int *py)
// Scale XY pair into 'px' and 'py'
// Note that 'px' and 'py' may point to 'x' and 'y' themselves.
{
  if(!fScaleSetup)RScaleSetup();
  *px=(int)(scaleX*x);
  *py=(int)(scaleY*y);
}
void RScaleRect(const QRect *rIn,QRect *rOut)
// Scale a rectangle
{
  if(!fScaleSetup)RScaleSetup();
  rOut->x=(int)(rIn->x*scaleX);
  rOut->y=(int)(rIn->y*scaleY);
  rOut->wid=(int)(rIn->wid*scaleX);
  rOut->hgt=(int)(rIn->hgt*scaleY);
}
rfloat RScaleX(int n)
{
  if(!fScaleSetup)RScaleSetup();
  return n*scaleX;
}
rfloat RScaleY(int n)
{
  if(!fScaleSetup)RScaleSetup();
  return n*scaleY;
}

