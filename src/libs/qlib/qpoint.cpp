/*
 * QLib/QPoint - points and rects, QSize etc
 * 28-04-97: Created!
 * (C) MarketGraph/RVG
 */

#include <qlib/object.h>
#include <qlib/point.h>

QRect::QRect(int ix,int iy,int iwid,int ihgt)
// Constructor with x/y/wid/hgt
{
  x=ix; y=iy;
  wid=iwid; hgt=ihgt;
}

QRect::QRect(QRect& r)
// Copy constructor
{
  x=r.x; y=r.y;
  wid=r.wid; hgt=r.hgt;
}
QRect::QRect(QRect *r)
// Construct based on another rectangle
{
  x=r->x; y=r->y;
  wid=r->wid; hgt=r->hgt;
}

QRect::~QRect()
{
}

void QRect::SetXY(int nx,int ny)
{ x=nx; y=ny;
}

void QRect::SetSize(int nwid,int nhgt)
{ wid=nwid; hgt=nhgt;
}

/************
* COLLISION *
************/
bool QRect::Contains(int px,int py)
{
  if(px<x||py<y)return FALSE;
  if(px>=x+wid)return FALSE;
  if(py>=y+hgt)return FALSE;
  return TRUE;
}

bool QRect::Overlaps(QRect *r)
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

void QRect::Union(QRect *r)
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
void QRect::Intersect(QRect *r)
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

/**************
* Information *
**************/
bool QRect::IsEmpty()
// Returns TRUE if rectangle is empty (has no area)
// This means either the width or height is 0
{
  if(wid>0&&hgt>0)return FALSE;
  return TRUE;
}
int QRect::Area()
{
  return wid*hgt;
}

/********
* QSIZE *
********/
QSize::QSize()
{
  wid=hgt=0;
}
QSize::QSize(int w,int h)
{
  wid=w; hgt=h;
}

