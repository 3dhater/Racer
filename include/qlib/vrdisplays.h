// vrdisplays.h - Vos & Ramselaar Displays interface
// V&R standard interface

#ifndef __QLIB_VRDISPLAYS_H
#define __QLIB_VRDISPLAYS_H

#include <qlib/serial.h>

class QVRDisplays
{
 public: 
  enum MaxThings
  { MAX_DISPLAY=256
  };
  enum States
  { OFF=0,
    ON=1
  };

 protected:
  // Class member variables
  QSerial *ser;
  int  count;
  char state[MAX_DISPLAY];

 public:
  QVRDisplays(int count,int serUnit=QSerial::UNIT2);
  ~QVRDisplays();

  // Internal info
  int      GetCount(){ return count; }
  QSerial *GetSerial(){ return ser; }

  // Display states
  int  GetState(int display);
  void SetState(int display,int state);
  void SetStates(int state);

  void Reset();				// Reset all
  void Update();

  // Debug
  void DbgShowState();
};

#endif
