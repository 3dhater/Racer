// qlib/mathpaint.h

#ifndef __QLIB_MATHPAINT_H
#define __QLIB_MATHPAINT_H

#include <qlib/image.h>

class QMathPaint
{
 public:
  enum
  { MAX_SRC=10
  };

  QBitMap *src[MAX_SRC];
  QBitMap *curSource;
  QBitMap *dst;

 public:
  QMathPaint();
  ~QMathPaint();

};

#endif
