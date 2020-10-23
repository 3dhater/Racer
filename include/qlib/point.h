// qlib/qpoint.h

#ifndef __QLIB_QPOINT_H
#define __QLIB_QPOINT_H

class QPoint
{public:
  int x,y;
};

class QPoint3
{public:
  int x,y,z;
};

class QSize
{public:
  QSize();
  QSize(int w,int h);
  int wid,hgt;
};

class QRect
{public:
  int x,y;
  int wid,hgt;

  QRect(int x=0,int y=0,int wid=0,int hgt=0);
  QRect(QRect& r);
  QRect(QRect *r);
  ~QRect();

  void SetXY(int x,int y);
  void SetSize(int wid,int hgt);

  // Information
  bool IsEmpty();
  int  Area();

  // Boolean
  bool Contains(int x,int y);	// Point in rectangle
  bool Overlaps(QRect *r);	// Do they touch?
  void Union(QRect *r);		// Union with other rect
  void Intersect(QRect *r);	// Intersection with other rect
};

#endif
