// qlib/touch.h

#ifndef __QLIB_TOUCH_H
#define __QLIB_TOUCH_H

#include <qlib/serial.h>
#include <qlib/thread.h>

class QTouch
// Touchscreen generic driver
// Supported are: ELO Touch E281-2310 controller (serial)
// ELO also supplies USB and PC-Bus and MicroChannel touchscreens.
// See www.elotouch.com for details and technical manual
// IMPORTANT: touchscreens can generate a lot of events, especially
// in studio circumstances. So, try to turn off event generation at times
// you don't need them.
{
  QSerial *ser;
  int      serUnit;	// Serial unit; used in event generation

  QThread *thread;
  int x,y,z;		// 'z' is pressure
  int b;                // Button state (bits)
  int flags;
  int type;             // What type of controller?
  int skip,eventSkip;	// Compressing events a bit
  QRect window;		// Min max coordinates (virtual; scaled)
  QRect calibration;	// Min max raw coordinates from touch device

 public:
  enum Flags
  {
    MANUAL_HANDLE=1,    // Handle mouse yourself by calling Handle()
    NO_EVENTS=2,	// Don't generate mouse events
    FLIP_X=4,		// Flip X coordinates
    FLIP_Y=8,		// Flip Y coordinates
    INITIAL_TOUCHES=16	// Only report initial touches
  };
  enum Buttons
  {
    LMB=1,
    MMB=2,
    RMB=4
  };
  enum Types
  { ELO281_2310=0	// ELO touch controller type (used in Kunstkoppen)
  };

 protected:
  void HandleCmd(cstring s,int nWaiting);
  void ClipXY();
  void GenerateEvent(int type,int x,int y,int but);

 public:
  QTouch(int serUnit,int type=ELO281_2310,int flags=0);
  ~QTouch();

  void Handle();
  void Thread();

  // Ranges
  void SetMinMax(QRect *r);
  void SetCalibration(QRect *r);

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
