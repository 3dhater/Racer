// qlib/moviebob.h - movie gel

#ifndef __QLIB_MOVIEBOB_H
#define __QLIB_MOVIEBOB_H

#include <qlib/bob.h>
#include <qlib/movie.h>
#include <qlib/control.h>
#include <qlib/audport.h>

class QMovieBob : public QBob
{
 public:
  string ClassName(){ return "moviebob"; }

 protected:
  QMovie  *movie;	// Associated movie
  int      /*frame,*/ frames;	// Info about movie
  int      cropLeft,cropRight,	// #pixels to crop
           cropTop,cropBottom;
  //int      playSpeed;		// 0=still
  //int      loopsToGo;
  // Audio
  QAudioPort *ap;
  bool        mute;

 public:
  // Dynamic controls; taken over from the 'bob' member
  //QControl *x,*y,*wid,*hgt;
  //QControl *scale_x,*scale_y;

 public:
  QMovieBob(int wid,int hgt,int flags=0);
  ~QMovieBob();

  QMovie *GetMovie(){ return movie; }
  void SetMovie(QMovie *movie);
  //int GetWidth();
  //int GetHeight();
  bool GetFramed();
  void SetFramed(bool yn);

  // Info
  bool IsPlaying();

  // Displaying
  void CreateStill();
  void Crop(int left,int right,int top,int bottom);
  void Loop(int nTimes=-1);		// Loop mode, -1=infinite
  void Play(int speed=1);		// Continuous play
  void Stop();

  void Update();

  void Paint();
  void PaintPart(QRect *r);
};

#endif
