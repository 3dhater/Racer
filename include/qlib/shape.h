// qlib/shape.h - shape gel

#ifndef __QLIB_SHAPE_H
#define __QLIB_SHAPE_H

#include <qlib/gel.h>
#include <qlib/image.h>
#include <qlib/control.h>
#include <qlib/filter.h>

// Shape types
#define QST_RECTFILL	1		// Rectfill

class QShape : public QGel
{
 public:
  string ClassName(){ return "shape"; }	// Optimize this
 protected:
  int     type;
  QColor *col[4];		// Corner points for rects
 public:
  // Dynamic controls
  QControl *x,*y,*wid,*hgt;

 public:
  QShape(QRect *r,int type);
  ~QShape();

  int GetWidth(){ return wid->Get(); }
  int GetHeight(){ return hgt->Get(); }

  void Paint();
  void PaintPart(QRect *r);
};

#endif
