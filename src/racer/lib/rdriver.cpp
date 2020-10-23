/*
 * RDriver - a car driver
 * 26-07-01: Created!
 * 07-01-02: Track names with spaces are cleaned up to avoid fields
 * with spaces or other characters. Also, the track name is stored.
 * All this for the convenience of RacerRank.
 * NOTES:
 * (C) Dolphinity/RvG
 */

#include <racer/racer.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

// Local module trace?
#define LTRACE

#undef  DBG_CLASS
#define DBG_CLASS "RDriver"

/**********
* Helpers *
**********/
cstring MakeNiceName(cstring s)
// Given a track name, make a nice name out of it, without spaces and
// other garbage. For example, 'Elkhart Lake' -> 'ElkhartLake'
{
  static char buf[256];
  char *d=buf;
  for(;*s;s++)
  {
    if((*s>='A'&&*s<='Z') ||
       (*s>='a'&&*s<='z'))
      *d++=*s;
  }
  *d=0;
  return buf;
}

/*********
* C/DTOR *
*********/
RDriver::RDriver(cstring _dirName)
{
  char buf[256];

  // Always get a driver
  if(!_dirName)_dirName="default";

  dirName=_dirName;
  sprintf(buf,"data/drivers/%s/driver.ini",dirName.cstr());
  info=new QInfo(buf);

  // Init members
  age=0;
}
RDriver::~RDriver()
{
  QDELETE(info);
}

void RDriver::SetTrackName(cstring tname)
// You must set a track name before results can be loaded or stored
{
  trackName=tname;
}

bool RDriver::Load()
// Get info on driver
{
  char buf[256];

  // Any track to load info for?
  if(trackName.IsEmpty())return FALSE;

  sprintf(buf,"results.%s",MakeNiceName(trackName.cstr()));
  stats.Load(info,buf);

  return TRUE;
}

bool RDriver::Save()
// Put info on driver
{
  char buf[256];

  // Any track to load info for?
  if(trackName.IsEmpty())return FALSE;

  sprintf(buf,"results.%s",MakeNiceName(trackName.cstr()));
  stats.Save(info,buf);

  return TRUE;
}

void RDriver::Update()
// Update any info/statistics on disk
{
  char buf[256];

#ifdef LTRACE
qdbg("RDriver::Update()\n");
#endif

  sprintf(buf,"results.%s",MakeNiceName(trackName.cstr()));
  stats.Update(info,buf);

  // Write full name of current track; this is for convenience
  // of RacerRank (avoiding to have to look it up elsewhere).
  if(RMGR->GetTrack())
  {
    sprintf(buf,"results.%s.name",MakeNiceName(trackName.cstr()));
#ifdef LTRACE
qdbg("  Track present; write full name (%s)\n",RMGR->GetTrack()->GetName());
qdbg("    '%s'='%s'\n",buf,RMGR->GetTrack()->GetFullName());
#endif
    info->SetString(buf,RMGR->GetTrack()->GetFullName());
    info->Write();
  }
}
