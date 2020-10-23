// qlib/psize.h

#ifndef __QLIB_PSIZE_H
#define __QLIB_PSIZE_H

#include <qlib/types.h>

class QProcessSize
{
  void *lastBrk;		// Last recorded sbrk value
 public:
  QProcessSize();
  ~QProcessSize();

 private:
  void UpdateBrk();		// Update 'lastBrk'

 public:
  int GetSize();

  // High-level
  bool AutoDetectChange(string s=0);
};

#endif
