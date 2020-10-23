// qlib/qscreen.h

#ifndef __QLIB_QSCREEN_H
#define __QLIB_QSCREEN_H

#include <qlib/object.h>

class QScreen : public QObject
{
 protected:
  int n;

 public:
  QScreen(int n=0);
  ~QScreen();

  // Information
  int GetNumber();
  int GetWidth();
  int GetHeight();
};

#endif
