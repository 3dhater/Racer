// qlib/cube.h - 3D cube primitive

#ifndef __QLIB_CUBE_H
#define __QLIB_CUBE_H

#include <qlib/polygon.h>

class QCube : public QObject
{
 protected:
  QPolygon *poly[6];		// 6 sides for the entire cube
  qfloat    wid,hgt,dep;	// Size of cube
  int       flags;

 public:
  QCube(qfloat wid,qfloat hgt,qfloat dep);
  ~QCube();

  void DefineTexture(int side,QBitMap *src,QRect *r);
  void Paint();
};

#endif

