// qlib/qshell.h

#ifndef __QLIB_QSHELL_H
#define __QLIB_QSHELL_H

#include <qlib/window.h>
#include <qlib/draw.h>
#include <qlib/canvas.h>

//class QShell : public QWindow
class QShell : public QDraw
// A regular window on the screen, which
// will contain other QWindows
// This class actually uses an actual X window
{
 public:
  string ClassName(){ return "shell"; }

 protected:
  //QCanvas *cv;
  //QWindow *share;

 public:
  QShell(QWindow *parent,int x=0,int y=0,int wid=100,int hgt=100);
    //QXWindow *share=0);
  ~QShell();

  //bool Create();

  //QCanvas *GetCanvas(){ return cv; }

  void Paint(QRect *r=0);

  bool EvExpose(int x,int y,int wid,int hgt);
};

#endif
