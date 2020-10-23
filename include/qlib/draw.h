// qlib/draw.h - Drawn window (shell - draw - bc)

#ifndef __QLIB_DRAW_H
#define __QLIB_DRAW_H

#include <qlib/window.h>

class QDraw;

typedef void (*QDrawPaintProc)(QDraw*,void*);

class QDraw : public QWindow
// A user-drawn window
{
 public:
  string ClassName(){ return "draw"; }
 public:

 protected:
  int flags;
  QDrawPaintProc paintProc;
  void *userData;

 public:

 public:
  QDraw(QWindow *parent,int x,int y,int wid,int hgt,QDrawPaintProc pp=0,
    void *userData=0);
  ~QDraw();

  void SetPaintProc(QDrawPaintProc pp,void *userData);
  QDrawPaintProc GetPaintProc();

  void Paint(QRect *r=0);
  bool ScreenShot(cstring fileName);
  virtual void Swap();
};

#endif
