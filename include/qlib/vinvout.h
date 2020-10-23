// qlib/vinvout.h - high level video output
//
// class QVinVout is a handy class for screen->video output
// The constructor takes a QDraw to indicate what you want to send

#ifndef __QLIB_VINVOUT_H
#define __QLIB_VINVOUT_H

#include <qlib/dmvideoin.h>
#include <qlib/dmvideoout.h>
#include <qlib/draw.h>

class QVinVout : public QObject
{
 public:
  string ClassName(){ return "vinvout"; }
 protected:
  bool   genlock;		// Use genlocking?
  int    buffers;
  int    timing;
  int    vClass,vSource;
  int    ox,oy;			// Origin
  int    readThreadPID;
  QDMVideoIn  *vin;
  QDMVideoOut *vout;
  QDMBPool *vinPool;

 public:
  void Thread();		// The handling thread

 public:
  QVinVout(QDraw *draw=0);
  ~QVinVout();

  QDMVideoIn  *GetVideoIn(){ return vin; }
  QDMVideoOut *GetVideoOut(){ return vout; }

  // Turn genlocking on or off (internal/external)
  void SetGenlock(bool genlock);
  void SetBuffers(int bufs);

  //void SetOrigin(int x,int y);
  bool Create();

  void Run();			// Start the lot
};

#endif
