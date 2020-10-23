/*
 * RStats - best lap statistics
 * 26-07-01: Created!
 * NOTES:
 * - Used by RDriver for driver statistics and RTrackVRML
 * for general track records.
 * (C) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "RStats"

/*********
* C/DTOR *
*********/
RStats::RStats()
{
  int i;

  // Init members
  for(i=0;i<MAX_BEST_LAP;i++)
  {
    lapTime[i]=0;
    date[i]=0;
    flags[i]=0;
  }
  fDirty=FALSE;
}
RStats::~RStats()
{
}

bool RStats::Load(QInfo *info,cstring path)
// Load best laps
{
  int i;
  char buf[256];

//qdbg("RStats:Load(path=%s)\n",path);

  for(i=0;i<MAX_BEST_LAP;i++)
  {
    sprintf(buf,"%s.best%d.laptime",path,i);
    lapTime[i]=info->GetInt(buf);
    sprintf(buf,"%s.best%d.date",path,i);
    date[i]=info->GetInt(buf);
    sprintf(buf,"%s.best%d.car",path,i);
    info->GetString(buf,car[i]);
    sprintf(buf,"%s.best%d.flags",path,i);
    flags[i]=info->GetInt(buf);
  }

  fDirty=FALSE;
  return TRUE;
}

bool RStats::Save(QInfo *info,cstring path)
// Save info on best laps
{
  int i;
  char buf[256];

  // Don't save if not needed
  if(!fDirty)return TRUE;

//qdbg("RStats:Save(); dirty\n");
//qdbg("  info=%p\n",info);

  // Best laps
  for(i=0;i<MAX_BEST_LAP;i++)
  {
    sprintf(buf,"%s.best%d.laptime",path,i);
    info->SetInt(buf,lapTime[i]);
    sprintf(buf,"%s.best%d.date",path,i);
    info->SetInt(buf,date[i]);
    sprintf(buf,"%s.best%d.car",path,i);
    info->SetString(buf,car[i]);
    sprintf(buf,"%s.best%d.flags",path,i);
    info->SetInt(buf,flags[i]);
  }
  info->Write();

  fDirty=FALSE;
  return TRUE;
}

void RStats::CheckTime(int lastLapTime,RCar *theCar)
// The driver has done a lap in the 'car'.
// Check if this is worthy of remembering.
{
  int i,j;
//qdbg("CheckTime(%d)\n",lastLapTime);
  for(i=0;i<MAX_BEST_LAP;i++)
  {
    if(lapTime[i]==0||lastLapTime<lapTime[i])
    {
      // New record! Shift other records
//qdbg("RECORD! Place %d\n",i);
      //for(j=i+1;j<MAX_BEST_LAP;j++)
      for(j=MAX_BEST_LAP-1;j>i;j--)
      {
        lapTime[j]=lapTime[j-1];
        date[j]=date[j-1];
        car[j]=car[j-1];
        flags[j]=flags[j-1];
      }

      // Store this lap
      lapTime[i]=lastLapTime;
      date[i]=time(0);
      flags[i]=0;
      car[i]=theCar->GetName();  // QFileBase(theCar->GetDir());

      // Mark statistics dirty; save at next occasion
      fDirty=TRUE;

      // Don't seek on
      return;
    }
  }
}

void RStats::Update(QInfo *info,cstring path)
// Update any info/statistics on disk
{
  if(fDirty)
    Save(info,path);
}
