/*
 * AI Waypoint editing
 * 10-04-2001: Created! (17:31:32)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

// GUI
cstring  opNameAI[]=
{ // Track cam ops
  "Goto line start",
  "Declare as waypoint",
  "Declare road left/right",
  0
};

int curWPIndex=-1;
RWayPoint *curWP;

void AISetMode()
// Install us as the current mode
{
  SetOpButtons(opNameAI);
}

/********
* Stats *
********/
void AIStats()
{
  char buf[40],buf2[40];
  strcpy(buf,"Current waypoint: ");
  if(!curWP)strcpy(buf2,"NONE");
  else      sprintf(buf2,"#%d",curWPIndex);
  strcat(buf,buf2);
  StatsAdd(buf);
}

/******
* Gfx *
******/
static void HiliteCurrentWP()
{
  int i;
  for(i=0;i<track->GetWayPointCount();i++)
  {
    if(curWPIndex==i)
      track->wayPoint[i]->flags|=RWayPoint::SELECTED;
    else
      track->wayPoint[i]->flags&=~RWayPoint::SELECTED;
  }
}

/******
* Ops *
******/
static void DeclareWP()
// Declare a direction and position for a waypoint
{
  int n;
  n=GetIndex("Select waypoint index",track->GetWayPointCount());
  if(n!=-1)
  {
    RWayPoint *p;
    if(n>=track->GetWayPointCount())
    {
      p=new RWayPoint();
      track->AddWayPoint(p);
      //track->wayPoint.push_back(p);
    } else
    {
      p=track->wayPoint[n];
    }
    p->pos=vPick;
    p->posDir=vPickEnd;

    // Make it current
    curWP=p;
    curWPIndex=n;
    HiliteCurrentWP();
  }
}
static void DeclareLR()
// Declare a left-right road width indicator for the current WP
{
  if(!curWP)return;
  curWP->left=vPick;
  curWP->right=vPickEnd;
}

void AIOp(int op)
{
  switch(op)
  {
    case 0: GotoPickPoint(); break;
    case 1: DeclareWP(); break;
    case 2: DeclareLR(); break;
  }
}

/*******
* Keys *
*******/
static void NextWP()
{
  if(!curWP)
  {
    // First cam?
    if(track->GetWayPointCount())
    {
      curWP=track->wayPoint[0];
      curWPIndex=0;
    }
  } else
  {
    // Next cam
    if(curWPIndex<track->GetWayPointCount()-1)
    {
      curWPIndex++;
      curWP=track->wayPoint[curWPIndex];
    }
  }
  HiliteCurrentWP();
}
static void PrevWP()
{
  if(!curWP)
  {
    NextWP();
  } else
  {
    if(curWPIndex>0)
    {
      curWPIndex--;
      curWP=track->wayPoint[curWPIndex];
    }
  }
  HiliteCurrentWP();
}

/*******
* Keys *
*******/
bool AIKey(int key)
{
  if(key==QK_PAGEUP)
  { PrevWP();
  } else if(key==QK_PAGEDOWN)
  {
    NextWP();
  }
  return FALSE;
}
