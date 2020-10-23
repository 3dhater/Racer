// d3/obb.h

#ifndef __D3_OBB_H
#define __D3_OBB_H

#include <d3/types.h>

class DOBB
// 3-dimensional oriented bounding box
{
 public:
  DVector3 center;         // World center of box
  DVector3 extents;        // Size of box (XYZ)
  DVector3 axis[3];        // Axes that indicate current box orientation
  DVector3 linVel;         // Linear velocity

 public:
  DOBB();
  DOBB(DVector3 *center,DVector3 *extents);
  ~DOBB();

  DVector3 *GetCenter(){ return &center; }
  DVector3 *GetExtents(){ return &extents; }

  // World values
  DVector3 *GetWorldCenter(){ return &center; }
  DVector3 *GetWorldAxes(){ return &axis[0]; }
  dfloat   *GetWorldExtent(){ return &extents.x; }

  void DbgPrint(cstring s) const;
};

#endif
