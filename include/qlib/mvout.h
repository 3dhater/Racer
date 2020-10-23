// qlib/mvout.h - video output
//
// Based on QMVOut1, this is a second different approach (hopefully better)
//
// class QMVout is a handy class for screen->video output with movie support
// The constructor takes a QDraw to indicate what you want to send
// Basically an extended QVinVout with other timing

#ifndef __QLIB_MVOUT_H
#define __QLIB_MVOUT_H

#include <qlib/dmmoviein.h>
#include <qlib/dmdecoder.h>
#include <qlib/dmvideoin.h>
#include <qlib/dmvideoout.h>
#include <qlib/movie.h>
#include <qlib/audport.h>
#include <qlib/draw.h>

class QMVOut : public QObject
{
 public:
  string ClassName(){ return "vinvout"; }
  enum Signal
  { SCREEN,
    MOVIE
  };
  enum Max
  { MAX_MBUFFER=10,   // Compressed in pool (MovieJPEG ->)
    MAX_OBUFFER=10    // Decompressed out pool (-> Video)
  };

  enum Flags
  { GENLOCK=1,			// Turn on genlocking?
    SILENTDROPS=2,		// Don't report dropped fields (handy 4 >50Hz)
    IS_RUNNING=4,		// Video thread is active?
    DISPLAY_GFX=8,		// Use graphics output
    MOVIE_FIELDS=16,		// Movie is using fields
    REQUEST_CRITICAL=32,	// Set if want to modify thread variables
    REQUEST_ACK=64		// Request seen; thread is waiting
  };

 protected:
  int    input;			// Screen or MOVIE
  //bool   genlock;		// Use genlocking?
  int    flags;			// Genlocking, verbose etc.
  int    buffers;
  int    targetFreeBufs;	// Keep stream delay constant (-1=free)
  int    timing;
  int    vClass,vSource;
  int    ox,oy;			// Origin
  int    readThreadPID;
  QMovie      *movie,*movieFuture;	// Movies to play
  QAudioPort  *ap;		// If audio
  QDMVideoIn  *vin;
  QDMMovieIn  *min;		// The movie
  QDMDecoder  *decoder;
  QDMVideoOut *vout;
  QDMBPool *minPool,		// JPEG frames
           *outPool;		// For video output
  DMbuffer  mbuf[MAX_MBUFFER];	// Movie buffering
  int       mbufIndex;
  DMbuffer  obuf[MAX_OBUFFER];	// Output buffering
  int       obufIndex;

  void DecompressFrame();
  void PushAudio();		// Play audio for the frame

 public:
  void Thread();		// The handling thread

 protected:
  void EnterCritical();
  void LeaveCritical();

 public:
  QMVOut(QDraw *draw=0);
  ~QMVOut();

  // Access to underlying dmedia objects
  QDMVideoIn  *GetVideoIn()    { return vin; }
  QDMMovieIn  *GetMovieIn()    { return min; }
  QDMVideoOut *GetVideoOut()   { return vout; }
  QAudioPort  *GetAudioPort()  { return ap; }
  QDMDecoder  *GetDecoder()    { return decoder; }
  QMovie      *GetMovie()      { return movie; }
  QMovie      *GetFutureMovie(){ return movieFuture; }
  QDMBPool    *GetMinPool()    { return minPool; }
  QDMBPool    *GetOutPool()    { return outPool; }

  void AttachMovie(QMovie *movie);
  void AttachFutureMovie(QMovie *movie);

  // Preview on canvas
  void EnablePreview(bool yn);

  // Turn genlocking on or off (internal/external)
  void SetGenlock(bool genlock=TRUE);
  void SetBuffers(int bufs);
  void SetTargetFreeBuffers(int bufs);
  void SetSilentDrops(bool yn=TRUE);

  void SetOrigin(int x,int y);
  bool IsRunning();

  bool Create();

  void Run();			// Start the lot

  void SetInput(int n);
};

#endif
