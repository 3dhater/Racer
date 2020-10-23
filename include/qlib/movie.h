// qlib/qmovie.h

#ifndef __QLIB_QMOVIE_H
#define __QLIB_QMOVIE_H

#include <qlib/window.h>
#include <qlib/audport.h>
#include <qlib/types.h>
#include <dmedia/dm_buffer.h>
#define __MV_OPEN_GL__		// Use OpenGL movie library
#include <dmedia/moviefile.h>
#include <dmedia/movieplay.h>
#include <stdio.h>
#include <fcntl.h>

// QMovie Track Types
#define QMTT_VIDEO	DM_IMAGE	// Video track
#define QMTT_AUDIO	DM_AUDIO	// Audio track

class QMovie;			// Interdependency

class QMovieTrack : public QObject
{
 protected:
  MVid trackID;			// SGI track ID
  QMovie *movie;		// Parent movie
 public:
  QMovieTrack(QMovie *movie,int trackType);
  ~QMovieTrack();

  // Internal
  MVid GetMVid(){ return trackID; }

  bool Exists();
  int  GetLength();
  int  GetImageWidth();
  int  GetImageHeight();
  int  GetImageInterlacing();

  int  GetCompressedImageSize(int frame);
  int  MapToTrack(QMovieTrack *dstTrack,int srcFrame);
  bool ReadFrames(int frame,int count,void *buf,int bufSize=-1);
  bool ReadCompressedImage(int frame,void *buf,int BufSize=-1);
};

class QMovie : public QObject
// Movie encapsulation; supports playback to video & graphics
{
 protected:
  MVid mvid;			// SGI movie ID
  QMovieTrack *trkImage,*trkAudio;
  string fileName;
  int    flags;

  // Playback
  bool playing;
  int  playMode;
  int  firstFrame,		// Playback range
       lastFrame;
  int  speed;			// Relative speed
  double frameRate;		// Frames/sec PAL=25.0, NTSC=29.97
  int  curFrame;		// Current frame
  //QAudioPort *apPlay;		// Playback sound

 public:
  // Flags
  enum Flags
  {
    MUTE=1
  };
  enum PlayMode
  { LOOP=1,
    REVERSE=2,
    PINGPONG=4
  };

 public:
  QMovie(cstring fName);
  ~QMovie();

  // Internal
  MVid GetMVid(){ return mvid; }

  bool Open(int mode=O_RDONLY);	// mode=file mode (O_RDONLY)

  // Query functions
  bool IsMovieFile();		// Is this a movie file?
  int  GetMaxImageSize();
  QMovieTrack *GetImageTrack();	// First image track
  QMovieTrack *GetAudioTrack();	// First audio track (!)

  // Bind to window for framebuffered playback
  bool BindWindow(QWindow *win);
  void SetMute(bool yn);
  bool GetMute();

  // Playback (through DMbuffers or other)
  bool IsPlaying();
  void SetPlayMode(int mode);
  int  GetPlayMode(){ return playMode; }
  void Play(int from=-1,int to=-1);
  void Stop();
  void WaitForEnd();
  void SetFirstFrame(int frame);
  int  GetFirstFrame(){ return firstFrame; }

  // Seeking
  int  GetCurFrame();
  bool SetCurFrame(int n);
  bool Goto(int n);		// Alias for SetCurFrame()
  void GotoFirst();
  void GotoLast();
  void Advance();

  // Rendering
  void RenderToDMB(DMbuffer dmb1,DMbuffer dmb2);
  void RenderToAP(QAudioPort *ap,int frame);
  void RenderToAP(QAudioPort *ap);
  void RenderToBitMap(QBitMap *bm);
  void RenderToOpenGL(int x=0,int y=0,int alpha=255);
};

#endif

