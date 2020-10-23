// qlib/dxsbuf.h - Win32 only - DirectSound buffer

#ifndef __QLIB_DXSBUF_H
#define __QLIB_DXSBUF_H

#include <qlib/sample.h>
#include <qlib/dxsound.h>

class QDXSoundBuffer
{
#ifdef WIN32
  LPDIRECTSOUNDBUFFER dsBuf;
  LPDIRECTSOUND3DBUFFER dsBuf3D;
  DS3DBUFFER params;
#endif

 public:
  enum Flags
  { IS3D=1,		// Use DirectSound3D
    LOOP=2		// Loop buffer
  };
  enum Algorithms
  {
    ALG_NO_VIRTUALIZATION,	// No 3D filtering (only panning)
    ALG_HRTF_FULL,		// Full CPU intensive filtering
    ALG_HRTF_LIGHT,		// Light filtering
    ALG_DEFAULT=ALG_NO_VIRTUALIZATION
  };

 protected:
  QSample *smp;			// Associated sample (optional)
  int flags;
  int alg;			// Chosen 3D algorithm

 public:
  QDXSoundBuffer(QSample *smp,int flags=0,int alg=ALG_DEFAULT);
  ~QDXSoundBuffer();

#ifdef WIN32
  // Internals
  LPDIRECTSOUNDBUFFER GetDSBuf(){ return dsBuf; }
  LPDIRECTSOUND3DBUFFER GetDSBuf3D(){ return dsBuf3D; }
#endif

  bool Create();
  void Destroy();
  bool IsCreated();
  void EnableLoop();
  void DisableLoop();

  void Restore();

  bool Play();
  void Stop();

  // Playback
  void SetFrequency(int freq);
  void SetVolume(int vol);
  void SetPan(int pan);
};

#endif
