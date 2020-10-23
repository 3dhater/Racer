/*
 * Racer - console (uhm, like Quake etc)
 * 13-08-01: Created!
 * NOTES:
 * - Just outputs to the shell at the moment
 * FUTURE:
 * - A circular text buffer would be faster and not really complex.
 * (c) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <racer/console.h>
#include <stdarg.h>
DEBUG_ENABLE

RConsole::RConsole()
{
  int i;

  flags=0;
  x=RMGR->infoMain->GetInt("console.x");
  y=RMGR->infoMain->GetInt("console.y");
  RScaleXY(x,y,&x,&y);
  maxLine=RMGR->infoMain->GetInt("console.maxline",MAX_LINE);
  if(maxLine>MAX_LINE)
    maxLine=MAX_LINE;
  // Init text (caught by PurifyNT)
  for(i=0;i<maxLine;i++)
    text[i][0]=0;
}
RConsole::~RConsole()
{
}

void RConsole::ShiftLines()
// Make room for a new line
{
  int i;
  for(i=1;i<maxLine;i++)
  {
    strcpy(text[i-1],text[i]);
  }
}

void RConsole::printf(cstring fmt,...)
{
  va_list args;
  char buf[1024];

  ShiftLines();

  va_start(args,fmt);
  vsprintf(buf,fmt,args);
  va_end(args);

  // Copy text in the last line
  strncpy(text[maxLine-1],buf,MAX_LINE_WID-1);
  text[maxLine-1][MAX_LINE_WID-1]=0;

  // Extra shell output
  qdbg("Console: %s\n",buf);
}

void RConsole::Paint()
{
  int i;

  if(IsHidden())return;

  for(i=0;i<maxLine;i++)
  {
    RMGR->fontDefault->Paint(text[i],x,y-i*16);
  }
}

