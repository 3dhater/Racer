// QButtons.h - declaration
// V&R standard button interface

#ifndef __QBUTTONS_H
#define __QBUTTONS_H

#include <qlib/serial.h>

#define QB_CMDLEN	40
#define QB_MAXBUTTON	64

class QButtons
{
 protected:
  // Class member variables
  QSerial *ser;
  int count;
  char cmd[QB_CMDLEN];
  int  cmdIndex;
  char butState[QB_MAXBUTTON];
  char butLatch[QB_MAXBUTTON];

 public:
  QButtons(int count=16);
  ~QButtons();

  QSerial *GetSerial(){ return ser; }

  void Init();
  void Flush();
  void HandleMsg();
  bool Handle();
  void Reset();				// Release latches

  int GetState(int n);			// Current button state
  int GetLatch(int n);			// Latched state (stays on)
  void SetLatch(int n,int state);	// Set Latch state

  // Debug
  void DbgShowState();
};

#endif
