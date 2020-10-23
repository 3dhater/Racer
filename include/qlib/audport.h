/*
 * qaudport.h - audio port (SGI) encapsulation
 * (C) MarketGraph/RVG
 */

#ifndef __QLIB_QAUDPORT_H
#define __QLIB_QAUDPORT_H

#include <qlib/object.h>
#include <dmedia/audio.h>
#include <dmedia/audiofile.h>

#ifdef OBS
struct QAPInternal
{
  ALconfig audioConfig;		// Sample width, #channels etc.
  ALport   audioPort;		// The file port
};
#endif

class QAudioPort : public QObject
// Encapsulates the SGI audio port concept
{
 protected:
  //QAPInternal *internal;
  ALconfig audioConfig;		// Sample width, #channels etc.
  ALport   audioPort;		// The file port
  void *buffer;
  int   bufferSize;

 public:
  enum Width
  { WIDTH_8_BIT=1,
    WIDTH_16_BIT=2,
    WIDTH_24_BIT=4
  };
  enum Defaults
  { DEFAULT_QUEUESIZE=32768
  };

  QAudioPort();
  ~QAudioPort();

  // Internals
  ALconfig GetAudioConfig(){ return audioConfig; }
  ALport   GetAudioPort(){ return audioPort; }

  // Functions before Open()-ing
  int SetWidth(int smpWidth);	// 1, 2 or 4 (24-bit)
  int SetChannels(int nChannels);	// 1, 2 or 4 (Indy only?)
  int SetQueueSize(int queueSize);	// In sample frames (!)
  int GetQueueSize();

  bool Open(bool write=TRUE);		// Open port/file
  void Close();				// Temporary release

  // After opening
  int GetFD();				// Get FD for select()
  int GetFillable();
  int GetFilled();
  int SetFillPoint(int n);
  int WriteSamps(void *p,int count);
  int ReadSamps(void *p,int count);
  // AL2.0 features; note the changed unit
  void ZeroFrames(int count);

  // Convenience; a buffer is usually needed to read samples in
  void *AllocBuffer(int size);
  void  FreeBuffer();
  void *GetBuffer();
  int   GetBufferSize();
};

#ifdef NOTDEF_QSAMPLE

#define QSAMPLE_DOIO            // Sync play commands
#define QSAMPLE_STREAM	1	// Sample is streamed (not stored in mem)
#define QSAMPLE_STREAMDISK 1    // Stream realtime from disk
#define QSAMPLE_LOOP	2	// Looping

class QSample
{public:
  void *mem;                    // Memory that stores sample
  int    flags;
  int fd;
  AFfilehandle afh;
  long   frames;                // #frames in sample
  double file_rate;             // Default output rate for audio port
  int    sampwidth;             // Sample width in bits
  int    samps_per_frame;
  int    compression;
  long   filefmt;               // File format
  long   vers;                  // Version
  //ALport audioport;
  bool   busy;                  // Being played?

 public:
  // Streaming
  long   streamBufSize;         // Read-ahead buffer
  long   streamBufSizeFrames;

 private:
  int SampleWidth();

 public:
  QSample(char *fname,int flags=0);
  ~QSample();

  bool  Load(char *fname);
  void  Play(void);
  bool  IsPlaying();
};

#endif

#endif

