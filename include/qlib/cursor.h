// qlib/qcursor.h - X cursor encapsulation

#ifndef __QLIB_QCURSOR_H
#define __QLIB_QCURSOR_H

#include <qlib/object.h>
#include <X11/X.h>

// From which 'cursor library'; X (cursorfont) or Q
#define QCFROM_X	0
#define QCFROM_Q	1

// Q cursor definitions
#define QC_EMPTY	0
#define QC_ARROW	1		// My crappy 'first Sony' arrow
// and more to come...

class QDisplay;
class QWidget;

class QCursor : public QObject
{
 private:
  // X11
  Cursor xc;

 protected:
  int wid,hgt;
  //QWidget  *wg;
  void CreateQCursor(int image);

 public:
  QCursor(int image,int from=QCFROM_Q);
  ~QCursor();

  // X11; for use in Q
  Cursor GetXCursor(){ return xc; }
};

#endif

