// qlib/cd.h

#ifndef __QLIB_CD_H
#define __QLIB_CD_H

#include <qlib/object.h>
#include <sys/types.h>
#include <dmedia/cdaudio.h>

class QCD : public QObject
// Encapsulate CD operations
{
  CDPLAYER *cdp;
 public:
  enum
  { READ=0,
    WRITE=1,
    READWRITE=2
  };
  enum State
  {
    READY=CD_READY,
    NODISC=CD_NODISC,
    CDROM=CD_CDROM,
    ERROR=CD_ERROR,
    PLAYING=CD_PLAYING,
    PAUSED=CD_PAUSED,
    STILL=CD_STILL
  };

 private:
  bool GetStatus(CDSTATUS *status);

 public:
  QCD(cstring name=0,int mode=READ);
  ~QCD();

  bool    IsOpen();
  int     GetFirstTrack();
  int     GetLastTrack();
  int     GetState();
  cstring GetStateName(int state);

  bool Play(int n,bool pause=FALSE);
  bool Eject();
  bool Stop();
};

#endif
