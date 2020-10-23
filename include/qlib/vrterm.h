// qlib/vrterm - Yellowspot terminal

#ifndef __QLIB_VRTERM_H
#define __QLIB_VRTERM_H

#include <qlib/serial.h>

class QVRTerm : public QSerial
// The terminal has a 80-char line buffer
// Can send every char directly to us
{
 public:
  QVRTerm(int unit,int bps);
  ~QVRTerm();

  void Reset();
  // Output line
  void Clear();
};

#endif
