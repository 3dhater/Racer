// qlib/record.h

#ifndef __QLIB_RECORD_H
#define __QLIB_RECORD_H

#include <qlib/common/dmedia.h>
#include <qlib/timecode.h>
#include <qlib/thread.h>
#include <qlib/listbox.h>
#include <qlib/check.h>
#include <qlib/radio.h>
#include <qlib/scrollbar.h>

class QRecord;

class QRecordSettings
// A class to store recording settings
// and to interface them to the user
{
 public:
  enum MaxValues
  { MAX_VIDEO_INPUT=4
  };

 protected:
  QWindow *parent;
  int      x,y;			// Position of upper lefthand control
  //QButton *bInput[MAX_VIDEO_INPUT];
  QListBox *lbInput;
  QListBox *lbPreset;		// Preset video settings
  QCheck   *cPreviewJPEG;	// See JPEG effect?
  QCheck   *cVideoOutput;       // See input and JPEG on video out?
  
  int      vinInputNodeNumber[MAX_VIDEO_INPUT];
  int      inputs;

  QRecord *recorder;		// Which recorder we are controlling

  void DetectVideoInputs(int x,int y);

 public:
  QRecordSettings();
  ~QRecordSettings();

  void SetRecorder(QRecord *rec);
  QRecord *GetRecorder(){ return recorder; }

  // GUI
  void CreateControls(QWindow *parent,int x,int y);
  void GetBoundingBox(QRect *r);
  void DestroyControls();
  bool Event(QEvent *e);

  // High level
  bool SelectPreset(int n);
};

class QRecord
// A class to easily record video and audio
// to a movie file or screen
{
  QThread *thread;		// The recording thread
  bool     threadOk;		// Thread done with initializing?

  // Audio recording
  QAudioPort *audioIn,*audioOut;
  char *audioBuf;
  int   audioBufLen;
  int   audioMinimum;		// Minimal #frames filled before a read occurs

 public:
  enum Flags
  {
    RECORDING=1,		// Are we recording to disk?
    CAPTURE_AUDIO=2,		// Capture audio
    CAPTURE_VIDEO=4,		// Capture video
    CAPTURE_FIELDS=8,		// Capture fields instead of frames
    PREVIEW_VIDEO=16,		// Show video in QBC
    PREVIEW_AUDIO=32,		// Monitor audio
    ISRUNNING=64,		// Thread is active
    PREVIEW_JPEG=128,		// Show JPEG-ed preview, NOT original
    SHOW_VIDEO=256,		// Show things on video output
    GENLOCK_OUTPUT=0x200	// Genlock the video output
  };

 protected:
  // Video recording
  QDMVideoIn  *vin;
  QDMVideoOut *vout;
  QDMEncoder  *encode;
  QDMDecoder  *decode;			// Testing JPEG
  QDMMovieOut *mout;
  QDMPBuffer  *pbuf;
  QDMBPool    *vinPool,*encPool,*outPool;
  QGLContext  *glCtxPreview;		// Preview OpenGL context
  QTimeCode   *tcRecord;		// Timecode of recording frame
  // Video settings
  qstring      fileName;		// Movie output filename
  int          vinInputNumber;
  int          flags;
  int          quality;			// 1-100
  int          zoomWid,zoomHgt;

  void PaintDMBuffer(DMbuffer dmb,int w,int h,int field);

 public:
  QRecord();
  ~QRecord();

  QTimeCode   *GetTimeCodeRecord(){ return tcRecord; }
  QThread     *GetThread(){ return thread; }
  QDMMovieOut *GetMovieOut(){ return mout; }
  qstring&     GetFileName(){ return fileName; }

  // Attribs
  int  GetFlags(){ return flags; }
  void SetFlags(int n);
  void SetFileName(cstring name);
  bool CapturesAudio();
  bool CapturesVideo();
  bool CapturesFields();

  bool IsPreviewEnabled();
  bool IsRunning();
  bool IsRecording();
  void EnablePreview(bool yn);
  void EnableRecording(bool yn);

  // Events
  bool Event(QEvent *e);

  // Video stream
  bool Create();
  void Destroy();
  void SetZoomSize(int wid,int hgt);
  void SetFieldCapture(bool yn);
  void SetVideoInput(int n);

  // Thread
  void Start();
  void Thread();
 protected:
  void EnterCritical();
  void LeaveCritical();
};

#endif
