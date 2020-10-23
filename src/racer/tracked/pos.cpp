/*
 * Position editing
 * 01-04-2001: Created! (17:35:41)
 * NOTES:
 * (C) MarketGraph/RvG
 */

#include "main.h"
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

// GUI
cstring  opNamePos[]=
{ // Track cam ops
  "Goto line start",
  "Declare grid pos",
  "Declare pits pos",
  "Declare timeline",
  0
};

/*******
* Mode *
*******/
void PosSetMode()
// Install us as the current mode
{
  SetOpButtons(opNamePos);
}

/********
* Stats *
********/
void PosStats()
{
  char buf[40],buf2[40];
  sprintf(buf,"Grid positions: %d",track->GetGridPosCount());
  StatsAdd(buf);
  sprintf(buf,"Pit positions : %d",track->GetPitPosCount());
  StatsAdd(buf);
  sprintf(buf,"Timelines     : %d",track->GetTimeLines());
  StatsAdd(buf);
}

static void DeclareGridPos()
{
  int n;
  // Declare grid position
  n=GetIndex("Select grid position index",
    track->GetGridPosCount()<RTrackVRML::MAX_GRID_POS?
    track->GetGridPosCount():RTrackVRML::MAX_GRID_POS-1);
  if(n!=-1)
  {
    RCarPos *pos;
    pos=&track->gridPos[n]; //GetGridPos(n);
    pos->from=vPick;
    pos->to=vPickEnd;
    if(n==track->GetGridPosCount())track->gridPosCount++;
  }
}
static void DeclarePitPos()
{
  int n;
  // Declare pit position
  n=GetIndex("Select pit position index",
    track->GetPitPosCount()<RTrackVRML::MAX_GRID_POS?
    track->GetPitPosCount():RTrackVRML::MAX_GRID_POS-1);
  if(n!=-1)
  {
    RCarPos *pos;
    pos=&track->pitPos[n]; //GetPitPos(n);
    pos->from=vPick;
    pos->to=vPickEnd;
    if(n==track->GetPitPosCount())track->pitPosCount++;
  }
}

static void DeclareTimeline()
{
  int n;
  // Declare time line
  n=GetIndex("Select timeline index",
    track->GetTimeLines()<RTrackVRML::MAX_GRID_POS?
    track->GetTimeLines():RTrackVRML::MAX_GRID_POS-1);
  if(n!=-1)
  {
    if(n<track->GetTimeLines())
    { QDELETE(track->timeLine[n]);
    } else track->timeLines++;
    // Install new timeline
    track->timeLine[n]=new RTimeLine(&vPick,&vPickEnd);
#ifdef OBS
    RTimeLine *tl;
    tl=track->timeLine[n]; //GetTimeLine(n);
    tl->from=vPick;
    tl->to=vPickEnd;
#endif
    //if(n==track->GetTimeLines())track->timeLines++;
  }
}

/******
* Ops *
******/
void PosOp(int op)
{
  switch(op)
  {
    case 0: GotoPickPoint(); break;
    case 1: DeclareGridPos(); break;
    case 2: DeclarePitPos(); break;
    case 3: DeclareTimeline(); break;
  }
}

/*******
* Keys *
*******/
bool PosKey(int key)
{
  return FALSE;
}
