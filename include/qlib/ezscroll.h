// qlib/qezscroll.h

#ifndef __QLIB_QEZSCROLL_H
#define __QLIB_QEZSCROLL_H

#include <qlib/scroll.h>

class QEZScroller : public QScroller
{
 public:
  enum Align
  {
    LEFT,
    CENTRE,
    RIGHT
  };
  enum Flags
  {
    OWN_FONT=1                  // Font is alloc'd by us
  };

 protected:
  int    flags;
  QFont *curFont;
  int    curAlign;
  int    curX,curY;             // Location of next gel
  int    spacingX,spacingY;     // Inter-gel spacing
  void FreeFont();

 public:
  QEZScroller(QCanvas *cv,QRect *r=0);
  ~QEZScroller();

  // Remembering attributes
  void SetFont(QFont *font);
  void SetFont(string fname,int size);
  void SetAlignment(int align);

  void AddText(string s);
  void AddBitMap(QBitMap *bm);
};

#endif
