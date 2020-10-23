// qlib/polygon.h - 3D polygons

#ifndef __QLIB_POLYGON_H
#define __QLIB_POLYGON_H

// OLD OBSOLETE USAGE; USE D3/POLY.H INSTEAD

#include <qlib/bitmap.h>

class QPolygon : public QObject
{
 protected:
  qfloat x[4],y[4],z[4];	// Coordinates
  qfloat tx[4],ty[4];		// Texture coords
  int    vertices;
  int    flags;
  char  *texture;		// Processed texture
  QPoint textureSize;
  int    texID;			// OpenGL texture object

 public:
  enum Flags
  {
    TEXTURE=1
  };

 public:
  QPolygon(int vertices=3);
  ~QPolygon();

  void SetVertex(int n,qfloat x,qfloat y,qfloat z);

  void DefineTexture(QBitMap *src,QRect *r);
  void Paint();
};

#endif

