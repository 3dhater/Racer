// qlib/gfx.h - Paint window (not broadcasted)
// Future:
// - Hold GelList
// - Derive QBC and QShell from this class

#ifndef __QLIB_GFX_H
#define __QLIB_GFX_H

#include <qlib/window.h>

//class QWindow;

class QGFX : public QWindow
// A graphics window; no gels, just paint stuff
{
 public:
  string ClassName(){ return "gfx"; }
 public:

 protected:
  int flags;
 public:
  QBitMap *bmEmul[2];		// Emulation storage
  int      buffer;		// Double buffered backing store

 public:
  QGFX(QWindow *parent,int typ=PAL,int x=48,int y=48,int wid=320,int hgt=256);
  ~QGFX();

  //void Create();

  void Paint();
  //void Swap();
};

#endif
