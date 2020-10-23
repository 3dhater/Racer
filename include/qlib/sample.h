/*
 * QAudio - samples wrapping
 * (C) MarketGraph/RVG
 */

#ifndef __QLIB_QSAMPLE_H
#define __QLIB_QSAMPLE_H

#include <qlib/types.h>
#include <qlib/audport.h>
#include <dmedia/audio.h>
#include <dmedia/audiofile.h>

#ifdef Q_USE_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#endif 

#ifdef Q_USE_FMOD

#include <stdlib.h>

#ifdef linux
#define PLATFORM_LINUX
#include <fmod/wincompat.h>
#endif

#include <fmod/fmod.h>
//#include <fmod/fmod_errors.h>
#endif //FMOD 

/** QChannel is used when a sample is being played **/

class QSample;
class QChannel
{
 public:
  char    *streamBuf;           // Stream sample buffer
  long     streamBufSize;       // In bytes
  long     streamBufSizeFrames; // In frames
  long     queuesize;           // Size of system audio queue


  int      frames;              // Total #frames in the playing sample
  int      curFrame;
  int      volume;		// Relative volume 0..256
  // Volume transition
  int      volFrom,volTo,volCur,volTotal;
  QAudioPort *port;		// Used audio port
  //ALconfig audioConfig;         // Sample width, #channels etc.
  //ALport   audioPort;           // The read/write port
  bool     busy;                // Doing something?
  QSample *smp;                 // Sample being used
  //AFfilehandle afh;           // File handle for disk streaming

 public:
  QChannel();
  ~QChannel();

  bool IsTransitioning();
  int  GetVolume(){ return volume; }
  int  GetFrames(){ return frames; }
  int  GetCurFrame(){ return curFrame; }

  void SetVolume(int v,int iterations=0);
};

#define QSAMPLE_DOIO            // Sync play commands
#define QSAMPLE_STREAM	1	// Sample is streamed (not stored in mem)
#define QSAMPLE_STREAMDISK 1    // Stream realtime from disk
#define QSAMPLE_LOOP	2	// Looping
#define QSAMPLE_OFFSET	4	// Offset for video


class QSample
{public:

#ifdef Q_USE_SDL
  Mix_Chunk *mixchunk;		/* mixer chunk (see SDL_mixer.h) */
  static int mix_channel;	/* SDL Channel to use */
  int channel;			/* this objects channel number */
#endif

#ifdef Q_USE_FMOD
  FSOUND_SAMPLE *sound; 
  static int mix_channel;
  int channel; 
#endif 

  void *mem;                    // Memory that stores sample
  int    flags;
  int fd;
#ifdef __sgi
  AFfilehandle afh;
#endif


  int    frames;                // #frames in sample
  double file_rate;             // Default output rate for audio port
  int    sampwidth;             // Sample width in bits
  int    samps_per_frame;
  int    compression;
  int    filefmt;               // File format
  int    vers;                  // Version
  //ALport audioport;
  bool   busy;                  // Being played?
  int    loopOffset;		// Restart point

 public:

  // Streaming
  int    streamBufSize;         // Read-ahead buffer
  int    streamBufSizeFrames;

 public:
  enum Flags
  { STREAM=1,
    LOOP=2,
    OFFSET=4,
    HW3D=8,                      // 3D sample (FMOD)
    FORCE_2D=16                  // 2D sample (FMOD)
  };

 private:
  int SampleWidth();

 public:
  QSample(cstring fname,int flags=0);
  ~QSample();

  // Info
  bool  IsOK();
#if defined(Q_USE_SDL) || defined(Q_USE_FMOD)
  void  SetVolume (int); 
  int   GetChannel() {return mix_channel;}
#endif

#ifdef Q_USE_FMOD
  void SetFrequency(int); // set playback sound frequency
#endif 

  int   GetNoofFrames();
  void *GetBuffer(){ return mem; }
  bool  IsPlaying();

  // Sample modification
  //void  SetVolume(int perc);	// Modifies sample (!)
  void  Volumize(int perc);	// Modifies sample (!)

  // I/O
  bool  Load(cstring fname);

  // Playback
  QChannel *Play(int startVol=256);	// Returns chn on which it plays
  void  Stop();			// Stop looping sample
  void  Loop(bool yn);
  void  SetLoopOffset(int n);

  // 3D support (FMOD)
  void Set3DAttributes(float *pos,float *vel);
};

// Flat functions
void QSampleSetStreamSize(int size);
void QSampleSetOffset(int off);
void QSampleKillProcess();

#endif
