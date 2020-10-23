// qlib/qvideo.h - SGI Video objects

#ifndef __QLIB_VIDEO_H
#define __QLIB_VIDEO_H

#include <vl/vl.h>
#include <vl/dev_ev1.h>
#include <vl/dev_mvp.h>
#include <dmedia/dm_buffer.h>
#include <dmedia/dm_params.h>
#include <dmedia/dm_imageconvert.h>
#include <qlib/movie.h>
#include <qlib/info.h>
#include <qlib/window.h>

// VL knows about 3 buffering interfaces (14-11-99); VLBuffer, O2 DMBuffers
// and 6.5 DMBuffers

#define FORCE_VL_65
#ifdef  FORCE_VL_65
#define USE_VL_65
// IRIX 6.5 VL DMBuffer interface; make sure old interface (6.3/O2)
// is detected and gives errors
// This does not include the first interface (VLBuffer), but we never
// used this.
#define vlDMPoolGetParams old_interface_use_vlDMGetParams
#define vlDMBufferSend old_interface_use_vlDMBufferPutValid
#define vlEventRecv old_interface_use_vlNextEvent_and_Pending
#define vlEventToDMBuffer old_interface_use_vlDMBufferGetValid
#define vlPathGetFD old_interface_use_vlNodeGetFd
// USE_VL_65 also gives an extra set of event handling in the QVideoServer
#endif

class QVideoServer : public QObject
{
 protected:
  VLServer svr;
 public:
  QVideoServer();
  ~QVideoServer();

  VLServer GetSGIServer(){ return svr; }

  // Management functions
  void SaveSystemDefaults();
  void RestoreSystemDefaults();

  // Getting language
  string Reason2Str(int r);
  string Control2Str(int r);

#ifdef USE_VL_65
  // Event handling (global; before 6.5 is was per-path)
  int     NextEvent(VLEvent *ev);
  int     PeekEvent(VLEvent *ev);
  int     CheckEvent(VLEventMask mask,VLEvent *ev);
  cstring EventToName(int type);
  int     Pending();
#endif
};

class QVideoPath;

class QVideoNode : public QObject
{
 protected:
  VLNode node;
  QVideoPath *path;		// Assuming only 1 path for each node
  bool SetControl(int ctrlType,VLControlValue *val);
  QVideoServer *server;

 public:
  QVideoNode(QVideoServer *s,int nodeType,int devType,int devnr=0);
  ~QVideoNode();

  int         GetFD();
  VLNode      GetSGINode(){ return node; }

  void        SetPath(QVideoPath *path);
  QVideoPath *GetPath(){ return path; }

  // General control usage
  bool SetControl(int typ,int n);
  bool SetControl(int typ,int x,int y); 
  bool GetIntControl(int typ,int *n);
  bool GetXYControl(int typ,int *x,int *y);

  // Commonly used controls
  bool SetOrigin(int x,int y);
  bool SetSize(int wid,int hgt);
  bool SetWindow(QWindow *win);
  bool Freeze(bool onOff);

  // DMbuffers
  int  GetFilled();
};

class QVideoPath : public QObject
{
 protected:
  VLPath path;
  QVideoServer *srv;
 public:
  QVideoPath(QVideoServer *s,int dev,QVideoNode *src,QVideoNode *drn);
  ~QVideoPath();

  VLPath GetSGIPath(){ return path; }
  // Setup hardware for transfer
  bool Setup(int ctrlUsage,int streamUsage,QVideoPath *path2=0);
  bool SelectEvents(VLEventMask evMask);
  bool BeginTransfer();
  bool EndTransfer();
  bool AddNode(QVideoNode *node);
#ifdef OBS_NOT_POSSIBLE_IN_VL_65
  int  GetFD();			// for select()
#endif

  // Capturing
  int  GetTransferSize();
};

/*
 * Prefab Video environments
 */

class QVideoCosmoLink : public QObject
// A complete Cosmo to video & screen output
// Cosmo needs timing information from some sync generator
{
 protected:
  QVideoServer *srv;

  QVideoNode *devNode;		// 'ev1' device node for sync setting
  QVideoNode *src;			// Cosmo framebuffer
  QVideoNode *drnScreen,*drnVideo;	// Drains (video, screen)
  QVideoNode *timingSrc,*timingDrn,*dataTiming;

  QVideoPath *pathTiming,*pathVideo;

 public:
  QVideoCosmoLink(QVideoServer *s);
  ~QVideoCosmoLink();

  bool Show();			// Use this path
  bool Hide();
};

class QVideoOut : public QObject
{
 public:
  string ClassName(){ return "videoout"; }
 protected:
  QVideoServer *vs;
  QVideoPath   *pathIn,*pathOut;
  // Screen to Memory path (input)
  QVideoNode *srcVid,*drnMem;   // srcVid=srcScr / Source Screen
  // Memory to Video path (output)
  QVideoNode *srcMem,*drnVid;

  DMbufferpool dmbPool;         // Collect pool
  int          dmbPoolFD;       // For select()
  int          outBufSize;
  DMbuffer     dmb;
  DMparams    *dmp;

  // Movie support
  DMbufferpool inPool;          // inPool=movie dmIC input
  int          inPoolFD;        // For select()
  int          inBufSize;	// Size of the DMbuffers
  DMimageconverter ic;          // The decompressor
  QMovie      *movie;		// Playback
  QMovieTrack *trkImg,*trkAud;	// The tracks
  MVid mvid,mvidImg,mvidAud;	// SGI originals
  QAudioPort  *ap;		// Movie sound

  // Tracking available options
 public:
  enum OKFlags
  { OK_INBUF=1,
    OK_OUTBUF=2,
    OK_INOUT=3          // Can perform video throughput
  };
  int okFlags;

 public:
  // Realtime switching between input signal (movie/screen/video)
  enum Signal
  { SS_MOVIE=0,		    // Movie file/dmIC
    SS_SCREEN=1,  	  // Screen grabs
    SS_VIDEO=2    		// Input video
  };
 protected:
  int srcSignal;

 private:
  bool FindImageConverter();
  bool SetupImageConverter(int ib,int ob);
  void GetInputDMbuffer(DMbuffer *bufptr);
  void SendDMbuffer(DMbuffer dmbuf);
  char *FindSecondField(char *buf,int size);

 public:
  QVideoOut();
  ~QVideoOut();

  void SetOrigin(int x,int y);
  bool AttachMovie(QMovie *movie);
  bool Create(QInfo *info=0);	// Build paths and pools
  bool Run();			// Start the lot
  void Process();		// The thread
  void ProcessMovie();		// For MOVIE input
  void ProcessScreen();		// For SCREEN input

  // Realtime switching (modifications)
  int  GetInput();
  void SetInput(int n);		// SS_xxx; movie or screen or video(?)
};

#endif
