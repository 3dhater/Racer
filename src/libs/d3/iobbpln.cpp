/*
 * D3 - intersection of plane and OBB
 * 23-11-01: Created!
 * NOTES:
 * - Pseudocode from the book Real-Time Rendering (Moller/Haines), pp. 311-312.
 * (C) MarketGraph/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/intersect.h>
DEBUG_ENABLE

// Method #1 is to transform the plane's normal, and then do an AABB-plane
// test instead. Bugs: some offset need to be negated or something. It's
// close, but actually reverses some numbers, so the answers are not yet right.
//#define USE_TRANSFORMED_NORMAL

// Method #2 is to calculate the radius of the OBB and check the OBB's
// center distance to the plane and see if it's greater than this radius.
#define USE_RADIUS

dfloat d3FindOBBPlaneIntersection(const DOBB *obb,const DPlane3 *plane,
  DVector3 *point)
// Finds the intersection of an OBB (oriented bounding box) and a plane.
// Returns the penetration depth, or 0 if no penetration is present.
// Returns contact point in 'point'. Addmitedly, the point is very crude
// at this point though and may be of limited use.
// Assumes the plane's normal is normalized (!).
{
//qdbg("d3FindOBBPlaneIntersection()\n");
//plane->DbgPrint("plane");
//obb->DbgPrint("obb");

#ifdef USE_RADIUS
  dfloat r,d;

  // Calculate the radius of the OBB
  r=fabs(obb->extents.x*plane->n.Dot(&obb->axis[0]))+
    fabs(obb->extents.y*plane->n.Dot(&obb->axis[1]))+
    fabs(obb->extents.z*plane->n.Dot(&obb->axis[2]));

  // Now check how far the center is distantiated from the plane
  d=fabs(obb->center.Dot(plane->n)+plane->d);
//qdbg("OBB r=%f, distance to plane=%f\n",r,d);
  if(d>r)
  {
    // No intersection
    return 0;
  } else
  {
    // Return penetration depth
//qdbg("  pen=%.4f\n",r-d);
    return r-d;
  }
#endif

#ifdef USE_TRANSFORMED_NORMAL
  DVector3 n,vMin,vMax,bMin,bMax;
  dfloat   pen;

  // Transform the plane's normal so that we can simplify the test
  // to an AABB-plane intersection.
  n.x=plane->n.Dot(&obb->axis[0]);
  n.y=plane->n.Dot(&obb->axis[1]);
  n.z=plane->n.Dot(&obb->axis[2]);
n.DbgPrint("n'");

  // Create min/max vertex of the OBB
  bMin.x=obb->center.x-obb->extents.x;
  bMin.y=obb->center.y-obb->extents.y;
  bMin.z=obb->center.z-obb->extents.z;
  bMax.x=obb->center.x+obb->extents.x;
  bMax.y=obb->center.y+obb->extents.y;
  bMax.z=obb->center.z+obb->extents.z;

  // Find the closest and farthest points of the OBB wrt the plane
  if(n.x>=0){ vMin.x=bMin.x; vMax.x=bMax.x; }
  else      { vMin.x=bMax.x; vMax.x=bMin.x; }
  if(n.y>=0){ vMin.y=bMin.y; vMax.y=bMax.y; }
  else      { vMin.y=bMax.y; vMax.y=bMin.y; }
  if(n.z>=0){ vMin.z=bMin.z; vMax.z=bMax.z; }
  else      { vMin.z=bMax.z; vMax.z=bMin.z; }

bMin.DbgPrint("bMin");
bMax.DbgPrint("bMax");
vMin.DbgPrint("vMin");
vMax.DbgPrint("vMax");
qdbg("vMin dot = %f\n",n.Dot(&vMin)+plane->d);

  // If the minimum point is in front of the plane, no overlap takes place.
  if((n.Dot(&vMin)+plane->d)>0)
    return 0;
  // If the maximum point is at the back of the plane, the box intersects
  // the plane.
  pen=n.Dot(&vMax)+plane->d;
qdbg("  pen=%.2f\n",pen);
  if(pen>=0)
  {
    // Overlap; calculate penetration depth
    n.Normalize();
    pen=n.Dot(&vMax)+plane->d;
qdbg("OBB-Plane; penetration depth=%.2f\n",pen);
    return pen;
  }

  // Otherwise both points are on the same side of the plane and don't
  // intersect. Note that you may STILL want to detect this; i.e. a very
  // fast traveling suddenly jumping through a plane (of a triangle) and
  // suddenly being totally behind the triangle's plane. However, that
  // can be tricky.
  return 0;
#endif
}

