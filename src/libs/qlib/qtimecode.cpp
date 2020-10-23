/*
 * QTimeCode - audio/video timecodes
 * 31-03-2000: Created! (13:24:01)
 * NOTES:
 * - A wrapper around SGI's dmTC routines
 * - A DMtimecode (sys/dmcommon.h) contains hour,minute,second,frame,tc_type
 * (C) MarketGraph/RvG
 */

#include <qlib/timecode.h>
#pragma hdrstop
#include <qlib/debug.h>
DEBUG_ENABLE

QTimeCode::QTimeCode(int _type)
// Create a timecode keeper
// Timecode types are directly mapped to SGI's definitions
// Default type is TC_PAL25 (no dropframe)
{
  type=_type;
  memset(&ostc,0,sizeof(ostc));
  ostc.tc_type=type;
}

QTimeCode::~QTimeCode()
{
}

/**********
* TEXTUAL *
**********/
bool QTimeCode::ToString(string d)
// Generate a string in the form 00:00:00:00
{
  DMstatus r;
  r=dmTCToString(d,&ostc);
  if(r==DM_FAILURE)
  { qwarn("QTimeCode:ToString() failed");
  }
  return r==DM_SUCCESS;
}
bool QTimeCode::FromString(cstring src)
{
  DMstatus r;
  r=dmTCFromString(&ostc,src,type);
  return r==DM_SUCCESS;
}
cstring QTimeCode::GetString()
// Returns static string representation of timecode (convenience)
{
  static char buf[12];
  if(!ToString(buf))
    strcpy(buf,"00:00:00:00");
  return buf;
}

/*******
* MATH *
*******/
void QTimeCode::Reset()
// Reset to 0
{
  ostc.hour=0;
  ostc.minute=0;
  ostc.second=0;
  ostc.frame=0;
}

bool QTimeCode::Set(int frames)
// Set timecode to exact frame
{
  bool r;
  Reset();
  r=Add(frames);
  return r;
}

bool QTimeCode::Add(QTimeCode *tc,int *overflow)
// Adds two timecodes
// If 'overflow' is passed, any overflow is stored there
{
  DMstatus r;
  int dummy;
  
  r=dmTCAddTC(&ostc,&ostc,tc->GetOSTC(),overflow?overflow:&dummy);
  if(r==DM_FAILURE)
  { qwarn("QTimeCode:Add(tc) failed");
  }
  return r==DM_SUCCESS;
}
bool QTimeCode::Add(int frames,int *overunderflow)
// Adds the number of frames to the timecode (frames may be <0)
// If 'overunderflow' is passed, any over/underflow is stored there
{
  DMstatus r;
  int dummy;
  
  r=dmTCAddFrames(&ostc,&ostc,frames,overunderflow?overunderflow:&dummy);
  if(r==DM_FAILURE)
  { qwarn("QTimeCode:Add(frames) failed");
  }
  return r==DM_SUCCESS;
}
int QTimeCode::FramesBetween(QTimeCode *tc)
{
  DMstatus r;
  int frames;
  
  r=dmTCFramesBetween(&frames,&ostc,tc->GetOSTC());
  if(r==DM_FAILURE)
  { // Error
    frames=0;
  }
  return frames;
}

void QTimeCode::DbgPrint(cstring s)
{
  qdbg("%s: %02d:%02d:%02d:%02d\n",s,ostc.hour,ostc.minute,ostc.second,
    ostc.frame);
}
