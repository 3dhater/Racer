// qlib/sermouse.h

#ifndef __QLIB_SERMOUSE_H
#define __QLIB_SERMOUSE_H
  
#include <qlib/serial.h>
#include <qlib/thread.h>

class QSerMouse
// Serial mouse driver
// Microsoft mode mouses that use serial data can be used
// Note it doesn't move the X pointer, it just keeps information on the
// X and Y coordinates
// IMPORTANT: serial mice can generate a lot of events, especially
// in studio circumstances. So, try to turn off event generation at times
// you don't need them. Has been known to cause problems in 'Bommen'.
{
  QSerial *ser;
  int      serUnit;	// Serial unit; used in event generation

  QThread *thread;
  int x,y;
  int b;                // Button state (bits)
  int flags;
  int skip,eventSkip;	// Compressing events a bit
  QRect window;		// Min max coordinates

 public:
  enum Flags
  {
    MANUAL_HANDLE=1,    // Handle mouse yourself by calling Handle()
    NO_EVENTS=2		// Don't generate mouse events
  };
  enum Buttons
  {
    LMB=1,
    MMB=2,
    RMB=4
  };

 protected:
  void HandleCmd(cstring s,int nWaiting);
  void ClipXY();
  void GenerateEvent(int type,int x,int y,int but);

 public:
  QSerMouse(int unit,int flags=0);
  ~QSerMouse();

  void Handle();
  void Thread();

  // Ranges
  void SetMinMax(QRect *r);

  // Mouse information
  int  GetUnit(){ return serUnit; }
  int  GetX();
  int  GetY();
  void SetXY(int x,int y);
  int  GetButtons();
  bool GetLMB();
  bool GetMMB();
  bool GetRMB();

  // Events (SM_MOTIONNOTIFY etc)
  int  GetEventSkip(){ return eventSkip; }
  void SetEventSkip(int n);
  void EnableEvents();
  void DisableEvents();
};

#endif
