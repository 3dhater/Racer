// qlib/dxsound.h - Win32 only

#ifndef __QLIB_DXSOUND_H
#define __QLIB_DXSOUND_H

#include <qlib/types.h>

#ifdef WIN32
#include <winsock.h>
#include <windows.h>
#include <mmreg.h>
#include <dsound.h>
#endif

// Globals
class QDXSound;
extern QDXSound *qDXSound;

class QDXSound
{
#ifdef WIN32
  LPDIRECTSOUND ds;
  LPDIRECTSOUND3DLISTENER listener;
  DS3DLISTENER listenerParams;
#endif

  enum Flags
  { USE3D=1		// Use DirectSound3D
  };

  int frequency,
      bits,
      channels;
  int flags;

 public:
  QDXSound();
  ~QDXSound();

#ifdef WIN32
  LPDIRECTSOUND GetDS(){ return ds; }
#endif

  bool Open();
  void Close();
  bool IsOpen();

  cstring Error2Str(int hr);
};

#endif
