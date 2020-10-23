// d3/intersect.h

#ifndef __D3_INTERSECT_H
#define __D3_INTERSECT_H

#include <d3/types.h>
#include <d3/obb.h>

// OBB-Triangle

// Return value is 0 for intersection, not zero otherwise.  The nonzero
// value indicates which separating axis separates the triangle and box.

// triangle and box are moving, each with constant velocity
/*unsigned int d3TestTriObb(float dt,DVector3* tri[3],
  const DVector3& triVel,DOBB& box,const DVector3& boxVel);*/

// Intersection tests

// Intersection between moving triangle and moving OBB
unsigned int d3TestTriObb(float dt,DVector3* tri[3],
  const DVector3& triVel,DOBB& box,const DVector3& boxVel);

// Intersection finding

// Intersection between moving triangle and moving OBB
unsigned int d3FindTriObb(float dt,DVector3* tri[3],
  const DVector3& triVel,DOBB& box,const DVector3& boxVel,
  dfloat& T,DVector3& P);

// Tri-quad intersecting
bool d3RayQuadIntersection2D(DVector3 *org,DVector3 *dir,
  DVector3 *v0,DVector3 *v1,DVector3 *v2,DVector3 *v3,
  dfloat *t,dfloat *u,dfloat *v);

// OBB-plane intersection
dfloat d3FindOBBPlaneIntersection(const DOBB *obb,const DPlane3 *plane,
  DVector3 *point);

#endif
