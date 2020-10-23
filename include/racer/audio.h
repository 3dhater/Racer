// raudio.h - Audio for the environment

#ifndef __RAUDIO_H
#define __RAUDIO_H

#include <qlib/types.h>
#include <qlib/audport.h>
#include <qlib/sample.h>
#include <qlib/dxsbuf.h>

class RAudioProducer
{
  QDXSoundBuffer *sbuf;
  int flags;
  int speedSample,		// Speed at which sample was taken
      speedCurrent;		// Current speed to playback
  QSample *smp;			// A wave
  // The sample buffer
  void    *buffer;
  int      bufferSize;		// In frames
  int      curPos;
  int      delta;		// Interpolation error
  int      vol;

 public:
  enum Flags
  { ENABLED=1,			// Producer can be heard at all
    PRIVATE_SMP=2		// 'smp' was created by the producer
  };

 public:
  RAudioProducer();
  ~RAudioProducer();

  QSample *GetSample(){ return smp; }

  void SetSampleSpeed(int n);
  void SetCurrentSpeed(int n);

  int  GetVolume(){ return vol; }
  void SetVolume(int n);

  bool LoadSample(cstring fileName);
  void SetBuffer(void *buffer,int size);
  void SetSample(QSample *smp);

  void ProduceFrames(void *buffer,int frames);
};

class RAudio
// The environment in which the cars are located
{
#ifdef WIN32
  QDXSound *dxSound;
#endif
  QAudioPort *port;
  int fps;			// Frames/second
  int frequency;
  int bits;
  int channels;
  bool enable;

 public:
  enum Max
  { MAX_PRODUCER=10
  };

  RAudioProducer *producer[MAX_PRODUCER];
  int             producers;

 public:
  RAudio();
  ~RAudio();

  // Attribs
  int       GetFrequency(){ return frequency; }
#ifdef WIN32
  QDXSound *GetDXSound(){ return dxSound; }
#endif

  bool Load(cstring fileName);
  bool Create();

  bool AddProducer(RAudioProducer *prod);
  bool RemoveProducer(RAudioProducer *prod);

  void Run(float time);
};

#endif
