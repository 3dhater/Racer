// tdo.h

#ifndef __GPL_TDO_H
#define __GPL_TDO_H

#include <qlib/types.h>

struct GPL_XYZ
{
  float s,x,y,z;
};
struct GPL_Plane
{
  float a,b,c,d;
};

class GPLTDO
{
 public:
  FILE *fp;
  qstring fname;
  GPL_XYZ   *xyz;
  int        xyzs;
  GPL_Plane *plane;
  int        planes;
  // Primitives
  long      *prim;
  int        primSize;
  // Strings
  char      *strnData;
  int        strnSize;
  // Paint angle
  float xa,ya,za;
  
 public:
  GPLTDO(cstring fname);
  ~GPLTDO();
  
  bool Read();
  
  cstring GetString(long offset);

  void SetColor(long col);
  void PaintVertex(GPL_XYZ *v);
  void PaintPrim(long root,int dep);
  void PaintPrimitives();
  void Paint();
};

#endif
