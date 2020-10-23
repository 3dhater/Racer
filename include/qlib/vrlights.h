// vrlights.h - Vos & Ramselaar Lights interface
// V&R standard button interface

#ifndef __QLIB_VRLIGHTS_H
#define __QLIB_VRLIGHTS_H

#include <qlib/serial.h>

class QVRLights
{
 public: 
  enum MaxThings
  { MAX_LIGHTS=256
  };
  enum States
  { OFF=0,
    ON=1
  };
  enum Flags
  {
    PRIVATE_SER=1              // We own the QSerial object
  };

 protected:
  // Class member variables
  QSerial *ser;
  int  count;
  char state[MAX_LIGHTS];
  int  flags;

 public:
  QVRLights(int count,int serUnit=QSerial::UNIT2,QSerial *shareSer=0);
  ~QVRLights();

  // Internal info
  int      GetCount(){ return count; }
  QSerial *GetSerial(){ return ser; }

  // Light states
  int  GetState(int light);
  void SetState(int light,int state);
  void SetStates(int state);

  void Reset();				// Reset all
  void Update();

  // Debug
  void DbgShowState();
};

#endif
