/*
 * Racer Intro
 * 20-06-2001: Created! (23:07:03)
 * NOTES:
 * - Just for kicks & glamour
 * - An attempt is made to be able to exit the intro as quickly
 * as possible once a key (or mousebutton) is pressed.
 * (C) Dolphinity/RvG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

#define CHECK() if(CheckKey()){fAbort=TRUE;return;}
// Quick exit test
#define X() if(fAbort)goto abort;

// Module-wide variables
static QTimer *tmr;
static QRect   rView,               // Full window
               rViewClipped;        // Letterbox window
static DTexFont *font[2];
static QEvent  ev;
static bool    fAbort;

enum RenderArea
{
  RA_FULLSCREEN,
  RA_LETTERBOX
};

// Proto
static void SetRenderArea(int area);
static void BgrPaint();

/*********
* Helper *
*********/
static void Swap()
{
  QSHELL->Swap();
}
static void Text3D(cstring s,int x,int y,int z)
{
  DTexFont *f=font[0];
  glPushMatrix();
  glTranslatef(0,0,z);
  f->Paint(s,x,y);
  glPopMatrix();
}
static bool CheckKey()
// Returns TRUE if some key (or mousebutton) was pressed
// to interrupt the intro
// Based lightly on ZLib's ZInEvent()
{
  int  r;
  bool rb=FALSE;
  QAppEventProc oldProc;
  QAppIdleProc idleProc;
  
  oldProc=app->GetEventProc();
  //idleProc=app->GetIdleProc();
  app->SetEventProc(0);
  r=app->Run1();
  if(r==1)
  {
    // Event happened
    ev=*app->GetCurrentEvent();
    if(ev.type==QEvent::KEYPRESS||ev.type==QEvent::BUTTONPRESS)
      rb=TRUE;
  } else rb=FALSE;
  app->SetEventProc(oldProc);
  return rb;
}
static void Pause(int delay)
// Wait 'delay' ms
{
  tmr->Reset();
  tmr->Start();
  while(tmr->GetMilliSeconds()<delay)
  {
    CHECK();
  }
#ifdef OBS
  for(i=0;i<delay;i++)
  {
    Swap();
    CHECK();
  }
#endif
}

/*****
* FX *
*****/
static void FadeText(cstring s)
// Fade a text from back to front
{
  int  x,y,z,t,max=2000;
  int  baseYear,curYear=1968;
  int  i;
  DTexFont *f=font[0];
  
  tmr->Reset();
  tmr->Start();
  x=y=0;
  while(1)
  {
    t=tmr->GetMilliSeconds();
    if(t>max)t=max;
    BgrPaint();
    SetRenderArea(RA_LETTERBOX);
    z=t;
    Text3D(s,x-f->GetWidth(s)/2,y-20,z/2+100);
    Swap();
    CHECK();
    if(t>=max)break;
  }
}
static void WalkText(cstring s,int x,int y,int delay=70)
// Enter character one by one
{
  int  x0=x,xNew;
  int  z=0;
  char buf[2];
  
  // Z-buffer may cause problems
  glDisable(GL_DEPTH_TEST);
  for(;*s;s++)
  {
    if(*s==10)
    {
      y-=20;
      x=x0;
      Pause(delay); X();
    } else if(*s=='*')
    {
      Pause(delay*8); X();
    } else
    {
      buf[0]=*s; buf[1]=0;
      xNew=font[1]->Paint(buf,x,y); Swap();
      font[1]->Paint(buf,x,y);
      x=xNew;
      // May not work on all OpenGL cards...
      Pause(delay); X();
    }
  }
 abort:;
}

/*****************
* Rendering area *
*****************/
static void SetRenderArea(int area)
// 0=fullscreen
// 1=letterbox
{
  QRect r;
  int   dy;
  
  r.x=QSHELL->GetX();
  r.y=QSHELL->GetY();
  r.wid=QSHELL->GetWidth();
  r.hgt=QSHELL->GetHeight();
  rView=r;
  
  // Subtraction from view
  dy=r.hgt/10;
  rViewClipped=rView;
  rViewClipped.y+=dy;
  rViewClipped.hgt-=2*dy;
  
  QCV->Select();
  if(area==RA_FULLSCREEN)
  {
    QCV->Set2D();
    QCV->Set3D();
    QCV->Disable(QCanvas::CLIP);
  } else if(area==RA_LETTERBOX)
  {
    QCV->Set2D();
    QCV->Set3D();
    // Omit some part of the screen
    QCV->Select();
    glViewport(rViewClipped.x,rViewClipped.y,rViewClipped.wid,
      rViewClipped.hgt);
    QCV->SetClipRect(&r);
    QCV->Enable(QCanvas::CLIP);
  }
}

/*************
* Background *
*************/
static int bgrType;
enum BgrType
{
  BGR_BLACK,
  BGR_WHITE_LBOX
};
static void BgrPaint()
{
  if(bgrType==0)
  {
    QCV->SetColor(0,0,0);
    QCV->Clear();
  } else if(bgrType==1)
  {
    QCV->SetColor(255,255,255);
    //QCV->SetColor(100,100,100);
    QCV->Clear();
    QCV->Rectfill(&rViewClipped);
  }
}

/*********
* Scenes *
*********/
static void Opening()
{
  SetRenderArea(RA_FULLSCREEN);
  bgrType=BGR_BLACK;
  BgrPaint(); Swap();
}
static void FlashToBlack()
// Go to letterbox
{
  int i,t,c,max=511,d=2;
  SetRenderArea(RA_LETTERBOX);
  tmr->Reset();
  tmr->Start();
  while(1)
  {
    t=tmr->GetMilliSeconds();
    if(t>max)t=max;
    c=max-t;
    QCV->SetColor(c/d,c/d,c/d);
    QCV->Rectfill(&rView);
    Swap();
    CHECK();
    if(t>=max)break;
  }
}
static void FadeToLetterbox()
// Go to letterbox
{
  int i,t,c,max=2047,d=8;
  SetRenderArea(RA_LETTERBOX);
  tmr->Reset();
  tmr->Start();
  while(1)
  {
    t=tmr->GetMilliSeconds();
    if(t>max)t=max;
    c=t;
    QCV->SetColor(c/d,c/d,c/d);
    QCV->Rectfill(&rViewClipped);
    Swap();
    CHECK();
    if(t>=max)break;
  }
  bgrType=BGR_WHITE_LBOX;
}
static void TheYears(int fromYear,int curYear)
{
  int  x,y,z,t,max=3000;
  int  baseYear;
  int  i;
  char buf[100];
  DTexFont *f=font[0];
  
  SetRenderArea(RA_LETTERBOX);
  tmr->Reset();
  tmr->Start();
  x=y=0;
  while(1)
  {
    t=tmr->GetMilliSeconds();
    if(t>max)t=max;
    // 100 years to display
    baseYear=t/30+fromYear;
    z=(t%100)*10;
    BgrPaint();
    SetRenderArea(RA_LETTERBOX);
    //QCV->Set3D();
    for(i=0;i<3;i++)
    {
      if(baseYear>=curYear)z=100;
      sprintf(buf,"%d",baseYear+i);
      Text3D(buf,x-f->GetWidth(buf)/2,y-20,z+i*100);
      if(baseYear>=curYear)break;
    }
    Swap();
    CHECK();
    if(t>max)break;
    if(baseYear>=curYear)break;
  }
}
static void CurYear(cstring s)
{
  FadeText(s);
}
static void Blab(int year)
{
  int x,y;
  
  SetRenderArea(RA_LETTERBOX);
  bgrType=BGR_WHITE_LBOX;
  BgrPaint(); Swap();
  BgrPaint(); QCV->Set3D();
  //SetRenderArea(RA_LETTERBOX);
  
  x=-rViewClipped.wid/2+30; y=rViewClipped.hgt/2-40;
  switch(year)
  {
    case 1817:
      WalkText("Ackerman steering is patented in London.",x,y); X();
      break;
    case 1967:
      WalkText("Jim Clark shines in the Formula 1 races\n"
        "that year.* After his final victory, he comments:\n"
        "\"Boy, they should really do a computer simulation\n"
        "on this year, so everyone can get the same feeling as I have had!\"."
        ,x,y); X();
      break;
    case 1968:
      WalkText("Spring, 1968.\n"
        "\nSomewhere in a little town in Holland,\n"
        "a programmer is born.*\n\n"
        "Just missing the great 1967 Formula 1 year,\n"
        "he quickly decides on building a time machine.*\n\n"
        "That made sense ofcourse to a little boy."
        ,x,y); X();
      break;
    case 1986:
      WalkText("At the beach, a young man is looking through some code"
        " snippets.*\n"
        "Through his mind a thought is running.*\n\n"
        "\'Perhaps a time machine was a bit too much to ask for'\n"
        ,x,y); X();
      break;
    case 2000:
      WalkText("Although the time machine would solve all problems\n"
        "by giving seemlingly unlimited time to do anything, including\n"
        "building the machine itself, a more realistic goal is set.*"
        "\n\nWork on Racer is started."
        ,x,y); X();
      break;
    case 2001:
      WalkText("The next version of Racer is released.\n"
        "\nFeaturing basic multiplayer and 3D sound,\n"
        "things are moving in the right direction...*\n"
        "\nEnjoy a little taste...\n"
        ,x,y); X();
      break;
    default: qwarn("Blab() nothing to shout on the year %d",year); break;
  }
  Pause(1500);
 abort:;
}
static void ExitIntro()
{
  SetRenderArea(RA_FULLSCREEN);
  bgrType=BGR_BLACK;
  BgrPaint(); Swap();
}

/*************
* High level *
*************/
void RacerIntro()
// Some fun
{
  // Allocate resources
  tmr=new QTimer();
  font[0]=new DTexFont("data/fonts/euro40b");
  font[1]=new DTexFont("data/fonts/euro16b");
  fAbort=FALSE;
  
  Opening(); X();
  FlashToBlack(); X();
  FadeToLetterbox(); X();
  
  TheYears(1760,1817); X();
  CurYear("1817"); X();
  Blab(1817); X();
  
  TheYears(1920,1967); X();
  CurYear("1967"); X();
  Blab(1967); X();
  
  TheYears(1920,1968); X();
  CurYear("1968"); X();
  Blab(1968); X();
  
  TheYears(1940,1986); X();
  CurYear("1986"); X();
  Blab(1986); X();
  
  TheYears(1950,2000); X();
  CurYear("2000"); X();
  Blab(2000); X();
  
  TheYears(1950,2001); X();
  CurYear("2001"); X();
  Blab(2001); X();
  
  FadeText("RACER v0.4.8"); X();
  FadeText("NO LIMITS"); X();
  FadeText("NO ERA"); X();
  FadeText("NO ARCADE"); X();
  FadeText("Just drive!"); X();
  
 abort:
  // Cleanup
  ExitIntro();
  
  QDELETE(font[0]);
  QDELETE(tmr);
}
