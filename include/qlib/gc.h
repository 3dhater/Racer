// qlib/gc.h - The X GC wrapper

#ifndef __QLIB_QGC_H
#define __QLIB_QGC_H

#include <qlib/window.h>
#include <qlib/bitmap.h>
#include <qlib/font.h>
#include <qlib/point.h>
#include <X11/Xlib.h>

class QGC : public QObject
{
 public:

 protected:
  QDrawable *drw;
  int      flags;
  QFont   *font;
  GC       xgc;			// X11

 public:
  QGC(QDrawable *w);
  ~QGC();

  // Attributes
  void SetFont(QFont *font);
  QFont *GetFont();

  void Select();		// Select our context

  void SetForeground(int indPixel);
  //void SetOffset(int ox,int oy);
  //void SetClipRect(int x,int y,int wid,int hgt);
  //void SetClipRect(QRect *r);

  void Enable(int newFlags);
  void Disable(int clrFlags);

  // Rectangles
  void Rectfill(QRect *rz,QColor *colUL,QColor *colUR,QColor *colLR,
    QColor *colLL);
  // Shapes
  void Triangle(int x1,int y1,int x2,int y2,int x3,int y3);

  // Text
  void Text(cstring txt,int dx=0,int dy=0,int len=-1);
};

#endif
