// qlib/screensaver.h

#ifndef __QLIB_SCREENSAVER_H
#define __QLIB_SCREENSAVER_H

#include <qlib/bc.h>
#include <qlib/timer.h>
#include <qlib/draw.h>

class QScreenSaver : public QObject
{
  QTimer *tmr;
  int     type;
  int     timeOut;
  int     flags;
  QDraw  *drw;

 public:
  enum Flags
  { ACTIVE=1,			// Screensaver active?
    RUNNING=2,			// Actually screensaving?
  };

  enum Type
  {
    DEFAULT=0
  };
 public:
  QScreenSaver(int timeout=15*60,int type=DEFAULT);
  ~QScreenSaver();

  QTimer *GetTimer(){ return tmr; }
  QDraw  *GetDraw(){ return drw; }

  void Off();
  void On();

  bool IsRunning();
  bool IsActive();

  bool Handle();
 protected:
  void DoStep();

};

#endif
