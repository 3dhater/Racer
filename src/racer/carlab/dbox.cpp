/*
 * DBox - a 3D box primitive type
 * 11-12-00: Created!
 * (C) MarketGraph/RVG
 */

#include <d3/types.h>
#include <qlib/debug.h>
DEBUG_ENABLE

#undef  DBG_CLASS
#define DBG_CLASS "DBox"

/***************
* Constructors *
***************/
DBox::DBox()
{
}
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
DBox::DBox(DVector3 *_center,DVector3 *_size)
// Construct from center and size vectors
{
  center=*_center;
  size=*_size;
}

DBox::DBox(DBox& b)
// Copy constructor
{
  center=b.center;
  size=b.size;
}
DBox::DBox(DBox *b)
// Construct based on another rectangle
{
  center.x=b->center.x;
  center.y=b->center.y;
  center.z=b->center.z;
  size.x=b->size.x;
  size.y=b->size.y;
  size.z=b->size.z;
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
#ifdef FUTURE
bool DBox::Contains(int px,int py)
{
  if(px<x||py<y)return FALSE;
  if(px>=x+wid)return FALSE;
  if(py>=y+hgt)return FALSE;
  return TRUE;
}

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

void DBox::Union(DBox *r)
// Combine rectangles. Result is in this
{
  if(r->x<x)
  { // Expand to left
    wid+=x-r->x;
    x=r->x;
  }
  if(r->y<y)
  { // Expand to top
    hgt+=y-r->y;
    y=r->y;
  }
  if(r->x+r->wid>x+wid)
  { wid=r->x+r->wid-x;
  }
  if(r->y+r->hgt>y+hgt)
  { hgt=r->y+r->hgt-y;
  }
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
  return size.x*size.y*size.z;
}

