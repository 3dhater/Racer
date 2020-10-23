/*
 * DBox - a 3D box primitive type
 * 11-12-00: Created!
 * (C) MarketGraph/RVG
 */

#include <d3/d3.h>
#include <qlib/debug.h>
#pragma hdrstop
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "DBox"

/***************
* Constructors *
***************/
DBox::DBox()
// Default constructor uses default values
{
  min.SetToZero();
  max.SetToZero();
}
#ifdef ND_FUTURE
DBox::DBox(dfloat x,dfloat y,dfloat z,dfloat wid,dfloat hgt,dfloat dep)
// Constructor with x/y/wid/hgt
{
  center.x=x;
  center.y=y;
  center.z=z;
  size.x=wid;
  size.y=hgt;
  size.z=dep;
}
#endif
DBox::DBox(DVector3 *_min,DVector3 *_max)
// Construct from center and size vectors
{
  min=*_min;
  max=*_max;
}

DBox::DBox(DBox& b)
// Copy constructor
{
  min=b.min;
  max=b.max;
}
DBox::DBox(DBox *b)
// Construct based on another rectangle
{
  min.x=b->min.x;
  min.y=b->min.y;
  min.z=b->min.z;
  max.x=b->max.x;
  max.y=b->max.y;
  max.z=b->max.z;
}

DBox::~DBox(){}

#ifdef OBS
void DBox::SetXY(int nx,int ny)
{ x=nx; y=ny;
}

void DBox::SetSize(int nwid,int nhgt)
{ wid=nwid; hgt=nhgt;
}
#endif

/************
* COLLISION *
************/
bool DBox::Contains(dfloat px,dfloat py,dfloat pz)
{
  if(px<min.x||py<min.y||pz<min.z||
     px>=max.x||py>=max.y||pz>=max.z)
    return FALSE;
  return TRUE;
}

void DBox::Union(DBox *r)
// Combine boxes. Result is in 'this'.
{
  // Union 3 axes separately
  if(r->min.x<min.x)min.x=r->min.x;
  if(r->max.x>max.x)max.x=r->max.x;
  if(r->min.y<min.y)min.y=r->min.y;
  if(r->max.y>max.y)max.y=r->max.y;
  if(r->min.z<min.z)min.z=r->min.z;
  if(r->max.z>max.z)max.z=r->max.z;
}

#ifdef FUTURE
bool DBox::Overlaps(DBox *r)
// Do this and r overlap?
{
  // Check X
  if(r->x>=x+wid)return FALSE;
  if(r->x+r->wid<=x)return FALSE;
  // X overlaps, check Y
  if(r->y>=y+hgt)return FALSE;
  if(r->y+r->hgt<=y)return FALSE;
  return TRUE;
}

void DBox::Intersect(DBox *r)
// 'this' will hold the intersection with 'r'
// If the intersection is empty, the size will be 0, and x/y undefined.
{
  // Determine intersection (x,y) start
  if(r->x>x)
  { wid+=x-r->x;
    x=r->x;
  }
  if(r->y>y)
  { hgt+=y-r->y;
    y=r->y;
  }
  if(wid<0||hgt<0)
  { // Intersection is empty
   do_empty:
    wid=hgt=0;
    return;
  }
  if(r->x+r->wid<x+wid)
  { // Cut out right part
    wid=r->x+r->wid-x;
  }
  if(r->y+r->hgt<y+hgt)
  { // Cut out bottom part
    hgt=r->y+r->hgt-y;
  }
  if(wid<0||hgt<0)
    goto do_empty;
}
#endif

/**************
* Information *
**************/
dfloat DBox::Volume()
// Returns the volume of the box
{
  return (max.x-min.x)*(max.y-min.y)*(max.z-min.z);
}
void DBox::GetCenter(DVector3 *v)
// Returns center in 'v'
{
  v->x=(max.x+min.x)/2;
  v->y=(max.y+min.y)/2;
  v->z=(max.z+min.z)/2;
}
dfloat DBox::GetRadius()
// Assuming a virtual center (see GetCenter), calculate the radius that
// would be needed to enclose this box in a sphere.
// Useful for bounding box -> sphere conversions.
{
  // Take the diagonal from min to max. That is the line going through
  // the center to the endpoints. Which is the diameter of the sphere,
  // so the radius is half the diagonal's length.
  return (max-min).Length()/2.0f;
}

void DBox::DbgPrint(cstring s)
// Dump
{
  qdbg("Box (%.2f,%.2f,%.2f)-(%.2f,%.2f,%.2f) [%s]\n",
    min.x,min.y,min.z,max.x,max.y,max.z,s);
}

