// qlib/qscroll.h

#ifndef __QLIB_QSCROLL_H
#define __QLIB_QSCROLL_H

#include <qlib/text.h>

class QScroller : public QObject
{
 protected:
  int curX,curY;                // Location of virtual screen (UL)
  QCanvas *cv;			// Canvas in which to scroll
  QBob **gel;                   // List of gels
  int    gels,maxGel;
  QRect area;                   // Scroll area

  void ScrollGel(QBob *gel,int vx,int vy);

 public:
  QScroller(QCanvas *cv,QRect *r);
  ~QScroller();

  int   GetGelCount(){ return gels; }
  QBob *GetGel(int index);
  bool Add(QBob *gel);
  void Scroll(int dx,int dy);
};

#endif
