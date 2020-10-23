/*
 * QCD - CD support
 * 01-09-1999: Created!
 * NOTES: 
 * - Based on libcdaudio (link with -lcdaudio -lmediad -lds)
 * (C) MarketGraph/RvG
 */

#if defined(__sgi)

#include <qlib/cd.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QCD::QCD(cstring name,int mode)
{
  cstring rm;
  switch(mode)
  { case READ:      rm="r"; break;
    case WRITE:     rm="w"; break;
    case READWRITE: rm="rw"; break;
    default:        rm="r"; break;
  }
  cdp=CDopen(name,rm);
qdbg("QCD ctor: cdp=%p\n",cdp);
}
QCD::~QCD()
{
qdbg("qcd dtor\n");
  if(cdp)
  { if(!CDclose(cdp))
    { qwarn("QCD: CDclose() failed");
    }
  }
}

bool QCD::IsOpen()
// Returns TRUE if CD could be openend
// Note on default IRIX6.5.3f systems, write permission is off for CD's
// so only root can open CD's.
// For a workaround, enter "/dev/scis/sc0d4l0 0666 root sys" into /etc/ioperms
// and run ioconfig -f /hw (as root). This sets write permissions for all.
// (see knowledgebase)
{
  if(cdp)return TRUE;
  return FALSE;
}

bool QCD::GetStatus(CDSTATUS *status)
// Private function to get info on CDROM
{
  if(!cdp)return FALSE;
  if(CDgetstatus(cdp,status)==0)
  { // Error
    return FALSE;
  }
  return TRUE;
}

int QCD::GetFirstTrack()
// Returns 0 for error
// Returns first legal track number
{
  if(!cdp)return 0;
  CDSTATUS status;
  if(GetStatus(&status))
    return status.first;
  return 0;
}
int QCD::GetLastTrack()
// Returns 0 for error
// Returns last legal track number
{
  if(!cdp)return 0;
  CDSTATUS status;
  if(GetStatus(&status))
    return status.last;
  return 0;
}

int QCD::GetState()
// Returns state of CD (enum State)
{
  if(!cdp)return 0;
  CDSTATUS status;
  if(GetStatus(&status))
    return status.state;
  return 0;
}
cstring QCD::GetStateName(int state)
{
  switch(state)
  { case READY:   return "ready";
    case NODISC:  return "nodisc";
    case CDROM:   return "cdrom";
    case ERROR:   return "error";
    case PLAYING: return "playing";
    case PAUSED:  return "paused";
    case STILL:   return "still";
    default:      return "cdUnkownState";
  }
}

bool QCD::Play(int track,bool pause)
// Starts playing of track.
// If 'pause'==TRUE, then the CD is put at the start of the track
// and won't start playing until you call TogglePause()
// Returns TRUE if succesful
{
  if(!cdp)return FALSE;
  if(CDplay(cdp,track,!pause))		// Mind the !
    return TRUE;
  return FALSE;
}

bool QCD::Eject()
// Ejects the drive
{
  if(!cdp)return FALSE;
  if(CDeject(cdp))return TRUE;
  return FALSE;
}

bool QCD::Stop()
// Stop playing (if at all)
{
  if(!cdp)return FALSE;
  if(CDstop(cdp))return TRUE;
  return FALSE;
}

#endif
