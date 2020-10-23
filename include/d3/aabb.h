// d3/aabb.h

#ifndef __D3_AABB_H
#define __D3_AABB_H

#include <d3/types.h>

class DAABB
// Axis-aligned bounding box (3D)
{
 public:
  DVector3 min,max;

 public:
  DAABB(){}         // For performance, nothing is cleared
  ~DAABB(){}

  void Clear()
  {
    min.Clear();
    max.Clear();
  }
  dfloat GetVolume() const
  {
    return (max.x-min.x)*(max.y-min.y)*(max.z-min.z);
  }

  // Creating an AABB from other primitives
  void   MakeFromRay(const DVector3 *org,const DVector3 *dir,dfloat len);
  void   MakeFromOBB(const DVector3 *center,const DVector3 *extents,
                     const DMatrix4 *m);
  void   MakeFromOBB(const DVector3 *center,const DVector3 *extents,
                     const DMatrix3 *m);

  dfloat GetWidth(){ return max.x-min.x; }
  dfloat GetHeight(){ return max.y-min.y; }
  dfloat GetDepth(){ return max.z-min.z; }

  bool   Overlaps(DAABB *aabb)
  // Returns TRUE if the boxes overlap
  {
    return(aabb->min.x<max.x&&aabb->max.x>min.x&&
           aabb->min.y<max.y&&aabb->max.y>min.y&&
           aabb->min.z<max.z&&aabb->max.z>min.z);
  }
  bool   Contains(DAABB *aabb)
  // Returns TRUE if 'aabb' is completely contained in this object
  {
    return(aabb->min.x>=min.x&&aabb->max.x<=max.x&&
	   aabb->min.y>=min.y&&aabb->max.y<=max.y&&
           aabb->min.z>=min.z&&aabb->max.z<=max.z);
  }

  // Debugging
  void DbgPrint(cstring s);
};

#endif

