/*
 * DAABB - axis aligned bounding box class
 * 11-11-01: Created!
 * NOTES:
 * - Used in the DAABBTree class.
 * (c) Dolphinity/RvG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
#include <d3/aabb.h>
DEBUG_ENABLE

// Output status on tree building?
//#define VERBOSE

#ifdef VERBOSE
#define QDBG  qdbg
#else
#define QDBG  1?(void)0:qdbg
#endif

/***********
* PolyInfo *
***********/
#ifdef OBS_INLINE
DAABB::DAABB()
{
  //min.Clear();
  //max.Clear();
}
DAABB::~DAABB()
{
}
#endif

#ifdef OBS_INLINED
dfloat DAABB::GetVolume() const
// 
{
  return (max.x-min.x)*(max.y-min.y)*(max.z-min.z);
}
#endif

/*********************************
* Creating from other primitives *
*********************************/
void DAABB::MakeFromRay(const DVector3 *org,const DVector3 *dir,dfloat len)
// Create an AABB from a ray with a specific length.
// This is useful for example if you want to raytrace into an AABBTree
// with a ray of limited length (like wheel vs. track intersections
// in Racer). The resulting AABB can be fed to the AABBTree and the potentially
// intersection boxes will be returned.
{
//qdbg("DAABB:MakeFromRay(); len=%f\n",len);
//org->DbgPrint("org");
//dir->DbgPrint("dir");
  // Find out maximum extents of the ray
  if(dir->x<0){ min.x=org->x+dir->x*len; max.x=org->x; }
  else        { min.x=org->x; max.x=org->x+dir->x*len; }
  if(dir->y<0){ min.y=org->y+dir->y*len; max.y=org->y; }
  else        { min.y=org->y; max.y=org->y+dir->y*len; }
  if(dir->z<0){ min.z=org->z+dir->z*len; max.z=org->z; }
  else        { min.z=org->z; max.z=org->z+dir->z*len; }
}

//void DAABB::MakeFromOBB(const DOBB *obb)
void DAABB::MakeFromOBB(const DVector3 *center,const DVector3 *extents,
  const DMatrix4 *m)
// Create an AABB enclosing the entire OBB.
// 'center' is the OBB's center position, 'extents' is a triple with
// the OBB's halfsizes, and 'm' is the rotation matrix.
// This is used in Racer for example, to get a quick optimization for
// querying car-track collisions. An AABB for the car is created, which
// is then fed into DAABBTree::QueryAABB() and resulting tris are tested only.
// This function was NEVER TESTED!
{
  dfloat l;

  // Project the axes of the OBB onto an AABB with dot products. As the AABB
  // has an identity rotation matrix, lots of the dot products are simple.

  // X-axis
  l=fabs(m->GetRC(0,0))*extents->x+fabs(m->GetRC(0,1))*extents->y+
    fabs(m->GetRC(0,2))*extents->z;
  min.x=center->x-l;
  max.x=center->x+l;
  // Y-axis
  l=fabs(m->GetRC(1,0))*extents->x+fabs(m->GetRC(1,1))*extents->y+
    fabs(m->GetRC(1,2))*extents->z;
  min.y=center->y-l;
  max.y=center->y+l;
  // Z-axis
  l=fabs(m->GetRC(2,0))*extents->x+fabs(m->GetRC(2,1))*extents->y+
    fabs(m->GetRC(2,2))*extents->z;
  min.z=center->z-l;
  max.z=center->z+l;
}

void DAABB::MakeFromOBB(const DVector3 *center,const DVector3 *extents,
  const DMatrix3 *m)
// Create an AABB enclosing the entire OBB.
// 'center' is the OBB's center position, 'extents' is a triple with
// the OBB's halfsizes, and 'm' is the rotation matrix.
// This is used in Racer for example, to get a quick optimization for
// querying car-track collisions. An AABB for the car is created, which
// is then fed into DAABBTree::QueryAABB() and resulting tris are tested only.
{
  dfloat l;

  // Project the axes of the OBB onto an AABB with dot products. As the AABB
  // has an identity rotation matrix, lots of the dot products are simple.

  // X-axis
  l=fabs(m->GetRC(0,0))*extents->x+fabs(m->GetRC(0,1))*extents->y+
    fabs(m->GetRC(0,2))*extents->z;
  min.x=center->x-l;
  max.x=center->x+l;
  // Y-axis
  l=fabs(m->GetRC(1,0))*extents->x+fabs(m->GetRC(1,1))*extents->y+
    fabs(m->GetRC(1,2))*extents->z;
  min.y=center->y-l;
  max.y=center->y+l;
  // Z-axis
  l=fabs(m->GetRC(2,0))*extents->x+fabs(m->GetRC(2,1))*extents->y+
    fabs(m->GetRC(2,2))*extents->z;
  min.z=center->z-l;
  max.z=center->z+l;
}

/************
* Debugging *
************/
void DAABB::DbgPrint(cstring s)
{
  qdbg("%s: (%.2f,%.2f,%.2f)-(%.2f,%.2f,%.2f)\n",
    s,min.x,min.y,min.z,max.x,max.y,max.z);
}

