// gfxcursor.h - Graphically blitted cursor

#ifndef __GFXCURSOR_H
#define __GFXCURSOR_H

#include <qlib/bob.h>
#include <qlib/image.h>

class QGFXCursor
{
 protected:
  int x,y;
  QImage *img;
  QBob *bob;

 public:
  QGFXCursor(string filename);
  ~QGFXCursor();

  // Enable/disable
  void Enable();
  void Disable();

  void Move(int x,int y);

  // Event handling; explicit
  void HandleEvent(QEvent *e);
};

#endif
